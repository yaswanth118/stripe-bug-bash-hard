#include "payload_transformer.h"
#include <sstream>

SettlementRequest PayloadTransformer::to_settlement_request(const GatewayEvent& event, const FeeBreakdown& fee) {
    SettlementRequest req;
    req.event_id = event.id;
    req.destination = event.destination_account;
    req.net_amount_cents = fee.net_payout_cents;
    req.currency = event.currency;
    req.idempotency_key = "idemp_" + event.id;
    return req;
}

std::string PayloadTransformer::to_form_urlencoded(const SettlementRequest& req) {
    std::ostringstream ss;
    ss << "amount=" << req.net_amount_cents;
    ss << "&currency=" << currency_to_string(req.currency);
    
    // BUG 1: The settlement API expects "destination", but here it writes "destination_account"
    ss << "&destination_account=" << req.destination;

    // BUG 2: Forgets to include "idempotency_key=" in the encoded body!
    // The settlement endpoint requires idempotency_key and returns 400 if missing.

    return ss.str();
}
