#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "dependency/dependency_graph.h"

namespace Paker {

// 依赖分析结果
struct DependencyAnalysis {
    // 基本统计
    size_t total_packages;
    size_t direct_dependencies;
    size_t transitive_dependencies;
    size_t circular_dependencies;
    size_t version_conflicts;
    
    // 版本分布
    std::map<std::string, std::set<std::string>> version_distribution;
    
    // 依赖深度
    std::map<std::string, size_t> dependency_depth;
    
    // 包大小统计
    std::map<std::string, size_t> package_sizes;
    
    // 冲突信息
    std::vector<std::string> conflict_details;
    
    // 建议
    std::vector<std::string> recommendations;
};

// 依赖分析器
class DependencyAnalyzer {
private:
    const DependencyGraph& graph_;
    
public:
    explicit DependencyAnalyzer(const DependencyGraph& graph);
    
    // 执行完整分析
    DependencyAnalysis analyze();
    
    // 生成分析报告
    std::string generate_analysis_report(const DependencyAnalysis& analysis);
    
    // 生成依赖树可视化
    std::string generate_dependency_tree_visualization();
    
    // 分析版本分布
    std::map<std::string, std::set<std::string>> analyze_version_distribution();
    
    // 计算依赖深度
    std::map<std::string, size_t> calculate_dependency_depth();
    
    // 分析包大小
    std::map<std::string, size_t> analyze_package_sizes();
    
    // 检测潜在问题
    std::vector<std::string> detect_potential_issues();
    
    // 生成优化建议
    std::vector<std::string> generate_recommendations(const DependencyAnalysis& analysis);
    
    // 导出分析结果
    bool export_analysis(const DependencyAnalysis& analysis, const std::string& filename);
    
private:
    // 计算单个包的依赖深度
    size_t calculate_package_depth(const std::string& package, 
                                 std::map<std::string, size_t>& depth_cache);
    
    // 获取包的大小信息
    size_t get_package_size(const std::string& package);
    
    // 格式化大小
    std::string format_size(size_t bytes) const;
    
    // 检查包的健康状态
    std::string check_package_health(const std::string& package);
};

} // namespace Paker 