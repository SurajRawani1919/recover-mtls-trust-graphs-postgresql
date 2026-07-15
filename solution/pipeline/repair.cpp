#include "repair.hpp"
#include "json_util.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

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

void set_data_value(xmlNodePtr node, const std::string& key_id, const std::string& value) {
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, BAD_CAST "data") == 0) {
            xmlChar* key_attr = xmlGetProp(child, BAD_CAST "key");
            if (key_attr && key_id == reinterpret_cast<const char*>(key_attr)) {
                xmlNodeSetContent(child, BAD_CAST value.c_str());
                xmlFree(key_attr);
                return;
            }
            xmlFree(key_attr);
        }
    }

    xmlNodePtr data_node = xmlNewChild(node, nullptr, BAD_CAST "data", BAD_CAST value.c_str());
    xmlSetProp(data_node, BAD_CAST "key", BAD_CAST key_id.c_str());
}

void ensure_keys(
    xmlNodePtr graphml,
    xmlNodePtr graph,
    std::vector<std::pair<std::string, std::string>>& repairs) {
    struct KeySpec {
        const char* id;
        const char* target;
        const char* name;
        const char* type;
    };

    const KeySpec specs[] = {
        {"d_svc", "node", "service_id", "string"},
        {"d_label", "node", "label", "string"},
        {"d_serial", "node", "cert_serial", "string"},
        {"d_version", "node", "version", "int"},
        {"d_policy", "edge", "policy", "string"},
        {"d_sig", "edge", "signature", "string"},
    };

    bool added = false;
    for (const auto& spec : specs) {
        bool exists = false;
        for (xmlNodePtr child = graphml->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, BAD_CAST "key") == 0) {
                xmlChar* id_attr = xmlGetProp(child, BAD_CAST "id");
                if (id_attr && spec.id == reinterpret_cast<const char*>(id_attr)) {
                    exists = true;
                }
                xmlFree(id_attr);
            }
        }
        if (!exists) {
            xmlNodePtr key_node = xmlNewNode(nullptr, BAD_CAST "key");
            xmlSetProp(key_node, BAD_CAST "id", BAD_CAST spec.id);
            xmlSetProp(key_node, BAD_CAST "for", BAD_CAST spec.target);
            xmlSetProp(key_node, BAD_CAST "attr.name", BAD_CAST spec.name);
            xmlSetProp(key_node, BAD_CAST "attr.type", BAD_CAST spec.type);
            xmlAddPrevSibling(graph, key_node);
            added = true;
        }
    }

    if (added) {
        repairs.emplace_back("inject_keys", "Restored missing GraphML key declarations");
    }
}

void deduplicate_nodes(xmlNodePtr graph, std::vector<std::pair<std::string, std::string>>& repairs) {
    struct NodeInfo {
        xmlNodePtr node;
        int version;
        int order;
    };

    std::map<std::string, NodeInfo> best;
    std::vector<xmlNodePtr> to_remove;
    int order = 0;

    for (xmlNodePtr child = graph->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST "node") != 0) {
            continue;
        }
        std::string service_id = get_data_value(child, "d_svc");
        if (service_id.empty()) {
            continue;
        }
        std::string version_text = get_data_value(child, "d_version");
        int version = version_text.empty() ? 0 : std::stoi(version_text);
        const int current_order = order++;

        auto found = best.find(service_id);
        if (found == best.end()) {
            best[service_id] = {child, version, current_order};
        } else {
            bool replace = version > found->second.version ||
                           (version == found->second.version && current_order > found->second.order);
            if (replace) {
                to_remove.push_back(found->second.node);
                found->second = {child, version, current_order};
            } else {
                to_remove.push_back(child);
            }
        }
    }

    for (xmlNodePtr node : to_remove) {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
    }

    if (!to_remove.empty()) {
        repairs.emplace_back("deduplicate_nodes", "Collapsed duplicate service_id nodes by version");
    }
}

void reattach_signatures(
    xmlNodePtr graph,
    const std::map<std::string, std::string>& signatures,
    std::vector<std::pair<std::string, std::string>>& repairs) {
    bool attached = false;
    for (xmlNodePtr child = graph->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST "edge") != 0) {
            continue;
        }
        xmlChar* id_attr = xmlGetProp(child, BAD_CAST "id");
        if (!id_attr) {
            continue;
        }
        std::string edge_id = reinterpret_cast<const char*>(id_attr);
        xmlFree(id_attr);

        auto found = signatures.find(edge_id);
        if (found == signatures.end()) {
            continue;
        }
        if (get_data_value(child, "d_sig").empty()) {
            set_data_value(child, "d_sig", found->second);
            attached = true;
        }
    }

    if (attached) {
        repairs.emplace_back("reattach_signatures", "Bound detached Ed25519 signatures to edges");
    }
}

}  // namespace

RepairResult repair_graph(
    const std::string& fragments_dir,
    const std::string& detached_signatures_path) {
    RepairResult result;

    const std::string head = read_file(fragments_dir + "/head.graphml");
    const std::string tail = read_file(fragments_dir + "/tail.graphml");
    const std::string merged = head + tail;
    result.repairs.emplace_back("merge_fragments", "Merged head.graphml and tail.graphml");

    xmlInitParser();
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadMemory(
        merged.c_str(),
        static_cast<int>(merged.size()),
        "merged.graphml",
        nullptr,
        XML_PARSE_NONET | XML_PARSE_NOBLANKS);

    if (!doc) {
        xmlCleanupParser();
        throw std::runtime_error("Failed to parse merged GraphML");
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root || xmlStrcmp(root->name, BAD_CAST "graphml") != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        throw std::runtime_error("Missing graphml root element");
    }

    xmlNodePtr graph = nullptr;
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, BAD_CAST "graph") == 0) {
            graph = child;
            break;
        }
    }

    if (!graph) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        throw std::runtime_error("Missing graph element");
    }

    ensure_keys(root, graph, result.repairs);

    deduplicate_nodes(graph, result.repairs);

    const auto signatures = parse_detached_signatures(read_file(detached_signatures_path));
    reattach_signatures(graph, signatures, result.repairs);

    xmlChar* xml_buf = nullptr;
    int xml_len = 0;
    xmlDocDumpFormatMemoryEnc(doc, &xml_buf, &xml_len, "UTF-8", 1);
    if (!xml_buf) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        throw std::runtime_error("Failed to serialize repaired GraphML");
    }

    result.repaired_xml.assign(reinterpret_cast<const char*>(xml_buf), static_cast<std::size_t>(xml_len));
    xmlFree(xml_buf);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return result;
}
