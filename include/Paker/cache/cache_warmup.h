#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <glog/logging.h>
#include "Paker/core/service_container.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/dependency/dependency_resolver.h"

namespace Paker {

// 预热优先级
enum class WarmupPriority {
    CRITICAL = 0,    // 关键包（系统核心依赖）
    HIGH = 1,        // 高优先级（常用包）
    NORMAL = 2,      // 普通优先级
    LOW = 3,         // 低优先级（可选包）
    BACKGROUND = 4   // 后台预热
};

// 预热策略
enum class WarmupStrategy {
    IMMEDIATE,       // 立即预热（阻塞启动）
    ASYNC,          // 异步预热（非阻塞）
    BACKGROUND,     // 后台预热（低优先级）
    ON_DEMAND,      // 按需预热
    SCHEDULED       // 定时预热
};

// 包预热信息
struct PackageWarmupInfo {
    std::string package_name;
    std::string version;
    std::string repository_url;
    WarmupPriority priority;
    size_t estimated_size;
    std::chrono::system_clock::time_point last_accessed;
    size_t access_frequency;
    double popularity_score;
    bool is_essential;
    bool is_preloaded;
    
    PackageWarmupInfo() 
        : estimated_size(0), access_frequency(0), popularity_score(0.0)
        , is_essential(false), is_preloaded(false) {}
};

// 预热统计信息
struct WarmupStats {
    size_t total_packages;
    size_t preloaded_packages;
    size_t failed_packages;
    size_t skipped_packages;
    std::chrono::milliseconds total_time;
    std::chrono::milliseconds average_time_per_package;
    size_t total_size_preloaded;
    double success_rate;
    
    WarmupStats() 
        : total_packages(0), preloaded_packages(0), failed_packages(0)
        , skipped_packages(0), total_size_preloaded(0), success_rate(0.0) {}
};

// 预热进度回调
using WarmupProgressCallback = std::function<void(const std::string& package, 
                                                const std::string& version, 
                                                size_t current, 
                                                size_t total, 
                                                bool success)>;

// 缓存预热服务
class CacheWarmupService : public IService {
private:
    // 配置参数
    size_t max_concurrent_preloads_;
    size_t max_preload_size_;
    std::chrono::seconds preload_timeout_;
    WarmupStrategy default_strategy_;
    
    // 预热数据
    std::vector<PackageWarmupInfo> packages_to_preload_;
    std::map<std::string, PackageWarmupInfo> package_registry_;
    std::map<WarmupPriority, std::vector<std::string>> priority_queues_;
    
    // 异步控制
    std::atomic<bool> is_preloading_;
    std::atomic<bool> should_stop_;
    std::vector<std::thread> preload_threads_;
    mutable std::mutex preload_mutex_;
    std::condition_variable preload_cv_;
    
    // 进度监控
    std::atomic<size_t> current_preload_count_;
    std::atomic<size_t> total_preload_count_;
    WarmupProgressCallback progress_callback_;
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    WarmupStats stats_;
    
    // 依赖服务
    CacheManager* cache_manager_;
    DependencyResolver* dependency_resolver_;
    
public:
    CacheWarmupService();
    ~CacheWarmupService();
    
    // IService接口实现
    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "CacheWarmupService"; }
    
    // 配置管理
    void set_max_concurrent_preloads(size_t max) { max_concurrent_preloads_ = max; }
    void set_max_preload_size(size_t max) { max_preload_size_ = max; }
    void set_preload_timeout(std::chrono::seconds timeout) { preload_timeout_ = timeout; }
    void set_default_strategy(WarmupStrategy strategy) { default_strategy_ = strategy; }
    
    // 包注册和管理
    bool register_package(const std::string& package, const std::string& version,
                         const std::string& repository_url, WarmupPriority priority = WarmupPriority::NORMAL);
    bool unregister_package(const std::string& package, const std::string& version = "");
    bool update_package_priority(const std::string& package, const std::string& version, WarmupPriority priority);
    
    // 预热控制
    bool start_preload(WarmupStrategy strategy = WarmupStrategy::ASYNC);
    bool stop_preload();
    bool is_preloading() const { return is_preloading_.load(); }
    
    // 智能预热
    bool start_smart_preload(const std::vector<std::string>& project_dependencies = {});
    bool preload_essential_packages();
    bool preload_popular_packages(size_t count = 10);
    
    // 进度监控
    void set_progress_callback(WarmupProgressCallback callback) { progress_callback_ = callback; }
    size_t get_current_progress() const { return current_preload_count_.load(); }
    size_t get_total_progress() const { return total_preload_count_.load(); }
    double get_progress_percentage() const;
    
    // 统计信息
    WarmupStats get_statistics() const;
    std::vector<PackageWarmupInfo> get_preload_queue() const;
    std::vector<PackageWarmupInfo> get_preloaded_packages() const;
    
    // 智能分析
    bool analyze_usage_patterns(const std::string& project_path = "");
    bool update_popularity_scores();
    bool optimize_preload_order();
    
    // 配置持久化
    bool save_preload_config(const std::string& config_path) const;
    bool load_preload_config(const std::string& config_path);
    bool load_default_config();
    
private:
    // 内部预热逻辑
    void preload_worker_thread();
    bool preload_single_package(const PackageWarmupInfo& package_info);
    void update_preload_progress(const std::string& package, const std::string& version, bool success);
    bool copy_installed_package_to_cache(const PackageWarmupInfo& package_info);
    
    // 优先级管理
    void rebuild_priority_queues();
    std::vector<PackageWarmupInfo> get_packages_by_priority(WarmupPriority priority) const;
    
    // 智能分析
    double calculate_popularity_score(const PackageWarmupInfo& package) const;
    bool is_package_essential(const std::string& package) const;
    std::vector<std::string> analyze_project_dependencies(const std::string& project_path) const;
    void scan_installed_packages_for_warmup();
    
    // 资源管理
    bool check_preload_resources(const PackageWarmupInfo& package) const;
    void cleanup_failed_preloads();
    
    // 配置管理
    void apply_configuration();
};

// 预热配置
struct WarmupConfig {
    bool enable_auto_preload;
    WarmupStrategy default_strategy;
    size_t max_concurrent_preloads;
    size_t max_preload_size_mb;
    std::chrono::seconds preload_timeout;
    std::vector<std::string> essential_packages;
    std::vector<std::string> excluded_packages;
    bool enable_smart_analysis;
    std::chrono::hours analysis_interval;
    
    WarmupConfig() 
        : enable_auto_preload(true)
        , default_strategy(WarmupStrategy::ASYNC)
        , max_concurrent_preloads(4)
        , max_preload_size_mb(1024)  // 1GB
        , preload_timeout(std::chrono::seconds(300))  // 5分钟
        , enable_smart_analysis(true)
        , analysis_interval(std::chrono::hours(24)) {}
};

// 全局预热服务实例
extern std::unique_ptr<CacheWarmupService> g_cache_warmup_service;

// 便捷函数
bool initialize_cache_warmup_service();
void cleanup_cache_warmup_service();

} // namespace Paker
