#include "event_pipeline.h"
#include "json_parser.h"
#include "fee_calculator.h"
#include "payload_transformer.h"
#include <iostream>

EventPipeline::EventPipeline(std::shared_ptr<HttpClient> client)
    : client_(std::move(client)) {}

EventPipeline::PipelineResult EventPipeline::run() {
    PipelineResult result;

    // 1. Fetch pending gateway events
    auto fetch_res = client_->get("/v1/gateway/events?status=pending");
    if (!fetch_res.is_success()) {
        return result;
    }

    // 2. Parse events
    auto events = JsonParser::parse_events(fetch_res.body);
    result.total_events_fetched = events.size();

    // 3. Process each event
    for (const auto& event : events) {
        // Calculate fees
        auto fee = FeeCalculator::calculate(event.amount_cents, event.risk_score);

        // Transform into settlement request
        auto settlement_req = PayloadTransformer::to_settlement_request(event, fee);
        auto form_body = PayloadTransformer::to_form_urlencoded(settlement_req);

        // POST to settlement endpoint
        auto settle_res = client_->post("/v1/transfers/settle", form_body, "application/x-www-form-urlencoded");
        auto parsed_settle = JsonParser::parse_settlement_response(settle_res.status_code, settle_res.body);

        if (parsed_settle.success) {
            result.successful_settlements++;
            result.settled_transfer_ids.push_back(parsed_settle.transfer_id);

            std::string ack_path = "/v1/events/" + event.id + "/ack";
            client_->post(ack_path, R"({"ack":true})", "application/json");

            break;
        } else {
            result.failed_settlements++;
        }
    }

    return result;
}
