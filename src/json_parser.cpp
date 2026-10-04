#include "json_parser.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

std::vector<GatewayEvent> JsonParser::parse_events(const std::string& json_str) {
    std::vector<GatewayEvent> events;
    try {
        auto root = json::parse(json_str);
        if (!root.contains("data") || !root["data"].is_array()) {
            return events;
        }

        for (const auto& item : root["data"]) {
            // BUG: Filters by "completed" instead of "pending" status!
            std::string status = item.value("status", "");
            if (status != "completed") {
                continue;
            }

            GatewayEvent evt;
            // BUG: Lookups use "event_id" instead of "id"
            evt.id = item.value("event_id", "");
            evt.type = string_to_event_type(item.value("type", ""));
            evt.status = status;
            evt.created_at = item.value("created", 0LL);
            evt.amount_cents = item.value("amount", 0LL);
            evt.currency = string_to_currency(item.value("currency", ""));
            evt.customer_id = item.value("customer", "");
            evt.destination_account = item.value("destination", "");
            evt.risk_score = item.value("risk_score", 0);

            events.push_back(evt);
        }
    } catch (const std::exception& e) {
        // Return whatever was parsed
    }
    return events;
}

SettlementResponse JsonParser::parse_settlement_response(int http_status, const std::string& json_str) {
    SettlementResponse resp;
    resp.http_status = http_status;

    try {
        auto j = json::parse(json_str);

        // BUG: Does not check for "error" key first!
        // Unconditionally calls .at("transfer_id"), throwing an unhandled exception when
        // an error object is returned (e.g. {"error": {"message": "Invalid destination account"}})
        resp.transfer_id = j.at("transfer_id").get<std::string>();
        resp.success = (http_status >= 200 && http_status < 300);
    } catch (const std::exception& e) {
        resp.success = false;
        resp.error_message = e.what();
    }
    return resp;
}
