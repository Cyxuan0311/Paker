#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include "cache_manager.h"

namespace Paker {

// 缓存监控指标
struct CacheMetrics {
    size_t total_packages;
    size_t total_size_bytes;
    size_t cache_hits;
    size_t cache_misses;
    size_t packages_installed;
    size_t packages_removed;
    std::chrono::system_clock::time_point last_cleanup;
    std::chrono::system_clock::time_point last_optimization;
    
    CacheMetrics() : total_packages(0), total_size_bytes(0), 
                     cache_hits(0), cache_misses(0), 
                     packages_installed(0), packages_removed(0) {}
};

// 缓存性能统计
struct CachePerformance {
    double hit_rate;           // 缓存命中率
    double avg_install_time;   // 平均安装时间（毫秒）
    double avg_access_time;    // 平均访问时间（毫秒）
    size_t concurrent_operations; // 并发操作数
    
    CachePerformance() : hit_rate(0.0), avg_install_time(0.0), 
                        avg_access_time(0.0), concurrent_operations(0) {}
};

// 缓存监控器
class CacheMonitor {
private:
    std::atomic<bool> monitoring_active_;
    std::thread monitor_thread_;
    std::mutex metrics_mutex_;
    
    CacheMetrics current_metrics_;
    CachePerformance current_performance_;
    
    std::map<std::string, std::chrono::system_clock::time_point> operation_times_;
    std::vector<double> install_times_;
    std::vector<double> access_times_;
    
    // 监控配置
    std::chrono::seconds monitoring_interval_;
    size_t max_history_size_;
    
public:
    CacheMonitor();
    ~CacheMonitor();
    
    // 监控控制
    bool start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const { return monitoring_active_.load(); }
    
    // 指标记录
    void record_cache_hit(const std::string& package);
    void record_cache_miss(const std::string& package);
    void record_package_install(const std::string& package, double duration_ms);
    void record_package_remove(const std::string& package);
    void record_package_access(const std::string& package, double duration_ms);
    
    // 指标获取
    CacheMetrics get_current_metrics() const;
    CachePerformance get_current_performance() const;
    
    // 性能分析
    std::string generate_performance_report() const;
    std::vector<std::string> get_performance_recommendations() const;
    
    // 自动优化
    bool auto_optimize_cache();
    bool should_perform_cleanup() const;
    bool should_perform_optimization() const;
    
    // 告警系统
    struct CacheAlert {
        enum class Level { INFO, WARNING, ERROR, CRITICAL };
        
        Level level;
        std::string message;
        std::string recommendation;
        std::chrono::system_clock::time_point timestamp;
        
        CacheAlert(Level l, const std::string& msg, const std::string& rec)
            : level(l), message(msg), recommendation(rec), 
              timestamp(std::chrono::system_clock::now()) {}
    };
    
    std::vector<CacheAlert> get_active_alerts() const;
    void clear_alerts();
    
private:
    // 监控线程函数
    void monitoring_loop();
    
    // 指标计算
    void update_performance_metrics();
    void cleanup_old_metrics();
    
    // 告警检测
    void check_cache_alerts();
    std::vector<CacheAlert> active_alerts_;
    
    // 性能阈值
    static constexpr double HIT_RATE_WARNING_THRESHOLD = 0.7;
    static constexpr double HIT_RATE_ERROR_THRESHOLD = 0.5;
    static constexpr size_t SIZE_WARNING_THRESHOLD = 5ULL * 1024 * 1024 * 1024;  // 5GB
    static constexpr size_t SIZE_ERROR_THRESHOLD = 8ULL * 1024 * 1024 * 1024;    // 8GB
};

// 全局缓存监控器实例
extern std::unique_ptr<CacheMonitor> g_cache_monitor;

// 便捷函数
bool initialize_cache_monitor();
void cleanup_cache_monitor();

} // namespace Paker 