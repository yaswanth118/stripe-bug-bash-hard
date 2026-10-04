#include "http_client.h"
#include <httplib.h>

HttpClient::HttpClient(std::string base_url, std::string api_key)
    : base_url_(std::move(base_url)), api_key_(std::move(api_key)) {}

HttpClient::HttpResponse HttpClient::get(const std::string& path,
                                         const std::vector<std::pair<std::string, std::string>>& custom_headers) {
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(5, 0);

    httplib::Headers headers;
    headers.emplace("Authorization", "Basic " + api_key_);
    headers.emplace("User-Agent", "Stripe-Integration/1.0");

    for (const auto& [k, v] : custom_headers) {
        headers.emplace(k, v);
    }

    auto res = cli.Get(path, headers);
    if (!res) {
        return {500, "Connection error"};
    }
    return {res->status, res->body};
}

HttpClient::HttpResponse HttpClient::post(const std::string& path,
                                          const std::string& body,
                                          const std::string& content_type,
                                          const std::vector<std::pair<std::string, std::string>>& custom_headers) {
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(5, 0);

    httplib::Headers headers;
    headers.emplace("Authorization", "Bearer " + api_key_);
    headers.emplace("User-Agent", "Stripe-Integration/1.0");

    for (const auto& [k, v] : custom_headers) {
        headers.emplace(k, v);
    }

    auto res = cli.Post(path, headers, body, "application/json");
    if (!res) {
        return {500, "Connection error"};
    }
    return {res->status, res->body};
}
