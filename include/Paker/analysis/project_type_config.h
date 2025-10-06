#pragma once

#include <string>
#include <vector>
#include <map>

namespace Paker {
namespace Analysis {

/**
 * 项目类型配置类
 * 负责管理项目类型检测的所有关键词和配置
 */
class ProjectTypeConfig {
public:
    /**
     * 构造函数
     */
    ProjectTypeConfig();

    /**
     * 获取指定项目类型的指示器
     * @param project_type 项目类型
     * @return 指示器列表
     */
    const std::vector<std::string>& get_project_indicators(const std::string& project_type) const;

    /**
     * 获取所有项目类型
     * @return 项目类型列表
     */
    std::vector<std::string> get_all_project_types() const;

    /**
     * 获取性能指示器
     * @return 性能指示器列表
     */
    const std::vector<std::string>& get_performance_indicators() const;

    /**
     * 获取安全指示器
     * @return 安全指示器列表
     */
    const std::vector<std::string>& get_security_indicators() const;

    /**
     * 获取测试指示器
     * @return 测试指示器列表
     */
    const std::vector<std::string>& get_testing_indicators() const;

    /**
     * 获取机器学习特征
     * @return 机器学习特征列表
     */
    const std::vector<std::string>& get_ml_features() const;

    /**
     * 获取代码质量指示器
     * @return 代码质量指示器列表
     */
    const std::vector<std::string>& get_code_quality_indicators() const;

    /**
     * 获取架构模式
     * @return 架构模式列表
     */
    const std::vector<std::string>& get_architecture_patterns() const;

private:
    /**
     * 初始化项目指示器
     */
    void initialize_project_indicators();

    /**
     * 初始化性能指示器
     */
    void initialize_performance_indicators();

    /**
     * 初始化安全指示器
     */
    void initialize_security_indicators();

    /**
     * 初始化测试指示器
     */
    void initialize_testing_indicators();

    /**
     * 初始化机器学习特征
     */
    void initialize_ml_features();

    /**
     * 初始化代码质量指示器
     */
    void initialize_code_quality_indicators();

    /**
     * 初始化架构模式
     */
    void initialize_architecture_patterns();

    // 项目类型指示器映射
    std::map<std::string, std::vector<std::string>> project_indicators_;
    
    // 性能指示器
    std::vector<std::string> performance_indicators_;
    
    // 安全指示器
    std::vector<std::string> security_indicators_;
    
    // 测试指示器
    std::vector<std::string> testing_indicators_;
    
    // 机器学习特征
    std::vector<std::string> ml_features_;
    
    // 代码质量指示器
    std::vector<std::string> code_quality_indicators_;
    
    // 架构模式
    std::vector<std::string> architecture_patterns_;
};

} // namespace Analysis
} // namespace Paker
