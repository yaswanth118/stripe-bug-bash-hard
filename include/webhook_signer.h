#pragma once
#include <string>
#include <cstdint>

class WebhookSigner {
public:
    // Computes Stripe-style signature header:
    // Header format: "t={timestamp},v1={hex_hmac}"
    // where hex_hmac = HMAC_SHA256(secret, timestamp + "." + payload)
    static std::string compute_signature(const std::string& secret, int64_t timestamp, const std::string& payload);

    // Verifies whether header signature matches payload
    static bool verify_signature(const std::string& secret, const std::string& signature_header,
                                 const std::string& payload, int64_t tolerance_seconds = 300,
                                 int64_t current_time = 0);
};
