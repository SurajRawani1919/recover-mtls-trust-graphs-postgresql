#include "xsd_validate.hpp"

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

const char* kSchemaUrl = "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd";

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string fetch_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "mtls-recover-graph/1.0");
    curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");

    const CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        throw std::runtime_error(std::string("Failed to fetch schema: ") + curl_easy_strerror(code));
    }
    return response;
}

}  // namespace

XsdValidationResult validate_graphml_xsd(const std::string& graphml_xml) {
    XsdValidationResult result;
    result.schema_url = kSchemaUrl;

    xmlInitParser();
    LIBXML_TEST_VERSION

    const std::string schema_text = fetch_url(kSchemaUrl);
    const std::filesystem::path schema_path = std::filesystem::temp_directory_path() / "graphml.xsd";
    {
        std::ofstream schema_file(schema_path, std::ios::binary);
        schema_file << schema_text;
    }

    xmlDocPtr schema_doc = xmlReadFile(schema_path.string().c_str(), nullptr, XML_PARSE_NONET);
    if (!schema_doc) {
        result.errors.push_back("Unable to parse downloaded GraphML XSD");
        xmlCleanupParser();
        return result;
    }

    xmlSchemaParserCtxtPtr parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
    xmlSchemaPtr schema = xmlSchemaParse(parser_ctxt);
    if (!schema) {
        result.errors.push_back("Unable to compile GraphML XSD");
        xmlSchemaFreeParserCtxt(parser_ctxt);
        xmlFreeDoc(schema_doc);
        xmlCleanupParser();
        return result;
    }

    xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
    xmlDocPtr graph_doc = xmlReadMemory(
        graphml_xml.c_str(),
        static_cast<int>(graphml_xml.size()),
        "repaired.graphml",
        nullptr,
        XML_PARSE_NONET | XML_PARSE_NOBLANKS);

    if (!graph_doc) {
        result.errors.push_back("Unable to parse repaired GraphML for validation");
    } else {
        const int valid = xmlSchemaValidateDoc(valid_ctxt, graph_doc);
        result.passed = valid == 0;
        if (!result.passed) {
            result.errors.push_back("GraphML document failed XSD validation");
        }
        xmlFreeDoc(graph_doc);
    }

    xmlSchemaFreeValidCtxt(valid_ctxt);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(parser_ctxt);
    xmlFreeDoc(schema_doc);
    xmlCleanupParser();
    return result;
}
