#pragma once
#include "types.h"
#include <string>

class PayloadTransformer {
public:
    static SettlementRequest to_settlement_request(const GatewayEvent& event, const FeeBreakdown& fee);
    
    // Encodes the request into standard application/x-www-form-urlencoded format:
    // "amount=<net_amount>&currency=<currency>&destination=<destination>&idempotency_key=<key>"
    static std::string to_form_urlencoded(const SettlementRequest& req);
};
