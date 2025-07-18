#include "Paker/list.h"
#include "Paker/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace fs = std::filesystem;

static void print_tree(const std::string& pkg, std::set<std::string>& visited, int depth) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";
    std::cout << "- " << pkg << "\n";
    if (visited.count(pkg)) return;
    visited.insert(pkg);
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path dep_json = pkg_dir / "Paker.json";
    if (!fs::exists(dep_json)) dep_json = pkg_dir / "paker.json";
    if (fs::exists(dep_json)) {
        std::ifstream ifs(dep_json);
        try {
            json j; ifs >> j;
            if (j.contains("dependencies")) {
                for (auto& [dep, ver] : j["dependencies"].items()) {
                    print_tree(dep, visited, depth + 1);
                }
            }
        } catch (...) {
            LOG(WARNING) << "Failed to parse dependencies for " << pkg;
            std::cout << "Warning: failed to parse dependencies for " << pkg << "\n";
        }
    }
}

void pm_list() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    LOG(INFO) << "Project: " << j["name"] << " v" << j["version"];
    std::cout << "Project: " << j["name"] << " v" << j["version"] << "\n";
    if (!j["description"].get<std::string>().empty()) {
        LOG(INFO) << "Description: " << j["description"];
        std::cout << "Description: " << j["description"] << "\n";
    }
    std::cout << "\nDependencies (declared):\n";
    if (j["dependencies"].empty()) {
        std::cout << "  (none)\n";
    } else {
        for (auto& [k, v] : j["dependencies"].items()) {
            LOG(INFO) << "  " << k << ": " << v;
            std::cout << "  " << k << ": " << v << "\n";
        }
    }
    // 显示本地已下载依赖
    std::cout << "\nDependencies (downloaded):\n";
    fs::path pkg_dir = "packages";
    if (fs::exists(pkg_dir) && fs::is_directory(pkg_dir)) {
        bool found = false;
        for (const auto& entry : fs::directory_iterator(pkg_dir)) {
            if (entry.is_directory()) {
                std::string dep = entry.path().filename().string();
                std::string version = "unknown";
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            version = head_line.substr(head_line.find_last_of('/') + 1);
                        } else {
                            version = head_line;
                        }
                    }
                }
                LOG(INFO) << "  " << dep << ": " << version;
                std::cout << "  " << dep << ": " << version << "\n";
                found = true;
            }
        }
        if (!found) std::cout << "  (none)\n";
    } else {
        std::cout << "  (none)\n";
    }
}

void pm_tree() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j; ifs >> j;
    LOG(INFO) << "Dependency Tree:";
    std::cout << "Dependency Tree:" << std::endl;
    std::set<std::string> visited;
    if (j.contains("dependencies")) {
        for (auto& [dep, ver] : j["dependencies"].items()) {
            print_tree(dep, visited, 1);
        }
    }
} 