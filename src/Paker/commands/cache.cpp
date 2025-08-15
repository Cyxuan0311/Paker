#include "Paker/commands/cache.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/output.h"
#include "Paker/core/utils.h"
#include "Paker/sources.h"
#include <glog/logging.h>
#include <iomanip>
#include <sstream>

namespace Paker {

int pm_cache_install(const std::string& package, const std::string& version) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        // 获取仓库URL
        auto all_repos = get_all_repos();
        auto it = all_repos.find(package);
        if (it == all_repos.end()) {
            Output::error("No repository found for package: " + package);
            return 1;
        }
        
        std::string repo_url = it->second;
        std::string target_version = version.empty() ? "latest" : version;
        
        Output::info("Installing " + package + "@" + target_version + " to cache...");
        
        if (g_cache_manager->install_package_to_cache(package, target_version, repo_url)) {
            Output::success("Successfully installed " + package + "@" + target_version + " to cache");
            return 0;
        } else {
            Output::error("Failed to install " + package + "@" + target_version + " to cache");
            return 1;
        }
        
    } catch (const std::exception& e) {
        Output::error("Error installing package to cache: " + std::string(e.what()));
        LOG(ERROR) << "Error installing package to cache: " << e.what();
        return 1;
    }
}

int pm_cache_remove(const std::string& package, const std::string& version) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        std::string target_version = version.empty() ? "all versions" : version;
        Output::info("Removing " + package + "@" + target_version + " from cache...");
        
        if (g_cache_manager->remove_package_from_cache(package, version)) {
            Output::success("Successfully removed " + package + "@" + target_version + " from cache");
            return 0;
        } else {
            Output::error("Failed to remove " + package + "@" + target_version + " from cache");
            return 1;
        }
        
    } catch (const std::exception& e) {
        Output::error("Error removing package from cache: " + std::string(e.what()));
        LOG(ERROR) << "Error removing package from cache: " << e.what();
        return 1;
    }
}

int pm_cache_list() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        auto package_list = g_cache_manager->get_package_list();
        
        if (package_list.empty()) {
            Output::info("No packages in cache");
            return 0;
        }
        
        Output::info("Cached packages:");
        
        // 按包名分组显示
        std::map<std::string, std::vector<PackageCacheInfo>> grouped_packages;
        for (const auto& info : package_list) {
            grouped_packages[info.package_name].push_back(info);
        }
        
        for (const auto& [package_name, versions] : grouped_packages) {
            Output::info("  " + package_name + ":");
            for (const auto& info : versions) {
                std::ostringstream oss;
                oss << "    " << info.version << " (" << format_size(info.size_bytes) << ")";
                if (info.is_active) {
                    oss << " [active]";
                }
                Output::info(oss.str());
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error listing cached packages: " + std::string(e.what()));
        LOG(ERROR) << "Error listing cached packages: " << e.what();
        return 1;
    }
}

int pm_cache_info(const std::string& package) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        auto package_list = g_cache_manager->get_package_list();
        
        Output::info("Package: " + package);
        
        bool found = false;
        for (const auto& info : package_list) {
            if (info.package_name == package) {
                found = true;
                Output::info("  Version: " + info.version);
                Output::info("  Cache path: " + info.cache_path);
                Output::info("  Repository: " + info.repository_url);
                Output::info("  Size: " + format_size(info.size_bytes));
                Output::info("  Access count: " + std::to_string(info.access_count));
                Output::info("  Active: " + std::string(info.is_active ? "yes" : "no"));
                
                // 格式化时间
                auto install_time = std::chrono::system_clock::to_time_t(info.install_time);
                auto last_access = std::chrono::system_clock::to_time_t(info.last_access);
                
                std::ostringstream install_oss, access_oss;
                install_oss << std::put_time(std::localtime(&install_time), "%Y-%m-%d %H:%M:%S");
                access_oss << std::put_time(std::localtime(&last_access), "%Y-%m-%d %H:%M:%S");
                
                Output::info("  Install time: " + install_oss.str());
                Output::info("  Last access: " + access_oss.str());
                Output::info("");
            }
        }
        
        if (!found) {
            Output::warning("Package not found in cache");
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting package info: " + std::string(e.what()));
        LOG(ERROR) << "Error getting package info: " << e.what();
        return 1;
    }
}

int pm_cache_cleanup() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        Output::info("Cleaning up cache...");
        
        // 清理未使用的包
        if (g_cache_manager->cleanup_unused_packages()) {
            Output::success("Cleaned up unused packages");
        }
        
        // 清理旧版本
        if (g_cache_manager->cleanup_old_versions()) {
            Output::success("Cleaned up old versions");
        }
        
        Output::success("Cache cleanup completed");
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error cleaning up cache: " + std::string(e.what()));
        LOG(ERROR) << "Error cleaning up cache: " << e.what();
        return 1;
    }
}

int pm_cache_stats() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        auto stats = g_cache_manager->get_cache_statistics();
        
        Output::info("📊 Cache Statistics:");
        Output::info("  Total packages: " + std::to_string(stats.total_packages));
        Output::info("  Total size: " + format_size(stats.total_size_bytes));
        Output::info("  Unused packages: " + std::to_string(stats.unused_packages));
        
        // 缓存路径信息
        Output::info("  Global cache: " + g_cache_manager->get_global_cache_path());
        Output::info("  User cache: " + g_cache_manager->get_user_cache_path());
        Output::info("  Project cache: " + g_cache_manager->get_project_cache_path());
        
        // 缓存策略信息
        Output::info("  Cache strategy: " + std::to_string(static_cast<int>(g_cache_manager->get_cache_strategy())));
        Output::info("  Version storage: " + std::to_string(static_cast<int>(g_cache_manager->get_version_storage())));
        
        // 性能建议
        if (stats.unused_packages > 0) {
            Output::warning("  💡 Recommendation: Run 'paker cache-cleanup' to free space");
        }
        
        if (stats.total_size_bytes > 5ULL * 1024 * 1024 * 1024) {  // 5GB
            Output::warning("  💡 Recommendation: Consider cleaning up old versions");
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting cache statistics: " + std::string(e.what()));
        LOG(ERROR) << "Error getting cache statistics: " << e.what();
        return 1;
    }
}

int pm_cache_migrate(const std::string& project_path) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        std::string target_path = project_path.empty() ? fs::current_path().string() : project_path;
        
        Output::info("Migrating project from legacy mode: " + target_path);
        
        if (g_cache_manager->migrate_from_legacy_mode(target_path)) {
            Output::success("Successfully migrated project to cache mode");
            return 0;
        } else {
            Output::error("Failed to migrate project to cache mode");
            return 1;
        }
        
    } catch (const std::exception& e) {
        Output::error("Error migrating project: " + std::string(e.what()));
        LOG(ERROR) << "Error migrating project: " << e.what();
        return 1;
    }
}

int pm_cache_config_set(const std::string& key, const std::string& value) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        // 这里可以实现配置设置逻辑
        Output::info("Setting cache config: " + key + " = " + value);
        Output::warning("Configuration setting not yet implemented");
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error setting cache config: " + std::string(e.what()));
        LOG(ERROR) << "Error setting cache config: " << e.what();
        return 1;
    }
}

int pm_cache_config_get(const std::string& key) {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        // 这里可以实现配置获取逻辑
        Output::info("Getting cache config: " + key);
        Output::warning("Configuration getting not yet implemented");
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting cache config: " + std::string(e.what()));
        LOG(ERROR) << "Error getting cache config: " << e.what();
        return 1;
    }
}

int pm_cache_config_list() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        Output::info("Cache Configuration:");
        Output::info("  Strategy: " + std::to_string(static_cast<int>(g_cache_manager->get_cache_strategy())));
        Output::info("  Version storage: " + std::to_string(static_cast<int>(g_cache_manager->get_version_storage())));
        Output::info("  Global cache: " + g_cache_manager->get_global_cache_path());
        Output::info("  User cache: " + g_cache_manager->get_user_cache_path());
        Output::info("  Project cache: " + g_cache_manager->get_project_cache_path());
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error listing cache config: " + std::string(e.what()));
        LOG(ERROR) << "Error listing cache config: " << e.what();
        return 1;
    }
}

// 辅助函数
std::string format_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

int pm_cache_status() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        Output::info("🔍 Cache Status Report");
        Output::info("=====================");
        
        // 基本状态
        auto stats = g_cache_manager->get_cache_statistics();
        Output::info("📦 Package Status:");
        Output::info("  Total packages: " + std::to_string(stats.total_packages));
        Output::info("  Total size: " + format_size(stats.total_size_bytes));
        Output::info("  Unused packages: " + std::to_string(stats.unused_packages));
        
        // 缓存健康度
        double health_score = 100.0;
        std::vector<std::string> issues;
        
        if (stats.unused_packages > 0) {
            health_score -= 20.0;
            issues.push_back("Unused packages detected");
        }
        
        if (stats.total_size_bytes > 5ULL * 1024 * 1024 * 1024) {
            health_score -= 15.0;
            issues.push_back("Cache size is large");
        }
        
        // 显示健康度
        Output::info("🏥 Cache Health: " + std::to_string(static_cast<int>(health_score)) + "%");
        
        if (!issues.empty()) {
            Output::warning("⚠️  Issues detected:");
            for (const auto& issue : issues) {
                Output::warning("  - " + issue);
            }
        } else {
            Output::success("✅ Cache is healthy");
        }
        
        // 路径状态
        Output::info("📁 Cache Locations:");
        Output::info("  User cache: " + g_cache_manager->get_user_cache_path());
        Output::info("  Global cache: " + g_cache_manager->get_global_cache_path());
        Output::info("  Project cache: " + g_cache_manager->get_project_cache_path());
        
        // 策略信息
        Output::info("⚙️  Cache Configuration:");
        Output::info("  Strategy: " + std::to_string(static_cast<int>(g_cache_manager->get_cache_strategy())));
        Output::info("  Version storage: " + std::to_string(static_cast<int>(g_cache_manager->get_version_storage())));
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting cache status: " + std::string(e.what()));
        LOG(ERROR) << "Error getting cache status: " << e.what();
        return 1;
    }
}

int pm_cache_optimize() {
    try {
        if (!g_cache_manager) {
            Output::error("Cache manager not initialized");
            return 1;
        }
        
        Output::info("🚀 Optimizing cache...");
        
        // 执行优化操作
        bool optimized = false;
        
        // 清理未使用的包
        if (g_cache_manager->cleanup_unused_packages()) {
            Output::success("Cleaned up unused packages");
            optimized = true;
        }
        
        // 清理旧版本
        if (g_cache_manager->cleanup_old_versions()) {
            Output::success("Cleaned up old versions");
            optimized = true;
        }
        
        if (optimized) {
            Output::success("Cache optimization completed successfully");
        } else {
            Output::info("Cache is already optimized");
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error optimizing cache: " + std::string(e.what()));
        LOG(ERROR) << "Error optimizing cache: " << e.what();
        return 1;
    }
}

} // namespace Paker 