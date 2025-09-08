#include "Paker/core/utils.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/conflict/conflict_resolver.h"
#include "Paker/core/output.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/version_history.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <glog/logging.h>
using json = nlohmann::json;
namespace fs = std::filesystem;

// 全局依赖解析器实例
static Paker::DependencyResolver* g_resolver = nullptr;
static Paker::DependencyGraph* g_graph = nullptr;

// 初始化依赖解析器
static void init_resolver() {
    if (!g_resolver) {
        g_resolver = new Paker::DependencyResolver();
        g_graph = &g_resolver->get_dependency_graph();
    }
}

// 清理依赖解析器
static void cleanup_resolver() {
    if (g_resolver) {
        delete g_resolver;
        g_resolver = nullptr;
        g_graph = nullptr;
    }
}

void pm_init() {
    std::string json_file = get_json_file();
    if (fs::exists(json_file)) {
        LOG(INFO) << "Project already initialized.";
        Paker::Output::info("Project already initialized.");
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
    Paker::Output::success("Initialized Paker project: " + project_name);
    
    // 初始化全局缓存管理器（默认启用）
    if (!Paker::g_cache_manager) {
        if (Paker::initialize_cache_manager()) {
            Paker::Output::success("Global cache system initialized (default mode)");
            Paker::Output::info("Cache locations:");
            Paker::Output::info("  - User cache: ~/.paker/cache");
            Paker::Output::info("  - Global cache: /usr/local/share/paker/cache");
            Paker::Output::info("  - Project links: .paker/links");
        } else {
            Paker::Output::warning("Failed to initialize global cache system, falling back to legacy mode");
        }
    }
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
    init_resolver();
    
    LOG(INFO) << "Resolving project dependencies";
    Paker::Output::info("Resolving project dependencies...");
    
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        Paker::Output::error("Failed to resolve project dependencies");
        return;
    }
    
    Paker::Output::success("Dependencies resolved successfully");
}

void pm_check_conflicts() {
    init_resolver();
    
    LOG(INFO) << "Checking for dependency conflicts";
    Paker::Output::info("Checking for dependency conflicts...");
    
    // 确保依赖已解析
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for conflict checking";
        Paker::Output::error("Failed to resolve dependencies for conflict checking");
        return;
    }
    
    // 检测冲突
    Paker::ConflictDetector detector(*g_graph);
    auto conflicts = detector.detect_all_conflicts();
    
    if (conflicts.empty()) {
        Paker::Output::success("No conflicts detected");
    } else {
        Paker::Output::warning("Found " + std::to_string(conflicts.size()) + " conflicts");
        std::string report = detector.generate_conflict_report(conflicts);
        Paker::Output::info(report);
    }
}

void pm_resolve_conflicts() {
    init_resolver();
    
    LOG(INFO) << "Resolving dependency conflicts";
    Paker::Output::info("Resolving dependency conflicts...");
    
    // 确保依赖已解析
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for conflict resolution";
        Paker::Output::error("Failed to resolve dependencies for conflict resolution");
        return;
    }
    
    // 检测冲突
    Paker::ConflictDetector detector(*g_graph);
    auto conflicts = detector.detect_all_conflicts();
    
    if (conflicts.empty()) {
        Paker::Output::success("No conflicts to resolve");
        return;
    }
    
    // 解决冲突
    Paker::ConflictResolver resolver(*g_graph);
    
    // 询问用户是否自动解决
    Paker::Output::info("Found " + std::to_string(conflicts.size()) + " conflicts");
    Paker::Output::info("Auto-resolve conflicts? [Y/n/i]: ");
    
    std::string response;
    std::getline(std::cin, response);
    
    bool success = false;
    if (response.empty() || response[0] == 'Y' || response[0] == 'y') {
        // 自动解决
        success = resolver.auto_resolve_conflicts(conflicts);
    } else if (response[0] == 'i' || response[0] == 'I') {
        // 交互式解决
        success = resolver.interactive_resolve_conflicts(conflicts);
    } else {
        Paker::Output::info("Conflict resolution cancelled");
        return;
    }
    
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
}

void pm_validate_dependencies() {
    init_resolver();
    
    LOG(INFO) << "Validating dependencies";
    Paker::Output::info("Validating dependencies...");
    
    // 解析依赖
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for validation";
        Paker::Output::error("Failed to resolve dependencies for validation");
        return;
    }
    
    // 验证依赖图
    if (!g_resolver->validate_dependencies()) {
        LOG(ERROR) << "Dependency validation failed";
        Paker::Output::error("Dependency validation failed");
        return;
    }
    
    // 检测冲突
    Paker::ConflictDetector detector(*g_graph);
    if (!detector.validate_dependency_graph()) {
        LOG(ERROR) << "Dependency graph validation failed";
        Paker::Output::error("Dependency graph validation failed");
        return;
    }
    
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

// 在程序退出时清理资源
class ResolverCleanup {
public:
    ~ResolverCleanup() {
        cleanup_resolver();
    }
};

static ResolverCleanup cleanup; 