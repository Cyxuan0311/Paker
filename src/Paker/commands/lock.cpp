#include "Paker/commands/lock.h"
#include "Paker/commands/install.h"
#include "Paker/core/utils.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_lock() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j; ifs >> j;
    
    // 添加依赖验证
    Paker::DependencyResolver resolver;
    if (!resolver.resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        std::cout << "Failed to resolve project dependencies\n";
        return;
    }
    
    Paker::ConflictDetector detector(resolver.get_dependency_graph());
    auto conflicts = detector.detect_all_conflicts();
    
    if (!conflicts.empty()) {
        LOG(ERROR) << "Conflicts detected in dependency tree";
        std::cout << "Conflicts detected in dependency tree:\n";
        std::cout << detector.generate_conflict_report(conflicts);
        std::cout << "Cannot generate lock file with conflicts\n";
        return;
    }
    
    json lock_j;
    lock_j["dependencies"] = json::object();
    fs::path pkg_dir = "packages";
    if (fs::exists(pkg_dir) && fs::is_directory(pkg_dir)) {
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
                lock_j["dependencies"][dep] = version;
            }
        }
    }
    std::ofstream ofs("Paker.lock");
    ofs << lock_j.dump(4);
    LOG(INFO) << "Generated Paker.lock";
    std::cout << "Generated Paker.lock\n";
}

void pm_install_lock() {
    if (!fs::exists("Paker.lock")) {
        LOG(ERROR) << "No Paker.lock file found. Run 'paker lock' first.";
        std::cout << "No Paker.lock file found. Run 'paker lock' first.\n";
        return;
    }
    std::ifstream ifs("Paker.lock");
    json lock_j; ifs >> lock_j;
    if (!lock_j.contains("dependencies")) {
        LOG(ERROR) << "Paker.lock missing dependencies field.";
        std::cout << "Paker.lock missing dependencies field.\n";
        return;
    }
    for (auto& [dep, ver] : lock_j["dependencies"].items()) {
        std::string dep_str = dep;
        if (!ver.is_null() && ver != "*" && ver != "unknown") dep_str += "@" + ver.get<std::string>();
        pm_add(dep_str);
    }
    LOG(INFO) << "Installed dependencies from Paker.lock";
    std::cout << "Installed dependencies from Paker.lock\n";
}

void pm_upgrade(const std::string& pkg) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j; ifs >> j;
    if (!j.contains("dependencies")) {
        LOG(INFO) << "No dependencies to upgrade.";
        std::cout << "No dependencies to upgrade.\n";
        return;
    }
    if (pkg.empty()) {
        for (auto& [dep, ver] : j["dependencies"].items()) {
            LOG(INFO) << "Upgrading " << dep << " to latest...";
            std::cout << "Upgrading " << dep << " to latest...\n";
            pm_remove(dep);
            pm_add(dep);
        }
    } else {
        if (j["dependencies"].contains(pkg)) {
            LOG(INFO) << "Upgrading " << pkg << " to latest...";
            std::cout << "Upgrading " << pkg << " to latest...\n";
            pm_remove(pkg);
            pm_add(pkg);
        } else {
            LOG(WARNING) << "Dependency not found: " << pkg;
            std::cout << "Dependency not found: " << pkg << "\n";
        }
    }
    LOG(INFO) << "Upgrade complete.";
    std::cout << "Upgrade complete.\n";
} 