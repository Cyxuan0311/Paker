#include "Paker/conflict/conflict_resolver.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/dependency/version_manager.h"
#include "Paker/core/output.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <glog/logging.h>

namespace Paker {

ConflictResolver::ConflictResolver(DependencyGraph& graph) : graph_(graph) {}

bool ConflictResolver::auto_resolve_conflicts(const std::vector<ConflictInfo>& conflicts) {
    if (conflicts.empty()) {
        return true;
    }
    
    LOG(INFO) << "Auto-resolving " << conflicts.size() << " conflicts";
    Output::info("Auto-resolving " + std::to_string(conflicts.size()) + " conflicts...");
    
    bool all_resolved = true;
    
    for (const auto& conflict : conflicts) {
        bool resolved = false;
        
        switch (conflict.type) {
            case ConflictInfo::Type::VERSION_CONFLICT:
                resolved = resolve_version_conflict(conflict);
                break;
                
            case ConflictInfo::Type::CIRCULAR_DEPENDENCY:
                resolved = resolve_circular_dependency(conflict);
                break;
                
            case ConflictInfo::Type::MISSING_DEPENDENCY:
                resolved = resolve_missing_dependency(conflict);
                break;
        }
        
        if (!resolved) {
            all_resolved = false;
            LOG(WARNING) << "Failed to auto-resolve conflict for package: " << conflict.package_name;
            Output::warning("Failed to auto-resolve conflict for package: " + conflict.package_name);
        }
    }
    
    if (all_resolved) {
        Output::success("All conflicts resolved automatically");
    } else {
        Output::warning("Some conflicts could not be resolved automatically");
    }
    
    return all_resolved;
}

bool ConflictResolver::resolve_version_conflict(const ConflictInfo& conflict) {
    if (conflict.conflicting_versions.size() < 2) {
        return false;
    }
    
    // 选择最佳版本
    std::string best_version = select_best_version(conflict.package_name, conflict.conflicting_versions);
    
    if (best_version.empty()) {
        return false;
    }
    
    // 更新依赖图中的版本
    auto* node = graph_.get_node(conflict.package_name);
    if (node) {
        node->version = best_version;
        LOG(INFO) << "Resolved version conflict for " << conflict.package_name 
                 << " by selecting version " << best_version;
        Output::info("Resolved version conflict for " + conflict.package_name + 
                    " by selecting version " + best_version);
        return true;
    }
    
    return false;
}

bool ConflictResolver::resolve_circular_dependency(const ConflictInfo& conflict) {
    if (conflict.conflict_path.size() < 3) {
        return false;
    }
    
    // 简单的循环依赖解决策略：移除最后一个依赖
    std::string from = conflict.conflict_path[conflict.conflict_path.size() - 2];
    std::string to = conflict.conflict_path.back();
    
    auto* node = graph_.get_node(from);
    if (node) {
        node->dependencies.erase(to);
        LOG(INFO) << "Resolved circular dependency by removing " << from << " -> " << to;
        Output::info("Resolved circular dependency by removing " + from + " -> " + to);
        return true;
    }
    
    return false;
}

bool ConflictResolver::resolve_missing_dependency(const ConflictInfo& conflict) {
    // 对于缺失依赖，我们尝试从可用版本中选择一个
    auto it = available_versions_.find(conflict.package_name);
    if (it != available_versions_.end() && !it->second.empty()) {
        std::string selected_version = it->second[0]; // 选择第一个可用版本
        
        // 创建新的依赖节点
        DependencyNode new_node(conflict.package_name, selected_version);
        graph_.add_node(new_node);
        
        LOG(INFO) << "Resolved missing dependency " << conflict.package_name 
                 << " by adding version " << selected_version;
        Output::info("Resolved missing dependency " + conflict.package_name + 
                    " by adding version " + selected_version);
        return true;
    }
    
    return false;
}

std::vector<std::string> ConflictResolver::suggest_solutions(const ConflictInfo& conflict) {
    std::vector<std::string> solutions;
    
    switch (conflict.type) {
        case ConflictInfo::Type::VERSION_CONFLICT: {
            if (conflict.conflicting_versions.size() >= 2) {
                solutions.push_back("Use version " + conflict.conflicting_versions[0]);
                solutions.push_back("Use version " + conflict.conflicting_versions[1]);
                
                // 尝试找到兼容版本
                auto it = available_versions_.find(conflict.package_name);
                if (it != available_versions_.end()) {
                    for (const auto& version : it->second) {
                        bool compatible = true;
                        for (const auto& conflicting : conflict.conflicting_versions) {
                            if (!VersionManager::is_version_compatible(version, conflicting)) {
                                compatible = false;
                                break;
                            }
                        }
                        if (compatible) {
                            solutions.push_back("Use compatible version " + version);
                            break;
                        }
                    }
                }
            }
            break;
        }
        
        case ConflictInfo::Type::CIRCULAR_DEPENDENCY: {
            solutions.push_back("Remove dependency " + conflict.conflict_path.back());
            solutions.push_back("Restructure packages to break circular dependency");
            solutions.push_back("Use interface/abstraction to break dependency cycle");
            break;
        }
        
        case ConflictInfo::Type::MISSING_DEPENDENCY: {
            solutions.push_back("Add missing package to repository");
            solutions.push_back("Use alternative package");
            solutions.push_back("Remove dependency on " + conflict.package_name);
            break;
        }
    }
    
    return solutions;
}

bool ConflictResolver::apply_solution(const std::string& package, const std::string& solution) {
    // 解析解决方案
    if (solution.find("Use version ") == 0) {
        std::string version = solution.substr(12);
        auto* node = graph_.get_node(package);
        if (node) {
            node->version = version;
            return true;
        }
    } else if (solution.find("Remove dependency ") == 0) {
        std::string dep = solution.substr(18);
        auto* node = graph_.get_node(package);
        if (node) {
            node->dependencies.erase(dep);
            return true;
        }
    }
    
    return false;
}

bool ConflictResolver::interactive_resolve_conflicts(const std::vector<ConflictInfo>& conflicts) {
    if (conflicts.empty()) {
        return true;
    }
    
    Output::info("Interactive conflict resolution mode");
    
    for (const auto& conflict : conflicts) {
        Output::info("Conflict for package: " + conflict.package_name);
        Output::info("Type: " + std::to_string(static_cast<int>(conflict.type)));
        
        auto solutions = suggest_solutions(conflict);
        
        if (solutions.empty()) {
            Output::warning("No solutions available for this conflict");
            continue;
        }
        
        Output::info("Available solutions:");
        for (size_t i = 0; i < solutions.size(); ++i) {
            Output::info("  " + std::to_string(i + 1) + ". " + solutions[i]);
        }
        
        Output::info("Select solution (1-" + std::to_string(solutions.size()) + ") or 's' to skip: ");
        
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "s" || input == "S") {
            Output::info("Skipping this conflict");
            continue;
        }
        
        try {
            int choice = std::stoi(input);
            if (choice >= 1 && choice <= static_cast<int>(solutions.size())) {
                std::string selected_solution = solutions[choice - 1];
                if (apply_solution(conflict.package_name, selected_solution)) {
                    Output::success("Applied solution: " + selected_solution);
                } else {
                    Output::error("Failed to apply solution: " + selected_solution);
                }
            } else {
                Output::error("Invalid choice");
            }
        } catch (const std::exception& e) {
            Output::error("Invalid input: " + std::string(e.what()));
        }
    }
    
    return true;
}

void ConflictResolver::set_available_versions(const std::string& package, const std::vector<std::string>& versions) {
    available_versions_[package] = versions;
}

std::string ConflictResolver::select_best_version(const std::string& package, 
                                                const std::vector<std::string>& conflicting_versions) {
    if (conflicting_versions.empty()) {
        return "";
    }
    
    // 策略1：选择最新版本
    std::string latest_version = conflicting_versions[0];
    SemanticVersion latest_semver(latest_version);
    
    for (const auto& version : conflicting_versions) {
        SemanticVersion current_semver(version);
        if (current_semver > latest_semver) {
            latest_version = version;
            latest_semver = current_semver;
        }
    }
    
    // 策略2：检查是否有稳定版本
    for (const auto& version : conflicting_versions) {
        if (VersionManager::is_stable(version)) {
            return version;
        }
    }
    
    // 策略3：从可用版本中选择兼容版本
    auto it = available_versions_.find(package);
    if (it != available_versions_.end()) {
        for (const auto& version : it->second) {
            bool compatible = true;
            for (const auto& conflicting : conflicting_versions) {
                if (!VersionManager::is_version_compatible(version, conflicting)) {
                    compatible = false;
                    break;
                }
            }
            if (compatible) {
                return version;
            }
        }
    }
    
    // 默认返回最新版本
    return latest_version;
}

bool ConflictResolver::downgrade_package(const std::string& package, const std::string& target_version) {
    auto* node = graph_.get_node(package);
    if (node) {
        node->version = target_version;
        return true;
    }
    return false;
}

bool ConflictResolver::upgrade_package(const std::string& package, const std::string& target_version) {
    auto* node = graph_.get_node(package);
    if (node) {
        node->version = target_version;
        return true;
    }
    return false;
}

bool ConflictResolver::remove_conflicting_dependency(const std::string& package, const std::string& dependency) {
    auto* node = graph_.get_node(package);
    if (node) {
        node->dependencies.erase(dependency);
        return true;
    }
    return false;
}

bool ConflictResolver::add_alternative_dependency(const std::string& package, const std::string& alternative) {
    auto* node = graph_.get_node(package);
    if (node) {
        node->dependencies.insert(alternative);
        return true;
    }
    return false;
}

bool ConflictResolver::check_resolution_success(const std::vector<ConflictInfo>& original_conflicts) {
    // 重新检测冲突
    ConflictDetector detector(graph_);
    auto remaining_conflicts = detector.detect_all_conflicts();
    
    return remaining_conflicts.size() < original_conflicts.size();
}

std::string ConflictResolver::generate_resolution_report(const std::vector<ConflictInfo>& resolved_conflicts) {
    std::ostringstream report;
    report << "Conflict Resolution Report\n";
    report << "========================\n\n";
    
    for (const auto& conflict : resolved_conflicts) {
        report << "Package: " << conflict.package_name << "\n";
        report << "Resolution: " << conflict.suggested_solution << "\n\n";
    }
    
    return report.str();
}

} // namespace Paker 