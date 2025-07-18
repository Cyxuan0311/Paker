#include "Paker/utils.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string get_project_name() {
    std::string name = fs::current_path().filename().string();
    if (name.empty()) name = "myproject";
    return name;
}

std::string get_json_file() {
    return get_project_name() + ".json";
}

std::pair<std::string, std::string> parse_name_version(const std::string& input) {
    auto pos = input.find('@');
    if (pos == std::string::npos) return {input, ""};
    return {input.substr(0, pos), input.substr(pos + 1)};
} 