#pragma once

#include <string>

namespace Paker {

// 缓存管理命令
int pm_cache_install(const std::string& package, const std::string& version = "");
int pm_cache_remove(const std::string& package, const std::string& version = "");
int pm_cache_list();
int pm_cache_info(const std::string& package);
int pm_cache_cleanup();
int pm_cache_stats();
int pm_cache_status();
int pm_cache_optimize();
int pm_cache_migrate(const std::string& project_path = "");

// 缓存配置命令
int pm_cache_config_set(const std::string& key, const std::string& value);
int pm_cache_config_get(const std::string& key);
int pm_cache_config_list();

// LRU缓存管理函数
void pm_cache_init_lru();
void pm_cache_lru_stats();
void pm_cache_lru_status();
void pm_cache_smart_cleanup();
void pm_cache_most_accessed();
void pm_cache_oldest_items();
void pm_cache_optimization_advice();

} // namespace Paker 