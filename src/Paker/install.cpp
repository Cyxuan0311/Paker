#include "Paker/install.h"
#include "Paker/utils.h"
#include "Paker/output.h"
#include "Recorder/record.h"
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
        Output::error("Invalid package name.");
        return;
    }
    
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["dependencies"][pkg] = version.empty() ? "*" : version;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    
    LOG(INFO) << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version));
    Output::success("Added dependency: " + pkg + (version.empty() ? "" : ("@" + version)));
    
    // 优先查找自定义源
    #include "Paker/sources.h"
    auto all_repos = get_all_repos();
    auto it = all_repos.find(pkg);
    if (it == all_repos.end()) {
        LOG(WARNING) << "No repo for package: " << pkg;
        Output::warning("No repo for package: " + pkg + ". Please add manually.");
        return;
    }
    
    std::string repo_url = it->second;
    fs::path pkg_dir = fs::path("packages") / pkg;
    if (fs::exists(pkg_dir)) {
        LOG(WARNING) << "Package already exists in packages/" << pkg;
        Output::warning("Package already exists in packages/" + pkg);
        return;
    }
    
    fs::create_directories(pkg_dir.parent_path());
    
    // 显示安装进度
    Output::info("Installing package: " + pkg);
    ProgressBar progress(3, 40, "Installing: ");
    
    // 步骤1: 克隆仓库
    progress.update(1);
    Output::debug("Cloning repository: " + repo_url);
    
    std::ostringstream cmd;
    cmd << "git clone --depth 1 " << repo_url << " " << pkg_dir.string();
    int ret = std::system(cmd.str().c_str());
    if (ret != 0) {
        LOG(ERROR) << "Failed to clone repo: " << repo_url;
        Output::error("Failed to clone repository: " + repo_url);
        return;
    }
    
    // 步骤2: 检出版本
    progress.update(2);
    if (!version.empty() && version != "*") {
        Output::debug("Checking out version: " + version);
        std::ostringstream checkout_cmd;
        checkout_cmd << "cd " << pkg_dir.string() << " && git fetch --tags && git checkout " << version;
        int ret2 = std::system(checkout_cmd.str().c_str());
        if (ret2 != 0) {
            LOG(WARNING) << "Failed to checkout version/tag: " << version;
            Output::warning("Failed to checkout version/tag: " + version);
        } else {
            LOG(INFO) << "Checked out " << pkg << " to version " << version;
            Output::info("Checked out " + pkg + " to version " + version);
        }
    }
    
    // 步骤3: 记录文件
    progress.update(3);
    Output::debug("Recording package files...");
    
    // 使用Record类记录安装的文件
    Recorder::Record record(get_record_file_path());
    std::vector<std::string> installed_files = collect_package_files(pkg_dir.string());
    
    // 记录包信息
    record.addPackageRecord(pkg, pkg_dir.string(), installed_files);
    LOG(INFO) << "Recorded " << installed_files.size() << " files for package: " << pkg;
    
    progress.finish();
    Output::success("Successfully installed " + pkg + " (" + std::to_string(installed_files.size()) + " files recorded)");
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
        
        // 使用Record类获取包的文件信息
        Recorder::Record record(get_record_file_path());
        if (record.isPackageInstalled(pkg)) {
            std::vector<std::string> files = record.getPackageFiles(pkg);
            std::string install_path = record.getPackageInstallPath(pkg);
            
            LOG(INFO) << "Found " << files.size() << " files to remove for package: " << pkg;
            std::cout << "Found " << files.size() << " files to remove for package: " << pkg << "\n";
            
            // 删除记录的文件
            for (const auto& file : files) {
                if (fs::exists(file)) {
                    fs::remove(file);
                    LOG(INFO) << "Removed file: " << file;
                }
            }
            
            // 删除安装目录
            if (!install_path.empty() && fs::exists(install_path)) {
                fs::remove_all(install_path);
                LOG(INFO) << "Removed install directory: " << install_path;
                std::cout << "Removed install directory: " << install_path << "\n";
            }
            
            // 从记录中删除包
            record.removePackageRecord(pkg);
            LOG(INFO) << "Removed package record: " << pkg;
        }
        
        // 删除本地包目录（如果还存在）
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