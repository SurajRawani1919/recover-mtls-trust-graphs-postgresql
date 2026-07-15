#include "verifier_client.hpp"
#include "json_util.hpp"

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string http_post_json(const std::string& url, const std::string& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    const CURLcode code = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        throw std::runtime_error(std::string("HTTP request failed: ") + curl_easy_strerror(code));
    }
    return response;
}

bool json_bool_field(const std::string& json, const std::string& field) {
    const std::string pattern = "\"" + field + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }
    auto true_pos = json.find("true", pos);
    auto false_pos = json.find("false", pos);
    if (true_pos == std::string::npos) {
        return false;
    }
    if (false_pos == std::string::npos) {
        return true;
    }
    return true_pos < false_pos;
}

std::string json_string_field(const std::string& json, const std::string& field) {
    const std::string pattern = "\"" + field + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) {
        return "";
    }
    auto colon = json.find(':', pos);
    auto start = json.find('"', colon);
    if (start == std::string::npos) {
        return "";
    }
    auto end = json.find('"', start + 1);
    if (end == std::string::npos) {
        return "";
    }
    return json.substr(start + 1, end - start - 1);
}

std::string get_data_value(xmlNodePtr node, const std::string& key_id) {
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, BAD_CAST "data") == 0) {
            xmlChar* key_attr = xmlGetProp(child, BAD_CAST "key");
            if (key_attr && key_id == reinterpret_cast<const char*>(key_attr)) {
                xmlChar* content = xmlNodeGetContent(child);
                std::string value = content ? reinterpret_cast<const char*>(content) : "";
                xmlFree(content);
                xmlFree(key_attr);
                return value;
            }
            xmlFree(key_attr);
        }
    }
    return "";
}

}  // namespace

VerificationSummary verify_graph(
    const std::string& graphml_xml,
    const std::string& verifier_url) {
    VerificationSummary summary;

    xmlInitParser();
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadMemory(
        graphml_xml.c_str(),
        static_cast<int>(graphml_xml.size()),
        "repaired.graphml",
        nullptr,
        XML_PARSE_NONET | XML_PARSE_NOBLANKS);

    if (!doc) {
        xmlCleanupParser();
        throw std::runtime_error("Failed to parse repaired GraphML for verification");
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr graph = nullptr;
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, BAD_CAST "graph") == 0) {
            graph = child;
            break;
        }
    }

    std::unordered_map<std::string, std::string> node_service_ids;
    std::unordered_map<std::string, NodeRecord> node_records;

    for (xmlNodePtr child = graph->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST "node") != 0) {
            continue;
        }

        xmlChar* id_attr = xmlGetProp(child, BAD_CAST "id");
        if (!id_attr) {
            continue;
        }
        std::string node_id = reinterpret_cast<const char*>(id_attr);
        xmlFree(id_attr);

        NodeRecord record;
        record.node_id = node_id;
        record.service_id = get_data_value(child, "d_svc");
        record.label = get_data_value(child, "d_label");
        record.cert_serial = get_data_value(child, "d_serial");
        node_service_ids[node_id] = record.service_id;
        node_records[node_id] = record;

        summary.nodes_checked++;
        const std::string key_payload =
            "{\"cert_serial\":\"" + json_escape(record.cert_serial) + "\"}";
        const std::string key_response =
            http_post_json(verifier_url + "/verify/key", key_payload);
        const std::string revocation_response = http_post_json(
            verifier_url + "/verify/revocation",
            key_payload);

        const bool key_valid = json_bool_field(key_response, "valid");
        const bool revoked = json_bool_field(revocation_response, "revoked");
        if (key_valid && !revoked) {
            summary.nodes_verified++;
            NodeRecord verified = record;
            verified.label = record.label;
            summary.verified_nodes.push_back(verified);
        } else {
            summary.rejected_nodes.push_back(record.service_id);
        }
    }

    for (xmlNodePtr child = graph->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST "edge") != 0) {
            continue;
        }

        xmlChar* id_attr = xmlGetProp(child, BAD_CAST "id");
        xmlChar* source_attr = xmlGetProp(child, BAD_CAST "source");
        xmlChar* target_attr = xmlGetProp(child, BAD_CAST "target");
        if (!id_attr || !source_attr || !target_attr) {
            xmlFree(id_attr);
            xmlFree(source_attr);
            xmlFree(target_attr);
            continue;
        }

        EdgeRecord edge;
        edge.edge_id = reinterpret_cast<const char*>(id_attr);
        const std::string source_node = reinterpret_cast<const char*>(source_attr);
        const std::string target_node = reinterpret_cast<const char*>(target_attr);
        edge.source_service_id = node_service_ids[source_node];
        edge.target_service_id = node_service_ids[target_node];
        edge.policy = get_data_value(child, "d_policy");
        edge.signature = get_data_value(child, "d_sig");

        xmlFree(id_attr);
        xmlFree(source_attr);
        xmlFree(target_attr);

        summary.edges_checked++;

        const bool source_ok = std::find_if(
                                   summary.verified_nodes.begin(),
                                   summary.verified_nodes.end(),
                                   [&](const NodeRecord& node) {
                                       return node.service_id == edge.source_service_id;
                                   }) != summary.verified_nodes.end();
        const bool target_ok = std::find_if(
                                   summary.verified_nodes.begin(),
                                   summary.verified_nodes.end(),
                                   [&](const NodeRecord& node) {
                                       return node.service_id == edge.target_service_id;
                                   }) != summary.verified_nodes.end();

        if (!source_ok || !target_ok) {
            summary.rejected_edges.push_back(edge.edge_id);
            continue;
        }

        std::ostringstream payload;
        payload << "{"
                << "\"source\":\"" << json_escape(edge.source_service_id) << "\","
                << "\"target\":\"" << json_escape(edge.target_service_id) << "\","
                << "\"policy\":\"" << json_escape(edge.policy) << "\","
                << "\"signature\":\"" << json_escape(edge.signature) << "\""
                << "}";

        const std::string edge_response =
            http_post_json(verifier_url + "/verify/edge", payload.str());
        if (json_bool_field(edge_response, "valid")) {
            summary.edges_verified++;
            summary.verified_edges.push_back(edge);
        } else {
            summary.rejected_edges.push_back(edge.edge_id);
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return summary;
}
