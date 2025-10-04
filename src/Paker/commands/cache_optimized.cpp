#include "Paker/commands/cache.h"
#include "Paker/cache/lru_cache_manager.h"
#include "Paker/core/output.h"
#include "Paker/core/utils.h"
#include "Paker/core/package_manager.h"
#include <glog/logging.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace Paker {

// 获取缓存目录
std::string get_cache_directory() {
    // 默认缓存目录
    std::string home_dir = std::getenv("HOME") ? std::getenv("HOME") : "/tmp";
    return fs::path(home_dir) / ".paker" / "cache";
}

// 格式化字节数显示
std::string format_bytes(size_t bytes) {
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

// 初始化LRU缓存管理器
void pm_cache_init_lru() {
    if (g_lru_cache_manager) {
        Output::warning("LRU cache manager is already initialized");
        return;
    }
    
    // 确保服务已初始化
    if (!initialize_paker_services()) {
        Output::error("Failed to initialize services");
        return;
    }
    
    std::string cache_dir = get_cache_directory();
    if (initialize_lru_cache_manager(cache_dir)) {
        Output::success("LRU cache manager initialized successfully");
    } else {
        Output::error("Failed to initialize LRU cache manager");
    }
}

// 显示LRU缓存统计
void pm_cache_lru_stats() {
    if (!g_lru_cache_manager) {
        // 尝试自动初始化
        Output::info("Initializing LRU cache manager...");
        pm_cache_init_lru();
        if (!g_lru_cache_manager) {
            Output::error("Failed to initialize LRU cache manager");
            return;
        }
    }
    
    auto stats = g_lru_cache_manager->get_statistics();
    
    Output::info("LRU Cache Statistics");
    Output::info("========================");
    
    std::ostringstream ss;
    ss << "Total Items: " << stats.total_items << "\n";
    ss << "Total Size: " << format_bytes(stats.total_size_bytes) << "\n";
    ss << "Hit Rate: " << std::fixed << std::setprecision(2) << (stats.hit_rate * 100) << "%\n";
    ss << "Hit Count: " << stats.hit_count << "\n";
    ss << "Miss Count: " << stats.miss_count << "\n";
    
    auto last_cleanup = std::chrono::system_clock::to_time_t(stats.last_cleanup);
    ss << "Last Cleanup: " << std::put_time(std::localtime(&last_cleanup), "%Y-%m-%d %H:%M:%S");
    
    Output::info(ss.str());
    
    // 显示包大小分布
    if (!stats.package_sizes.empty()) {
        Output::info("\nPackage Size Distribution:");
        for (const auto& [package, size] : stats.package_sizes) {
            Output::info("  " + package + ": " + format_bytes(size));
        }
    }
}

// 显示LRU缓存状态
void pm_cache_lru_status() {
    if (!g_lru_cache_manager) {
        // 尝试自动初始化
        Output::info("Initializing LRU cache manager...");
        pm_cache_init_lru();
        if (!g_lru_cache_manager) {
            Output::error("Failed to initialize LRU cache manager");
            return;
        }
    }
    
    auto stats = g_lru_cache_manager->get_statistics();
    size_t cache_size = g_lru_cache_manager->get_cache_size();
    size_t item_count = g_lru_cache_manager->get_cache_items_count();
    double hit_rate = g_lru_cache_manager->get_hit_rate();
    
    Output::info("LRU Cache Status Report");
    Output::info("===========================");
    
    // 缓存健康度评估
    std::string health_status;
    if (hit_rate > 0.8) {
        health_status = "Excellent";
    } else if (hit_rate > 0.6) {
        health_status = "Good";
    } else if (hit_rate > 0.4) {
        health_status = "Fair";
    } else {
        health_status = "Poor";
    }
    
    std::ostringstream ss;
    ss << "Cache Health: " << health_status << "\n";
    ss << "Cache Size: " << format_bytes(cache_size) << "\n";
    ss << "Item Count: " << item_count << "\n";
    ss << "Hit Rate: " << std::fixed << std::setprecision(2) << (hit_rate * 100) << "%\n";
    
    // 内存使用情况
    double memory_usage_mb = cache_size / (1024.0 * 1024.0);
    ss << "Memory Usage: " << std::fixed << std::setprecision(2) << memory_usage_mb << " MB";
    
    Output::info(ss.str());
    
    // 推荐操作
    Output::info("\n💡 Recommendations:");
    if (hit_rate < 0.5) {
        Output::warning("  - Consider increasing cache size or adjusting eviction policy");
    }
    if (item_count > 500) {
        Output::warning("  - Consider running cache cleanup to remove unused items");
    }
    if (memory_usage_mb > 1000) {
        Output::warning("  - Cache is using significant memory, consider optimization");
    }
}

// 智能缓存清理
void pm_cache_smart_cleanup() {
    if (!g_lru_cache_manager || !g_smart_cache_cleaner) {
        Output::error("LRU cache manager not initialized. Run 'cache-init-lru' first.");
        return;
    }
    
    Output::info("🧹 Starting smart cache cleanup...");
    
    // 获取清理建议
    auto recommendation = g_smart_cache_cleaner->get_cleanup_recommendation();
    
    if (recommendation.type == SmartCacheCleaner::CleanupRecommendation::Type::NONE) {
        Output::info("No cleanup needed - cache is in good condition");
        return;
    }
    
    std::string cleanup_type;
    switch (recommendation.type) {
        case SmartCacheCleaner::CleanupRecommendation::Type::LIGHT:
            cleanup_type = "Light";
            break;
        case SmartCacheCleaner::CleanupRecommendation::Type::MODERATE:
            cleanup_type = "Moderate";
            break;
        case SmartCacheCleaner::CleanupRecommendation::Type::AGGRESSIVE:
            cleanup_type = "Aggressive";
            break;
        default:
            cleanup_type = "Unknown";
            break;
    }
    
    Output::info("Cleanup Recommendation: " + cleanup_type + " cleanup");
    Output::info("Reason: " + recommendation.reason);
    Output::info("Estimated space to free: " + format_bytes(recommendation.estimated_freed_space));
    Output::info("Items to remove: " + std::to_string(recommendation.items_to_remove.size()));
    
    // 执行清理
    bool success = g_smart_cache_cleaner->perform_smart_cleanup();
    
    if (success) {
        Output::success("Smart cache cleanup completed successfully");
        
        // 显示清理后的统计
        auto new_stats = g_lru_cache_manager->get_statistics();
        Output::info("After cleanup:");
        Output::info("  Items: " + std::to_string(new_stats.total_items));
        Output::info("  Size: " + format_bytes(new_stats.total_size_bytes));
        Output::info("  Hit Rate: " + std::to_string(new_stats.hit_rate * 100) + "%");
    } else {
        Output::error("Smart cache cleanup failed");
    }
}

// 显示最常访问的包
void pm_cache_most_accessed() {
    if (!g_lru_cache_manager) {
        Output::error("LRU cache manager not initialized. Run 'cache-init-lru' first.");
        return;
    }
    
    auto stats = g_lru_cache_manager->get_statistics();
    
    if (stats.access_counts.empty()) {
        Output::info("No access statistics available");
        return;
    }
    
    Output::info("Most Accessed Packages");
    Output::info("=========================");
    
    // 按访问次数排序
    std::vector<std::pair<std::string, size_t>> sorted_access;
    for (const auto& [package, count] : stats.access_counts) {
        sorted_access.emplace_back(package, count);
    }
    
    std::sort(sorted_access.begin(), sorted_access.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 显示前10个
    size_t count = std::min(size_t(10), sorted_access.size());
    for (size_t i = 0; i < count; ++i) {
        const auto& [package, access_count] = sorted_access[i];
        Output::info("  " + std::to_string(i + 1) + ". " + package + 
                    " (" + std::to_string(access_count) + " accesses)");
    }
}

// 显示最老的缓存项
void pm_cache_oldest_items() {
    if (!g_lru_cache_manager) {
        Output::error("LRU cache manager not initialized. Run 'cache-init-lru' first.");
        return;
    }
    
    auto oldest_items = g_lru_cache_manager->get_oldest_items(10);
    
    if (oldest_items.empty()) {
        Output::info("No cached items found");
        return;
    }
    
    Output::info("Oldest Cached Items");
    Output::info("======================");
    
    for (size_t i = 0; i < oldest_items.size(); ++i) {
        const auto& item = oldest_items[i];
        
        auto last_access = std::chrono::system_clock::to_time_t(item.last_access);
        std::ostringstream time_ss;
        time_ss << std::put_time(std::localtime(&last_access), "%Y-%m-%d %H:%M:%S");
        
        Output::info("  " + std::to_string(i + 1) + ". " + item.package_name + "@" + item.version);
        Output::info("     Last Access: " + time_ss.str());
        Output::info("     Size: " + format_bytes(item.size_bytes));
        Output::info("     Access Count: " + std::to_string(item.access_count));
        Output::info("");
    }
}

// 固定/取消固定包
void pm_cache_pin_package(const std::string& package_name, const std::string& version, bool pinned) {
    if (!g_lru_cache_manager) {
        Output::error("LRU cache manager not initialized. Run 'cache-init-lru' first.");
        return;
    }
    
    if (!g_lru_cache_manager->has_item(package_name, version)) {
        Output::error("Package " + package_name + "@" + version + " not found in cache");
        return;
    }
    
    g_lru_cache_manager->pin_item(package_name, version, pinned);
    
    std::string action = pinned ? "pinned" : "unpinned";
    Output::success("Package " + package_name + "@" + version + " " + action + " successfully");
}

// 缓存优化建议
void pm_cache_optimization_advice() {
    if (!g_lru_cache_manager) {
        Output::error("LRU cache manager not initialized. Run 'cache-init-lru' first.");
        return;
    }
    
    auto stats = g_lru_cache_manager->get_statistics();
    size_t cache_size = g_lru_cache_manager->get_cache_size();
    size_t item_count = g_lru_cache_manager->get_cache_items_count();
    double hit_rate = g_lru_cache_manager->get_hit_rate();
    
    Output::info("Cache Optimization Advice");
    Output::info("=============================");
    
    std::vector<std::string> recommendations;
    
    // 基于命中率的建议
    if (hit_rate < 0.3) {
        recommendations.push_back("Low hit rate (" + std::to_string(hit_rate * 100) + 
                                 "%): Consider increasing cache size or adjusting eviction policy");
    } else if (hit_rate < 0.6) {
        recommendations.push_back("Moderate hit rate (" + std::to_string(hit_rate * 100) + 
                                 "%): Cache is working but could be improved");
    } else {
        recommendations.push_back("Good hit rate (" + std::to_string(hit_rate * 100) + 
                                 "%): Cache is performing well");
    }
    
    // 基于大小的建议
    double size_gb = cache_size / (1024.0 * 1024.0 * 1024.0);
    if (size_gb > 5.0) {
        recommendations.push_back("Large cache size (" + std::to_string(size_gb) + 
                                 " GB): Consider cleanup or size limits");
    }
    
    // 基于项目数量的建议
    if (item_count > 1000) {
        recommendations.push_back("Many cached items (" + std::to_string(item_count) + 
                                 "): Consider removing unused packages");
    }
    
    // 基于访问模式的建议
    if (!stats.access_counts.empty()) {
        auto max_access = std::max_element(stats.access_counts.begin(), stats.access_counts.end(),
                                          [](const auto& a, const auto& b) { return a.second < b.second; });
        
        if (max_access->second > 100) {
            recommendations.push_back("High access package (" + max_access->first + 
                                     "): Consider pinning frequently used packages");
        }
    }
    
    // 显示建议
    if (recommendations.empty()) {
        Output::info("Cache is optimally configured - no specific recommendations");
    } else {
        for (const auto& rec : recommendations) {
            Output::info(rec);
        }
    }
    
    // 显示当前配置
    Output::info("\nCurrent Configuration:");
    Output::info("  Cache Size: " + format_bytes(cache_size));
    Output::info("  Item Count: " + std::to_string(item_count));
    Output::info("  Hit Rate: " + std::to_string(hit_rate * 100) + "%");
}

} // namespace Paker
