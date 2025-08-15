#pragma once

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

namespace Paker {

// 性能指标类型
enum class MetricType {
    INSTALL_TIME,      // 安装时间
    DOWNLOAD_SPEED,    // 下载速度
    MEMORY_USAGE,      // 内存使用
    DISK_USAGE,        // 磁盘使用
    NETWORK_LATENCY,   // 网络延迟
    PARSE_TIME,        // 解析时间
    RESOLVE_TIME       // 解析时间
};

// 性能指标
struct PerformanceMetric {
    MetricType type;
    std::string name;
    double value;
    std::string unit;
    std::chrono::steady_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
    
    PerformanceMetric(MetricType t, const std::string& n, double v, const std::string& u)
        : type(t), name(n), value(v), unit(u), timestamp(std::chrono::steady_clock::now()) {}
};

// 性能监控器
class PerformanceMonitor {
private:
    std::map<std::string, std::vector<PerformanceMetric>> metrics_;
    std::map<std::string, std::chrono::steady_clock::time_point> timers_;
    bool enabled_;
    
public:
    PerformanceMonitor();
    
    // 启用/禁用监控
    void enable(bool enabled = true) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    // 开始计时
    void start_timer(const std::string& name);
    
    // 结束计时并记录
    void end_timer(const std::string& name, MetricType type = MetricType::INSTALL_TIME);
    
    // 记录指标
    void record_metric(MetricType type, const std::string& name, double value, 
                      const std::string& unit = "", 
                      const std::map<std::string, std::string>& metadata = {});
    
    // 获取指标
    std::vector<PerformanceMetric> get_metrics(const std::string& category = "") const;
    
    // 生成性能报告
    std::string generate_performance_report() const;
    
    // 清空指标
    void clear();
    
    // 保存到文件
    bool save_to_file(const std::string& filename) const;
    
    // 从文件加载
    bool load_from_file(const std::string& filename);
    
private:
    std::string format_duration(const std::chrono::steady_clock::duration& duration) const;
    std::string format_speed(double bytes_per_second) const;
};

// 全局性能监控器实例
extern PerformanceMonitor g_performance_monitor;

// 便捷宏
#define PAKER_PERF_START(name) g_performance_monitor.start_timer(name)
#define PAKER_PERF_END(name, type) g_performance_monitor.end_timer(name, type)
#define PAKER_PERF_RECORD(type, name, value, unit) g_performance_monitor.record_metric(type, name, value, unit)

} // namespace Paker 