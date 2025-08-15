#pragma once

#include <string>
#include <map>
#include "cache_manager.h"

namespace Paker {

// 缓存配置管理器
class CacheConfig {
private:
    std::string config_file_path_;
    std::map<std::string, std::string> config_values_;
    
public:
    CacheConfig(const std::string& config_path = "");
    ~CacheConfig();
    
    // 配置操作
    bool load_config();
    bool save_config();
    bool set_value(const std::string& key, const std::string& value);
    std::string get_value(const std::string& key, const std::string& default_value = "") const;
    
    // 缓存策略配置
    void set_cache_strategy(CacheStrategy strategy);
    CacheStrategy get_cache_strategy() const;
    
    void set_version_storage(VersionStorage storage);
    VersionStorage get_version_storage() const;
    
    // 路径配置
    void set_global_cache_path(const std::string& path);
    std::string get_global_cache_path() const;
    
    void set_user_cache_path(const std::string& path);
    std::string get_user_cache_path() const;
    
    // 清理策略配置
    void set_max_cache_size(size_t size_bytes);
    size_t get_max_cache_size() const;
    
    void set_max_versions_per_package(size_t max_versions);
    size_t get_max_versions_per_package() const;
    
    void set_cleanup_interval_hours(size_t hours);
    size_t get_cleanup_interval_hours() const;
    
    // 应用配置到缓存管理器
    bool apply_to_cache_manager(CacheManager& manager) const;
    
private:
    // 配置键名常量
    static const std::string KEY_CACHE_STRATEGY;
    static const std::string KEY_VERSION_STORAGE;
    static const std::string KEY_GLOBAL_CACHE_PATH;
    static const std::string KEY_USER_CACHE_PATH;
    static const std::string KEY_MAX_CACHE_SIZE;
    static const std::string KEY_MAX_VERSIONS_PER_PACKAGE;
    static const std::string KEY_CLEANUP_INTERVAL_HOURS;
    
    // 默认配置
    void set_default_config();
    std::string strategy_to_string(CacheStrategy strategy) const;
    CacheStrategy string_to_strategy(const std::string& str) const;
    std::string storage_to_string(VersionStorage storage) const;
    VersionStorage string_to_storage(const std::string& str) const;
};

} // namespace Paker 