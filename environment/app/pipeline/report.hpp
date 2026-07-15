#pragma once

#include <string>
#include <vector>

#include "db_ingest.hpp"
#include "verifier_client.hpp"
#include "xsd_validate.hpp"

void write_recovery_report(
    const std::string& output_path,
    const std::string& input_file,
    const std::vector<std::pair<std::string, std::string>>& repairs,
    const XsdValidationResult& xsd_result,
    const VerificationSummary& verification,
    const DatabaseLoadResult& database,
    const std::string& status);
