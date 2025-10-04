#include "Paker/monitor/diagnostic_tool.h"
#include "Paker/core/output.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/core/utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <glog/logging.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

DiagnosticTool::DiagnosticTool(const DependencyGraph& graph) : graph_(graph) {
    initialize_rules();
}

void DiagnosticTool::initialize_rules() {
    rules_.push_back(std::make_unique<CircularDependencyRule>());
    rules_.push_back(std::make_unique<VersionConflictRule>());
    rules_.push_back(std::make_unique<MissingDependencyRule>());
}

DiagnosticResult DiagnosticTool::diagnose() {
    DiagnosticResult result;
    
    // 执行所有诊断规则
    for (const auto& rule : rules_) {
        auto issues = rule->check(graph_);
        result.issues.insert(result.issues.end(), issues.begin(), issues.end());
    }
    
    // 检查配置问题
    auto config_issues = check_configuration();
    result.issues.insert(result.issues.end(), config_issues.begin(), config_issues.end());
    
    // 检查依赖问题
    auto dep_issues = check_dependencies();
    result.issues.insert(result.issues.end(), dep_issues.begin(), dep_issues.end());
    
    // 检查性能问题
    auto perf_issues = check_performance();
    result.issues.insert(result.issues.end(), perf_issues.begin(), perf_issues.end());
    
    // 检查文件系统问题
    auto fs_issues = check_filesystem();
    result.issues.insert(result.issues.end(), fs_issues.begin(), fs_issues.end());
    
    // 统计问题级别
    for (const auto& issue : result.issues) {
        switch (issue.level) {
            case DiagnosticLevel::CRITICAL:
                result.has_critical_issues = true;
                break;
            case DiagnosticLevel::ERROR:
                result.has_errors = true;
                break;
            case DiagnosticLevel::WARNING:
                result.has_warnings = true;
                break;
            default:
                break;
        }
    }
    
    // 生成摘要
    std::ostringstream summary;
    summary << "Diagnostic completed. Found ";
    summary << result.issues.size() << " issues: ";
    
    if (result.has_critical_issues) {
        summary << "CRITICAL issues detected! ";
    }
    if (result.has_errors) {
        summary << "Errors found. ";
    }
    if (result.has_warnings) {
        summary << "Warnings found. ";
    }
    
    result.summary = summary.str();
    
    return result;
}

std::string DiagnosticTool::generate_diagnostic_report(const DiagnosticResult& result) {
    std::ostringstream report;
    report << " Diagnostic Report\n";
    report << "===================\n\n";
    
    report << "Summary: " << result.summary << "\n\n";
    
    if (result.issues.empty()) {
        report << "[OK] No issues found. Your project is healthy!\n";
        return report.str();
    }
    
    // 按级别分组
    std::map<DiagnosticLevel, std::vector<DiagnosticIssue>> grouped_issues;
    for (const auto& issue : result.issues) {
        grouped_issues[issue.level].push_back(issue);
    }
    
    // 按级别顺序显示
    std::vector<DiagnosticLevel> levels = {
        DiagnosticLevel::CRITICAL,
        DiagnosticLevel::ERROR,
        DiagnosticLevel::WARNING,
        DiagnosticLevel::INFO
    };
    
    for (auto level : levels) {
        auto it = grouped_issues.find(level);
        if (it == grouped_issues.end()) continue;
        
        report << format_level(level) << " (" << it->second.size() << ")\n";
        report << std::string(50, '-') << "\n";
        
        for (const auto& issue : it->second) {
            report << "Category: " << issue.category << "\n";
            report << "Message: " << issue.message << "\n";
            
            if (!issue.description.empty()) {
                report << "Description: " << issue.description << "\n";
            }
            
            if (!issue.suggestions.empty()) {
                report << "Suggestions:\n";
                for (const auto& suggestion : issue.suggestions) {
                    report << "  - " << suggestion << "\n";
                }
            }
            
            if (!issue.context.empty()) {
                report << "Context:\n";
                for (const auto& [key, value] : issue.context) {
                    report << "  " << key << ": " << value << "\n";
                }
            }
            
            report << "\n";
        }
    }
    
    return report.str();
}

std::vector<DiagnosticIssue> DiagnosticTool::check_configuration() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查项目配置文件
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Configuration", "Project configuration file not found");
        issue.description = "Paker.json file is missing. Run 'paker init' to create it.";
        issue.suggestions.push_back("Run 'paker init' to initialize the project");
        issues.push_back(issue);
        return issues;
    }
    
    // 检查配置文件格式
    try {
        std::ifstream file(json_file);
        json j;
        file >> j;
        
        if (!j.contains("name")) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Configuration", "Project name not specified");
            issue.description = "The 'name' field is missing in Paker.json";
            issue.suggestions.push_back("Add a 'name' field to your Paker.json");
            issues.push_back(issue);
        }
        
        if (!j.contains("version")) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Configuration", "Project version not specified");
            issue.description = "The 'version' field is missing in Paker.json";
            issue.suggestions.push_back("Add a 'version' field to your Paker.json");
            issues.push_back(issue);
        }
        
    } catch (const std::exception& e) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Configuration", "Invalid JSON format in Paker.json");
        issue.description = "The Paker.json file contains invalid JSON";
        issue.suggestions.push_back("Fix the JSON syntax in Paker.json");
        issue.context["error"] = e.what();
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_dependencies() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查依赖图是否为空
    if (graph_.empty()) {
        DiagnosticIssue issue(DiagnosticLevel::INFO, "Dependencies", "No dependencies found");
        issue.description = "The project has no dependencies configured";
        issue.suggestions.push_back("Add dependencies using 'paker add <package>'");
        issues.push_back(issue);
        return issues;
    }
    
    // 检查依赖解析
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph_.has_node(dep)) {
                DiagnosticIssue issue(DiagnosticLevel::ERROR, "Dependencies", 
                                    "Missing dependency: " + dep);
                issue.description = "Package '" + package + "' depends on '" + dep + 
                                  "' which is not available";
                issue.suggestions.push_back("Install the missing dependency: 'paker add " + dep + "'");
                issue.suggestions.push_back("Check if the dependency name is correct");
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_performance() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查依赖深度
    std::map<std::string, size_t> depths;
    for (const auto& [package, _] : graph_.get_nodes()) {
        size_t depth = 0;
        const auto* node = graph_.get_node(package);
        if (node) {
            for (const auto& dep : node->dependencies) {
                depth = std::max(depth, depths[dep] + 1);
            }
        }
        depths[package] = depth;
        
        if (depth > 5) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Performance", 
                                "Deep dependency chain: " + package);
            issue.description = "Package '" + package + "' has a dependency depth of " + 
                              std::to_string(depth) + " levels";
            issue.suggestions.push_back("Consider flattening the dependency tree");
            issue.suggestions.push_back("Look for alternative packages with fewer dependencies");
            issue.context["depth"] = std::to_string(depth);
            issues.push_back(issue);
        }
    }
    
    // 检查包大小
    for (const auto& [package, node] : graph_.get_nodes()) {
        std::string package_path = "packages/" + package;
        if (fs::exists(package_path)) {
            size_t total_size = 0;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(package_path)) {
                    if (entry.is_regular_file()) {
                        total_size += fs::file_size(entry.path());
                    }
                }
                
                if (total_size > 100 * 1024 * 1024) { // 100MB
                    DiagnosticIssue issue(DiagnosticLevel::WARNING, "Performance", 
                                        "Large package: " + package);
                    issue.description = "Package '" + package + "' is very large (" + 
                                      std::to_string(total_size / (1024 * 1024)) + "MB)";
                    issue.suggestions.push_back("Consider using a lighter alternative");
                    issue.suggestions.push_back("Check if you need all components of this package");
                    issue.context["size_mb"] = std::to_string(total_size / (1024 * 1024));
                    issues.push_back(issue);
                }
            } catch (const std::exception& e) {
                // 忽略文件系统错误
            }
        }
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_filesystem() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查packages目录
    if (!fs::exists("packages")) {
        DiagnosticIssue issue(DiagnosticLevel::INFO, "Filesystem", "Packages directory not found");
        issue.description = "The 'packages' directory does not exist";
        issue.suggestions.push_back("This is normal for new projects");
        issue.suggestions.push_back("Run 'paker add <package>' to install dependencies");
        issues.push_back(issue);
        return issues;
    }
    
    // 检查packages目录权限
    try {
        fs::directory_iterator("packages");
    } catch (const std::exception& e) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Filesystem", "Cannot access packages directory");
        issue.description = "Permission denied or directory is corrupted";
        issue.suggestions.push_back("Check directory permissions");
        issue.suggestions.push_back("Try running with elevated privileges");
        issue.context["error"] = e.what();
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_network() {
    std::vector<DiagnosticIssue> issues;
    
    // 这里可以添加网络连接检查
    // 简化实现，实际可以ping仓库地址
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_security() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查是否有可疑的依赖
    std::vector<std::string> suspicious_patterns = {
        "test", "example", "demo", "sample"
    };
    
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& pattern : suspicious_patterns) {
            if (package.find(pattern) != std::string::npos) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Security", 
                                    "Suspicious package name: " + package);
                issue.description = "Package name contains suspicious pattern: " + pattern;
                issue.suggestions.push_back("Verify this is the correct package");
                issue.suggestions.push_back("Check the package source and authenticity");
                issue.context["pattern"] = pattern;
                issues.push_back(issue);
                break;
            }
        }
    }
    
    return issues;
}

std::vector<std::string> DiagnosticTool::generate_fix_suggestions(const DiagnosticResult& result) {
    std::vector<std::string> suggestions;
    
    for (const auto& issue : result.issues) {
        suggestions.insert(suggestions.end(), issue.suggestions.begin(), issue.suggestions.end());
    }
    
    // 去重
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());
    
    return suggestions;
}

bool DiagnosticTool::export_diagnostic_result(const DiagnosticResult& result, const std::string& filename) {
    try {
        json j;
        j["summary"] = result.summary;
        j["has_critical_issues"] = result.has_critical_issues;
        j["has_errors"] = result.has_errors;
        j["has_warnings"] = result.has_warnings;
        
        j["issues"] = json::array();
        for (const auto& issue : result.issues) {
            json issue_json;
            issue_json["level"] = static_cast<int>(issue.level);
            issue_json["category"] = issue.category;
            issue_json["message"] = issue.message;
            issue_json["description"] = issue.description;
            issue_json["suggestions"] = issue.suggestions;
            issue_json["context"] = issue.context;
            j["issues"].push_back(issue_json);
        }
        
        std::ofstream file(filename);
        file << j.dump(4);
        
        LOG(INFO) << "Diagnostic result exported to: " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to export diagnostic result: " << e.what();
        return false;
    }
}

std::string DiagnosticTool::format_level(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::CRITICAL: return "🚨 CRITICAL";
        case DiagnosticLevel::ERROR: return "[FAIL] ERROR";
        case DiagnosticLevel::WARNING: return "⚠️  WARNING";
        case DiagnosticLevel::INFO: return "ℹ️  INFO";
        default: return "UNKNOWN";
    }
}

// 诊断规则实现
std::vector<DiagnosticIssue> CircularDependencyRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    auto cycles = graph.detect_cycles();
    for (const auto& cycle : cycles) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Circular Dependency", 
                            "Circular dependency detected");
        
        std::ostringstream desc;
        desc << "Circular dependency: ";
        for (size_t i = 0; i < cycle.size(); ++i) {
            if (i > 0) desc << " -> ";
            desc << cycle[i];
        }
        issue.description = desc.str();
        
        issue.suggestions.push_back("Break the circular dependency by restructuring packages");
        issue.suggestions.push_back("Use interfaces or abstractions to decouple packages");
        
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> VersionConflictRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    ConflictDetector detector(graph);
    auto conflicts = detector.detect_version_conflicts();
    
    for (const auto& conflict : conflicts) {
        DiagnosticIssue issue(DiagnosticLevel::WARNING, "Version Conflict", 
                            "Version conflict: " + conflict.package_name);
        
        std::ostringstream desc;
        desc << "Conflicting versions: ";
        for (size_t i = 0; i < conflict.conflicting_versions.size(); ++i) {
            if (i > 0) desc << ", ";
            desc << conflict.conflicting_versions[i];
        }
        issue.description = desc.str();
        
        issue.suggestions.push_back("Resolve version conflicts using 'paker resolve-conflicts'");
        issue.suggestions.push_back("Update or downgrade conflicting packages");
        
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> MissingDependencyRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    for (const auto& [package, node] : graph.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph.has_node(dep)) {
                DiagnosticIssue issue(DiagnosticLevel::ERROR, "Missing Dependency", 
                                    "Missing dependency: " + dep);
                issue.description = "Package '" + package + "' depends on '" + dep + 
                                  "' which is not available";
                issue.suggestions.push_back("Install the missing dependency: 'paker add " + dep + "'");
                issue.suggestions.push_back("Check if the dependency name is correct");
                
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}

} // namespace Paker 