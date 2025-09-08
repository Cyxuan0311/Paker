#pragma once

#include <vector>
#include <string>
#include "dependency/dependency_graph.h"

namespace Paker {

// 冲突检测器
class ConflictDetector {
private:
    const DependencyGraph& graph_;
    
public:
    explicit ConflictDetector(const DependencyGraph& graph);
    
    // 检测所有类型的冲突
    std::vector<ConflictInfo> detect_all_conflicts();
    
    // 检测版本冲突
    std::vector<ConflictInfo> detect_version_conflicts();
    
    // 检测循环依赖
    std::vector<ConflictInfo> detect_circular_dependencies();
    
    // 检测缺失依赖
    std::vector<ConflictInfo> detect_missing_dependencies();
    
    // 生成冲突报告
    std::string generate_conflict_report(const std::vector<ConflictInfo>& conflicts);
    
    // 检查特定包的冲突
    std::vector<ConflictInfo> detect_package_conflicts(const std::string& package_name);
    
    // 验证依赖图的完整性
    bool validate_dependency_graph();
    
private:
    // 计算路径中要求的版本
    std::string calculate_required_version(const std::vector<std::string>& path) const;
    
    // 检查版本兼容性
    bool is_version_compatible(const std::string& version1, const std::string& version2) const;
    
    // 生成解决建议
    std::string generate_solution_suggestion(const std::string& package, 
                                           const std::string& version1, 
                                           const std::string& version2) const;
    
    // 获取包的所有可用版本
    std::vector<std::string> get_available_versions(const std::string& package) const;
    
    // 检查包是否存在于仓库中
    bool package_exists_in_repository(const std::string& package) const;
};

} // namespace Paker 