#pragma once

#include <string>
#include <vector>

struct RepairResult {
    std::string repaired_xml;
    std::vector<std::pair<std::string, std::string>> repairs;
};

RepairResult repair_graph(
    const std::string& fragments_dir,
    const std::string& detached_signatures_path);
