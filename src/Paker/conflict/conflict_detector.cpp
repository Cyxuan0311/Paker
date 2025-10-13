#include "Paker/conflict/conflict_detector.h"
#include "Paker/dependency/version_manager.h"
#include "Paker/core/output.h"
#include <algorithm>
#include <sstream>
#include <glog/logging.h>

namespace Paker {

ConflictDetector::ConflictDetector(const DependencyGraph& graph) : graph_(graph) {}

std::vector<ConflictInfo> ConflictDetector::detect_all_conflicts() {
    std::vector<ConflictInfo> all_conflicts;
    
    // 检测版本冲突
    auto version_conflicts = detect_version_conflicts();
    all_conflicts.insert(all_conflicts.end(), version_conflicts.begin(), version_conflicts.end());
    
    // 检测循环依赖
    auto circular_conflicts = detect_circular_dependencies();
    all_conflicts.insert(all_conflicts.end(), circular_conflicts.begin(), circular_conflicts.end());
    
    // 检测缺失依赖
    auto missing_conflicts = detect_missing_dependencies();
    all_conflicts.insert(all_conflicts.end(), missing_conflicts.begin(), missing_conflicts.end());
    
    return all_conflicts;
}

std::vector<ConflictInfo> ConflictDetector::detect_version_conflicts() {
    std::vector<ConflictInfo> conflicts;
    
    // 遍历所有包
    for (const auto& [package, node] : graph_.get_nodes()) {
        // 检查该包在不同路径中的版本要求
        auto paths = graph_.get_all_paths_to_package(package);
        
        std::map<std::string, std::vector<std::vector<std::string>>> version_paths;
        
        for (const auto& path : paths) {
            std::string required_version = calculate_required_version(path);
            if (!required_version.empty()) {
                version_paths[required_version].push_back(path);
            }
        }
        
        // 检查是否有多个不同的版本要求
        if (version_paths.size() > 1) {
            std::vector<std::string> conflicting_versions;
            std::vector<std::string> conflict_path;
            
            for (const auto& [version, paths] : version_paths) {
                conflicting_versions.push_back(version);
                // 使用第一个路径作为冲突路径示例
                if (conflict_path.empty() && !paths.empty()) {
                    conflict_path = paths[0];
                }
            }
            
            // 检查版本是否真的不兼容
            bool has_conflict = false;
            for (size_t i = 0; i < conflicting_versions.size(); ++i) {
                for (size_t j = i + 1; j < conflicting_versions.size(); ++j) {
                    if (!is_version_compatible(conflicting_versions[i], conflicting_versions[j])) {
                        has_conflict = true;
                        break;
                    }
                }
                if (has_conflict) break;
            }
            
            if (has_conflict) {
                ConflictInfo conflict(ConflictInfo::Type::VERSION_CONFLICT, package);
                conflict.conflicting_versions = conflicting_versions;
                conflict.conflict_path = conflict_path;
                conflict.suggested_solution = generate_solution_suggestion(package, 
                                                                         conflicting_versions[0], 
                                                                         conflicting_versions[1]);
                conflicts.push_back(conflict);
            }
        }
    }
    
    return conflicts;
}

std::vector<ConflictInfo> ConflictDetector::detect_circular_dependencies() {
    std::vector<ConflictInfo> conflicts;
    auto cycles = graph_.detect_cycles();
    
    for (const auto& cycle : cycles) {
        ConflictInfo conflict(ConflictInfo::Type::CIRCULAR_DEPENDENCY, cycle.front());
        conflict.conflict_path = cycle;
        conflict.suggested_solution = "Consider breaking the circular dependency by restructuring packages";
        conflicts.push_back(conflict);
    }
    
    return conflicts;
}

std::vector<ConflictInfo> ConflictDetector::detect_missing_dependencies() {
    std::vector<ConflictInfo> conflicts;
    
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph_.has_node(dep) && !package_exists_in_repository(dep)) {
                ConflictInfo conflict(ConflictInfo::Type::MISSING_DEPENDENCY, dep);
                conflict.conflict_path = {package, dep};
                conflict.suggested_solution = "Package '" + dep + "' is not available in any repository";
                conflicts.push_back(conflict);
            }
        }
    }
    
    return conflicts;
}

std::string ConflictDetector::generate_conflict_report(const std::vector<ConflictInfo>& conflicts) {
    if (conflicts.empty()) {
        return "No conflicts detected.";
    }
    
    std::ostringstream report;
    report << "Dependency Conflicts Detected\n\n";
    
    for (size_t i = 0; i < conflicts.size(); ++i) {
        const auto& conflict = conflicts[i];
        report << "Conflict " << (i + 1) << ":\n";
        report << "Package: " << conflict.package_name << "\n";
        
        switch (conflict.type) {
            case ConflictInfo::Type::VERSION_CONFLICT:
                report << "Type: Version Conflict\n";
                report << "Conflicting Versions:\n";
                for (const auto& version : conflict.conflicting_versions) {
                    report << "  - " << version << "\n";
                }
                break;
                
            case ConflictInfo::Type::CIRCULAR_DEPENDENCY:
                report << "Type: Circular Dependency\n";
                report << "Dependency Cycle:\n";
                for (size_t j = 0; j < conflict.conflict_path.size(); ++j) {
                    report << "  " << conflict.conflict_path[j];
                    if (j < conflict.conflict_path.size() - 1) {
                        report << " -> ";
                    }
                }
                report << "\n";
                break;
                
            case ConflictInfo::Type::MISSING_DEPENDENCY:
                report << "Type: Missing Dependency\n";
                report << "Missing Package: " << conflict.package_name << "\n";
                break;
        }
        
        if (!conflict.conflict_path.empty()) {
            report << "Conflict Path: ";
            for (size_t j = 0; j < conflict.conflict_path.size(); ++j) {
                report << conflict.conflict_path[j];
                if (j < conflict.conflict_path.size() - 1) {
                    report << " -> ";
                }
            }
            report << "\n";
        }
        
        if (!conflict.suggested_solution.empty()) {
            report << "Suggested Solution: " << conflict.suggested_solution << "\n";
        }
        
        report << "\n";
    }
    
    return report.str();
}

std::vector<ConflictInfo> ConflictDetector::detect_package_conflicts(const std::string& package_name) {
    std::vector<ConflictInfo> conflicts;
    
    // 检测该包的版本冲突
    auto paths = graph_.get_all_paths_to_package(package_name);
    std::map<std::string, std::vector<std::vector<std::string>>> version_paths;
    
    for (const auto& path : paths) {
        std::string required_version = calculate_required_version(path);
        if (!required_version.empty()) {
            version_paths[required_version].push_back(path);
        }
    }
    
    if (version_paths.size() > 1) {
        std::vector<std::string> conflicting_versions;
        for (const auto& [version, _] : version_paths) {
            conflicting_versions.push_back(version);
        }
        
        ConflictInfo conflict(ConflictInfo::Type::VERSION_CONFLICT, package_name);
        conflict.conflicting_versions = conflicting_versions;
        if (!version_paths.empty() && !version_paths.begin()->second.empty()) {
            conflict.conflict_path = version_paths.begin()->second[0];
        }
        conflict.suggested_solution = generate_solution_suggestion(package_name, 
                                                                 conflicting_versions[0], 
                                                                 conflicting_versions[1]);
        conflicts.push_back(conflict);
    }
    
    return conflicts;
}

bool ConflictDetector::validate_dependency_graph() {
    auto conflicts = detect_all_conflicts();
    return conflicts.empty();
}

std::string ConflictDetector::calculate_required_version(const std::vector<std::string>& path) const {
    if (path.size() < 2) {
        return "";
    }
    
    // 从路径中提取版本要求
    // 这里简化处理，实际应该从包的元数据中读取
    const auto* node = graph_.get_node(path[path.size() - 2]);
    if (node) {
        auto it = node->version_constraints.find(path.back());
        if (it != node->version_constraints.end()) {
            return it->second.version;
        }
    }
    
    return "";
}

bool ConflictDetector::is_version_compatible(const std::string& version1, const std::string& version2) const {
    return VersionManager::is_version_compatible(version1, version2);
}

std::string ConflictDetector::generate_solution_suggestion(const std::string& package,
                                                          const std::string& version1, 
                                                          const std::string& version2) const {
    (void)package; // 避免未使用参数警告
    SemanticVersion v1(version1);
    SemanticVersion v2(version2);
    
    std::ostringstream suggestion;
    
    if (v1.major() != v2.major()) {
        suggestion << "Major version conflict. Consider using a compatible version or updating dependent packages.";
    } else if (v1.minor() != v2.minor()) {
        suggestion << "Minor version conflict. Consider upgrading to the newer version " << version2;
    } else {
        suggestion << "Patch version conflict. Consider using the latest patch version.";
    }
    
    return suggestion.str();
}

std::vector<std::string> ConflictDetector::get_available_versions(const std::string& package) const {
    (void)package; // 避免未使用参数警告
    // 这里应该从仓库中获取可用版本
    // 简化实现，返回一些示例版本
    return {"1.0.0", "1.1.0", "1.2.0", "2.0.0"};
}

bool ConflictDetector::package_exists_in_repository(const std::string& package) const {
    (void)package; // 避免未使用参数警告
    // 这里应该检查包是否存在于任何配置的仓库中
    // 简化实现，假设所有包都存在
    return true;
}

} // namespace Paker 