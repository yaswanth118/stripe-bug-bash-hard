#include "webhook_signer.h"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <ctime>

#if defined(__APPLE__)
#include <CommonCrypto/CommonHMAC.h>

static std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char result[CC_SHA256_DIGEST_LENGTH];
    CCHmac(kCCHmacAlgSHA256, key.data(), key.size(), data.data(), data.size(), result);
    std::ostringstream ss;
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(result[i]);
    }
    return ss.str();
}
#else
// Fallback for non-Apple platforms using a simple HMAC-SHA256
#include <openssl/hmac.h>
static std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char result[32];
    unsigned int len = 32;
    HMAC(EVP_sha256(), key.data(), key.size(),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         result, &len);
    std::ostringstream ss;
    for (unsigned int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(result[i]);
    }
    return ss.str();
}
#endif

std::string WebhookSigner::compute_signature(const std::string& secret, int64_t timestamp, const std::string& payload) {
    std::string signature = hmac_sha256(secret, payload);
    return "t=" + std::to_string(timestamp) + ",v1=" + signature;
}

bool WebhookSigner::verify_signature(const std::string& secret, const std::string& signature_header,
                                     const std::string& payload, int64_t tolerance_seconds,
                                     int64_t current_time) {
    int64_t timestamp = 0;
    std::string expected_sig;

    std::stringstream ss(signature_header);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.rfind("t=", 0) == 0) {
            try {
                timestamp = std::stoll(item.substr(2));
            } catch (...) {
                return false;
            }
        } else if (item.rfind("v1=", 0) == 0) {
            expected_sig = item.substr(3);
        }
    }

    if (timestamp == 0 || expected_sig.empty()) {
        return false;
    }

    if (current_time > 0 && tolerance_seconds > 0) {
        if (std::abs(current_time - timestamp) > tolerance_seconds) {
            return false;
        }
    }

    std::string computed = hmac_sha256(secret, std::to_string(timestamp) + "." + payload);
    return computed == expected_sig;
}
