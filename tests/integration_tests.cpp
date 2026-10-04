#include <gtest/gtest.h>
#include "mock_gateway_server.h"
#include "http_client.h"
#include "event_pipeline.h"
#include <random>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Pick random ephemeral port between 18000 and 19000
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(18000, 19000);
        port_ = dis(gen);

        api_key_ = "sk_test_integration_token_xyz";
        server_ = std::make_unique<MockGatewayServer>(api_key_);
        server_->start(port_);

        base_url_ = "http://127.0.0.1:" + std::to_string(port_);
        client_ = std::make_shared<HttpClient>(base_url_, api_key_);
    }

    void TearDown() override {
        server_->stop();
    }

    int port_{0};
    std::string api_key_;
    std::string base_url_;
    std::unique_ptr<MockGatewayServer> server_;
    std::shared_ptr<HttpClient> client_;
};

TEST_F(IntegrationTest, RejectsMissingOrIncorrectBearerToken) {
    // A client configured with wrong key or without Bearer header should get 401
    auto bad_client = std::make_shared<HttpClient>(base_url_, "wrong_token");
    auto res = bad_client->get("/v1/gateway/events");
    EXPECT_EQ(res.status_code, 401);

    // Our client should authenticate with Bearer token
    auto good_res = client_->get("/v1/gateway/events");
    EXPECT_EQ(good_res.status_code, 200);
}

TEST_F(IntegrationTest, RejectsInvalidContentTypeOnSettle) {
    // Settling requires application/x-www-form-urlencoded
    auto res = client_->post("/v1/transfers/settle", "amount=100", "application/x-www-form-urlencoded");
    // If client actually sends application/x-www-form-urlencoded, server will proceed to validate params (e.g. 400 for missing fields)
    // But if client ignores content_type and sends application/json, server returns 415
    EXPECT_NE(res.status_code, 415);
}

TEST_F(IntegrationTest, PipelineProcessesAllPendingEvents) {
    std::string events_json = R"({
        "object": "list",
        "data": [
            {
                "id": "evt_001",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000001,
                "amount": 5000,
                "currency": "usd",
                "customer": "cus_1",
                "destination": "acct_dest_1",
                "risk_score": 10
            },
            {
                "id": "evt_002",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000002,
                "amount": 25000,
                "currency": "usd",
                "customer": "cus_2",
                "destination": "acct_dest_2",
                "risk_score": 20
            },
            {
                "id": "evt_003",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000003,
                "amount": 8000,
                "currency": "usd",
                "customer": "cus_3",
                "destination": "acct_dest_3",
                "risk_score": 85
            },
            {
                "id": "evt_completed_999",
                "type": "payment_intent.succeeded",
                "status": "processed",
                "amount": 1000,
                "currency": "usd",
                "customer": "cus_old",
                "destination": "acct_dest_old",
                "risk_score": 0
            }
        ]
    })";

    server_->set_events_json(events_json);

    EventPipeline pipeline(client_);
    auto result = pipeline.run();

    // Exactly 3 pending events should be fetched
    EXPECT_EQ(result.total_events_fetched, 3u);
    // All 3 pending events should be successfully settled
    EXPECT_EQ(result.successful_settlements, 3u);
    EXPECT_EQ(result.failed_settlements, 0u);
    EXPECT_EQ(result.settled_transfer_ids.size(), 3u);

    // All 3 settled events must be acknowledged to the upstream gateway
    auto acks = server_->get_acknowledged_events();
    EXPECT_EQ(acks.size(), 3u);
}

TEST_F(IntegrationTest, PipelineHandlesSettlementFailureGracefully) {
    std::string events_json = R"({
        "object": "list",
        "data": [
            {
                "id": "evt_valid",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000001,
                "amount": 5000,
                "currency": "usd",
                "customer": "cus_1",
                "destination": "acct_dest_valid",
                "risk_score": 10
            },
            {
                "id": "evt_invalid_dest",
                "type": "payment_intent.succeeded",
                "status": "pending",
                "created": 1728000002,
                "amount": 5000,
                "currency": "usd",
                "customer": "cus_2",
                "destination": "acct_invalid",
                "risk_score": 10
            }
        ]
    })";

    server_->set_events_json(events_json);

    EventPipeline pipeline(client_);
    auto result = pipeline.run();

    EXPECT_EQ(result.total_events_fetched, 2u);
    EXPECT_EQ(result.successful_settlements, 1u);
    EXPECT_EQ(result.failed_settlements, 1u);

    // Only the successful one should be ACKed
    auto acks = server_->get_acknowledged_events();
    ASSERT_EQ(acks.size(), 1u);
    EXPECT_EQ(acks[0], "evt_valid");
}
