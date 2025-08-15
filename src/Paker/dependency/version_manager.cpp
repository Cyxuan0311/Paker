#include "Paker/dependency/version_manager.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <glog/logging.h>

namespace Paker {

// SemanticVersion 实现
SemanticVersion::SemanticVersion() : major_(0), minor_(0), patch_(0) {}

SemanticVersion::SemanticVersion(int major, int minor, int patch)
    : major_(major), minor_(minor), patch_(patch) {}

SemanticVersion::SemanticVersion(const std::string& version_string) {
    parse(version_string);
}

bool SemanticVersion::parse(const std::string& version_string) {
    if (version_string.empty()) {
        return false;
    }
    
    // 正则表达式匹配语义化版本
    std::regex version_regex(R"(^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?(?:\+([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$)");
    std::smatch match;
    
    if (!std::regex_match(version_string, match, version_regex)) {
        LOG(WARNING) << "Invalid version format: " << version_string;
        return false;
    }
    
    try {
        major_ = std::stoi(match[1].str());
        minor_ = std::stoi(match[2].str());
        patch_ = std::stoi(match[3].str());
        prerelease_ = match[4].matched ? match[4].str() : "";
        build_ = match[5].matched ? match[5].str() : "";
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to parse version numbers: " << e.what();
        return false;
    }
    
    return true;
}

int SemanticVersion::compare(const SemanticVersion& other) const {
    // 比较主版本号
    if (major_ != other.major_) {
        return major_ < other.major_ ? -1 : 1;
    }
    
    // 比较次版本号
    if (minor_ != other.minor_) {
        return minor_ < other.minor_ ? -1 : 1;
    }
    
    // 比较补丁版本号
    if (patch_ != other.patch_) {
        return patch_ < other.patch_ ? -1 : 1;
    }
    
    // 比较预发布版本
    if (prerelease_.empty() && !other.prerelease_.empty()) {
        return 1; // 正式版本 > 预发布版本
    }
    if (!prerelease_.empty() && other.prerelease_.empty()) {
        return -1; // 预发布版本 < 正式版本
    }
    if (!prerelease_.empty() && !other.prerelease_.empty()) {
        if (prerelease_ != other.prerelease_) {
            return prerelease_ < other.prerelease_ ? -1 : 1;
        }
    }
    
    // 构建版本不影响比较
    return 0;
}

bool SemanticVersion::satisfies(const VersionConstraint& constraint) const {
    return constraint.satisfies(to_string());
}

std::string SemanticVersion::to_string() const {
    std::ostringstream oss;
    oss << major_ << "." << minor_ << "." << patch_;
    
    if (!prerelease_.empty()) {
        oss << "-" << prerelease_;
    }
    
    if (!build_.empty()) {
        oss << "+" << build_;
    }
    
    return oss.str();
}

// VersionConstraintParser 实现
VersionConstraint VersionConstraintParser::parse(const std::string& constraint) {
    return VersionConstraint::parse(constraint);
}

std::vector<VersionConstraint> VersionConstraintParser::parse_multiple(const std::string& constraints) {
    std::vector<VersionConstraint> result;
    std::istringstream iss(constraints);
    std::string constraint;
    
    while (std::getline(iss, constraint, ',')) {
        // 去除空白字符
        constraint.erase(0, constraint.find_first_not_of(" \t"));
        constraint.erase(constraint.find_last_not_of(" \t") + 1);
        
        if (!constraint.empty()) {
            result.push_back(parse(constraint));
        }
    }
    
    return result;
}

bool VersionConstraintParser::satisfies_all(const std::string& version, 
                                          const std::vector<VersionConstraint>& constraints) {
    SemanticVersion semver(version);
    
    for (const auto& constraint : constraints) {
        if (!semver.satisfies(constraint)) {
            return false;
        }
    }
    
    return true;
}

std::string VersionConstraintParser::get_latest_satisfying_version(const std::vector<std::string>& versions,
                                                                 const std::vector<VersionConstraint>& constraints) {
    std::string latest_version;
    SemanticVersion latest_semver;
    
    for (const auto& version : versions) {
        if (satisfies_all(version, constraints)) {
            SemanticVersion current_semver(version);
            if (latest_version.empty() || current_semver > latest_semver) {
                latest_version = version;
                latest_semver = current_semver;
            }
        }
    }
    
    return latest_version;
}

std::string VersionConstraintParser::get_min_satisfying_version(const std::vector<std::string>& versions,
                                                              const std::vector<VersionConstraint>& constraints) {
    std::string min_version;
    SemanticVersion min_semver;
    
    for (const auto& version : versions) {
        if (satisfies_all(version, constraints)) {
            SemanticVersion current_semver(version);
            if (min_version.empty() || current_semver < min_semver) {
                min_version = version;
                min_semver = current_semver;
            }
        }
    }
    
    return min_version;
}

// VersionManager 实现
bool VersionManager::is_version_compatible(const std::string& version1, const std::string& version2) {
    SemanticVersion v1(version1);
    SemanticVersion v2(version2);
    
    // 主版本号不同表示不兼容
    if (v1.major() != v2.major()) {
        return false;
    }
    
    // 次版本号差异通常表示向后兼容
    if (v1.minor() != v2.minor()) {
        // 可以在这里添加更复杂的兼容性规则
        return true;
    }
    
    return true;
}

VersionManager::VersionDiffType VersionManager::get_version_diff_type(const std::string& version1, 
                                                                     const std::string& version2) {
    SemanticVersion v1(version1);
    SemanticVersion v2(version2);
    
    if (v1.major() != v2.major()) {
        return VersionDiffType::MAJOR;
    }
    
    if (v1.minor() != v2.minor()) {
        return VersionDiffType::MINOR;
    }
    
    if (v1.patch() != v2.patch()) {
        return VersionDiffType::PATCH;
    }
    
    if (v1.prerelease() != v2.prerelease()) {
        return VersionDiffType::PRERELEASE;
    }
    
    if (v1.build() != v2.build()) {
        return VersionDiffType::BUILD;
    }
    
    return VersionDiffType::PATCH; // 默认
}

bool VersionManager::is_prerelease(const std::string& version) {
    SemanticVersion semver(version);
    return !semver.prerelease().empty();
}

bool VersionManager::is_stable(const std::string& version) {
    return !is_prerelease(version);
}

std::string VersionManager::normalize_version(const std::string& version) {
    SemanticVersion semver(version);
    return semver.to_string();
}

} // namespace Paker 