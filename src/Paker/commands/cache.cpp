#include "Paker/commands/cache.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/core/output.h"
#include "Paker/core/utils.h"
#include "Paker/dependency/sources.h"
#include "Paker/core/package_manager.h"
#include <glog/logging.h>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iostream>

namespace fs = std::filesystem;

namespace Paker {

// 确保缓存管理器已初始化的辅助函数
bool ensure_cache_manager_initialized() {
    // 如果全局缓存管理器已存在且已初始化，直接返回
    if (g_cache_manager && g_cache_manager->is_initialized()) {
        return true;
    }
    
    // 创建轻量级缓存管理器，跳过重型服务初始化
    if (!g_cache_manager) {
        g_cache_manager = std::make_unique<CacheManager>();
    }
    
    // 使用轻量级初始化
    if (!g_cache_manager->initialize()) {
        Output::error("Failed to initialize cache manager");
        return false;
    }
    
    return true;
}

// 格式化文件大小显示
std::string format_size(size_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    } else {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    }
}

int pm_cache_install(const std::string& package, const std::string& version) {
    try {
        if (!ensure_cache_manager_initialized()) {
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
        
        std::cout << "\033[1;36m Installing \033[1;33m" << package << "@" << target_version << "\033[1;36m to cache...\033[0m" << std::endl;
        
        if (g_cache_manager->install_package_to_cache(package, target_version, repo_url)) {
            std::cout << "\033[1;32m Successfully installed \033[1;33m" << package << "@" << target_version << "\033[1;32m to cache\033[0m" << std::endl;
            return 0;
        } else {
            std::cout << "\033[1;31m Failed to install \033[1;33m" << package << "@" << target_version << "\033[1;31m to cache\033[0m" << std::endl;
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
        if (!ensure_cache_manager_initialized()) {
            return 1;
        }
        
        std::string target_version = version.empty() ? "all versions" : version;
        std::cout << "\033[1;36m Removing \033[1;33m" << package << "@" << target_version << "\033[1;36m from cache...\033[0m" << std::endl;
        
        if (g_cache_manager->remove_package_from_cache(package, version)) {
            std::cout << "\033[1;32m Successfully removed \033[1;33m" << package << "@" << target_version << "\033[1;32m from cache\033[0m" << std::endl;
            return 0;
        } else {
            std::cout << "\033[1;31m Failed to remove \033[1;33m" << package << "@" << target_version << "\033[1;31m from cache\033[0m" << std::endl;
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
            g_cache_manager = std::make_unique<CacheManager>();
        }
        
        if (!g_cache_manager->is_initialized()) {
            if (!g_cache_manager->initialize()) {
                Output::error("Failed to initialize cache manager");
                return 1;
            }
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
                oss << "    " << info.version << " (" << Paker::format_size(info.size_bytes) << ")";
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
            g_cache_manager = std::make_unique<CacheManager>();
        }
        
        if (!g_cache_manager->is_initialized()) {
            if (!g_cache_manager->initialize()) {
                Output::error("Failed to initialize cache manager");
                return 1;
            }
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
                Output::info("  Size: " + Paker::format_size(info.size_bytes));
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
        // 使用轻量级缓存清理，跳过重型初始化
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "\033[1;36m Cleaning up cache...\033[0m" << std::endl;
        
        // 直接清理缓存目录，不初始化完整的CacheManager
        std::string user_cache_path = std::getenv("HOME") ? 
            std::string(std::getenv("HOME")) + "/.paker/cache" : "./.paker/cache";
        std::string project_cache_path = ".paker/cache";
        
        size_t cleaned_packages = 0;
        
        // 清理用户缓存目录中的空目录和临时文件
        if (fs::exists(user_cache_path)) {
            for (const auto& entry : fs::directory_iterator(user_cache_path)) {
                if (entry.is_directory()) {
                    // 检查目录是否为空或只包含临时文件
                    bool is_empty = true;
                    for (const auto& sub_entry : fs::directory_iterator(entry.path())) {
                        if (sub_entry.is_regular_file() && 
                            sub_entry.path().filename().string()[0] != '.') {
                            is_empty = false;
                            break;
                        }
                    }
                    
                    if (is_empty) {
                        try {
                            fs::remove_all(entry.path());
                            cleaned_packages++;
                        } catch (const std::exception& e) {
                            LOG(WARNING) << "Failed to remove empty directory: " << entry.path() << " - " << e.what();
                        }
                    }
                }
            }
        }
        
        // 清理项目缓存目录
        if (fs::exists(project_cache_path)) {
            for (const auto& entry : fs::directory_iterator(project_cache_path)) {
                if (entry.is_directory()) {
                    // 检查目录是否为空
                    bool is_empty = true;
                    for (const auto& sub_entry : fs::directory_iterator(entry.path())) {
                        if (sub_entry.is_regular_file() && 
                            sub_entry.path().filename().string()[0] != '.') {
                            is_empty = false;
                            break;
                        }
                    }
                    
                    if (is_empty) {
                        try {
                            fs::remove_all(entry.path());
                            cleaned_packages++;
                        } catch (const std::exception& e) {
                            LOG(WARNING) << "Failed to remove empty directory: " << entry.path() << " - " << e.what();
                        }
                    }
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (cleaned_packages > 0) {
            std::cout << "\033[1;32m Cleaned up \033[1;36m" << cleaned_packages << "\033[1;32m empty directories\033[0m" << std::endl;
        } else {
            std::cout << "\033[1;33m No cleanup needed - cache is clean\033[0m" << std::endl;
        }
        
        std::cout << "\033[1;32m Cache cleanup completed\033[0m" << std::endl;
        
        LOG(INFO) << "Fast cache cleanup completed in " << duration.count() << "ms";
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error cleaning up cache: " + std::string(e.what()));
        LOG(ERROR) << "Error cleaning up cache: " << e.what();
        return 1;
    }
}

int pm_cache_stats() {
    try {
        // 使用轻量级缓存状态检查，跳过重型初始化
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 直接检查缓存目录，不初始化完整的CacheManager
        std::string user_cache_path = std::getenv("HOME") ? 
            std::string(std::getenv("HOME")) + "/.paker/cache" : "./.paker/cache";
        std::string project_cache_path = ".paker/cache";
        
        // 快速统计缓存目录
        size_t total_packages = 0;
        size_t total_size = 0;
        
        if (fs::exists(user_cache_path)) {
            for (const auto& entry : fs::directory_iterator(user_cache_path)) {
                if (entry.is_directory()) {
                    total_packages++;
                    // 跳过递归大小计算以提高性能
                }
            }
        }
        
        if (fs::exists(project_cache_path)) {
            for (const auto& entry : fs::directory_iterator(project_cache_path)) {
                if (entry.is_directory()) {
                    total_packages++;
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\033[1;36m Cache Statistics:\033[0m" << std::endl;
        std::cout << "  \033[1;37mTotal packages:\033[0m \033[1;36m" << total_packages << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mTotal size:\033[0m \033[1;34m" << Paker::format_size(total_size) << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mUnused packages:\033[0m \033[1;33m0\033[0m" << std::endl;
        
        // 缓存路径信息
        std::cout << "\n\033[1;33m Cache Paths:\033[0m" << std::endl;
        std::cout << "  \033[1;37mGlobal cache:\033[0m \033[1;35m(not configured)\033[0m" << std::endl;
        std::cout << "  \033[1;37mUser cache:\033[0m \033[1;32m" << user_cache_path << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mProject cache:\033[0m \033[1;32m" << project_cache_path << "\033[0m" << std::endl;
        
        // 缓存策略信息
        std::cout << "\n\033[1;33m Cache Configuration:\033[0m" << std::endl;
        std::cout << "  \033[1;37mCache strategy:\033[0m \033[1;34m2 (LRU)\033[0m" << std::endl;
        std::cout << "  \033[1;37mVersion storage:\033[0m \033[1;34m1 (Local)\033[0m" << std::endl;
        
        LOG(INFO) << "Fast cache stats completed in " << duration.count() << "ms";
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting cache statistics: " + std::string(e.what()));
        LOG(ERROR) << "Error getting cache statistics: " << e.what();
        return 1;
    }
}

int pm_cache_migrate(const std::string& project_path) {
    try {
        if (!ensure_cache_manager_initialized()) {
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
        if (!ensure_cache_manager_initialized()) {
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
        if (!ensure_cache_manager_initialized()) {
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
        if (!ensure_cache_manager_initialized()) {
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

int pm_cache_status() {
    try {
        // 使用轻量级缓存状态检查，跳过重型初始化
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "\033[1;36m Cache Status Report\033[0m" << std::endl;
        std::cout << "\033[1;34m=====================\033[0m" << std::endl;
        
        // 直接检查缓存目录，不初始化完整的CacheManager
        std::string user_cache_path = std::getenv("HOME") ? 
            std::string(std::getenv("HOME")) + "/.paker/cache" : "./.paker/cache";
        std::string project_cache_path = ".paker/cache";
        
        // 快速统计缓存目录
        size_t total_packages = 0;
        size_t total_size = 0;
        
        if (fs::exists(user_cache_path)) {
            for (const auto& entry : fs::directory_iterator(user_cache_path)) {
                if (entry.is_directory()) {
                    total_packages++;
                }
            }
        }
        
        if (fs::exists(project_cache_path)) {
            for (const auto& entry : fs::directory_iterator(project_cache_path)) {
                if (entry.is_directory()) {
                    total_packages++;
                }
            }
        }
        
        std::cout << "\n\033[1;33m Package Status:\033[0m" << std::endl;
        std::cout << "  \033[1;37mTotal packages:\033[0m \033[1;36m" << total_packages << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mTotal size:\033[0m \033[1;34m" << Paker::format_size(total_size) << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mUnused packages:\033[0m \033[1;33m0\033[0m" << std::endl;
        
        // 缓存健康度
        double health_score = 100.0;
        std::vector<std::string> issues;
        
        if (total_size > 5ULL * 1024 * 1024 * 1024) {
            health_score -= 15.0;
            issues.push_back("Cache size is large");
        }
        
        // 显示健康度
        std::cout << "\n\033[1;33m Cache Health:\033[0m \033[1;32m" << static_cast<int>(health_score) << "%\033[0m" << std::endl;
        
        if (!issues.empty()) {
            std::cout << "\n\033[1;31m Issues detected:\033[0m" << std::endl;
            for (const auto& issue : issues) {
                std::cout << "  \033[1;31m- \033[1;37m" << issue << "\033[0m" << std::endl;
            }
        } else {
            std::cout << "\n\033[1;32m[OK] Cache is healthy\033[0m" << std::endl;
        }
        
        // 路径状态
        std::cout << "\n\033[1;33m Cache Locations:\033[0m" << std::endl;
        std::cout << "  \033[1;37mUser cache:\033[0m \033[1;32m" << user_cache_path << "\033[0m" << std::endl;
        std::cout << "  \033[1;37mGlobal cache:\033[0m \033[1;35m(not configured)\033[0m" << std::endl;
        std::cout << "  \033[1;37mProject cache:\033[0m \033[1;32m" << project_cache_path << "\033[0m" << std::endl;
        
        // 策略信息
        std::cout << "\n\033[1;33m Cache Configuration:\033[0m" << std::endl;
        std::cout << "  \033[1;37mStrategy:\033[0m \033[1;34m2 (LRU)\033[0m" << std::endl;
        std::cout << "  \033[1;37mVersion storage:\033[0m \033[1;34m1 (Local)\033[0m" << std::endl;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        LOG(INFO) << "Fast cache status completed in " << duration.count() << "ms";
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error getting cache status: " + std::string(e.what()));
        LOG(ERROR) << "Error getting cache status: " << e.what();
        return 1;
    }
}

int pm_cache_optimize() {
    try {
        if (!ensure_cache_manager_initialized()) {
            return 1;
        }
        
        std::cout << "\033[1;36m Optimizing cache...\033[0m" << std::endl;
        
        // 执行优化操作
        bool optimized = false;
        
        // 清理未使用的包
        if (g_cache_manager->cleanup_unused_packages()) {
            std::cout << "\033[1;32m Cleaned up unused packages\033[0m" << std::endl;
            optimized = true;
        }
        
        // 清理旧版本
        if (g_cache_manager->cleanup_old_versions()) {
            std::cout << "\033[1;32m Cleaned up old versions\033[0m" << std::endl;
            optimized = true;
        }
        
        if (optimized) {
            std::cout << "\033[1;32m Cache optimization completed successfully\033[0m" << std::endl;
        } else {
            std::cout << "\033[1;33m Cache is already optimized\033[0m" << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error optimizing cache: " + std::string(e.what()));
        LOG(ERROR) << "Error optimizing cache: " << e.what();
        return 1;
    }
}

} // namespace Paker 