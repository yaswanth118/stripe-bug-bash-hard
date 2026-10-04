#pragma once
#include "types.h"
#include "http_client.h"
#include <memory>
#include <vector>

class EventPipeline {
public:
    explicit EventPipeline(std::shared_ptr<HttpClient> client);

    struct PipelineResult {
        size_t total_events_fetched{0};
        size_t successful_settlements{0};
        size_t failed_settlements{0};
        std::vector<std::string> settled_transfer_ids;
    };

    PipelineResult run();

private:
    std::shared_ptr<HttpClient> client_;
};
