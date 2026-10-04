#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class EventType {
    PaymentIntentSucceeded,
    ChargeRefunded,
    DisputeCreated,
    Unknown
};

enum class Currency {
    USD,
    EUR,
    GBP,
    Unknown
};

inline std::string currency_to_string(Currency c) {
    switch (c) {
        case Currency::USD: return "usd";
        case Currency::EUR: return "eur";
        case Currency::GBP: return "gbp";
        default: return "unknown";
    }
}

inline Currency string_to_currency(const std::string& str) {
    if (str == "usd" || str == "USD") return Currency::USD;
    if (str == "eur" || str == "EUR") return Currency::EUR;
    if (str == "gbp" || str == "GBP") return Currency::GBP;
    return Currency::Unknown;
}

inline std::string event_type_to_string(EventType t) {
    switch (t) {
        case EventType::PaymentIntentSucceeded: return "payment_intent.succeeded";
        case EventType::ChargeRefunded: return "charge.refunded";
        case EventType::DisputeCreated: return "dispute.created";
        default: return "unknown";
    }
}

inline EventType string_to_event_type(const std::string& str) {
    if (str == "payment_intent.succeeded") return EventType::PaymentIntentSucceeded;
    if (str == "charge.refunded") return EventType::ChargeRefunded;
    if (str == "dispute.created") return EventType::DisputeCreated;
    return EventType::Unknown;
}

struct GatewayEvent {
    std::string id;
    EventType type{EventType::Unknown};
    std::string status;          // "pending", "processed", "failed"
    int64_t created_at{0};       // unix timestamp
    int64_t amount_cents{0};
    Currency currency{Currency::Unknown};
    std::string customer_id;
    std::string destination_account; // e.g. "acct_12345"
    int risk_score{0};           // 0 - 100
};

struct FeeBreakdown {
    int64_t gross_amount_cents{0};
    int64_t processing_fee_cents{0};
    int64_t risk_withheld_cents{0};
    int64_t net_payout_cents{0};
};

struct SettlementRequest {
    std::string event_id;
    std::string destination;
    int64_t net_amount_cents{0};
    Currency currency{Currency::Unknown};
    std::string idempotency_key;
};

struct SettlementResponse {
    bool success{false};
    std::string transfer_id;
    std::string error_message;
    int http_status{0};
};
