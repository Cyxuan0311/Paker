#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <filesystem>

namespace Paker {

// 缓存策略
enum class CacheStrategy {
    GLOBAL_ONLY,      // 仅全局缓存
    USER_ONLY,        // 仅用户缓存
    HYBRID,           // 混合模式（推荐）
    PROJECT_LOCAL     // 项目本地（向后兼容）
};

// 版本存储策略
enum class VersionStorage {
    FULL_HISTORY,     // 完整Git历史
    SHALLOW_CLONE,    // 浅克隆（当前）
    ARCHIVE_ONLY,     // 仅归档文件
    COMPRESSED        // 压缩存储
};

// 包缓存信息
struct PackageCacheInfo {
    std::string package_name;
    std::string version;
    std::string cache_path;
    std::string repository_url;
    std::chrono::system_clock::time_point install_time;
    std::chrono::system_clock::time_point last_access;
    size_t size_bytes;
    size_t access_count;
    bool is_active;
    
    PackageCacheInfo() : size_bytes(0), access_count(0), is_active(true) {}
};

// 缓存统计信息
struct CacheStats {
    size_t total_packages;
    size_t total_size_bytes;
    size_t duplicate_packages;
    size_t unused_packages;
    std::chrono::system_clock::time_point last_cleanup;
    
    CacheStats() : total_packages(0), total_size_bytes(0), 
                   duplicate_packages(0), unused_packages(0) {}
};

// 全局缓存管理器
class CacheManager {
private:
    std::string global_cache_path_;
    std::string user_cache_path_;
    std::string project_cache_path_;
    CacheStrategy strategy_;
    VersionStorage version_storage_;
    
    // 缓存索引
    std::map<std::string, std::map<std::string, PackageCacheInfo>> package_index_;
    
    // 配置
    size_t max_cache_size_;
    size_t max_versions_per_package_;
    std::chrono::hours cleanup_interval_;
    
public:
    CacheManager();
    ~CacheManager();
    
    // 初始化缓存系统
    bool initialize(const std::string& config_path = "");
    
    // 缓存策略管理
    void set_cache_strategy(CacheStrategy strategy) { strategy_ = strategy; }
    CacheStrategy get_cache_strategy() const { return strategy_; }
    
    void set_version_storage(VersionStorage storage) { version_storage_ = storage; }
    VersionStorage get_version_storage() const { return version_storage_; }
    
    // 路径管理
    std::string get_global_cache_path() const { return global_cache_path_; }
    std::string get_user_cache_path() const { return user_cache_path_; }
    std::string get_project_cache_path() const { return project_cache_path_; }
    
    // 包缓存操作
    bool install_package_to_cache(const std::string& package, const std::string& version, 
                                 const std::string& repository_url);
    bool is_package_cached(const std::string& package, const std::string& version = "") const;
    std::string get_cached_package_path(const std::string& package, const std::string& version = "") const;
    bool remove_package_from_cache(const std::string& package, const std::string& version = "");
    
    // 项目链接管理
    bool create_project_link(const std::string& package, const std::string& version, 
                           const std::string& project_path);
    bool remove_project_link(const std::string& package, const std::string& project_path);
    std::string get_project_package_path(const std::string& package, const std::string& project_path) const;
    
    // 缓存维护
    bool cleanup_unused_packages();
    bool cleanup_old_versions();
    bool cleanup_by_size();
    bool cleanup_by_age(const std::chrono::hours& max_age);
    
    // 缓存统计
    CacheStats get_cache_statistics() const;
    std::vector<PackageCacheInfo> get_package_list() const;
    size_t get_cache_size() const;
    
    // 缓存优化
    bool optimize_cache();
    bool defragment_cache();
    bool validate_cache_integrity();
    
    // 迁移支持
    bool migrate_from_legacy_mode(const std::string& project_path);
    bool migrate_to_cache_mode(const std::string& project_path);
    
    // 缓存索引管理
    bool save_cache_index();
    
private:
    // 内部辅助方法
    bool load_cache_index();
    bool update_package_info(const std::string& package, const std::string& version);
    std::string generate_cache_key(const std::string& package, const std::string& version) const;
    bool create_symbolic_link(const std::string& target, const std::string& link_path);
    bool remove_symbolic_link(const std::string& link_path);
    size_t calculate_directory_size(const std::string& path) const;
    std::vector<std::string> get_all_versions(const std::string& package) const;
    
    // 配置管理
    bool load_configuration(const std::string& config_path);
    bool save_configuration(const std::string& config_path);
    
    // 路径解析
    std::string resolve_cache_path(const std::string& package, const std::string& version) const;
    std::string resolve_project_path(const std::string& package, const std::string& project_path) const;
    
    // 安装方法
    bool install_shallow_clone(const std::string& repo_url, const std::string& cache_path, const std::string& version);
    bool install_archive_only(const std::string& repo_url, const std::string& cache_path, const std::string& version);
    bool install_compressed(const std::string& repo_url, const std::string& cache_path, const std::string& version);
};

// 全局缓存管理器实例
extern std::unique_ptr<CacheManager> g_cache_manager;

// 便捷函数
bool initialize_cache_manager();
void cleanup_cache_manager();

} // namespace Paker 