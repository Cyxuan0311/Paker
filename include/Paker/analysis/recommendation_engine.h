#pragma once

#include "project_analyzer.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Paker {
namespace Analysis {

/**
 * 推荐引擎类
 */
class RecommendationEngine {
public:
    RecommendationEngine();
    ~RecommendationEngine() = default;

    /**
     * 生成智能推荐
     * @param analysis 项目分析结果
     * @param category_filter 类别过滤器
     * @param performance_filter 性能过滤器
     * @param security_filter 安全过滤器
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> generate_recommendations(
        const ProjectAnalysis& analysis,
        const std::string& category_filter = "",
        const std::string& performance_filter = "",
        const std::string& security_filter = ""
    );

    /**
     * 基于项目类型推荐
     * @param project_type 项目类型
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_type_based_recommendations(
        const std::string& project_type
    );

    /**
     * 基于现有依赖推荐
     * @param existing_deps 现有依赖
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_dependency_based_recommendations(
        const std::vector<std::string>& existing_deps
    );

    /**
     * 基于性能要求推荐
     * @param performance_level 性能等级
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_performance_based_recommendations(
        const std::string& performance_level
    );

    /**
     * 基于安全要求推荐
     * @param security_level 安全等级
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_security_based_recommendations(
        const std::string& security_level
    );

    /**
     * 基于测试要求推荐
     * @param testing_level 测试等级
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_testing_based_recommendations(
        const std::string& testing_level
    );

    /**
     * 基于代码模式推荐
     * @param code_patterns 代码模式
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_pattern_based_recommendations(
        const std::vector<std::string>& code_patterns
    );

    /**
     * 基于C++标准推荐
     * @param cpp_standard C++标准
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_standard_based_recommendations(
        const std::string& cpp_standard
    );

    /**
     * 基于构建系统推荐
     * @param build_system 构建系统
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_build_system_recommendations(
        const std::string& build_system
    );

    /**
     * 基于项目复杂度推荐
     * @param feature_scores 特征评分
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_complexity_based_recommendations(
        const std::map<std::string, double>& feature_scores
    );

    /**
     * 基于项目特征的智能推荐
     * @param analysis 项目分析结果
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_feature_based_recommendations(
        const ProjectAnalysis& analysis
    );

    /**
     * 基于GitHub热门包的推荐
     * @param analysis 项目分析结果
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_github_based_recommendations(const ProjectAnalysis& analysis);

    /**
     * 基于相似项目的推荐
     * @param analysis 项目分析结果
     * @return 推荐列表
     */
    std::vector<PackageRecommendation> get_similar_project_recommendations(const ProjectAnalysis& analysis);

    /**
     * 从项目名提取包名
     * @param project_name 项目名
     * @return 包名
     */
    std::string extract_package_from_project(const std::string& project_name);

    /**
     * 基于机器学习特征推荐
     * @param analysis 项目分析结果
     * @return 推荐包列表
     */
    std::vector<PackageRecommendation> get_ml_based_recommendations(const ProjectAnalysis& analysis);

    /**
     * 基于代码质量推荐
     * @param analysis 项目分析结果
     * @return 推荐包列表
     */
    std::vector<PackageRecommendation> get_quality_based_recommendations(const ProjectAnalysis& analysis);

    /**
     * 基于架构模式推荐
     * @param analysis 项目分析结果
     * @return 推荐包列表
     */
    std::vector<PackageRecommendation> get_architecture_based_recommendations(const ProjectAnalysis& analysis);

    /**
     * 基于复杂度推荐（新版本）
     * @param complexity_metrics 复杂度指标
     * @return 推荐包列表
     */
    std::vector<PackageRecommendation> get_complexity_based_recommendations_new(const std::map<std::string, double>& complexity_metrics);


    /**
     * 合并推荐结果
     * @param recommendations 推荐列表
     * @return 合并后的推荐列表
     */
    std::vector<PackageRecommendation> merge_recommendations(
        const std::vector<std::vector<PackageRecommendation>>& recommendations
    );

    /**
     * 排序推荐结果
     * @param recommendations 推荐列表
     * @param analysis 项目分析结果
     * @return 排序后的推荐列表
     */
    std::vector<PackageRecommendation> rank_recommendations(
        const std::vector<PackageRecommendation>& recommendations,
        const ProjectAnalysis& analysis
    );

    /**
     * 过滤推荐结果
     * @param recommendations 推荐列表
     * @param category_filter 类别过滤器
     * @param performance_filter 性能过滤器
     * @param security_filter 安全过滤器
     * @return 过滤后的推荐列表
     */
    std::vector<PackageRecommendation> filter_recommendations(
        const std::vector<PackageRecommendation>& recommendations,
        const std::string& category_filter,
        const std::string& performance_filter,
        const std::string& security_filter
    );

private:
    /**
     * 计算推荐评分
     * @param recommendation 推荐
     * @param analysis 项目分析
     * @return 评分
     */
    double calculate_recommendation_score(
        const PackageRecommendation& recommendation,
        const ProjectAnalysis& analysis
    );

    /**
     * 检查包兼容性
     * @param package_name 包名
     * @param analysis 项目分析
     * @return 兼容性评分
     */
    double check_package_compatibility(
        const std::string& package_name,
        const ProjectAnalysis& analysis
    );

    /**
     * 获取包元数据
     * @param package_name 包名
     * @return 包元数据
     */
    std::map<std::string, std::string> get_package_metadata(
        const std::string& package_name
    );

    // 包知识库
    std::map<std::string, std::vector<PackageRecommendation>> package_knowledge_base_;
    
    // 项目类型到包的映射
    std::map<std::string, std::vector<std::string>> type_package_mapping_;
    
    // 依赖关系映射
    std::map<std::string, std::vector<std::string>> dependency_relationships_;
    
    // 包元数据缓存
    std::map<std::string, std::map<std::string, std::string>> package_metadata_cache_;
    
    /**
     * 初始化包知识库
     */
    void initialize_package_knowledge_base();
    
    /**
     * 初始化项目类型到包的映射
     */
    void initialize_type_package_mapping();
    
    /**
     * 初始化依赖关系映射
     */
    void initialize_dependency_relationships();
};

} // namespace Analysis
} // namespace Paker
