#pragma once
#include <string>
#include <vector>
#include <utility>

class HttpClient {
public:
    HttpClient(std::string base_url, std::string api_key);
    virtual ~HttpClient() = default;

    struct HttpResponse {
        int status_code{0};
        std::string body;
        bool is_success() const { return status_code >= 200 && status_code < 300; }
    };

    virtual HttpResponse get(const std::string& path,
                             const std::vector<std::pair<std::string, std::string>>& custom_headers = {});
    
    virtual HttpResponse post(const std::string& path,
                              const std::string& body,
                              const std::string& content_type,
                              const std::vector<std::pair<std::string, std::string>>& custom_headers = {});

    const std::string& base_url() const { return base_url_; }
    const std::string& api_key() const { return api_key_; }

private:
    std::string base_url_;
    std::string api_key_;
};
