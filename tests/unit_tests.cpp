#include <gtest/gtest.h>
#include "fee_calculator.h"
#include "json_parser.h"
#include "webhook_signer.h"
#include "payload_transformer.h"

// -------------------------------------------------------------
// FeeCalculator Tests
// -------------------------------------------------------------
TEST(FeeCalculatorTest, SmallAmountTier1Calculation) {
    // Amount = $50.00 (5,000 cents), low risk (10)
    // Tier 1 (<= 10,000 cents): 2.9% + 30 cents = (5000 * 29)/1000 + 30 = 145 + 30 = 175 cents
    // Risk withheld = 0
    // Net payout = 5000 - 175 = 4825 cents
    auto res = FeeCalculator::calculate(5000, 10);
    EXPECT_EQ(res.gross_amount_cents, 5000);
    EXPECT_EQ(res.processing_fee_cents, 175);
    EXPECT_EQ(res.risk_withheld_cents, 0);
    EXPECT_EQ(res.net_payout_cents, 4825);
}

TEST(FeeCalculatorTest, LargeAmountTier2Calculation) {
    // Amount = $200.00 (20,000 cents), low risk (20)
    // Tier 2 (> 10,000 cents): 1.5% + 30 cents = (20000 * 15)/1000 + 30 = 300 + 30 = 330 cents
    // Risk withheld = 0
    // Net payout = 20000 - 330 = 19670 cents
    auto res = FeeCalculator::calculate(20000, 20);
    EXPECT_EQ(res.gross_amount_cents, 20000);
    EXPECT_EQ(res.processing_fee_cents, 330);
    EXPECT_EQ(res.risk_withheld_cents, 0);
    EXPECT_EQ(res.net_payout_cents, 19670);
}

TEST(FeeCalculatorTest, HighRiskWithholdingCalculation) {
    // Amount = $100.00 (10,000 cents), high risk (80)
    // Tier 1: 2.9% + 30 cents = (10000 * 29)/1000 + 30 = 290 + 30 = 320 cents
    // Risk withheld (10% of gross): (10000 * 10)/100 = 1000 cents
    // Net payout = 10000 - 320 - 1000 = 8680 cents
    auto res = FeeCalculator::calculate(10000, 80);
    EXPECT_EQ(res.gross_amount_cents, 10000);
    EXPECT_EQ(res.processing_fee_cents, 320);
    EXPECT_EQ(res.risk_withheld_cents, 1000);
    EXPECT_EQ(res.net_payout_cents, 8680);
}

// -------------------------------------------------------------
// JsonParser Tests
// -------------------------------------------------------------
TEST(JsonParserTest, ParsesValidPendingEvents) {
    std::string sample = R"({
        "object": "list",
        "data": [
            {
                "id": "evt_abc123",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000000,
                "amount": 15000,
                "currency": "usd",
                "customer": "cus_999",
                "destination": "acct_prime",
                "risk_score": 15
            }
        ]
    })";

    auto events = JsonParser::parse_events(sample);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].id, "evt_abc123");
    EXPECT_EQ(events[0].type, EventType::PaymentIntentSucceeded);
    EXPECT_EQ(events[0].status, "pending");
    EXPECT_EQ(events[0].amount_cents, 15000);
    EXPECT_EQ(events[0].currency, Currency::USD);
    EXPECT_EQ(events[0].customer_id, "cus_999");
    EXPECT_EQ(events[0].destination_account, "acct_prime");
    EXPECT_EQ(events[0].risk_score, 15);
}

TEST(JsonParserTest, FiltersOutNonPendingEvents) {
    std::string sample = R"({
        "object": "list",
        "data": [
            {
                "id": "evt_done",
                "type": "payment_intent.succeeded",
                "status": "processed",
                "amount": 5000,
                "currency": "usd"
            },
            {
                "id": "evt_pending_1",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "amount": 7500,
                "currency": "usd"
            }
        ]
    })";

    auto events = JsonParser::parse_events(sample);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].id, "evt_pending_1");
    EXPECT_EQ(events[0].status, "pending");
}

TEST(JsonParserTest, ParsesSettlementSuccessResponse) {
    std::string payload = R"({"transfer_id": "tr_9999", "status": "paid"})";
    auto resp = JsonParser::parse_settlement_response(200, payload);
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.transfer_id, "tr_9999");
    EXPECT_TRUE(resp.error_message.empty());
}

TEST(JsonParserTest, ParsesSettlementErrorResponse) {
    std::string payload = R"({
        "error": {
            "message": "Invalid destination account",
            "type": "invalid_request_error"
        }
    })";
    auto resp = JsonParser::parse_settlement_response(422, payload);
    EXPECT_FALSE(resp.success);
    EXPECT_TRUE(resp.transfer_id.empty());
    EXPECT_NE(resp.error_message.find("Invalid destination account"), std::string::npos);
}

// -------------------------------------------------------------
// WebhookSigner Tests
// -------------------------------------------------------------
TEST(WebhookSignerTest, ComputesAndVerifiesStripeSignature) {
    std::string secret = "whsec_test_secret_key_456";
    int64_t timestamp = 1728000000;
    std::string payload = R"({"id":"evt_123","type":"charge.succeeded"})";

    std::string header = WebhookSigner::compute_signature(secret, timestamp, payload);

    // Header must have t=... and v1=...
    EXPECT_NE(header.find("t=" + std::to_string(timestamp)), std::string::npos);
    EXPECT_NE(header.find("v1="), std::string::npos);

    // Must verify successfully
    EXPECT_TRUE(WebhookSigner::verify_signature(secret, header, payload, 300, timestamp));
}

TEST(WebhookSignerTest, RejectsTamperedPayload) {
    std::string secret = "whsec_test_secret_key_456";
    int64_t timestamp = 1728000000;
    std::string payload = R"({"id":"evt_123","amount":5000})";

    std::string header = WebhookSigner::compute_signature(secret, timestamp, payload);

    std::string tampered = R"({"id":"evt_123","amount":99999})";
    EXPECT_FALSE(WebhookSigner::verify_signature(secret, header, tampered, 300, timestamp));
}

// -------------------------------------------------------------
// PayloadTransformer Tests
// -------------------------------------------------------------
TEST(PayloadTransformerTest, EncodesToStandardFormUrlEncoded) {
    GatewayEvent evt;
    evt.id = "evt_tx_100";
    evt.destination_account = "acct_merchant_55";
    evt.amount_cents = 10000;
    evt.currency = Currency::USD;

    FeeBreakdown fee;
    fee.gross_amount_cents = 10000;
    fee.processing_fee_cents = 320;
    fee.risk_withheld_cents = 0;
    fee.net_payout_cents = 9680;

    auto req = PayloadTransformer::to_settlement_request(evt, fee);
    std::string body = PayloadTransformer::to_form_urlencoded(req);

    EXPECT_NE(body.find("amount=9680"), std::string::npos);
    EXPECT_NE(body.find("currency=usd"), std::string::npos);
    EXPECT_NE(body.find("destination=acct_merchant_55"), std::string::npos);
    EXPECT_NE(body.find("idempotency_key=idemp_evt_tx_100"), std::string::npos);
}
