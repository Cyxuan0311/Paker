#include "Paker/commands/list.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
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
            Paker::Output::warning("Failed to parse dependencies for " + pkg);
        }
    }
}

void pm_list() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    
    // 项目信息
    LOG(INFO) << "Project: " << j["name"] << " v" << j["version"];
    Paker::Output::info("Project: " + j["name"].get<std::string>() + " v" + j["version"].get<std::string>());
    
    if (!j["description"].get<std::string>().empty()) {
        LOG(INFO) << "Description: " << j["description"];
        Paker::Output::info("Description: " + j["description"].get<std::string>());
    }
    
    // 声明的依赖
    Paker::Output::info("\nDependencies (declared):");
    if (j["dependencies"].empty()) {
        Paker::Output::info("  (none)");
    } else {
        Paker::Table table;
        table.add_column("Package", 20);
        table.add_column("Version", 15);
        
        for (auto& [k, v] : j["dependencies"].items()) {
            LOG(INFO) << "  " << k << ": " << v;
            table.add_row({k, v.get<std::string>()});
        }
        Paker::Output::print_table(table);
    }
    
    // 已下载的依赖
    Paker::Output::info("\nDependencies (downloaded):");
    fs::path pkg_dir = "packages";
    if (fs::exists(pkg_dir) && fs::is_directory(pkg_dir)) {
        bool found = false;
        Paker::Table table;
        table.add_column("Package", 20);
        table.add_column("Version", 15);
        table.add_column("Status", 10);
        
        for (const auto& entry : fs::directory_iterator(pkg_dir)) {
            if (entry.is_directory()) {
                std::string dep = entry.path().filename().string();
                std::string version = "unknown";
                std::string status = "installed";
                
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            version = head_line.substr(head_line.find_last_of('/') + 1);
                        } else {
                            version = head_line.substr(0, 8); // 只显示前8位commit hash
                        }
                    }
                }
                
                LOG(INFO) << "  " << dep << ": " << version;
                table.add_row({dep, version, status});
                found = true;
            }
        }
        
        if (!found) {
            Paker::Output::info("  (none)");
        } else {
            Paker::Output::print_table(table);
        }
    } else {
        Paker::Output::info("  (none)");
    }
}

void pm_tree() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    std::ifstream ifs(json_file);
    json j; ifs >> j;
    
    LOG(INFO) << "Dependency Tree:";
    Paker::Output::info("Dependency Tree:");
    
    // 构建依赖树数据结构
    std::map<std::string, std::vector<std::string>> deps;
    std::map<std::string, std::string> versions;
    
    // 获取项目名称作为根节点
    std::string root_name = j["name"].get<std::string>();
    versions[root_name] = j["version"].get<std::string>();
    
    if (j.contains("dependencies")) {
        for (auto& [dep, ver] : j["dependencies"].items()) {
            deps[root_name].push_back(dep);
            versions[dep] = ver.get<std::string>();
            
            // 递归获取子依赖
            fs::path pkg_dir = fs::path("packages") / dep;
            fs::path dep_json = pkg_dir / "Paker.json";
            if (!fs::exists(dep_json)) dep_json = pkg_dir / "paker.json";
            
            if (fs::exists(dep_json)) {
                std::ifstream dep_ifs(dep_json);
                try {
                    json dep_j; dep_ifs >> dep_j;
                    if (dep_j.contains("dependencies")) {
                        for (auto& [sub_dep, sub_ver] : dep_j["dependencies"].items()) {
                            deps[dep].push_back(sub_dep);
                            versions[sub_dep] = sub_ver.get<std::string>();
                        }
                    }
                } catch (...) {
                    LOG(WARNING) << "Failed to parse dependencies for " << dep;
                }
            }
        }
    }
    
    // 使用新的依赖树输出
    Paker::Output::print_dependency_tree(root_name, deps, versions);
} 