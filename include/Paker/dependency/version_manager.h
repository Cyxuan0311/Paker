#pragma once

#include <string>
#include <vector>
#include "dependency/dependency_graph.h"

namespace Paker {

// 语义化版本
class SemanticVersion {
private:
    int major_;
    int minor_;
    int patch_;
    std::string prerelease_;
    std::string build_;
    
public:
    SemanticVersion();
    SemanticVersion(int major, int minor, int patch);
    SemanticVersion(const std::string& version_string);
    
    // 解析版本字符串
    bool parse(const std::string& version_string);
    
    // 版本比较
    int compare(const SemanticVersion& other) const;
    
    // 检查是否满足约束
    bool satisfies(const VersionConstraint& constraint) const;
    
    // 获取版本组件
    int major() const { return major_; }
    int minor() const { return minor_; }
    int patch() const { return patch_; }
    const std::string& prerelease() const { return prerelease_; }
    const std::string& build() const { return build_; }
    
    // 转换为字符串
    std::string to_string() const;
    
    // 比较操作符
    bool operator==(const SemanticVersion& other) const { return compare(other) == 0; }
    bool operator!=(const SemanticVersion& other) const { return compare(other) != 0; }
    bool operator<(const SemanticVersion& other) const { return compare(other) < 0; }
    bool operator<=(const SemanticVersion& other) const { return compare(other) <= 0; }
    bool operator>(const SemanticVersion& other) const { return compare(other) > 0; }
    bool operator>=(const SemanticVersion& other) const { return compare(other) >= 0; }
};

// 版本约束解析器
class VersionConstraintParser {
public:
    // 解析单个约束
    static VersionConstraint parse(const std::string& constraint);
    
    // 解析多个约束（用逗号分隔）
    static std::vector<VersionConstraint> parse_multiple(const std::string& constraints);
    
    // 检查版本是否满足所有约束
    static bool satisfies_all(const std::string& version, const std::vector<VersionConstraint>& constraints);
    
    // 获取满足约束的最新版本
    static std::string get_latest_satisfying_version(const std::vector<std::string>& versions, 
                                                   const std::vector<VersionConstraint>& constraints);
    
    // 获取满足约束的最小版本
    static std::string get_min_satisfying_version(const std::vector<std::string>& versions, 
                                                const std::vector<VersionConstraint>& constraints);
};

// 版本管理工具
class VersionManager {
public:
    // 检查版本兼容性
    static bool is_version_compatible(const std::string& version1, const std::string& version2);
    
    // 获取版本差异类型
    enum class VersionDiffType {
        MAJOR,      // 主版本差异
        MINOR,      // 次版本差异
        PATCH,      // 补丁版本差异
        PRERELEASE, // 预发布版本差异
        BUILD       // 构建版本差异
    };
    
    static VersionDiffType get_version_diff_type(const std::string& version1, const std::string& version2);
    
    // 检查是否为预发布版本
    static bool is_prerelease(const std::string& version);
    
    // 检查是否为稳定版本
    static bool is_stable(const std::string& version);
    
    // 规范化版本字符串
    static std::string normalize_version(const std::string& version);
};

} // namespace Paker 