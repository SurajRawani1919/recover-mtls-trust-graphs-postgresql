#include <curl/curl.h>
#include <iostream>
#include <string>

#include "db_ingest.hpp"
#include "json_util.hpp"
#include "repair.hpp"
#include "report.hpp"
#include "verifier_client.hpp"
#include "xsd_validate.hpp"

namespace {

struct Options {
    std::string fragments_dir = "/app/graph/fragments";
    std::string detached_signatures = "/app/graph/detached_signatures.json";
    std::string output_graph = "/app/output/repaired.graphml";
    std::string output_report = "/app/output/report.json";
    std::string verifier_url = "http://127.0.0.1:5001";
    std::string database_url = "postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust";
};

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const std::string& flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for " + flag);
            }
            return argv[++i];
        };

        if (arg == "--fragments-dir") {
            options.fragments_dir = require_value(arg);
        } else if (arg == "--detached-signatures") {
            options.detached_signatures = require_value(arg);
        } else if (arg == "--output-graph") {
            options.output_graph = require_value(arg);
        } else if (arg == "--output-report") {
            options.output_report = require_value(arg);
        } else if (arg == "--verifier-url") {
            options.verifier_url = require_value(arg);
        } else if (arg == "--database-url") {
            options.database_url = require_value(arg);
        }
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        const Options options = parse_args(argc, argv);

        const RepairResult repair = repair_graph(options.fragments_dir, options.detached_signatures);
        write_file(options.output_graph, repair.repaired_xml);

        const XsdValidationResult xsd_result = validate_graphml_xsd(repair.repaired_xml);
        if (!xsd_result.passed) {
            DatabaseLoadResult empty_db;
            VerificationSummary empty_verification;
            write_recovery_report(
                options.output_report,
                options.fragments_dir,
                repair.repairs,
                xsd_result,
                empty_verification,
                empty_db,
                "failed");
            curl_global_cleanup();
            return 1;
        }

        VerificationSummary verification = verify_graph(repair.repaired_xml, options.verifier_url);
        DatabaseLoadResult database = ingest_verified_records(
            options.database_url,
            verification.verified_nodes,
            verification.verified_edges);

        std::string status = "success";
        if (!verification.rejected_nodes.empty() || !verification.rejected_edges.empty()) {
            status = "partial";
        }
        if (database.nodes_loaded == 0) {
            status = "failed";
        }

        write_recovery_report(
            options.output_report,
            options.fragments_dir,
            repair.repairs,
            xsd_result,
            verification,
            database,
            status);

        curl_global_cleanup();
        return status == "failed" ? 1 : 0;
    } catch (const std::exception& ex) {
        std::cerr << "recover_graph failed: " << ex.what() << '\n';
        curl_global_cleanup();
        return 1;
    }
}
