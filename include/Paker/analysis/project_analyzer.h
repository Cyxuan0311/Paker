#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <filesystem>
#include "Paker/analysis/project_type_config.h"

namespace Paker {
namespace Analysis {

/**
 * GitHub项目分析结果结构
 */
struct GitHubAnalysis {
    bool is_github_project = false;    // 是否是GitHub项目
    std::string owner;                  // 项目所有者
    std::string repo;                   // 仓库名
    int stars = 0;                      // 星标数
    int forks = 0;                      // 分支数
    int watchers = 0;                   // 观察者数
    std::string language;               // 主要语言
    std::string description;             // 项目描述
    std::string updated_at;             // 最后更新时间
    std::string license;                // 许可证
    std::vector<std::string> topics;    // 项目标签
};

/**
 * GitHub包详细信息结构
 */
struct GitHubPackageInfo {
    std::string name;                   // 包名
    std::string full_name;              // 完整名称 (owner/repo)
    std::string description;            // 项目描述
    std::string github_url;             // GitHub地址
    int stars = 0;                      // 星标数
    int forks = 0;                      // 分支数
    std::string language;               // 主要语言
    std::string license;                // 许可证
    std::vector<std::string> topics;    // 项目标签
    bool found = false;                 // 是否找到项目
};

/**
 * 项目分析结果结构
 */
struct ProjectAnalysis {
    std::string project_type;           // 项目类型 (web, desktop, embedded, game)
    std::string build_system;           // 构建系统 (cmake, make, meson)
    std::string cpp_standard;          // C++标准 (c++11, c++14, c++17, c++20)
    std::vector<std::string> existing_dependencies;  // 现有依赖
    std::vector<std::string> code_patterns;          // 代码模式
    std::string performance_requirements;          // 性能要求 (low, medium, high)
    std::string security_requirements;             // 安全要求 (low, medium, high)
    std::string testing_requirements;              // 测试要求 (low, medium, high)
    std::map<std::string, double> feature_scores;   // 特征评分
    GitHubAnalysis github_analysis;                 // GitHub分析结果
    std::vector<std::string> trending_packages;     // 热门包
    std::vector<std::string> similar_projects;      // 相似项目
    std::vector<std::string> ml_features;           // 机器学习特征
    double code_quality_score;                      // 代码质量评分
    std::vector<std::string> architecture_patterns; // 架构模式
    std::map<std::string, double> complexity_metrics; // 复杂度指标
    std::vector<std::string> performance_indicators; // 性能指标
    bool is_initialized;                             // 是否已初始化
};

/**
 * 包推荐结构
 */
struct PackageRecommendation {
    std::string name;                   // 包名
    std::string description;            // 描述
    std::string reason;                 // 推荐理由
    std::string category;              // 类别
    double confidence;                  // 置信度 (0.0-1.0)
    double compatibility;               // 兼容性 (0.0-1.0)
    double popularity;                  // 流行度 (0.0-1.0)
    double maintenance;                 // 维护性 (0.0-1.0)
    std::string priority;               // 优先级 (low, medium, high)
    std::vector<std::string> tags;     // 标签
    std::string install_command;       // 安装命令
    std::string github_url;            // GitHub地址
};

/**
 * 项目分析器类
 */
class ProjectAnalyzer {
public:
    ProjectAnalyzer();
    ~ProjectAnalyzer() = default;

    /**
     * 分析项目
     * @param project_path 项目路径
     * @return 项目分析结果
     */
    ProjectAnalysis analyze_project(const std::string& project_path = ".");

    /**
     * 检测项目类型
     * @param project_path 项目路径
     * @return 项目类型
     */
    std::string detect_project_type(const std::string& project_path);

    /**
     * 检测构建系统
     * @param project_path 项目路径
     * @return 构建系统类型
     */
    std::string detect_build_system(const std::string& project_path);

    /**
     * 检测C++标准
     * @param project_path 项目路径
     * @return C++标准
     */
    std::string detect_cpp_standard(const std::string& project_path);

    /**
     * 扫描现有依赖
     * @param project_path 项目路径
     * @return 依赖列表
     */
    std::vector<std::string> scan_dependencies(const std::string& project_path);

    /**
     * 分析代码模式
     * @param project_path 项目路径
     * @return 代码模式列表
     */
    std::vector<std::string> analyze_code_patterns(const std::string& project_path);

    /**
     * 评估性能要求
     * @param project_path 项目路径
     * @return 性能要求等级
     */
    std::string assess_performance_needs(const std::string& project_path);

    /**
     * 评估安全要求
     * @param project_path 项目路径
     * @return 安全要求等级
     */
    std::string assess_security_needs(const std::string& project_path);

    /**
     * 评估测试要求
     * @param project_path 项目路径
     * @return 测试要求等级
     */
    std::string assess_testing_needs(const std::string& project_path);
    
    /**
     * 获取GitHub包详细信息
     * @param package_name 包名
     * @return GitHub包信息
     */
    GitHubPackageInfo get_github_package_info(const std::string& package_name);

private:
    /**
     * 读取文件内容
     * @param file_path 文件路径
     * @return 文件内容
     */
    std::string read_file_content(const std::filesystem::path& file_path);

    /**
     * 检查文件是否存在
     * @param file_path 文件路径
     * @return 是否存在
     */
    bool file_exists(const std::filesystem::path& file_path);

    /**
     * 扫描目录中的文件
     * @param dir_path 目录路径
     * @param extensions 文件扩展名列表
     * @return 文件列表
     */
    std::vector<std::filesystem::path> scan_directory(
        const std::filesystem::path& dir_path, 
        const std::vector<std::string>& extensions
    );

    /**
     * 分析CMakeLists.txt文件
     * @param file_path CMakeLists.txt路径
     * @return 分析结果
     */
    std::map<std::string, std::string> analyze_cmake_file(const std::filesystem::path& file_path);

    /**
     * 分析源代码文件
     * @param file_path 源代码文件路径
     * @return 代码特征
     */
    std::vector<std::string> analyze_source_file(const std::filesystem::path& file_path);

    /**
     * 计算特征评分
     * @param project_path 项目路径
     * @return 特征评分映射
     */
    std::map<std::string, double> calculate_feature_scores(const std::string& project_path);

    /**
     * 分析GitHub项目
     * @param project_path 项目路径
     * @return GitHub分析结果
     */
    GitHubAnalysis analyze_github_project(const std::string& project_path);

    /**
     * 获取热门包
     * @param project_type 项目类型
     * @return 热门包列表
     */
    std::vector<std::string> get_trending_packages(const std::string& project_type);

    /**
     * 查找相似项目
     * @param project_type 项目类型
     * @return 相似项目列表
     */
    std::vector<std::string> find_similar_projects(const std::string& project_type);

    /**
     * 获取备用热门包（当GitHub API失败时）
     * @param project_type 项目类型
     * @return 备用包列表
     */
    std::vector<std::string> get_fallback_trending_packages(const std::string& project_type);

    /**
     * 获取备用相似项目（当GitHub API失败时）
     * @param project_type 项目类型
     * @return 备用相似项目列表
     */
    std::vector<std::string> get_fallback_similar_projects(const std::string& project_type);

    /**
     * 发送GitHub API请求
     * @param url API URL
     * @return 响应内容
     */
    std::string make_github_request(const std::string& url);

    /**
     * CURL写回调函数
     */
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s);

    /**
     * 解析GitHub API响应
     * @param response API响应
     * @param analysis 分析结果
     */
    void parse_github_response(const std::string& response, GitHubAnalysis& analysis);

    /**
     * 构建搜索查询
     * @param project_type 项目类型
     * @return 搜索查询字符串
     */
    std::string build_search_query(const std::string& project_type);

    /**
     * 提取包名
     * @param repo_name 仓库名
     * @return 包名
     */
    std::string extract_package_name(const std::string& repo_name);

    /**
     * 检测机器学习特征
     * @param project_path 项目路径
     * @return 机器学习特征列表
     */
    std::vector<std::string> detect_ml_features(const std::string& project_path);

    /**
     * 计算代码质量评分
     * @param project_path 项目路径
     * @return 代码质量评分
     */
    double calculate_code_quality_score(const std::string& project_path);

    /**
     * 检测架构模式
     * @param project_path 项目路径
     * @return 架构模式列表
     */
    std::vector<std::string> detect_architecture_patterns(const std::string& project_path);

    /**
     * 计算复杂度指标
     * @param project_path 项目路径
     * @return 复杂度指标映射
     */
    std::map<std::string, double> calculate_complexity_metrics(const std::string& project_path);

    /**
     * 检测性能指标
     * @param project_path 项目路径
     * @return 性能指标列表
     */
    std::vector<std::string> detect_performance_indicators(const std::string& project_path);

    // 项目类型配置系统
    ProjectTypeConfig config_;

    // GitHub API相关
    std::string github_api_base_;
    const char* github_token_;
};

} // namespace Analysis
} // namespace Paker
