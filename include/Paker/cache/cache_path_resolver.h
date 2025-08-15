#pragma once

#include <string>
#include <vector>
#include <map>
#include "cache_manager.h"

namespace Paker {

// 缓存路径解析器
class CachePathResolver {
public:
    // 缓存位置类型
    enum class CacheLocation {
        USER_CACHE,      // 用户缓存 (~/.paker/cache)
        GLOBAL_CACHE,    // 全局缓存 (/usr/local/share/paker/cache)
        PROJECT_CACHE,   // 项目缓存 (.paker/cache)
        PROJECT_LINKS    // 项目链接 (.paker/links)
    };
    
    // 路径解析结果
    struct PathResolution {
        std::string resolved_path;
        CacheLocation location;
        bool exists;
        size_t size_bytes;
        std::chrono::system_clock::time_point last_modified;
        
        PathResolution() : exists(false), size_bytes(0) {}
    };
    
    CachePathResolver();
    
    // 路径解析
    PathResolution resolve_package_path(const std::string& package, 
                                      const std::string& version = "",
                                      const std::string& project_path = "");
    
    // 智能路径选择
    std::string select_optimal_cache_path(const std::string& package, 
                                        const std::string& version = "");
    
    // 路径验证
    bool validate_cache_path(const std::string& path);
    bool is_cache_path_writable(const std::string& path);
    
    // 路径统计
    struct PathStats {
        size_t total_packages;
        size_t total_size_bytes;
        size_t available_space;
        std::chrono::system_clock::time_point last_cleanup;
    };
    
    PathStats get_path_statistics(CacheLocation location);
    
    // 路径优化
    bool optimize_cache_paths();
    std::vector<std::string> get_recommended_cleanup_paths();
    
    // 迁移辅助
    bool can_migrate_to_location(const std::string& source_path, CacheLocation target_location);
    std::string get_migration_path(const std::string& package, CacheLocation target_location);
    
private:
    // 缓存路径配置
    std::map<CacheLocation, std::string> cache_paths_;
    
    // 路径优先级（按访问频率和性能优化）
    std::vector<CacheLocation> path_priority_;
    
    // 内部方法
    void initialize_paths();
    std::string get_location_path(CacheLocation location) const;
    bool path_exists_and_valid(const std::string& path) const;
    size_t calculate_path_size(const std::string& path) const;
    std::chrono::system_clock::time_point get_path_last_modified(const std::string& path) const;
    size_t get_available_space(const std::string& path) const;
    
    // 智能选择算法
    CacheLocation select_best_location(const std::string& package, const std::string& version);
    double calculate_location_score(CacheLocation location, const std::string& package);
};

// 全局路径解析器实例
extern std::unique_ptr<CachePathResolver> g_path_resolver;

// 便捷函数
bool initialize_path_resolver();
void cleanup_path_resolver();

} // namespace Paker 