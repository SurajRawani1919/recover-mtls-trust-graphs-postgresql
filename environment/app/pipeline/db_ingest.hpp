#pragma once

#include <string>
#include <vector>

#include "verifier_client.hpp"

struct DatabaseLoadResult {
    int nodes_loaded = 0;
    int edges_loaded = 0;
    double copy_duration_ms = 0.0;
};

DatabaseLoadResult ingest_verified_records(
    const std::string& database_url,
    const std::vector<NodeRecord>& nodes,
    const std::vector<EdgeRecord>& edges);
