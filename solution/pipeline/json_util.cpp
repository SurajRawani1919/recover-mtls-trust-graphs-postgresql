#include "json_util.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to read file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void write_file(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to write file: " + path);
    }
    out << content;
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::map<std::string, std::string> parse_detached_signatures(const std::string& json_text) {
    std::map<std::string, std::string> signatures;
    const std::string marker = "\"signatures\"";
    auto marker_pos = json_text.find(marker);
    if (marker_pos == std::string::npos) {
        return signatures;
    }

    std::size_t pos = marker_pos;
    while (true) {
        auto key_pos = json_text.find('"', pos);
        if (key_pos == std::string::npos) {
            break;
        }
        auto key_end = json_text.find('"', key_pos + 1);
        if (key_end == std::string::npos) {
            break;
        }
        std::string key = json_text.substr(key_pos + 1, key_end - key_pos - 1);
        if (key == "signatures") {
            pos = key_end + 1;
            continue;
        }

        auto colon = json_text.find(':', key_end);
        if (colon == std::string::npos) {
            break;
        }
        auto value_start = json_text.find('"', colon);
        if (value_start == std::string::npos) {
            break;
        }
        auto value_end = json_text.find('"', value_start + 1);
        if (value_end == std::string::npos) {
            break;
        }
        std::string value = json_text.substr(value_start + 1, value_end - value_start - 1);
        if (key.rfind("e", 0) == 0) {
            signatures[key] = value;
        }
        pos = value_end + 1;
        if (json_text.find('}', pos) < json_text.find('"', pos)) {
            break;
        }
    }
    return signatures;
}
