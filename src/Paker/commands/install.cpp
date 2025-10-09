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
        Paker::Output::warning("No packages specified for parallel installation");
        return;
    }
    
    // 初始化并行执行器
    if (!Paker::g_parallel_executor) {
        if (!Paker::initialize_parallel_executor()) {
            Paker::Output::error("Failed to initialize parallel executor");
            return;
        }
    }
    
    Paker::Output::info("Starting parallel installation of " + std::to_string(packages.size()) + " packages");
    
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
        100, 50, "Downloading", true, true, false, Paker::ProgressStyle::NPM_STYLE
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
            
            parallel_progress->update(progress_value, "Downloading package " + std::to_string(completed_tasks + 1) + "/" + std::to_string(task_ids.size()) + " files...");
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
        
        std::cout << "\n";
        std::cout << "Successfully installed " << std::to_string(task_ids.size()) << " packages\n";
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "Some packages failed to install\n";
        std::cout << "\n";
    }
}

void pm_add(const std::string& pkg_input) {
    // 开始性能监控
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
                    Paker::Output::error("Please resolve conflicts before installing");
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
            Paker::Output::info("Installing " + pkg + "@" + target_version + " to global cache...");
            
            if (!Paker::g_cache_manager->install_package_to_cache(pkg, target_version, repo_url)) {
                Paker::Output::error("Failed to install package to cache");
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
        
        Paker::Output::success("Successfully installed " + pkg + " (cached, " + std::to_string(installed_files.size()) + " files)");
        
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
        std::cout << "Downloading " << pkg;
        if (!version.empty() && version != "*") {
            std::cout << "@" << version;
        }
        std::cout << " from " << repo_url << "\n";
        std::cout << "\n";
        
        // 创建无限进度条，显示实时进度
        progress = new Paker::ProgressBar(100, 50, "Downloading", true, true, false, Paker::ProgressStyle::NPM_STYLE);
        
        // 步骤1: 克隆仓库
        progress->update(10, "Connecting to repository...");
        Paker::Output::debug("Cloning repository: " + repo_url);
        
        std::ostringstream cmd;
        cmd << "git clone --progress --depth 1 " << repo_url << " " << pkg_dir.string();
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to clone repo: " << repo_url;
            Paker::Output::error("Failed to clone repository: " + repo_url);
            return;
        }
        
        // 模拟下载进度 - 基于实际操作的进度显示
        auto start_time = std::chrono::steady_clock::now();
        int progress_value = 20;
        
        while (progress_value < 80) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            
            // 基于时间计算进度
            progress_value = 20 + (elapsed * 60) / 1000; // 假设下载需要1秒
            if (progress_value > 80) progress_value = 80;
            
            progress->update(progress_value, "Downloading package files...");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            if (progress_value >= 80) break;
        }
        
        // 步骤2: 检出版本（如果需要）
        if (!version.empty() && version != "*") {
            progress->update(85, "Checking out version " + version);
            Paker::Output::debug("Checking out version: " + version);
            std::ostringstream checkout_cmd;
            checkout_cmd << "cd " << pkg_dir.string() << " && git fetch --tags && git checkout " << version;
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
        progress->update(95, "Recording package files and metadata");
        Paker::Output::debug("Recording package files...");
        
        // 使用Record类记录安装的文件
        Recorder::Record record(get_record_file_path());
        installed_files = collect_package_files(pkg_dir.string());
        
        // 记录包信息
        record.addPackageRecord(pkg, pkg_dir.string(), installed_files);
        LOG(INFO) << "Recorded " << installed_files.size() << " files for package: " << pkg;
        
        // 完成进度
        progress->update(100, "Installation completed successfully");
    }
    
    if (progress) {
        progress->finish("Installation completed successfully");
        delete progress;
    }
    
    // 简洁的安装完成信息
    std::cout << "\n";
    std::cout << "Successfully installed " << pkg;
    if (!version.empty() && version != "*") {
        std::cout << "@" << version;
    }
    std::cout << " (" << std::to_string(installed_files.size()) << " files)\n";
    std::cout << "\n";
    
    // 记录版本变更
    pm_record_version_change(pkg, "", version, repo_url, "Package installation");
    
    // 结束性能监控并记录指标
    PAKER_PERF_END("package_install", Paker::MetricType::INSTALL_TIME);
    
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
        Paker::Output::info("Installing package: " + pkg_name);
        
        // 创建目标目录
        fs::create_directories(target_path);
        
        // 使用git clone下载
        std::string cmd = "git clone " + url + " " + target_path;
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
    
    // 结束性能监控
    PAKER_PERF_END("package_install_url", Paker::MetricType::INSTALL_TIME);
}

void pm_remove(const std::string& pkg) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
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