#include "Paker/monitor/performance_monitor.h"
#include "Paker/core/output.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <glog/logging.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace Paker {

// 全局性能监控器实例
PerformanceMonitor g_performance_monitor;

PerformanceMonitor::PerformanceMonitor() : enabled_(true) {}

void PerformanceMonitor::start_timer(const std::string& name) {
    if (!enabled_) return;
    
    timers_[name] = std::chrono::steady_clock::now();
    LOG(INFO) << "Started timer: " << name;
}

void PerformanceMonitor::end_timer(const std::string& name, MetricType type) {
    if (!enabled_) return;
    
    auto it = timers_.find(name);
    if (it == timers_.end()) {
        LOG(WARNING) << "Timer not found: " << name;
        return;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = end_time - it->second;
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    record_metric(type, name, static_cast<double>(duration_ms), "ms");
    timers_.erase(it);
    
    LOG(INFO) << "Ended timer: " << name << " (" << duration_ms << "ms)";
}

void PerformanceMonitor::record_metric(MetricType type, const std::string& name, double value, 
                                      const std::string& unit, 
                                      const std::map<std::string, std::string>& metadata) {
    if (!enabled_) return;
    
    std::string category;
    switch (type) {
        case MetricType::INSTALL_TIME: category = "install"; break;
        case MetricType::DOWNLOAD_SPEED: category = "network"; break;
        case MetricType::MEMORY_USAGE: category = "memory"; break;
        case MetricType::DISK_USAGE: category = "disk"; break;
        case MetricType::NETWORK_LATENCY: category = "network"; break;
        case MetricType::PARSE_TIME: category = "parse"; break;
        case MetricType::RESOLVE_TIME: category = "resolve"; break;
    }
    
    PerformanceMetric metric(type, name, value, unit);
    metric.metadata = metadata;
    metrics_[category].push_back(metric);
    
    LOG(INFO) << "Recorded metric: " << name << " = " << value << " " << unit;
}

std::vector<PerformanceMetric> PerformanceMonitor::get_metrics(const std::string& category) const {
    if (category.empty()) {
        std::vector<PerformanceMetric> all_metrics;
        for (const auto& [cat, metrics] : metrics_) {
            all_metrics.insert(all_metrics.end(), metrics.begin(), metrics.end());
        }
        return all_metrics;
    }
    
    auto it = metrics_.find(category);
    return it != metrics_.end() ? it->second : std::vector<PerformanceMetric>();
}

std::string PerformanceMonitor::generate_performance_report() const {
    if (!enabled_ || metrics_.empty()) {
        return "No performance data available.";
    }
    
    std::ostringstream report;
    report << "📊 Performance Report\n";
    report << "====================\n\n";
    
    for (const auto& [category, metrics] : metrics_) {
        if (metrics.empty()) continue;
        
        report << "Category: " << category << "\n";
        report << std::string(20, '-') << "\n";
        
        // 计算统计信息
        double total_value = 0.0;
        double min_value = std::numeric_limits<double>::max();
        double max_value = std::numeric_limits<double>::lowest();
        
        for (const auto& metric : metrics) {
            total_value += metric.value;
            min_value = std::min(min_value, metric.value);
            max_value = std::max(max_value, metric.value);
        }
        
        double avg_value = total_value / metrics.size();
        
        report << "Total metrics: " << metrics.size() << "\n";
        report << "Average: " << std::fixed << std::setprecision(2) << avg_value;
        if (!metrics[0].unit.empty()) report << " " << metrics[0].unit;
        report << "\n";
        report << "Min: " << std::fixed << std::setprecision(2) << min_value;
        if (!metrics[0].unit.empty()) report << " " << metrics[0].unit;
        report << "\n";
        report << "Max: " << std::fixed << std::setprecision(2) << max_value;
        if (!metrics[0].unit.empty()) report << " " << metrics[0].unit;
        report << "\n\n";
        
        // 显示详细指标
        for (const auto& metric : metrics) {
            report << "  " << metric.name << ": " << std::fixed << std::setprecision(2) << metric.value;
            if (!metric.unit.empty()) report << " " << metric.unit;
            report << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

void PerformanceMonitor::clear() {
    metrics_.clear();
    timers_.clear();
    LOG(INFO) << "Performance monitor cleared";
}

bool PerformanceMonitor::save_to_file(const std::string& filename) const {
    try {
        json j;
        j["enabled"] = enabled_;
        j["metrics"] = json::object();
        
        for (const auto& [category, metrics] : metrics_) {
            j["metrics"][category] = json::array();
            for (const auto& metric : metrics) {
                json metric_json;
                metric_json["type"] = static_cast<int>(metric.type);
                metric_json["name"] = metric.name;
                metric_json["value"] = metric.value;
                metric_json["unit"] = metric.unit;
                metric_json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    metric.timestamp.time_since_epoch()).count();
                metric_json["metadata"] = metric.metadata;
                j["metrics"][category].push_back(metric_json);
            }
        }
        
        std::ofstream file(filename);
        file << j.dump(4);
        
        LOG(INFO) << "Performance data saved to: " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save performance data: " << e.what();
        return false;
    }
}

bool PerformanceMonitor::load_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << filename;
            return false;
        }
        
        json j;
        file >> j;
        
        enabled_ = j.value("enabled", true);
        metrics_.clear();
        
        if (j.contains("metrics")) {
            for (const auto& [category, metrics_array] : j["metrics"].items()) {
                std::vector<PerformanceMetric> category_metrics;
                for (const auto& metric_json : metrics_array) {
                    MetricType type = static_cast<MetricType>(metric_json["type"].get<int>());
                    std::string name = metric_json["name"];
                    double value = metric_json["value"];
                    std::string unit = metric_json["unit"];
                    
                    PerformanceMetric metric(type, name, value, unit);
                    
                    if (metric_json.contains("metadata")) {
                        metric.metadata = metric_json["metadata"].get<std::map<std::string, std::string>>();
                    }
                    
                    if (metric_json.contains("timestamp")) {
                        auto timestamp_ms = metric_json["timestamp"].get<int64_t>();
                        metric.timestamp = std::chrono::steady_clock::time_point(
                            std::chrono::milliseconds(timestamp_ms));
                    }
                    
                    category_metrics.push_back(metric);
                }
                metrics_[category] = category_metrics;
            }
        }
        
        LOG(INFO) << "Performance data loaded from: " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load performance data: " << e.what();
        return false;
    }
}

std::string PerformanceMonitor::format_duration(const std::chrono::steady_clock::duration& duration) const {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    if (ms < 1000) {
        return std::to_string(ms) + "ms";
    } else if (ms < 60000) {
        return std::to_string(ms / 1000.0) + "s";
    } else {
        auto minutes = ms / 60000;
        auto seconds = (ms % 60000) / 1000.0;
        return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    }
}

std::string PerformanceMonitor::format_speed(double bytes_per_second) const {
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit_index = 0;
    double speed = bytes_per_second;
    
    while (speed >= 1024.0 && unit_index < 3) {
        speed /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << speed << " " << units[unit_index];
    return oss.str();
}

} // namespace Paker 