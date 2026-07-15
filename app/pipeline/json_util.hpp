#pragma once

#include <map>
#include <string>

std::string read_file(const std::string& path);
void write_file(const std::string& path, const std::string& content);
std::map<std::string, std::string> parse_detached_signatures(const std::string& json_text);
std::string json_escape(const std::string& value);
