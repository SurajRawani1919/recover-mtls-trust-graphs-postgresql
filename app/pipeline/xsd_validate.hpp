#pragma once

#include <string>
#include <vector>

struct XsdValidationResult {
    bool passed = false;
    std::string schema_url;
    std::vector<std::string> errors;
};

XsdValidationResult validate_graphml_xsd(const std::string& graphml_xml);
