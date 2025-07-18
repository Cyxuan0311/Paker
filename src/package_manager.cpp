#include "package_manager.h"
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <filesystem>
#include <map>
#include <cstdlib>
#include <sstream>
#include <set>
#include <glog/logging.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

// 获取当前目录名作为项目名
static std::string get_project_name() {
    std::string name = fs::current_path().filename().string();
    if (name.empty()) name = "myproject";
    return name;
}

// 获取 json 文件名
static std::string get_json_file() {
    return get_project_name() + ".json";
}

// 解析包名和版本号
typedef std::pair<std::string, std::string> NameVer;
static NameVer parse_name_version(const std::string& input) {
    auto pos = input.find('@');
    if (pos == std::string::npos) return {input, ""};
    return {input.substr(0, pos), input.substr(pos + 1)};
}

void pm_init() {
    std::string json_file = get_json_file();
    if (fs::exists(json_file)) {
        LOG(INFO) << "Project already initialized.";
        std::cout << "Project already initialized.\n";
        return;
    }
    std::string project_name = get_project_name();
    json j = {
        {"name", project_name},
        {"version", "0.1.0"},
        {"description", ""},
        {"dependencies", json::object()}
    };
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Initialized Paker project: " << project_name;
    std::cout << "Initialized Paker project: " << project_name << "\n";
}

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
    // 1. 先写入依赖声明
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["dependencies"][pkg] = version.empty() ? "*" : version;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version));
    std::cout << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version)) << "\n";

    // 2. 自动 clone 到 packages/目录
    const auto& repos = get_builtin_repos();
    auto it = repos.find(pkg);
    if (it == repos.end()) {
        LOG(WARNING) << "No builtin repo for package: " << pkg;
        std::cout << "No builtin repo for package: " << pkg << ". Please add manually.\n";
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
                // 获取版本号（尝试读取 .git/HEAD 或 tags）
                std::string version = "unknown";
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            // 解析分支名
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

void pm_add_desc(const std::string& desc) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["description"] = desc;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project description.";
    std::cout << "Updated project description.\n";
}

void pm_add_version(const std::string& vers) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["version"] = vers;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project version.";
    std::cout << "Updated project version.\n";
}

// 递归安装依赖（防止重复/死循环）
static void add_recursive(const std::string& pkg, std::set<std::string>& installed) {
    if (installed.count(pkg)) return;
    installed.insert(pkg);
    pm_add(pkg);
    // 查找依赖包的 Paker.json
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

// 依赖树递归打印
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

void pm_tree() {
    // 读取主项目依赖
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

// 生成/更新 Paker.lock 文件，记录实际安装的依赖版本
void pm_lock() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j; ifs >> j;
    json lock_j;
    lock_j["dependencies"] = json::object();
    fs::path pkg_dir = "packages";
    if (fs::exists(pkg_dir) && fs::is_directory(pkg_dir)) {
        for (const auto& entry : fs::directory_iterator(pkg_dir)) {
            if (entry.is_directory()) {
                std::string dep = entry.path().filename().string();
                // 获取版本号（读取 .git/HEAD 或 tags）
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

// 根据 Paker.lock 文件安装依赖
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

// 升级所有依赖到最新或指定版本
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
        // 升级所有依赖
        for (auto& [dep, ver] : j["dependencies"].items()) {
            LOG(INFO) << "Upgrading " << dep << " to latest...";
            std::cout << "Upgrading " << dep << " to latest...\n";
            pm_remove(dep);
            pm_add(dep); // 不带版本号，默认拉取最新
        }
    } else {
        // 升级指定依赖
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

// 搜索可用依赖包
void pm_search(const std::string& keyword) {
    const auto& repos = get_builtin_repos();
    std::cout << "Search results for '" << keyword << "':\n";
    bool found = false;
    for (const auto& [name, url] : repos) {
        if (name.find(keyword) != std::string::npos) {
            std::cout << "  " << name << "\t" << url << "\n";
            found = true;
        }
    }
    if (!found) std::cout << "  (none)\n";
}

// 显示依赖包详细信息
void pm_info(const std::string& pkg) {
    const auto& repos = get_builtin_repos();
    auto it = repos.find(pkg);
    if (it == repos.end()) {
        std::cout << "No info for package: " << pkg << "\n";
        return;
    }
    std::cout << "Package: " << pkg << "\n";
    std::cout << "Repo: " << it->second << "\n";
    // 尝试读取本地包的 README/CMakeLists.txt 作为简介
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path readme = pkg_dir / "README.md";
    if (!fs::exists(readme)) readme = pkg_dir / "README.rst";
    if (fs::exists(readme)) {
        std::ifstream ifs(readme);
        std::string line;
        std::cout << "Description (from README):\n";
        int cnt = 0;
        while (std::getline(ifs, line) && cnt < 10) { // 只显示前10行
            std::cout << line << "\n";
            ++cnt;
        }
    }
}

// 同步/刷新本地依赖信息（git pull）
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

// 清理未使用或损坏的依赖包
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