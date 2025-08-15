#include "Paker/commands/update.h"
#include "Paker/core/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_update() {
    fs::path pkg_dir = "packages";
    if (!fs::exists(pkg_dir) || !fs::is_directory(pkg_dir)) {
        std::cout << "No packages to update.\n";
        return;
    }
    for (const auto& entry : fs::directory_iterator(pkg_dir)) {
        if (entry.is_directory()) {
            std::string dep = entry.path().filename().string();
            fs::path git_dir = entry.path() / ".git";
            if (fs::exists(git_dir)) {
                std::ostringstream cmd;
                cmd << "cd " << entry.path().string() << " && git pull";
                std::cout << "Updating " << dep << "...\n";
                int ret = std::system(cmd.str().c_str());
                if (ret != 0) {
                    std::cout << "  Failed to update " << dep << "\n";
                }
            }
        }
    }
    std::cout << "Update complete.\n";
}

void pm_clean() {
    std::string json_file = get_json_file();
    std::set<std::string> declared;
    if (fs::exists(json_file)) {
        std::ifstream ifs(json_file);
        json j; ifs >> j;
        if (j.contains("dependencies")) {
            for (auto& [dep, ver] : j["dependencies"].items()) {
                declared.insert(dep);
            }
        }
    }
    fs::path pkg_dir = "packages";
    if (!fs::exists(pkg_dir) || !fs::is_directory(pkg_dir)) {
        std::cout << "No packages to clean.\n";
        return;
    }
    for (const auto& entry : fs::directory_iterator(pkg_dir)) {
        if (entry.is_directory()) {
            std::string dep = entry.path().filename().string();
            fs::path git_dir = entry.path() / ".git";
            if (declared.count(dep) == 0) {
                std::cout << "Removing unused package: " << dep << "\n";
                fs::remove_all(entry.path());
            } else if (!fs::exists(git_dir)) {
                std::cout << "Removing broken package: " << dep << "\n";
                fs::remove_all(entry.path());
            }
        }
    }
    std::cout << "Clean complete.\n";
} 