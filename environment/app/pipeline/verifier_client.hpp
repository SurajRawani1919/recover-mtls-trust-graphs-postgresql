#pragma once

#include <string>
#include <vector>

struct NodeRecord {
    std::string node_id;
    std::string service_id;
    std::string label;
    std::string cert_serial;
};

struct EdgeRecord {
    std::string edge_id;
    std::string source_service_id;
    std::string target_service_id;
    std::string policy;
    std::string signature;
};

struct VerificationSummary {
    int nodes_checked = 0;
    int nodes_verified = 0;
    int edges_checked = 0;
    int edges_verified = 0;
    std::vector<std::string> rejected_nodes;
    std::vector<std::string> rejected_edges;
    std::vector<NodeRecord> verified_nodes;
    std::vector<EdgeRecord> verified_edges;
};

VerificationSummary verify_graph(
    const std::string& graphml_xml,
    const std::string& verifier_url);
