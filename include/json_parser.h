#pragma once
#include "types.h"
#include <vector>
#include <string>

class JsonParser {
public:
    static std::vector<GatewayEvent> parse_events(const std::string& json_str);
    static SettlementResponse parse_settlement_response(int http_status, const std::string& json_str);
};
