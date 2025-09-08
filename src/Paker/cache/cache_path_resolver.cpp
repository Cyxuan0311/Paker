#include "Paker/cache/cache_path_resolver.h"
#include "Paker/core/output.h"
#include <filesystem>
#include <algorithm>
#include <glog/logging.h>
#include <sys/statvfs.h>

namespace fs = std::filesystem;

namespace Paker {

// 全局路径解析器实例
std::unique_ptr<CachePathResolver> g_path_resolver;

CachePathResolver::CachePathResolver() {
    initialize_paths();
}

void CachePathResolver::initialize_paths() {
    // 设置缓存路径
    const char* home_dir = std::getenv("HOME");
    if (home_dir) {
        cache_paths_[CacheLocation::USER_CACHE] = std::string(home_dir) + "/.paker/cache";
    } else {
        cache_paths_[CacheLocation::USER_CACHE] = "./.paker/cache";
    }
    
    cache_paths_[CacheLocation::GLOBAL_CACHE] = "/usr/local/share/paker/cache";
    cache_paths_[CacheLocation::PROJECT_CACHE] = ".paker/cache";
    cache_paths_[CacheLocation::PROJECT_LINKS] = ".paker/links";
    
    // 设置路径优先级（按性能和访问效率排序）
    path_priority_ = {
        CacheLocation::USER_CACHE,      // 最高优先级：用户缓存
        CacheLocation::GLOBAL_CACHE,    // 次优先级：全局缓存
        CacheLocation::PROJECT_CACHE,   // 低优先级：项目缓存
        CacheLocation::PROJECT_LINKS    // 最低优先级：项目链接
    };
}

CachePathResolver::PathResolution CachePathResolver::resolve_package_path(
    const std::string& package, const std::string& version, const std::string& project_path) {
    
    PathResolution result;
    
    // 按优先级查找包
    for (auto location : path_priority_) {
        std::string base_path = get_location_path(location);
        std::string version_suffix = version.empty() ? "latest" : version;
        std::string package_path = base_path + "/" + package + "/" + version_suffix;
        
        if (path_exists_and_valid(package_path)) {
            result.resolved_path = package_path;
            result.location = location;
            result.exists = true;
            result.size_bytes = calculate_path_size(package_path);
            result.last_modified = get_path_last_modified(package_path);
            break;
        }
    }
    
    return result;
}

std::string CachePathResolver::select_optimal_cache_path(const std::string& package, const std::string& version) {
    // 首先检查是否已存在
    auto resolution = resolve_package_path(package, version);
    if (resolution.exists) {
        return resolution.resolved_path;
    }
    
    // 选择最佳位置进行安装
    CacheLocation best_location = select_best_location(package, version);
    std::string base_path = get_location_path(best_location);
    std::string version_suffix = version.empty() ? "latest" : version;
    
    return base_path + "/" + package + "/" + version_suffix;
}

bool CachePathResolver::validate_cache_path(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            return false;
        }
        
        // 检查是否为目录
        if (!fs::is_directory(path)) {
            return false;
        }
        
        // 检查权限
        fs::path pkg_path(path);
        auto perms = fs::status(pkg_path).permissions();
        return (perms & fs::perms::owner_read) != fs::perms::none;
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error validating cache path: " << e.what();
        return false;
    }
}

bool CachePathResolver::is_cache_path_writable(const std::string& path) {
    try {
        fs::path pkg_path(path);
        auto perms = fs::status(pkg_path).permissions();
        return (perms & fs::perms::owner_write) != fs::perms::none;
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error checking cache path writability: " << e.what();
        return false;
    }
}

CachePathResolver::PathStats CachePathResolver::get_path_statistics(CacheLocation location) {
    PathStats stats;
    std::string path = get_location_path(location);
    
    try {
        if (!fs::exists(path)) {
            return stats;
        }
        
        // 计算包数量和总大小
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                stats.total_packages++;
                stats.total_size_bytes += calculate_path_size(entry.path().string());
            }
        }
        
        // 计算可用空间
        stats.available_space = get_available_space(path);
        
        // 获取最后清理时间
        fs::path cleanup_file = fs::path(path) / ".last_cleanup";
        if (fs::exists(cleanup_file)) {
            auto ftime = fs::last_write_time(cleanup_file);
            // 将 file_time_type 转换为 system_clock::time_point
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            stats.last_cleanup = sctp;
        }
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error getting path statistics: " << e.what();
    }
    
    return stats;
}

bool CachePathResolver::optimize_cache_paths() {
    try {
        bool optimized = false;
        
        // 检查并创建必要的目录
        for (const auto& [location, path] : cache_paths_) {
            if (!fs::exists(path)) {
                fs::create_directories(path);
                optimized = true;
            }
        }
        
        // 优化权限
        for (const auto& [location, path] : cache_paths_) {
            if (fs::exists(path)) {
                fs::permissions(path, fs::perms::owner_all | fs::perms::group_read | fs::perms::others_read);
            }
        }
        
        return optimized;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error optimizing cache paths: " << e.what();
        return false;
    }
}

std::vector<std::string> CachePathResolver::get_recommended_cleanup_paths() {
    std::vector<std::string> cleanup_paths;
    
    try {
        // 检查每个缓存位置的空间使用情况
        for (const auto& [location, path] : cache_paths_) {
            if (!fs::exists(path)) {
                continue;
            }
            
            auto stats = get_path_statistics(location);
            
            // 如果使用率超过80%，建议清理
            if (stats.available_space > 0) {
                double usage_ratio = static_cast<double>(stats.total_size_bytes) / 
                                   (stats.total_size_bytes + stats.available_space);
                
                if (usage_ratio > 0.8) {
                    cleanup_paths.push_back(path);
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error getting recommended cleanup paths: " << e.what();
    }
    
    return cleanup_paths;
}

bool CachePathResolver::can_migrate_to_location(const std::string& source_path, CacheLocation target_location) {
    try {
        if (!fs::exists(source_path)) {
            return false;
        }
        
        std::string target_path = get_location_path(target_location);
        
        // 检查目标位置是否有足够空间
        size_t source_size = calculate_path_size(source_path);
        size_t available_space = get_available_space(target_path);
        
        return available_space >= source_size;
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error checking migration possibility: " << e.what();
        return false;
    }
}

std::string CachePathResolver::get_migration_path(const std::string& package, CacheLocation target_location) {
    std::string base_path = get_location_path(target_location);
    return base_path + "/" + package;
}

// 私有方法实现

std::string CachePathResolver::get_location_path(CacheLocation location) const {
    auto it = cache_paths_.find(location);
    return it != cache_paths_.end() ? it->second : "";
}

bool CachePathResolver::path_exists_and_valid(const std::string& path) const {
    try {
        return fs::exists(path) && fs::is_directory(path);
    } catch (const std::exception& e) {
        return false;
    }
}

size_t CachePathResolver::calculate_path_size(const std::string& path) const {
    size_t total_size = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                total_size += fs::file_size(entry.path());
            }
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error calculating path size: " << e.what();
    }
    return total_size;
}

std::chrono::system_clock::time_point CachePathResolver::get_path_last_modified(const std::string& path) const {
    try {
        auto ftime = fs::last_write_time(path);
        // 将 file_time_type 转换为 system_clock::time_point
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return sctp;
    } catch (const std::exception& e) {
        return std::chrono::system_clock::now();
    }
}

size_t CachePathResolver::get_available_space(const std::string& path) const {
    try {
        struct statvfs stat;
        if (statvfs(path.c_str(), &stat) == 0) {
            return static_cast<size_t>(stat.f_bsize) * stat.f_bavail;
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error getting available space: " << e.what();
    }
    return 0;
}

CachePathResolver::CacheLocation CachePathResolver::select_best_location(const std::string& package, const std::string& version) {
    // 智能选择算法：基于空间、性能和访问模式
    double best_score = -1.0;
    CacheLocation best_location = CacheLocation::USER_CACHE;  // 默认选择
    
    for (auto location : path_priority_) {
        double score = calculate_location_score(location, package);
        if (score > best_score) {
            best_score = score;
            best_location = location;
        }
    }
    
    return best_location;
}

double CachePathResolver::calculate_location_score(CacheLocation location, const std::string& package) {
    double score = 0.0;
    std::string path = get_location_path(location);
    
    try {
        if (!fs::exists(path)) {
            return -1.0;  // 路径不存在，分数最低
        }
        
        // 基础分数（基于优先级）
        switch (location) {
            case CacheLocation::USER_CACHE:
                score += 100.0;
                break;
            case CacheLocation::GLOBAL_CACHE:
                score += 80.0;
                break;
            case CacheLocation::PROJECT_CACHE:
                score += 60.0;
                break;
            case CacheLocation::PROJECT_LINKS:
                score += 40.0;
                break;
        }
        
        // 空间可用性分数
        size_t available_space = get_available_space(path);
        if (available_space > 0) {
            score += std::min(50.0, static_cast<double>(available_space) / (1024 * 1024 * 1024));  // 每GB加50分
        }
        
        // 访问性能分数（基于文件系统类型和位置）
        if (path.find("/home") != std::string::npos || path.find("~") != std::string::npos) {
            score += 20.0;  // 用户目录通常性能更好
        }
        
        // 包已存在分数
        std::string package_path = path + "/" + package;
        if (fs::exists(package_path)) {
            score += 30.0;  // 包已存在，避免重复下载
        }
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error calculating location score: " << e.what();
        score = -1.0;
    }
    
    return score;
}

// 全局函数实现
bool initialize_path_resolver() {
    g_path_resolver = std::make_unique<CachePathResolver>();
    return g_path_resolver->optimize_cache_paths();
}

void cleanup_path_resolver() {
    g_path_resolver.reset();
}

} // namespace Paker 