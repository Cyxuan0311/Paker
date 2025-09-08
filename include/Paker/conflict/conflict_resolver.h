#pragma once

#include <vector>
#include <string>
#include <map>
#include "dependency/dependency_graph.h"

namespace Paker {

// 冲突解决器
class ConflictResolver {
private:
    DependencyGraph& graph_;
    std::map<std::string, std::vector<std::string>> available_versions_;
    
public:
    explicit ConflictResolver(DependencyGraph& graph);
    
    // 自动解决冲突
    bool auto_resolve_conflicts(const std::vector<ConflictInfo>& conflicts);
    
    // 解决版本冲突
    bool resolve_version_conflict(const ConflictInfo& conflict);
    
    // 解决循环依赖
    bool resolve_circular_dependency(const ConflictInfo& conflict);
    
    // 解决缺失依赖
    bool resolve_missing_dependency(const ConflictInfo& conflict);
    
    // 提供解决建议
    std::vector<std::string> suggest_solutions(const ConflictInfo& conflict);
    
    // 应用解决策略
    bool apply_solution(const std::string& package, const std::string& solution);
    
    // 交互式解决冲突
    bool interactive_resolve_conflicts(const std::vector<ConflictInfo>& conflicts);
    
    // 设置可用版本
    void set_available_versions(const std::string& package, const std::vector<std::string>& versions);
    
    // 获取解决后的依赖图
    const DependencyGraph& get_resolved_graph() const { return graph_; }
    
private:
    // 选择最佳版本
    std::string select_best_version(const std::string& package, 
                                  const std::vector<std::string>& conflicting_versions);
    
    // 降级策略
    bool downgrade_package(const std::string& package, const std::string& target_version);
    
    // 升级策略
    bool upgrade_package(const std::string& package, const std::string& target_version);
    
    // 移除冲突依赖
    bool remove_conflicting_dependency(const std::string& package, const std::string& dependency);
    
    // 添加替代依赖
    bool add_alternative_dependency(const std::string& package, const std::string& alternative);
    
    // 检查解决后的冲突
    bool check_resolution_success(const std::vector<ConflictInfo>& original_conflicts);
    
    // 生成解决报告
    std::string generate_resolution_report(const std::vector<ConflictInfo>& resolved_conflicts);
};

} // namespace Paker 