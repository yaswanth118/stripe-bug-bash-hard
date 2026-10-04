#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>

class MockGatewayServer {
public:
    explicit MockGatewayServer(std::string expected_api_key = "sk_test_mock_key_999")
        : api_key_(std::move(expected_api_key)) {
        setup_routes();
    }

    ~MockGatewayServer() {
        stop();
    }

    void start(int port) {
        port_ = port;
        server_thread_ = std::thread([this]() {
            server_.listen("127.0.0.1", port_);
        });
        server_.wait_until_ready();
    }

    void stop() {
        if (server_.is_running()) {
            server_.stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    void set_events_json(std::string json_body) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_json_ = std::move(json_body);
    }

    std::vector<std::string> get_acknowledged_events() {
        std::lock_guard<std::mutex> lock(mutex_);
        return acknowledged_events_;
    }

    std::vector<std::string> get_received_content_types() {
        std::lock_guard<std::mutex> lock(mutex_);
        return received_content_types_;
    }

    int port() const { return port_; }

private:
    void setup_routes() {
        // GET /v1/gateway/events
        server_.Get("/v1/gateway/events", [this](const httplib::Request& req, httplib::Response& res) {
            std::string auth = req.get_header_value("Authorization");
            if (auth != "Bearer " + api_key_) {
                res.status = 401;
                res.set_content(R"({"error": {"message": "Invalid or missing Bearer API token"}})", "application/json");
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            res.status = 200;
            res.set_content(events_json_, "application/json");
        });

        // POST /v1/transfers/settle
        server_.Post("/v1/transfers/settle", [this](const httplib::Request& req, httplib::Response& res) {
            std::string auth = req.get_header_value("Authorization");
            if (auth != "Bearer " + api_key_) {
                res.status = 401;
                res.set_content(R"({"error": {"message": "Unauthorized"}})", "application/json");
                return;
            }

            std::string content_type = req.get_header_value("Content-Type");
            {
                std::lock_guard<std::mutex> lock(mutex_);
                received_content_types_.push_back(content_type);
            }

            // Expects application/x-www-form-urlencoded
            if (content_type != "application/x-www-form-urlencoded") {
                res.status = 415;
                res.set_content(R"({"error": {"message": "Unsupported Media Type: expected application/x-www-form-urlencoded"}})", "application/json");
                return;
            }

            // Parse urlencoded parameters
            std::map<std::string, std::string> params;
            std::stringstream ss(req.body);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                auto eq = pair.find('=');
                if (eq != std::string::npos) {
                    params[pair.substr(0, eq)] = pair.substr(eq + 1);
                }
            }

            if (!params.count("idempotency_key") || params["idempotency_key"].empty()) {
                res.status = 400;
                res.set_content(R"({"error": {"message": "Missing required parameter: idempotency_key"}})", "application/json");
                return;
            }

            if (!params.count("destination") || params["destination"].empty()) {
                res.status = 400;
                res.set_content(R"({"error": {"message": "Missing required parameter: destination"}})", "application/json");
                return;
            }

            if (params["destination"] == "acct_invalid") {
                res.status = 422;
                res.set_content(R"({"error": {"message": "Invalid destination account: acct_invalid"}})", "application/json");
                return;
            }

            static int transfer_counter = 100;
            std::string tr_id = "tr_" + std::to_string(++transfer_counter);

            nlohmann::json resp;
            resp["transfer_id"] = tr_id;
            resp["status"] = "paid";
            resp["amount"] = std::stoll(params["amount"]);
            resp["destination"] = params["destination"];

            res.status = 200;
            res.set_content(resp.dump(), "application/json");
        });

        // POST /v1/gateway/events/:id/ack
        server_.Post(R"(/v1/gateway/events/([^/]+)/ack)", [this](const httplib::Request& req, httplib::Response& res) {
            std::string event_id = req.matches[1];
            {
                std::lock_guard<std::mutex> lock(mutex_);
                acknowledged_events_.push_back(event_id);
            }
            res.status = 200;
            res.set_content(R"({"acknowledged": true})", "application/json");
        });
    }

    std::string api_key_;
    int port_{0};
    httplib::Server server_;
    std::thread server_thread_;
    std::mutex mutex_;
    std::string events_json_;
    std::vector<std::string> acknowledged_events_;
    std::vector<std::string> received_content_types_;
};
