#include "Paker/core/utils.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/conflict/conflict_resolver.h"
#include "Paker/core/output.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/version_history.h"
#include "Paker/core/service_container.h"
#include "Paker/core/core_services.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <glog/logging.h>
using json = nlohmann::json;
namespace fs = std::filesystem;

// 初始化所有Paker服务的函数
bool initialize_paker_services() {
    try {
        // 初始化服务管理器
        if (!Paker::initialize_service_manager()) {
            LOG(ERROR) << "Failed to initialize service manager";
            return false;
        }
        
        // 注册并初始化所有核心服务
        if (!Paker::ServiceFactory::register_all_core_services()) {
            LOG(ERROR) << "Failed to register core services";
            return false;
        }
        
        LOG(INFO) << "All Paker services initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception during service initialization: " << e.what();
        return false;
    }
}

void pm_init() {
    std::string json_file = get_json_file();
    if (fs::exists(json_file)) {
        Paker::Output::info("Project already initialized.");
        // 即使项目已存在，也要确保服务已初始化
        initialize_paker_services();
        return;
    }
    
    // 轻量级初始化 - 只创建必要的目录和文件
    std::string project_name = get_project_name();
    json j = {
        {"name", project_name},
        {"version", "0.1.0"},
        {"description", ""},
        {"dependencies", json::object()}
    };
    
    // 创建项目目录
    fs::create_directories(".paker");
    fs::create_directories(".paker/cache");
    fs::create_directories(".paker/links");
    
    // 写入配置文件
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    
    // 初始化所有Paker服务
    if (!initialize_paker_services()) {
        Paker::Output::warning("Some services failed to initialize, but project was created successfully");
    }
    
    Paker::Output::success("Initialized Paker project: " + project_name);
    Paker::Output::info("Project configuration: " + json_file);
    Paker::Output::info("Cache directory: .paker/cache");
}

void pm_add_desc(const std::string& desc) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["description"] = desc;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project description.";
    Paker::Output::success("Updated project description.");
}

void pm_add_version(const std::string& vers) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Paker::Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["version"] = vers;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project version.";
    Paker::Output::success("Updated project version.");
}

// ==================== 依赖冲突检测与解决功能 ====================

void pm_resolve_dependencies() {
    LOG(INFO) << "Resolving project dependencies";
    Paker::Output::info("Resolving project dependencies...");
    
    // 使用轻量级依赖解析器，跳过重型服务初始化
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 直接创建依赖解析器，不通过服务管理器
    std::unique_ptr<Paker::DependencyResolver> resolver = std::make_unique<Paker::DependencyResolver>();
    
    if (!resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        Paker::Output::error("Failed to resolve project dependencies");
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    LOG(INFO) << "Dependency resolution completed in " << duration.count() << "ms";
    Paker::Output::success("Dependencies resolved successfully");
}

void pm_check_conflicts() {
    LOG(INFO) << "Checking for dependency conflicts";
    Paker::Output::info("Checking for dependency conflicts...");
    
    // 使用轻量级依赖解析器，跳过重型服务初始化
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 直接创建依赖解析器，不通过服务管理器
    std::unique_ptr<Paker::DependencyResolver> resolver = std::make_unique<Paker::DependencyResolver>();
    
    if (!resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        Paker::Output::error("Failed to resolve project dependencies");
        return;
    }
    
    auto& graph = resolver->get_dependency_graph();
    
    // 检测冲突
    Paker::ConflictDetector detector(graph);
    auto conflicts = detector.detect_all_conflicts();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (conflicts.empty()) {
        Paker::Output::success("No conflicts detected");
    } else {
        Paker::Output::warning("Found " + std::to_string(conflicts.size()) + " conflicts");
        std::string report = detector.generate_conflict_report(conflicts);
        Paker::Output::info(report);
    }
    
    LOG(INFO) << "Conflict checking completed in " << duration.count() << "ms";
}

void pm_resolve_conflicts() {
    LOG(INFO) << "Resolving dependency conflicts";
    Paker::Output::info("Resolving dependency conflicts...");
    
    // 使用轻量级依赖解析器，跳过重型服务初始化
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 直接创建依赖解析器，不通过服务管理器
    std::unique_ptr<Paker::DependencyResolver> resolver = std::make_unique<Paker::DependencyResolver>();
    
    if (!resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        Paker::Output::error("Failed to resolve project dependencies");
        return;
    }
    
    auto& graph = resolver->get_dependency_graph();
    
    // 检测冲突
    Paker::ConflictDetector detector(graph);
    auto conflicts = detector.detect_all_conflicts();
    
    if (conflicts.empty()) {
        Paker::Output::success("No conflicts to resolve");
        return;
    }
    
    // 解决冲突
    Paker::ConflictResolver conflict_resolver(graph);
    
    // 询问用户是否自动解决
    Paker::Output::info("Found " + std::to_string(conflicts.size()) + " conflicts");
    Paker::Output::info("Auto-resolve conflicts? [Y/n/i]: ");
    
    std::string response;
    std::getline(std::cin, response);
    
    bool success = false;
    if (response.empty() || response[0] == 'Y' || response[0] == 'y') {
        // 自动解决
        success = conflict_resolver.auto_resolve_conflicts(conflicts);
    } else if (response[0] == 'i' || response[0] == 'I') {
        // 交互式解决
        success = conflict_resolver.interactive_resolve_conflicts(conflicts);
    } else {
        Paker::Output::info("Conflict resolution cancelled");
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (success) {
        Paker::Output::success("Conflicts resolved successfully");
        
        // 重新检测冲突以确认解决
        auto remaining_conflicts = detector.detect_all_conflicts();
        if (remaining_conflicts.empty()) {
            Paker::Output::success("All conflicts have been resolved");
        } else {
            Paker::Output::warning("Some conflicts remain: " + std::to_string(remaining_conflicts.size()));
        }
    } else {
        Paker::Output::error("Failed to resolve all conflicts");
    }
    
    LOG(INFO) << "Conflict resolution completed in " << duration.count() << "ms";
}

void pm_validate_dependencies() {
    LOG(INFO) << "Validating dependencies";
    Paker::Output::info("Validating dependencies...");
    
    // 使用轻量级依赖解析器，跳过重型服务初始化
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 直接创建依赖解析器，不通过服务管理器
    std::unique_ptr<Paker::DependencyResolver> resolver = std::make_unique<Paker::DependencyResolver>();
    
    // 解析依赖
    if (!resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for validation";
        Paker::Output::error("Failed to resolve dependencies for validation");
        return;
    }
    
    // 验证依赖图
    if (!resolver->validate_dependencies()) {
        LOG(ERROR) << "Dependency validation failed";
        Paker::Output::error("Dependency validation failed");
        return;
    }
    
    auto& graph = resolver->get_dependency_graph();
    
    // 检测冲突
    Paker::ConflictDetector detector(graph);
    if (!detector.validate_dependency_graph()) {
        LOG(ERROR) << "Dependency graph validation failed";
        Paker::Output::error("Dependency graph validation failed");
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    LOG(INFO) << "Dependency validation completed in " << duration.count() << "ms";
    Paker::Output::success("Dependencies validated successfully");
}

void pm_record_version_change(const std::string& package_name, 
                            const std::string& old_version,
                            const std::string& new_version,
                            const std::string& repository_url,
                            const std::string& reason) {
    auto* history_manager = Paker::get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        return;
    }
    
    if (!history_manager->record_version_change(package_name, old_version, new_version, repository_url, reason)) {
        LOG(ERROR) << "Failed to record version change for " << package_name;
    }
}

// ==================== 资源管理 ====================

// 在程序退出时清理资源 - 使用RAII模式
class ServiceCleanup {
public:
    ServiceCleanup() = default;
    
    ~ServiceCleanup() {
        try {
            Paker::cleanup_service_manager();
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during service cleanup: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Unknown exception during service cleanup";
        }
    }
    
    // 禁止拷贝和移动
    ServiceCleanup(const ServiceCleanup&) = delete;
    ServiceCleanup& operator=(const ServiceCleanup&) = delete;
    ServiceCleanup(ServiceCleanup&&) = delete;
    ServiceCleanup& operator=(ServiceCleanup&&) = delete;
};

// 全局清理对象 - 确保在程序退出时自动清理
static ServiceCleanup g_cleanup; 