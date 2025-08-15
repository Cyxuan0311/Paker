#include "Paker/core/utils.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/conflict/conflict_resolver.h"
#include "Paker/core/output.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <glog/logging.h>
using json = nlohmann::json;
namespace fs = std::filesystem;

// 全局依赖解析器实例
static DependencyResolver* g_resolver = nullptr;
static DependencyGraph* g_graph = nullptr;

// 初始化依赖解析器
static void init_resolver() {
    if (!g_resolver) {
        g_resolver = new DependencyResolver();
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
        Output::info("Project already initialized.");
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
    Output::success("Initialized Paker project: " + project_name);
}

void pm_add_desc(const std::string& desc) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["description"] = desc;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project description.";
    Output::success("Updated project description.");
}

void pm_add_version(const std::string& vers) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        Output::error("Not a Paker project. Run 'paker init' first.");
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["version"] = vers;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project version.";
    Output::success("Updated project version.");
}

// ==================== 依赖冲突检测与解决功能 ====================

void pm_resolve_dependencies() {
    init_resolver();
    
    LOG(INFO) << "Resolving project dependencies";
    Output::info("Resolving project dependencies...");
    
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve project dependencies";
        Output::error("Failed to resolve project dependencies");
        return;
    }
    
    Output::success("Dependencies resolved successfully");
}

void pm_check_conflicts() {
    init_resolver();
    
    LOG(INFO) << "Checking for dependency conflicts";
    Output::info("Checking for dependency conflicts...");
    
    // 确保依赖已解析
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for conflict checking";
        Output::error("Failed to resolve dependencies for conflict checking");
        return;
    }
    
    // 检测冲突
    ConflictDetector detector(*g_graph);
    auto conflicts = detector.detect_all_conflicts();
    
    if (conflicts.empty()) {
        Output::success("No conflicts detected");
    } else {
        Output::warning("Found " + std::to_string(conflicts.size()) + " conflicts");
        std::string report = detector.generate_conflict_report(conflicts);
        Output::info(report);
    }
}

void pm_resolve_conflicts() {
    init_resolver();
    
    LOG(INFO) << "Resolving dependency conflicts";
    Output::info("Resolving dependency conflicts...");
    
    // 确保依赖已解析
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for conflict resolution";
        Output::error("Failed to resolve dependencies for conflict resolution");
        return;
    }
    
    // 检测冲突
    ConflictDetector detector(*g_graph);
    auto conflicts = detector.detect_all_conflicts();
    
    if (conflicts.empty()) {
        Output::success("No conflicts to resolve");
        return;
    }
    
    // 解决冲突
    ConflictResolver resolver(*g_graph);
    
    // 询问用户是否自动解决
    Output::info("Found " + std::to_string(conflicts.size()) + " conflicts");
    Output::info("Auto-resolve conflicts? [Y/n/i]: ");
    
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
        Output::info("Conflict resolution cancelled");
        return;
    }
    
    if (success) {
        Output::success("Conflicts resolved successfully");
        
        // 重新检测冲突以确认解决
        auto remaining_conflicts = detector.detect_all_conflicts();
        if (remaining_conflicts.empty()) {
            Output::success("All conflicts have been resolved");
        } else {
            Output::warning("Some conflicts remain: " + std::to_string(remaining_conflicts.size()));
        }
    } else {
        Output::error("Failed to resolve all conflicts");
    }
}

void pm_validate_dependencies() {
    init_resolver();
    
    LOG(INFO) << "Validating dependencies";
    Output::info("Validating dependencies...");
    
    // 解析依赖
    if (!g_resolver->resolve_project_dependencies()) {
        LOG(ERROR) << "Failed to resolve dependencies for validation";
        Output::error("Failed to resolve dependencies for validation");
        return;
    }
    
    // 验证依赖图
    if (!g_resolver->validate_dependencies()) {
        LOG(ERROR) << "Dependency validation failed";
        Output::error("Dependency validation failed");
        return;
    }
    
    // 检测冲突
    ConflictDetector detector(*g_graph);
    if (!detector.validate_dependency_graph()) {
        LOG(ERROR) << "Dependency graph validation failed";
        Output::error("Dependency graph validation failed");
        return;
    }
    
    Output::success("Dependencies validated successfully");
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