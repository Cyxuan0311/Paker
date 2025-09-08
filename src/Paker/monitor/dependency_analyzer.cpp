#include "Paker/monitor/dependency_analyzer.h"
#include "Paker/core/output.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/core/utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <glog/logging.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

DependencyAnalyzer::DependencyAnalyzer(const DependencyGraph& graph) : graph_(graph) {}

DependencyAnalysis DependencyAnalyzer::analyze() {
    DependencyAnalysis analysis;
    
    // 基本统计
    analysis.total_packages = graph_.size();
    analysis.direct_dependencies = 0;
    analysis.transitive_dependencies = 0;
    
    // 计算直接依赖和传递依赖
    for (const auto& [package, node] : graph_.get_nodes()) {
        analysis.direct_dependencies += node.dependencies.size();
        
        // 计算传递依赖（简化实现）
        std::set<std::string> all_deps;
        for (const auto& dep : node.dependencies) {
            all_deps.insert(dep);
            const auto* dep_node = graph_.get_node(dep);
            if (dep_node) {
                all_deps.insert(dep_node->dependencies.begin(), dep_node->dependencies.end());
            }
        }
        analysis.transitive_dependencies += all_deps.size();
    }
    
    // 检测循环依赖
    auto cycles = graph_.detect_cycles();
    analysis.circular_dependencies = cycles.size();
    
    // 检测版本冲突
    ConflictDetector detector(graph_);
    auto conflicts = detector.detect_version_conflicts();
    analysis.version_conflicts = conflicts.size();
    
    // 分析版本分布
    analysis.version_distribution = analyze_version_distribution();
    
    // 计算依赖深度
    analysis.dependency_depth = calculate_dependency_depth();
    
    // 分析包大小
    analysis.package_sizes = analyze_package_sizes();
    
    // 生成冲突详情
    for (const auto& conflict : conflicts) {
        std::ostringstream oss;
        oss << "Package: " << conflict.package_name << " - ";
        oss << "Conflicting versions: ";
        for (size_t i = 0; i < conflict.conflicting_versions.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << conflict.conflicting_versions[i];
        }
        analysis.conflict_details.push_back(oss.str());
    }
    
    // 生成建议
    analysis.recommendations = generate_recommendations(analysis);
    
    return analysis;
}

std::string DependencyAnalyzer::generate_analysis_report(const DependencyAnalysis& analysis) {
    std::ostringstream report;
    report << "📋 Dependency Analysis Report\n";
    report << "============================\n\n";
    
    // 基本统计
    report << "📊 Basic Statistics\n";
    report << "-------------------\n";
    report << "Total packages: " << analysis.total_packages << "\n";
    report << "Direct dependencies: " << analysis.direct_dependencies << "\n";
    report << "Transitive dependencies: " << analysis.transitive_dependencies << "\n";
    report << "Circular dependencies: " << analysis.circular_dependencies << "\n";
    report << "Version conflicts: " << analysis.version_conflicts << "\n\n";
    
    // 版本分布
    if (!analysis.version_distribution.empty()) {
        report << "📦 Version Distribution\n";
        report << "------------------------\n";
        for (const auto& [package, versions] : analysis.version_distribution) {
            report << package << ":\n";
            for (const auto& version : versions) {
                report << "  - " << version << "\n";
            }
        }
        report << "\n";
    }
    
    // 依赖深度
    if (!analysis.dependency_depth.empty()) {
        report << "🌳 Dependency Depth\n";
        report << "--------------------\n";
        std::vector<std::pair<std::string, size_t>> sorted_depth;
        for (const auto& [package, depth] : analysis.dependency_depth) {
            sorted_depth.emplace_back(package, depth);
        }
        std::sort(sorted_depth.begin(), sorted_depth.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& [package, depth] : sorted_depth) {
            report << package << ": " << depth << " levels deep\n";
        }
        report << "\n";
    }
    
    // 包大小
    if (!analysis.package_sizes.empty()) {
        report << "💾 Package Sizes\n";
        report << "----------------\n";
        std::vector<std::pair<std::string, size_t>> sorted_sizes;
        for (const auto& [package, size] : analysis.package_sizes) {
            sorted_sizes.emplace_back(package, size);
        }
        std::sort(sorted_sizes.begin(), sorted_sizes.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& [package, size] : sorted_sizes) {
            report << package << ": " << format_size(size) << "\n";
        }
        report << "\n";
    }
    
    // 冲突详情
    if (!analysis.conflict_details.empty()) {
        report << "⚠️  Version Conflicts\n";
        report << "--------------------\n";
        for (const auto& conflict : analysis.conflict_details) {
            report << conflict << "\n";
        }
        report << "\n";
    }
    
    // 建议
    if (!analysis.recommendations.empty()) {
        report << "💡 Recommendations\n";
        report << "------------------\n";
        for (const auto& recommendation : analysis.recommendations) {
            report << "- " << recommendation << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

std::string DependencyAnalyzer::generate_dependency_tree_visualization() {
    std::ostringstream visualization;
    visualization << "🌳 Dependency Tree Visualization\n";
    visualization << "================================\n\n";
    
    // 获取拓扑排序
    auto sorted = graph_.topological_sort();
    
    for (const auto& package : sorted) {
        const auto* node = graph_.get_node(package);
        if (!node) continue;
        
        // 计算缩进
        size_t depth = 0;
        std::map<std::string, size_t> depth_cache;
        for (const auto& dep : node->dependencies) {
            depth = std::max(depth, calculate_package_depth(dep, depth_cache));
        }
        
        std::string indent(depth * 2, ' ');
        visualization << indent;
        
        if (depth == 0) {
            visualization << "📦 ";
        } else {
            visualization << "├── ";
        }
        
        visualization << package;
        if (!node->version.empty()) {
            visualization << " (" << node->version << ")";
        }
        visualization << "\n";
    }
    
    return visualization.str();
}

std::map<std::string, std::set<std::string>> DependencyAnalyzer::analyze_version_distribution() {
    std::map<std::string, std::set<std::string>> distribution;
    
    for (const auto& [package, node] : graph_.get_nodes()) {
        if (!node.version.empty()) {
            distribution[package].insert(node.version);
        }
        
        // 检查版本约束
        for (const auto& [dep, constraint] : node.version_constraints) {
            if (!constraint.version.empty()) {
                distribution[dep].insert(constraint.version);
            }
        }
    }
    
    return distribution;
}

std::map<std::string, size_t> DependencyAnalyzer::calculate_dependency_depth() {
    std::map<std::string, size_t> depth_cache;
    
    for (const auto& [package, _] : graph_.get_nodes()) {
        calculate_package_depth(package, depth_cache);
    }
    
    return depth_cache;
}

size_t DependencyAnalyzer::calculate_package_depth(const std::string& package, 
                                                 std::map<std::string, size_t>& depth_cache) {
    auto it = depth_cache.find(package);
    if (it != depth_cache.end()) {
        return it->second;
    }
    
    const auto* node = graph_.get_node(package);
    if (!node || node->dependencies.empty()) {
        depth_cache[package] = 0;
        return 0;
    }
    
    size_t max_depth = 0;
    for (const auto& dep : node->dependencies) {
        size_t dep_depth = calculate_package_depth(dep, depth_cache);
        max_depth = std::max(max_depth, dep_depth);
    }
    
    depth_cache[package] = max_depth + 1;
    return max_depth + 1;
}

std::map<std::string, size_t> DependencyAnalyzer::analyze_package_sizes() {
    std::map<std::string, size_t> sizes;
    
    for (const auto& [package, node] : graph_.get_nodes()) {
        sizes[package] = get_package_size(package);
    }
    
    return sizes;
}

size_t DependencyAnalyzer::get_package_size(const std::string& package) {
    std::string package_path = "packages/" + package;
    if (!fs::exists(package_path)) {
        return 0;
    }
    
    size_t total_size = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(package_path)) {
            if (entry.is_regular_file()) {
                total_size += fs::file_size(entry.path());
            }
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to calculate size for package " << package << ": " << e.what();
    }
    
    return total_size;
}

std::string DependencyAnalyzer::format_size(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

std::vector<std::string> DependencyAnalyzer::detect_potential_issues() {
    std::vector<std::string> issues;
    
    // 检查深度过大的依赖
    auto depths = calculate_dependency_depth();
    for (const auto& [package, depth] : depths) {
        if (depth > 5) {
            issues.push_back("Package '" + package + "' has deep dependency chain (" + 
                           std::to_string(depth) + " levels)");
        }
    }
    
    // 检查大包
    auto sizes = analyze_package_sizes();
    for (const auto& [package, size] : sizes) {
        if (size > 100 * 1024 * 1024) { // 100MB
            issues.push_back("Package '" + package + "' is very large (" + format_size(size) + ")");
        }
    }
    
    // 检查版本冲突
    ConflictDetector detector(graph_);
    auto conflicts = detector.detect_all_conflicts();
    if (!conflicts.empty()) {
        issues.push_back("Found " + std::to_string(conflicts.size()) + " dependency conflicts");
    }
    
    return issues;
}

std::vector<std::string> DependencyAnalyzer::generate_recommendations(const DependencyAnalysis& analysis) {
    std::vector<std::string> recommendations;
    
    if (analysis.circular_dependencies > 0) {
        recommendations.push_back("Consider breaking circular dependencies by restructuring packages");
    }
    
    if (analysis.version_conflicts > 0) {
        recommendations.push_back("Resolve version conflicts by updating or downgrading packages");
    }
    
    if (analysis.total_packages > 20) {
        recommendations.push_back("Consider consolidating dependencies to reduce complexity");
    }
    
    // 检查深度
    for (const auto& [package, depth] : analysis.dependency_depth) {
        if (depth > 5) {
            recommendations.push_back("Consider flattening dependency tree for package '" + package + "'");
        }
    }
    
    // 检查大小
    for (const auto& [package, size] : analysis.package_sizes) {
        if (size > 100 * 1024 * 1024) { // 100MB
            recommendations.push_back("Consider using a lighter alternative for package '" + package + "'");
        }
    }
    
    return recommendations;
}

bool DependencyAnalyzer::export_analysis(const DependencyAnalysis& analysis, const std::string& filename) {
    try {
        json j;
        j["total_packages"] = analysis.total_packages;
        j["direct_dependencies"] = analysis.direct_dependencies;
        j["transitive_dependencies"] = analysis.transitive_dependencies;
        j["circular_dependencies"] = analysis.circular_dependencies;
        j["version_conflicts"] = analysis.version_conflicts;
        
        // 版本分布
        j["version_distribution"] = json::object();
        for (const auto& [package, versions] : analysis.version_distribution) {
            j["version_distribution"][package] = json::array();
            for (const auto& version : versions) {
                j["version_distribution"][package].push_back(version);
            }
        }
        
        // 依赖深度
        j["dependency_depth"] = analysis.dependency_depth;
        
        // 包大小
        j["package_sizes"] = analysis.package_sizes;
        
        // 冲突详情
        j["conflict_details"] = analysis.conflict_details;
        
        // 建议
        j["recommendations"] = analysis.recommendations;
        
        std::ofstream file(filename);
        file << j.dump(4);
        
        LOG(INFO) << "Dependency analysis exported to: " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to export dependency analysis: " << e.what();
        return false;
    }
}

} // namespace Paker 