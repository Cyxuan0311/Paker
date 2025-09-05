#include "Paker/commands/install.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/conflict/conflict_resolver.h"
#include "Paker/monitor/performance_monitor.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/parallel_executor.h"
#include "Paker/core/incremental_updater.h"
#include "Paker/cache/lru_cache_manager.h"
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

// 辅助函数实现
std::string get_repository_url(const std::string& package) {
    const auto& repos = get_builtin_repos();
    auto it = repos.find(package);
    if (it != repos.end()) {
        return it->second;
    }
    return "";
}

std::string get_package_install_path(const std::string& package) {
    return "packages/" + package;
}

// 并行安装多个包
void pm_add_parallel(const std::vector<std::string>& packages) {
    if (packages.empty()) {
        Output::warning("No packages specified for parallel installation");
        return;
    }
    
    // 初始化并行执行器
    if (!g_parallel_executor) {
        if (!initialize_parallel_executor()) {
            Output::error("Failed to initialize parallel executor");
            return;
        }
    }
    
    Output::info("Starting parallel installation of " + std::to_string(packages.size()) + " packages");
    
    std::vector<std::string> task_ids;
    
    // 提交所有下载任务
    for (const auto& pkg_input : packages) {
        auto [pkg, version] = parse_name_version(pkg_input);
        if (pkg.empty()) {
            Output::warning("Invalid package name: " + pkg_input);
            continue;
        }
        
        std::string repo_url = get_repository_url(pkg);
        if (repo_url.empty()) {
            Output::warning("No repository found for package: " + pkg);
            continue;
        }
        
        // 创建下载任务
        std::string target_path = get_package_install_path(pkg);
        auto download_task = DownloadTaskFactory::create_download_task(pkg, version, repo_url, target_path);
        
        std::string task_id = g_parallel_executor->submit_task(download_task);
        if (!task_id.empty()) {
            task_ids.push_back(task_id);
        }
    }
    
    // 等待所有任务完成
    Output::info("Waiting for " + std::to_string(task_ids.size()) + " download tasks to complete...");
    
    bool all_success = true;
    for (const auto& task_id : task_ids) {
        if (!g_parallel_executor->wait_for_task(task_id, std::chrono::minutes(10))) {
            Output::error("Task " + task_id + " failed or timed out");
            all_success = false;
        } else {
            TaskStatus status = g_parallel_executor->get_task_status(task_id);
            if (status != TaskStatus::COMPLETED) {
                std::string error = g_parallel_executor->get_task_error(task_id);
                Output::error("Task " + task_id + " failed: " + error);
                all_success = false;
            }
        }
    }
    
    if (all_success) {
        Output::success("Parallel installation completed successfully");
    } else {
        Output::error("Some packages failed to install");
    }
}

void pm_add(const std::string& pkg_input) {
    // 开始性能监控
    PAKER_PERF_START("package_install");
    
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
    
    // 添加依赖冲突检测
    DependencyResolver resolver;
    if (!resolver.resolve_package(pkg, version)) {
        Output::error("Failed to resolve package dependencies");
        return;
    }
    
    ConflictDetector detector(resolver.get_dependency_graph());
    auto conflicts = detector.detect_all_conflicts();
    
    if (!conflicts.empty()) {
        Output::warning("Dependency conflicts detected:");
        Output::info(detector.generate_conflict_report(conflicts));
        
        // 询问用户是否自动解决
        Output::info("Auto-resolve conflicts? [Y/n]: ");
        std::string response;
        std::getline(std::cin, response);
        
        if (response.empty() || response[0] == 'Y' || response[0] == 'y') {
            ConflictResolver conflict_resolver(resolver.get_dependency_graph());
            if (!conflict_resolver.auto_resolve_conflicts(conflicts)) {
                Output::error("Failed to auto-resolve conflicts");
                return;
            }
            Output::success("Conflicts resolved automatically");
        } else {
            Output::error("Please resolve conflicts before installing");
            return;
        }
    }
    
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
    
    // 默认使用全局缓存模式（除非明确禁用）
    bool use_cache_mode = g_cache_manager != nullptr;
    
    if (use_cache_mode) {
        Output::info("Using global cache mode (default)");
        // 全局缓存模式
        std::string target_version = version.empty() ? "latest" : version;
        
        // 检查包是否已在缓存中
        if (!g_cache_manager->is_package_cached(pkg, target_version)) {
            Output::info("Installing " + pkg + "@" + target_version + " to global cache...");
            
            if (!g_cache_manager->install_package_to_cache(pkg, target_version, repo_url)) {
                Output::error("Failed to install package to cache");
                return;
            }
        } else {
            Output::info("Package " + pkg + "@" + target_version + " already in cache");
        }
        
        // 创建项目链接
        std::string project_path = fs::current_path().string();
        if (!g_cache_manager->create_project_link(pkg, target_version, project_path)) {
            Output::error("Failed to create project link");
            return;
        }
        
        // 获取链接路径用于记录
        std::string linked_path = g_cache_manager->get_project_package_path(pkg, project_path);
        if (linked_path.empty()) {
            Output::error("Failed to get project package path");
            return;
        }
        
        // 记录文件
        Recorder::Record record(get_record_file_path());
        std::vector<std::string> installed_files = collect_package_files(linked_path);
        record.addPackageRecord(pkg, linked_path, installed_files);
        
        Output::success("Successfully installed " + pkg + " (cached, " + std::to_string(installed_files.size()) + " files)");
        
    } else {
        // 传统模式（向后兼容）
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
    }
    
    progress.finish();
    Output::success("Successfully installed " + pkg + " (" + std::to_string(installed_files.size()) + " files recorded)");
    
    // 记录版本变更
    pm_record_version_change(pkg, "", version, repo_url, "Package installation");
    
    // 结束性能监控并记录指标
    PAKER_PERF_END("package_install", MetricType::INSTALL_TIME);
    
    // 记录包大小
    size_t total_size = 0;
    for (const auto& file : installed_files) {
        if (fs::exists(file)) {
            total_size += fs::file_size(file);
        }
    }
    PAKER_PERF_RECORD(MetricType::DISK_USAGE, pkg + "_size", static_cast<double>(total_size), "bytes");
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