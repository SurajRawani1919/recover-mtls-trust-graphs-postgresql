#include "db_ingest.hpp"

#include <libpq-fe.h>

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {

void copy_rows(
    PGconn* conn,
    const std::string& table,
    const std::vector<std::string>& columns,
    const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) {
        return;
    }

    std::ostringstream copy_sql;
    copy_sql << "COPY " << table << " (";
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            copy_sql << ", ";
        }
        copy_sql << columns[i];
    }
    copy_sql << ") FROM STDIN WITH (FORMAT csv)";

    PGresult* res = PQexec(conn, copy_sql.str().c_str());
    if (PQresultStatus(res) != PGRES_COPY_IN) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        throw std::runtime_error("COPY command failed: " + error);
    }
    PQclear(res);

    for (const auto& row : rows) {
        std::ostringstream line;
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                line << ',';
            }
            const std::string& value = row[i];
            bool needs_quotes = value.find(',') != std::string::npos ||
                                value.find('"') != std::string::npos ||
                                value.find('\n') != std::string::npos;
            if (needs_quotes) {
                line << '"';
                for (char ch : value) {
                    if (ch == '"') {
                        line << "\"\"";
                    } else {
                        line << ch;
                    }
                }
                line << '"';
            } else {
                line << value;
            }
        }
        line << '\n';

        const std::string data = line.str();
        if (PQputCopyData(conn, data.c_str(), static_cast<int>(data.size())) != 1) {
            throw std::runtime_error("PQputCopyData failed");
        }
    }

    if (PQputCopyEnd(conn, nullptr) != 1) {
        throw std::runtime_error("PQputCopyEnd failed");
    }

    res = PQgetResult(conn);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(res);
        throw std::runtime_error("COPY ingest failed: " + error);
    }
    PQclear(res);
}

}  // namespace

DatabaseLoadResult ingest_verified_records(
    const std::string& database_url,
    const std::vector<NodeRecord>& nodes,
    const std::vector<EdgeRecord>& edges) {
    DatabaseLoadResult result;
    const auto started = std::chrono::steady_clock::now();

    PGconn* conn = PQconnectdb(database_url.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        std::string error = PQerrorMessage(conn);
        PQfinish(conn);
        throw std::runtime_error("PostgreSQL connection failed: " + error);
    }

    PGresult* truncate_res = PQexec(conn, "TRUNCATE trust_edges, services");
    if (PQresultStatus(truncate_res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn);
        PQclear(truncate_res);
        PQfinish(conn);
        throw std::runtime_error("Failed to truncate tables: " + error);
    }
    PQclear(truncate_res);

    std::vector<std::vector<std::string>> node_rows;
    for (const auto& node : nodes) {
        node_rows.push_back({
            node.service_id,
            node.label,
            node.cert_serial,
            "platform-issuer-v1",
        });
    }

    copy_rows(conn, "services", {"service_id", "label", "cert_serial", "issuer_key_id"}, node_rows);
    result.nodes_loaded = static_cast<int>(node_rows.size());

    std::vector<std::vector<std::string>> edge_rows;
    for (const auto& edge : edges) {
        edge_rows.push_back({
            edge.edge_id,
            edge.source_service_id,
            edge.target_service_id,
            edge.policy,
            edge.signature,
        });
    }

    copy_rows(
        conn,
        "trust_edges",
        {"edge_id", "source_service_id", "target_service_id", "policy", "signature"},
        edge_rows);
    result.edges_loaded = static_cast<int>(edge_rows.size());

    PQfinish(conn);

    const auto ended = std::chrono::steady_clock::now();
    result.copy_duration_ms =
        std::chrono::duration<double, std::milli>(ended - started).count();
    return result;
}
