#include "report.hpp"
#include "json_util.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

}  // namespace

void write_recovery_report(
    const std::string& output_path,
    const std::string& input_file,
    const std::vector<std::pair<std::string, std::string>>& repairs,
    const XsdValidationResult& xsd_result,
    const VerificationSummary& verification,
    const DatabaseLoadResult& database,
    const std::string& status) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"task\": \"Recover mTLS Trust Graphs into PostgreSQL with C++\",\n";
    json << "  \"timestamp\": \"" << current_timestamp() << "\",\n";
    json << "  \"input_file\": \"" << json_escape(input_file) << "\",\n";
    json << "  \"repairs\": [\n";
    for (std::size_t i = 0; i < repairs.size(); ++i) {
        json << "    {\"action\": \"" << json_escape(repairs[i].first)
             << "\", \"detail\": \"" << json_escape(repairs[i].second) << "\"}";
        if (i + 1 < repairs.size()) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ],\n";
    json << "  \"xsd_validation\": {\n";
    json << "    \"passed\": " << (xsd_result.passed ? "true" : "false") << ",\n";
    json << "    \"schema_url\": \"" << json_escape(xsd_result.schema_url) << "\"";
    if (!xsd_result.errors.empty()) {
        json << ",\n    \"errors\": [";
        for (std::size_t i = 0; i < xsd_result.errors.size(); ++i) {
            json << "\"" << json_escape(xsd_result.errors[i]) << "\"";
            if (i + 1 < xsd_result.errors.size()) {
                json << ", ";
            }
        }
        json << "]";
    }
    json << "\n  },\n";
    json << "  \"verification\": {\n";
    json << "    \"nodes_checked\": " << verification.nodes_checked << ",\n";
    json << "    \"nodes_verified\": " << verification.nodes_verified << ",\n";
    json << "    \"edges_checked\": " << verification.edges_checked << ",\n";
    json << "    \"edges_verified\": " << verification.edges_verified << ",\n";
    json << "    \"rejected_nodes\": [";
    for (std::size_t i = 0; i < verification.rejected_nodes.size(); ++i) {
        json << "\"" << json_escape(verification.rejected_nodes[i]) << "\"";
        if (i + 1 < verification.rejected_nodes.size()) {
            json << ", ";
        }
    }
    json << "],\n";
    json << "    \"rejected_edges\": [";
    for (std::size_t i = 0; i < verification.rejected_edges.size(); ++i) {
        json << "\"" << json_escape(verification.rejected_edges[i]) << "\"";
        if (i + 1 < verification.rejected_edges.size()) {
            json << ", ";
        }
    }
    json << "]\n";
    json << "  },\n";
    json << "  \"database\": {\n";
    json << "    \"nodes_loaded\": " << database.nodes_loaded << ",\n";
    json << "    \"edges_loaded\": " << database.edges_loaded << ",\n";
    json << "    \"copy_duration_ms\": " << database.copy_duration_ms << "\n";
    json << "  },\n";
    json << "  \"status\": \"" << json_escape(status) << "\"\n";
    json << "}\n";

    write_file(output_path, json.str());
}
