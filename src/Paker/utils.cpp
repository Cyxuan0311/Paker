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

std::string get_record_file_path() {
    return get_project_name() + "_install_record.json";
}

std::vector<std::string> collect_package_files(const std::string& package_path) {
    std::vector<std::string> files;
    fs::path pkg_path(package_path);
    
    if (!fs::exists(pkg_path)) {
        return files;
    }
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(pkg_path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        // 忽略文件系统错误
    }
    
    return files;
} 