#include "Paker/commands/install.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/conflict/conflict_resolver.h"
#include "Paker/monitor/performance_monitor.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/parallel_executor.h"
#include "Paker/core/incremental_updater.h"
#include "Paker/cache/lru_cache_manager.h"
#include "Paker/dependency/sources.h"
#include "Recorder/record.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <thread>
#include <chrono>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
extern const std::map<std::string, std::string>& get_builtin_repos();
using json = nlohmann::json;
namespace fs = std::filesystem;

// 辅助函数实现
std::string get_repository_url(const std::string& package) {
    // 首先检查自定义远程源
    auto custom_repos = get_custom_repos();
    auto custom_it = custom_repos.find(package);
    if (custom_it != custom_repos.end()) {
        return custom_it->second;
    }
    
    // 然后检查内置仓库
    const auto& builtin_repos = get_builtin_repos();
    auto builtin_it = builtin_repos.find(package);
    if (builtin_it != builtin_repos.end()) {
        return builtin_it->second;
    }
    
    return "";
}

std::string get_package_install_path(const std::string& package) {
    return "packages/" + package;
}

// 更新JSON配置文件（用于并行安装）
void update_json_file(const std::vector<std::string>& packages) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    json j;
    try {
        std::ifstream ifs(json_file);
        ifs >> j;
        ifs.close();
        
        // 验证JSON结构
        if (!j.is_object()) {
            Paker::Output::error("Invalid project configuration file");
            return;
        }
    } catch (const std::exception& e) {
        Paker::Output::error("Failed to parse project configuration file");
        return;
    }
    
    // 确保dependencies字段存在
    if (!j.contains("dependencies")) {
        j["dependencies"] = json::object();
    }
    
    // 添加所有包到dependencies
    for (const auto& pkg_input : packages) {
        auto [pkg, version] = parse_name_version(pkg_input);
        if (!pkg.empty()) {
            j["dependencies"][pkg] = version.empty() ? "*" : version;
        }
    }
    
    // 安全地写入JSON文件
    try {
        std::ofstream ofs(json_file);
        if (!ofs.is_open()) {
            Paker::Output::error("Failed to save project configuration");
            return;
        }
        ofs << j.dump(4);
        ofs.close();
        
        Paker::Output::success("Updated project configuration with " + std::to_string(packages.size()) + " packages");
    } catch (const std::exception& e) {
        Paker::Output::error("Failed to save project configuration");
        return;
    }
}

// 并行安装多个包
void pm_add_parallel(const std::vector<std::string>& packages) {
    if (packages.empty()) {
        Paker::Output::warning("No packages specified for parallel download");
        return;
    }
    
    // 初始化并行执行器
    if (!Paker::g_parallel_executor) {
        if (!Paker::initialize_parallel_executor()) {
            Paker::Output::error("Failed to initialize parallel executor");
            return;
        }
    }
    
    Paker::Output::info("Starting parallel download of " + std::to_string(packages.size()) + " packages");
    
    std::vector<std::string> task_ids;
    
    // 提交所有下载任务
    for (const auto& pkg_input : packages) {
        auto [pkg, version] = parse_name_version(pkg_input);
        if (pkg.empty()) {
            Paker::Output::warning("Invalid package name: " + pkg_input);
            continue;
        }
        
        std::string repo_url = get_repository_url(pkg);
        if (repo_url.empty()) {
            Paker::Output::warning("No repository found for package: " + pkg);
            continue;
        }
        
        // 创建下载任务
        std::string target_path = get_package_install_path(pkg);
        auto download_task = Paker::DownloadTaskFactory::create_download_task(pkg, version, repo_url, target_path);
        
        std::string task_id = Paker::g_parallel_executor->submit_task(download_task);
        if (!task_id.empty()) {
            task_ids.push_back(task_id);
        }
    }
    
    // 简洁的并行安装界面
    std::cout << "\n";
    std::cout << "Downloading " << std::to_string(task_ids.size()) << " packages in parallel\n";
    std::cout << "\n";
    
    // 创建进度条显示并行下载进度
    Paker::ProgressBar* parallel_progress = new Paker::ProgressBar(
        100, 30, "", true, true, false, Paker::ProgressStyle::BASIC
    );
    
    bool all_success = true;
    int completed_tasks = 0;
    
    for (const auto& task_id : task_ids) {
        // 模拟下载进度 - 基于实际操作的进度显示
        auto start_time = std::chrono::steady_clock::now();
        int progress_value = 0;
        
        while (progress_value < 90) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            
            // 基于时间计算进度
            progress_value = (elapsed * 90) / 1000; // 假设每个包下载需要1秒
            if (progress_value > 90) progress_value = 90;
            
            parallel_progress->update(progress_value, "Downloading package " + std::to_string(completed_tasks + 1) + "/" + std::to_string(task_ids.size()) + "...");
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            
            if (progress_value >= 90) break;
        }
        
        if (!Paker::g_parallel_executor->wait_for_task(task_id, std::chrono::minutes(10))) {
            Paker::Output::error("Task " + task_id + " failed or timed out");
            all_success = false;
        } else {
            Paker::TaskStatus status = Paker::g_parallel_executor->get_task_status(task_id);
            if (status != Paker::TaskStatus::COMPLETED) {
                std::string error = Paker::g_parallel_executor->get_task_error(task_id);
                Paker::Output::error("Task " + task_id + " failed: " + error);
                all_success = false;
            }
        }
        
        // 更新进度
        completed_tasks++;
        int progress_percent = (completed_tasks * 100) / task_ids.size();
        parallel_progress->update(progress_percent, "Completed " + std::to_string(completed_tasks) + "/" + std::to_string(task_ids.size()) + " packages");
    }
    
    parallel_progress->finish("All packages downloaded successfully");
    delete parallel_progress;
    
    if (all_success) {
        // 更新配置文件
        update_json_file(packages);
        
        // 记录安装信息到Paker_install_record.json
        Recorder::Record record(get_record_file_path());
        for (const auto& pkg_input : packages) {
            auto [pkg, version] = parse_name_version(pkg_input);
            if (!pkg.empty()) {
                std::string install_path = get_package_install_path(pkg);
                if (fs::exists(install_path)) {
                    std::vector<std::string> installed_files = collect_package_files(install_path);
                    record.addPackageRecord(pkg, install_path, installed_files);
                    LOG(INFO) << "Recorded " << installed_files.size() << " files for package: " << pkg;
                }
            }
        }
        
        std::cout << "\n";
        std::cout << "Successfully downloaded " << std::to_string(task_ids.size()) << " packages\n";
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "Some packages failed to download\n";
        std::cout << "\n";
    }
}

void pm_add(const std::string& pkg_input) {
    // 开始性能监控
    LOG(INFO) << "Starting performance monitoring for package_install";
    PAKER_PERF_START("package_install");
    
    auto [pkg, version] = parse_name_version(pkg_input);
    if (pkg.empty()) {
        LOG(ERROR) << "Invalid package name.";
        Paker::Output::error("Invalid package name.");
        return;
    }
    
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    // 首先查找仓库，在修改JSON文件之前
    #include "Paker/dependency/sources.h"
    auto all_repos = get_all_repos();
    auto it = all_repos.find(pkg);
    if (it == all_repos.end()) {
        LOG(WARNING) << "No repo for package: " << pkg;
        Paker::Output::warning("No repo for package: " + pkg + ". Please add manually.");
        return;
    }
    
    std::string repo_url = it->second;
    
    // 现在修改JSON文件（添加错误处理）
    json j;
    try {
        std::ifstream ifs(json_file);
        if (!ifs.is_open()) {
            LOG(ERROR) << "Failed to open JSON file: " << json_file;
            Paker::Output::error("Failed to open project configuration file");
            return;
        }
        
        ifs >> j;
        ifs.close();
        
        // 验证JSON结构
        if (!j.is_object()) {
            LOG(ERROR) << "Invalid JSON structure in " << json_file;
            Paker::Output::error("Invalid project configuration file");
            return;
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to parse JSON file: " << e.what();
        Paker::Output::error("Failed to parse project configuration file");
        return;
    }
    
    // 确保dependencies字段存在
    if (!j.contains("dependencies")) {
        j["dependencies"] = json::object();
    }
    j["dependencies"][pkg] = version.empty() ? "*" : version;
    
    // 安全地写入JSON文件
    try {
        std::ofstream ofs(json_file);
        if (!ofs.is_open()) {
            LOG(ERROR) << "Failed to open JSON file for writing: " << json_file;
            Paker::Output::error("Failed to save project configuration");
            return;
        }
        ofs << j.dump(4);
        ofs.close();
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to write JSON file: " << e.what();
        Paker::Output::error("Failed to save project configuration");
        return;
    }
    
    LOG(INFO) << "Added dependency: " << pkg << (version.empty() ? "" : ("@" + version));
    Paker::Output::success("Added dependency: " + pkg + (version.empty() ? "" : ("@" + version)));
    
    // 添加依赖冲突检测（使用try-catch保护）
    try {
        Paker::DependencyResolver resolver;
        if (!resolver.resolve_package(pkg, version)) {
            LOG(WARNING) << "Failed to resolve package dependencies for " << pkg;
            // 不返回，继续执行安装
        } else {
            Paker::ConflictDetector detector(resolver.get_dependency_graph());
            auto conflicts = detector.detect_all_conflicts();
            
            if (!conflicts.empty()) {
                Paker::Output::warning("Dependency conflicts detected:");
                Paker::Output::info(detector.generate_conflict_report(conflicts));
                
                // 询问用户是否自动解决
                Paker::Output::info("Auto-resolve conflicts? [Y/n]: ");
                std::string response;
                std::getline(std::cin, response);
                
                if (response.empty() || response[0] == 'Y' || response[0] == 'y') {
                    Paker::ConflictResolver conflict_resolver(resolver.get_dependency_graph());
                    if (!conflict_resolver.auto_resolve_conflicts(conflicts)) {
                        Paker::Output::error("Failed to auto-resolve conflicts");
                        return;
                    }
                    Paker::Output::success("Conflicts resolved automatically");
                } else {
                    Paker::Output::error("Please resolve conflicts before downloading");
                    return;
                }
            }
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Dependency resolution failed: " << e.what();
        // 继续执行，不因依赖解析失败而停止安装
    }
    
    // 默认使用全局缓存模式（除非明确禁用）
    bool use_cache_mode = Paker::g_cache_manager != nullptr;
    
    // 声明变量用于函数末尾使用
    Paker::ProgressBar* progress = nullptr;
    std::vector<std::string> installed_files;
    
    if (use_cache_mode) {
        Paker::Output::info("Using global cache mode (default)");
        // 全局缓存模式
        std::string target_version = version.empty() ? "latest" : version;
        
        // 检查包是否已在缓存中
        if (!Paker::g_cache_manager->is_package_cached(pkg, target_version)) {
            Paker::Output::info("Downloading " + pkg + "@" + target_version + " to global cache...");
            
            if (!Paker::g_cache_manager->install_package_to_cache(pkg, target_version, repo_url)) {
                Paker::Output::error("Failed to download package to cache");
                return;
            }
        } else {
            Paker::Output::info("Package " + pkg + "@" + target_version + " already in cache");
        }
        
        // 创建项目链接
        std::string project_path = fs::current_path().string();
        if (!Paker::g_cache_manager->create_project_link(pkg, target_version, project_path)) {
            Paker::Output::error("Failed to create project link");
            return;
        }
        
        // 获取链接路径用于记录
        std::string linked_path = Paker::g_cache_manager->get_project_package_path(pkg, project_path);
        if (linked_path.empty()) {
            Paker::Output::error("Failed to get project package path");
            return;
        }
        
        // 记录文件
        Recorder::Record record(get_record_file_path());
        installed_files = collect_package_files(linked_path);
        record.addPackageRecord(pkg, linked_path, installed_files);
        
        Paker::Output::success("Successfully downloaded " + pkg + " (cached, " + std::to_string(installed_files.size()) + " files)");
        
    } else {
        // 传统模式（向后兼容）
        fs::path pkg_dir = fs::path("packages") / pkg;
        if (fs::exists(pkg_dir)) {
            LOG(WARNING) << "Package already exists in packages/" << pkg;
            Paker::Output::warning("Package already exists in packages/" + pkg);
            return;
        }
        
        fs::create_directories(pkg_dir.parent_path());
        
        // 简洁的安装界面
        std::cout << "\n";
        // 显示安装信息
        std::cout << "\n";
        std::cout << "Downloading " << pkg;
        if (!version.empty() && version != "*") {
            std::cout << "@" << version;
        }
        std::cout << "\n";
        std::cout << "Repository: " << repo_url << "\n";
        std::cout << "\n";
        
        // 创建简洁的进度条
        progress = new Paker::ProgressBar(100, 30, "", true, true, false, Paker::ProgressStyle::BASIC);
        
        // 步骤1: 克隆仓库
        progress->update(0, "Connecting to repository...");
        Paker::Output::debug("Cloning repository: " + repo_url);
        
        std::ostringstream cmd;
        cmd << "git clone --quiet --depth 1 " << repo_url << " " << pkg_dir.string() << " 2>/dev/null";
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to clone repo: " << repo_url;
            Paker::Output::error("Failed to clone repository: " + repo_url);
            return;
        }
        
        progress->update(30, "Repository cloned successfully");
        
        // 步骤2: 检出版本（如果需要）
        if (!version.empty() && version != "*") {
            progress->update(50, "Checking out version " + version);
            Paker::Output::debug("Checking out version: " + version);
            std::ostringstream checkout_cmd;
            checkout_cmd << "cd " << pkg_dir.string() << " && git fetch --tags --quiet && git checkout --quiet " << version << " 2>/dev/null";
            int ret2 = std::system(checkout_cmd.str().c_str());
            if (ret2 != 0) {
                LOG(WARNING) << "Failed to checkout version/tag: " << version;
                Paker::Output::warning("Failed to checkout version/tag: " + version);
            } else {
                LOG(INFO) << "Checked out " << pkg << " to version " << version;
                Paker::Output::info("Checked out " + pkg + " to version " + version);
            }
        }
        
        // 步骤3: 记录文件
        progress->update(70, "Recording package files and metadata");
        Paker::Output::debug("Recording package files...");
        
        // 使用Record类记录安装的文件
        Recorder::Record record(get_record_file_path());
        installed_files = collect_package_files(pkg_dir.string());
        
        // 记录包信息
        record.addPackageRecord(pkg, pkg_dir.string(), installed_files);
        LOG(INFO) << "Recorded " << installed_files.size() << " files for package: " << pkg;
        
        // 完成安装
        progress->finish("Download completed successfully");
        delete progress;
        progress = nullptr;
    }
    
    // 简洁的安装完成信息
    std::cout << "\n";
    std::cout << "Successfully downloaded " << pkg;
    if (!version.empty() && version != "*") {
        std::cout << "@" << version;
    }
    std::cout << " (" << std::to_string(installed_files.size()) << " files)\n";
    std::cout << "\n";
    
    // 记录版本变更
    pm_record_version_change(pkg, "", version, repo_url, "Package download");
    
    // 结束性能监控并记录指标
    LOG(INFO) << "Ending performance monitoring for package_install";
    PAKER_PERF_END("package_install", Paker::MetricType::INSTALL_TIME);
    
    // 检查性能监控器状态
    if (Paker::g_performance_monitor.is_enabled()) {
        LOG(INFO) << "Performance monitor is enabled";
        
        // 检查是否有数据被记录
        auto metrics = Paker::g_performance_monitor.get_metrics();
        LOG(INFO) << "Total metrics recorded: " << metrics.size();
    } else {
        LOG(WARNING) << "Performance monitor is disabled";
    }
    
    // 记录包大小
    size_t total_size = 0;
    for (const auto& file : installed_files) {
        if (fs::exists(file)) {
            total_size += fs::file_size(file);
        }
    }
    PAKER_PERF_RECORD(Paker::MetricType::DISK_USAGE, pkg + "_size", static_cast<double>(total_size), "bytes");
}

void pm_add_url(const std::string& url) {
    // 开始性能监控
    LOG(INFO) << "Starting performance monitoring for package_install_url";
    PAKER_PERF_START("package_install_url");
    
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    
    // 从URL提取包名（取最后一个路径部分，去掉.git后缀）
    std::string pkg_name = url;
    size_t last_slash = pkg_name.find_last_of('/');
    if (last_slash != std::string::npos) {
        pkg_name = pkg_name.substr(last_slash + 1);
    }
    if (pkg_name.size() >= 4 && pkg_name.substr(pkg_name.length() - 4) == ".git") {
        pkg_name = pkg_name.substr(0, pkg_name.length() - 4);
    }
    
    // 读取Paker.json
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    
    // 添加URL依赖
    if (!j.contains("url_dependencies")) {
        j["url_dependencies"] = json::object();
    }
    j["url_dependencies"][pkg_name] = url;
    
    // 写回文件
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    
    LOG(INFO) << "Added URL dependency: " << pkg_name << " -> " << url;
    Paker::Output::success("Added URL dependency: " + pkg_name + " -> " + url);
    
    // 实际下载包
    std::string target_path = get_package_install_path(pkg_name);
    if (!fs::exists(target_path)) {
        Paker::Output::info("Downloading package: " + pkg_name);
        
        // 创建目标目录
        fs::create_directories(target_path);
        
        // 使用git clone下载（静默模式）
        std::string cmd = "git clone --quiet --depth 1 " + url + " " + target_path + " 2>/dev/null";
        int result = system(cmd.c_str());
        
        if (result == 0) {
            LOG(INFO) << "Successfully downloaded package: " << pkg_name;
            Paker::Output::success("Successfully downloaded package: " + pkg_name);
        } else {
            LOG(ERROR) << "Failed to download package: " << pkg_name;
            Paker::Output::error("Failed to download package: " + pkg_name);
        }
    } else {
        LOG(INFO) << "Package already exists: " << pkg_name;
        Paker::Output::info("Package already exists: " + pkg_name);
    }
    
    // 结束性能监控（即使包已存在也要记录）
    LOG(INFO) << "Ending performance monitoring for package_install_url";
    PAKER_PERF_END("package_install_url", Paker::MetricType::INSTALL_TIME);
    
    // 检查性能监控器状态
    if (Paker::g_performance_monitor.is_enabled()) {
        LOG(INFO) << "Performance monitor is enabled";
        
        // 检查是否有数据被记录
        auto metrics = Paker::g_performance_monitor.get_metrics();
        LOG(INFO) << "Total metrics recorded: " << metrics.size();
    } else {
        LOG(WARNING) << "Performance monitor is disabled";
    }
    
    // 保存性能监控数据到文件
    std::string perf_file = ".paker/performance_data.json";
    Paker::g_performance_monitor.save_to_file(perf_file);
}

void pm_remove(const std::string& pkg) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'Paker init' first.");
        return;
    }
    
    bool removed_from_dependencies = false;
    bool removed_from_url_dependencies = false;
    
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    
    // 检查并移除dependencies中的包
    if (j["dependencies"].contains(pkg)) {
        j["dependencies"].erase(pkg);
        removed_from_dependencies = true;
    }
    
    // 检查并移除url_dependencies中的包
    if (j.contains("url_dependencies") && j["url_dependencies"].contains(pkg)) {
        j["url_dependencies"].erase(pkg);
        removed_from_url_dependencies = true;
    }
    
    // 如果从任何依赖字段中移除了包，更新JSON文件
    if (removed_from_dependencies || removed_from_url_dependencies) {
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
        // 检查是否有已下载但未声明的包
        fs::path pkg_dir = fs::path("packages") / pkg;
        if (fs::exists(pkg_dir)) {
            LOG(INFO) << "Removing downloaded package: " << pkg;
            std::cout << "Removing downloaded package: " << pkg << "\n";
            
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
            
            // 删除本地包目录
            fs::remove_all(pkg_dir);
            LOG(INFO) << "Deleted local package directory: packages/" << pkg;
            std::cout << "Deleted local package directory: packages/" << pkg << "\n";
        } else {
            LOG(WARNING) << "Dependency not found: " << pkg;
            std::cout << "Dependency not found: " << pkg << "\n";
        }
    }
}

static void add_recursive(const std::string& pkg, std::set<std::string>& installed) {
    if (installed.count(pkg)) return;
    installed.insert(pkg);
    
    // 检查是否是URL
    if (pkg.find("http://") == 0 || 
        pkg.find("https://") == 0 || 
        pkg.find("git@") == 0 ||
        pkg.find("git://") == 0) {
        // 使用URL添加
        pm_add_url(pkg);
    } else {
        // 使用普通添加
        pm_add(pkg);
    }
    
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

// ============================================================================
// 新的install命令实现
// ============================================================================

// Build system detection
BuildSystem detect_build_system(const std::string& package_path) {
    fs::path pkg_path(package_path);
    
    // Check for CMake
    if (fs::exists(pkg_path / "CMakeLists.txt")) {
        return BuildSystem::CMAKE;
    }
    
    // Check for Meson
    if (fs::exists(pkg_path / "meson.build")) {
        return BuildSystem::MESON;
    }
    
    // Check for Ninja
    if (fs::exists(pkg_path / "build.ninja")) {
        return BuildSystem::NINJA;
    }
    
    // Check for Makefile
    if (fs::exists(pkg_path / "Makefile") || fs::exists(pkg_path / "makefile")) {
        return BuildSystem::MAKE;
    }
    
    // Check for Autotools
    if (fs::exists(pkg_path / "configure") || fs::exists(pkg_path / "configure.ac")) {
        return BuildSystem::AUTOTOOLS;
    }
    
    return BuildSystem::UNKNOWN;
}

// Build and install package
bool build_and_install_package(const std::string& package_path, const std::string& package_name, BuildSystem build_system) {
    fs::path pkg_path(package_path);
    fs::path build_dir = pkg_path / "build";
    fs::path install_dir = pkg_path / "install";
    
    try {
        // Clean existing build directory if it exists
        if (fs::exists(build_dir)) {
            fs::remove_all(build_dir);
        }
        
        // Create build and install directories
        fs::create_directories(build_dir);
        fs::create_directories(install_dir);
        
        std::string build_cmd;
        std::string install_cmd;
        
        switch (build_system) {
            case BuildSystem::CMAKE: {
                // CMake configuration (silent)
                std::ostringstream cmake_cmd;
                cmake_cmd << "cd " << fs::absolute(build_dir).string() 
                         << " && cmake -DCMAKE_INSTALL_PREFIX=" << fs::absolute(install_dir).string() 
                         << " -DCMAKE_BUILD_TYPE=Release"
                         << " " << fs::absolute(pkg_path).string()
                         << " >/dev/null 2>&1";
                build_cmd = cmake_cmd.str();
                
                // CMake build (silent)
                std::ostringstream make_cmd;
                make_cmd << "cd " << fs::absolute(build_dir).string() << " && make -j$(nproc) >/dev/null 2>&1";
                install_cmd = make_cmd.str();
                
                // CMake install (silent)
                std::ostringstream install_cmake_cmd;
                install_cmake_cmd << "cd " << fs::absolute(build_dir).string() << " && make install >/dev/null 2>&1";
                install_cmd += " && " + install_cmake_cmd.str();
                break;
            }
            case BuildSystem::MESON: {
                // Meson configuration (silent)
                std::ostringstream meson_cmd;
                meson_cmd << "cd " << fs::absolute(build_dir).string() 
                         << " && meson setup --prefix=" << fs::absolute(install_dir).string() 
                         << " " << fs::absolute(pkg_path).string()
                         << " >/dev/null 2>&1";
                build_cmd = meson_cmd.str();
                
                // Meson build and install (silent)
                std::ostringstream ninja_cmd;
                ninja_cmd << "cd " << fs::absolute(build_dir).string() << " && ninja >/dev/null 2>&1 && ninja install >/dev/null 2>&1";
                install_cmd = ninja_cmd.str();
                break;
            }
            case BuildSystem::NINJA: {
                // Direct Ninja usage (silent)
                std::ostringstream ninja_cmd;
                ninja_cmd << "cd " << fs::absolute(pkg_path).string() << " && ninja >/dev/null 2>&1";
                build_cmd = ninja_cmd.str();
                
                std::ostringstream ninja_install_cmd;
                ninja_install_cmd << "cd " << fs::absolute(pkg_path).string() << " && ninja install >/dev/null 2>&1";
                install_cmd = ninja_install_cmd.str();
                break;
            }
            case BuildSystem::MAKE: {
                // Using Make (silent)
                std::ostringstream make_cmd;
                make_cmd << "cd " << fs::absolute(pkg_path).string() << " && make -j$(nproc) >/dev/null 2>&1";
                build_cmd = make_cmd.str();
                
                std::ostringstream make_install_cmd;
                make_install_cmd << "cd " << fs::absolute(pkg_path).string() << " && make install >/dev/null 2>&1";
                install_cmd = make_install_cmd.str();
                break;
            }
            case BuildSystem::AUTOTOOLS: {
                // Autotools configuration (silent)
                std::ostringstream configure_cmd;
                configure_cmd << "cd " << fs::absolute(pkg_path).string() 
                             << " && ./configure --prefix=" << fs::absolute(install_dir).string()
                             << " >/dev/null 2>&1";
                build_cmd = configure_cmd.str();
                
                // Autotools build and install (silent)
                std::ostringstream make_autotools_cmd;
                make_autotools_cmd << "cd " << fs::absolute(pkg_path).string() 
                                   << " && make -j$(nproc) >/dev/null 2>&1 && make install >/dev/null 2>&1";
                install_cmd = make_autotools_cmd.str();
                break;
            }
            default:
                Paker::Output::error("Unsupported build system");
                return false;
        }
        
        // Execute build command
        Paker::Output::info("Configuring and building package: " + package_name + " (this may take a while)...");
        int build_result = std::system(build_cmd.c_str());
        if (build_result != 0) {
            Paker::Output::error("Build failed: " + package_name);
            return false;
        }
        
        // Execute install command
        Paker::Output::info("Installing package: " + package_name + "...");
        int install_result = std::system(install_cmd.c_str());
        if (install_result != 0) {
            Paker::Output::error("Installation failed: " + package_name);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Paker::Output::error("Error during build and installation: " + std::string(e.what()));
        return false;
    }
}

// Collect installed files
std::vector<std::string> collect_installed_files(const std::string& package_path) {
    std::vector<std::string> files;
    fs::path install_path(package_path);
    
    try {
        if (fs::exists(install_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(install_path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error collecting installed files: " << e.what();
    }
    
    return files;
}

// Install to system and return installed file paths
std::vector<std::string> install_to_system_and_get_files(const std::string& package_path, const std::string& package_name, const std::vector<std::string>& installed_files) {
    // Determine system install directory
    std::string system_install_dir;
    const char* home_dir = std::getenv("HOME");
    if (home_dir) {
        // Install to user directory
        system_install_dir = std::string(home_dir) + "/.local";
    } else {
        // Fallback to current directory
        system_install_dir = ".local";
    }
    
    fs::path system_path(system_install_dir);
    fs::create_directories(system_path);
    
    // Copy files from package install directory to system directory
    fs::path package_install_path(package_path);
    std::vector<std::string> system_installed_files;
    
    try {
        for (const auto& file : installed_files) {
            fs::path source_file(file);
            if (fs::exists(source_file)) {
                // Calculate relative path from package install directory
                fs::path relative_path = fs::relative(source_file, package_install_path);
                fs::path dest_file = system_path / relative_path;
                
                // Create destination directory
                fs::create_directories(dest_file.parent_path());
                
                // Copy file
                fs::copy_file(source_file, dest_file, fs::copy_options::overwrite_existing);
                system_installed_files.push_back(dest_file.string());
            }
        }
        
        Paker::Output::info("Package " + package_name + " installed to system directory: " + system_install_dir);
        return system_installed_files;
        
    } catch (const std::exception& e) {
        Paker::Output::error("Failed to install to system: " + std::string(e.what()));
        return std::vector<std::string>();
    }
}

// Record installation information
void record_installation(const std::string& package_name, const std::string& install_path, const std::vector<std::string>& installed_files) {
    // Ensure .paker/record directory exists
    fs::path record_dir = ".paker/record";
    fs::create_directories(record_dir);
    
    // Record file path
    std::string record_file = record_dir.string() + "/Record_Installing.json";
    
    json record_data;
    
    // If file exists, read existing data
    if (fs::exists(record_file)) {
        try {
            std::ifstream ifs(record_file);
            ifs >> record_data;
            ifs.close();
        } catch (const std::exception& e) {
            LOG(WARNING) << "Failed to read installation record file: " << e.what();
            record_data = json::object();
        }
    }
    
    // Add new installation record
    json package_record;
    package_record["install_path"] = install_path;
    package_record["installed_files"] = installed_files;
    package_record["install_time"] = std::time(nullptr);
    package_record["build_system"] = "detected";
    
    record_data[package_name] = package_record;
    
    // Write record file
    try {
        std::ofstream ofs(record_file);
        ofs << record_data.dump(4);
        ofs.close();
        
        LOG(INFO) << "Recorded installation info for package: " << package_name;
        Paker::Output::success("Recorded installation info for package: " + package_name);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to write installation record file: " << e.what();
        Paker::Output::error("Failed to record installation info");
    }
}

// Remove installation record
void remove_installation_record(const std::string& package_name) {
    fs::path record_dir = ".paker/record";
    std::string record_file = record_dir.string() + "/Record_Installing.json";
    
    if (!fs::exists(record_file)) {
        Paker::Output::warning("Installation record file does not exist");
        return;
    }
    
    try {
        json record_data;
        std::ifstream ifs(record_file);
        ifs >> record_data;
        ifs.close();
        
        if (record_data.contains(package_name)) {
            record_data.erase(package_name);
            
            std::ofstream ofs(record_file);
            ofs << record_data.dump(4);
            ofs.close();
            
            LOG(INFO) << "Removed installation record for package: " << package_name;
            Paker::Output::success("Removed installation record for package: " + package_name);
        } else {
            Paker::Output::warning("Installation record not found for package: " + package_name);
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to remove installation record: " << e.what();
        Paker::Output::error("Failed to remove installation record");
    }
}

// Main install command implementation
void pm_install(const std::string& package) {
    LOG(INFO) << "Starting package installation: " << package;
    Paker::Output::info("Starting package installation: " + package);
    
    // Check if package exists - use standard packages/ directory
    std::string package_path = "packages/" + package;
    if (!fs::exists(package_path)) {
        Paker::Output::error("Package not found: " + package + ", please use 'Paker add' to download first");
        Paker::Output::info("Checked path: " + package_path);
        return;
    }
    
    Paker::Output::info("Found package path: " + package_path);
    
    // Detect build system
    BuildSystem build_system = detect_build_system(package_path);
    if (build_system == BuildSystem::UNKNOWN) {
        Paker::Output::error("Unable to detect supported build system");
        return;
    }
    
    std::string build_system_name;
    switch (build_system) {
        case BuildSystem::CMAKE: build_system_name = "CMake"; break;
        case BuildSystem::MESON: build_system_name = "Meson"; break;
        case BuildSystem::NINJA: build_system_name = "Ninja"; break;
        case BuildSystem::MAKE: build_system_name = "Make"; break;
        case BuildSystem::AUTOTOOLS: build_system_name = "Autotools"; break;
        default: build_system_name = "Unknown"; break;
    }
    
    Paker::Output::info("Detected build system: " + build_system_name);
    
    // Build and install package
    if (!build_and_install_package(package_path, package, build_system)) {
        Paker::Output::error("Package installation failed: " + package);
        return;
    }
    
    // Collect installed files from package install directory
    std::string install_path = package_path + "/install";
    std::vector<std::string> package_files = collect_installed_files(install_path);
    
    // Install to system and get system file paths
    std::vector<std::string> system_files = install_to_system_and_get_files(install_path, package, package_files);
    if (system_files.empty()) {
        Paker::Output::error("System installation failed: " + package);
        return;
    }
    
    // Get system install directory for recording
    std::string system_install_dir;
    const char* home_dir = std::getenv("HOME");
    if (home_dir) {
        system_install_dir = std::string(home_dir) + "/.local";
    } else {
        system_install_dir = ".local";
    }
    
    // Record installation information with system paths
    record_installation(package, system_install_dir, system_files);
    
    Paker::Output::success("Package " + package + " installed successfully (" + std::to_string(system_files.size()) + " files)");
}

// Parallel install command implementation
void pm_install_parallel(const std::vector<std::string>& packages) {
    if (packages.empty()) {
        Paker::Output::warning("No packages specified for installation");
        return;
    }
    
    LOG(INFO) << "Starting parallel installation of " << packages.size() << " packages";
    Paker::Output::info("Starting parallel installation of " + std::to_string(packages.size()) + " packages");
    
    // Initialize parallel executor
    if (!Paker::g_parallel_executor) {
        if (!Paker::initialize_parallel_executor()) {
            Paker::Output::error("Failed to initialize parallel executor");
            return;
        }
    }
    
    std::vector<std::string> task_ids;
    
    // Create installation tasks for each package
    for (const auto& package : packages) {
        // Check if package exists - use standard packages/ directory
        std::string package_path = "packages/" + package;
        if (!fs::exists(package_path)) {
            Paker::Output::warning("Package not found: " + package + ", skipping");
            continue;
        }
        
        // Create installation task
        auto install_task = std::make_shared<Paker::Task>(
            "install_" + package + "_" + std::to_string(std::time(nullptr)),
            Paker::TaskType::INSTALL,
            package
        );
        
        // Set task function
        install_task->task_function = [package]() -> bool {
            try {
                pm_install(package);
                return true;
            } catch (...) {
                return false;
            }
        };
        
        std::string task_id = Paker::g_parallel_executor->submit_task(install_task);
        if (!task_id.empty()) {
            task_ids.push_back(task_id);
        }
    }
    
    // Wait for all tasks to complete
    bool all_success = true;
    for (const auto& task_id : task_ids) {
        if (!Paker::g_parallel_executor->wait_for_task(task_id, std::chrono::minutes(30))) {
            Paker::Output::error("Installation task failed or timed out: " + task_id);
            all_success = false;
        }
    }
    
    if (all_success) {
        Paker::Output::success("All packages installed successfully");
    } else {
        Paker::Output::error("Some packages failed to install");
    }
}

// Uninstall command implementation
void pm_uninstall(const std::string& package) {
    LOG(INFO) << "Starting package uninstallation: " << package;
    Paker::Output::info("Starting package uninstallation: " + package);
    
    // Read installation record
    fs::path record_dir = ".paker/record";
    std::string record_file = record_dir.string() + "/Record_Installing.json";
    
    if (!fs::exists(record_file)) {
        Paker::Output::warning("Installation record file not found");
        return;
    }
    
    json record_data;
    try {
        std::ifstream ifs(record_file);
        ifs >> record_data;
        ifs.close();
    } catch (const std::exception& e) {
        Paker::Output::error("Failed to read installation record file");
        return;
    }
    
    if (!record_data.contains(package)) {
        Paker::Output::warning("Installation record not found for package: " + package);
        return;
    }
    
    // Get installed files list
    auto package_record = record_data[package];
    if (!package_record.contains("installed_files")) {
        Paker::Output::warning("Incomplete installation record for package: " + package);
        return;
    }
    
    std::vector<std::string> installed_files = package_record["installed_files"];
    
    // Delete installed files
    int deleted_count = 0;
    for (const auto& file : installed_files) {
        if (fs::exists(file)) {
            try {
                fs::remove(file);
                deleted_count++;
                LOG(INFO) << "Deleted file: " << file;
            } catch (const std::exception& e) {
                LOG(WARNING) << "Failed to delete file: " << file << " - " << e.what();
            }
        }
    }
    
    // Delete install directory
    if (package_record.contains("install_path")) {
        std::string install_path = package_record["install_path"];
        if (fs::exists(install_path)) {
            try {
                fs::remove_all(install_path);
                LOG(INFO) << "Deleted install directory: " << install_path;
            } catch (const std::exception& e) {
                LOG(WARNING) << "Failed to delete install directory: " << install_path << " - " << e.what();
            }
        }
    }
    
    // Remove installation record
    remove_installation_record(package);
    
    Paker::Output::success("Package " + package + " uninstalled successfully (deleted " + std::to_string(deleted_count) + " files)");
} 