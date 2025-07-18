#include "Paker/install.h"
#include "Paker/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
extern const std::map<std::string, std::string>& get_builtin_repos();
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_add(const std::string& pkg_input) {
    auto [pkg, version] = parse_name_version(pkg_input);
    if (pkg.empty()) {
        LOG(ERROR) << "Invalid package name.";
        std::cout << "Invalid package name.\n";
        return;
    }
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["dependencies"][pkg] = version.empty() ? "*" : version;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version));
    std::cout << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version)) << "\n";
    // 优先查找自定义源
    #include "Paker/sources.h"
    auto all_repos = get_all_repos();
    auto it = all_repos.find(pkg);
    if (it == all_repos.end()) {
        LOG(WARNING) << "No repo for package: " << pkg;
        std::cout << "No repo for package: " << pkg << ". Please add manually.\n";
        return;
    }
    std::string repo_url = it->second;
    fs::path pkg_dir = fs::path("packages") / pkg;
    if (fs::exists(pkg_dir)) {
        LOG(WARNING) << "Package already exists in packages/" << pkg;
        std::cout << "Package already exists in packages/" << pkg << "\n";
        return;
    }
    fs::create_directories(pkg_dir.parent_path());
    std::ostringstream cmd;
    cmd << "git clone --depth 1 " << repo_url << " " << pkg_dir.string();
    int ret = std::system(cmd.str().c_str());
    if (ret != 0) {
        LOG(ERROR) << "Failed to clone repo: " << repo_url;
        std::cout << "Failed to clone repo: " << repo_url << "\n";
        return;
    }
    // checkout 版本
    if (!version.empty() && version != "*") {
        std::ostringstream checkout_cmd;
        checkout_cmd << "cd " << pkg_dir.string() << " && git fetch --tags && git checkout " << version;
        int ret2 = std::system(checkout_cmd.str().c_str());
        if (ret2 != 0) {
            LOG(WARNING) << "Failed to checkout version/tag: " << version;
            std::cout << "Warning: failed to checkout version/tag: " << version << "\n";
        } else {
            LOG(INFO) << "Checked out " << pkg << " to version " << version;
            std::cout << "Checked out " << pkg << " to version " << version << "\n";
        }
    }
}

void pm_remove(const std::string& pkg) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    if (j["dependencies"].contains(pkg)) {
        j["dependencies"].erase(pkg);
        std::ofstream ofs(json_file);
        ofs << j.dump(4);
        LOG(INFO) << "Removed dependency: " << pkg;
        std::cout << "Removed dependency: " << pkg << "\n";
        // 删除本地包目录
        fs::path pkg_dir = fs::path("packages") / pkg;
        if (fs::exists(pkg_dir)) {
            fs::remove_all(pkg_dir);
            LOG(INFO) << "Deleted local package directory: packages/" << pkg;
            std::cout << "Deleted local package directory: packages/" << pkg << "\n";
        }
    } else {
        LOG(WARNING) << "Dependency not found: " << pkg;
        std::cout << "Dependency not found: " << pkg << "\n";
    }
}

static void add_recursive(const std::string& pkg, std::set<std::string>& installed) {
    if (installed.count(pkg)) return;
    installed.insert(pkg);
    pm_add(pkg);
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path dep_json = pkg_dir / "Paker.json";
    if (!fs::exists(dep_json)) dep_json = pkg_dir / "paker.json";
    if (fs::exists(dep_json)) {
        std::ifstream ifs(dep_json);
        try {
            json j; ifs >> j;
            if (j.contains("dependencies")) {
                for (auto& [dep, ver] : j["dependencies"].items()) {
                    std::string dep_str = dep;
                    if (!ver.is_null() && ver != "*") dep_str += "@" + ver.get<std::string>();
                    add_recursive(dep_str, installed);
                }
            }
        } catch (...) {
            LOG(WARNING) << "Failed to parse dependencies for " << pkg;
            std::cout << "Warning: failed to parse dependencies for " << pkg << "\n";
        }
    }
}

void pm_add_recursive(const std::string& pkg) {
    std::set<std::string> installed;
    add_recursive(pkg, installed);
} 