#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <memory>
#include <mutex>
#include <future>
#include <atomic>
#include "dependency_graph.h"
#include "dependency_resolver.h"

namespace Paker {

// 解析结果缓存项
struct ParseCacheEntry {
    std::string package_name;
    std::string version;
    std::string hash;  // 依赖内容的哈希值
    std::vector<std::string> dependencies;
    std::vector<std::string> dev_dependencies;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point last_parsed;
    std::chrono::system_clock::time_point last_accessed;
    size_t access_count;
    bool is_valid;
    
    ParseCacheEntry() : access_count(0), is_valid(true) {}
};

// 解析统计信息
struct ParseStats {
    size_t total_packages_parsed;
    size_t cache_hits;
    size_t cache_misses;
    size_t incremental_updates;
    size_t full_parses;
    std::chrono::milliseconds total_parse_time;
    std::chrono::milliseconds avg_parse_time;
    std::chrono::milliseconds cache_save_time;
    std::chrono::milliseconds cache_load_time;
    
    ParseStats() : total_packages_parsed(0), cache_hits(0), cache_misses(0), 
                   incremental_updates(0), full_parses(0) {}
};

// 变更检测结果
struct ChangeDetectionResult {
    std::set<std::string> changed_packages;
    std::set<std::string> new_packages;
    std::set<std::string> removed_packages;
    std::set<std::string> version_changed_packages;
    std::map<std::string, std::string> version_changes;
    bool has_changes;
    
    ChangeDetectionResult() : has_changes(false) {}
};

// 解析配置
struct ParseConfig {
    bool enable_caching;
    bool enable_incremental;
    bool enable_parallel;
    bool enable_prediction;
    size_t max_cache_size;
    size_t max_parallel_tasks;
    std::chrono::minutes cache_ttl;
    std::chrono::minutes prediction_window;
    
    ParseConfig() : enable_caching(true), enable_incremental(true), 
                    enable_parallel(true), enable_prediction(true),
                    max_cache_size(1000), max_parallel_tasks(4),
                    cache_ttl(std::chrono::minutes(60)),
                    prediction_window(std::chrono::minutes(30)) {}
};

// 增量解析器
class IncrementalParser {
private:
    // 缓存管理
    std::unordered_map<std::string, ParseCacheEntry> parse_cache_;
    std::string cache_file_path_;
    mutable std::mutex cache_mutex_;
    
    // 解析统计
    ParseStats stats_;
    mutable std::mutex stats_mutex_;
    
    // 配置
    ParseConfig config_;
    
    // 依赖解析器
    std::unique_ptr<DependencyResolver> resolver_;
    
    // 并行处理
    std::vector<std::future<void>> parallel_tasks_;
    std::atomic<size_t> active_tasks_;
    
    // 预测模型
    std::map<std::string, std::vector<std::string>> dependency_patterns_;
    std::map<std::string, std::chrono::system_clock::time_point> last_change_times_;
    
    // 内部方法
    std::string calculate_dependency_hash(const std::string& package_path) const;
    std::string calculate_package_hash(const std::string& package, const std::string& version) const;
    bool is_cache_valid(const ParseCacheEntry& entry) const;
    void update_cache_entry(ParseCacheEntry& entry);
    void evict_old_cache_entries();
    
    // 缓存操作
    bool load_cache_from_disk();
    bool save_cache_to_disk() const;
    void update_cache_stats(bool hit);
    
    // 变更检测
    ChangeDetectionResult detect_changes(const std::vector<std::string>& packages) const;
    bool has_package_changed(const std::string& package, const std::string& version) const;
    
    // 并行解析
    void parse_package_parallel(const std::string& package, const std::string& version);
    void wait_for_parallel_tasks();
    
    // 预测
    std::vector<std::string> predict_dependencies(const std::string& package) const;
    void update_prediction_model(const std::string& package, const std::vector<std::string>& dependencies);
    
public:
    IncrementalParser(const std::string& cache_directory = ".paker/cache");
    ~IncrementalParser();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 配置
    void set_config(const ParseConfig& config);
    ParseConfig get_config() const;
    
    // 解析操作
    bool parse_package(const std::string& package, const std::string& version = "");
    bool parse_packages(const std::vector<std::string>& packages);
    bool parse_project_dependencies();
    
    // 增量解析
    bool incremental_parse(const std::vector<std::string>& packages);
    ChangeDetectionResult detect_project_changes();
    
    // 缓存管理
    void clear_cache();
    void invalidate_package_cache(const std::string& package);
    void invalidate_all_cache();
    size_t get_cache_size() const;
    
    // 统计信息
    ParseStats get_stats() const;
    void reset_stats();
    
    // 依赖图访问
    const DependencyGraph& get_dependency_graph() const;
    DependencyGraph& get_dependency_graph();
    
    // 预测功能
    std::vector<std::string> predict_next_dependencies(const std::string& package) const;
    void update_prediction_data(const std::string& package, const std::vector<std::string>& dependencies);
    
    // 性能优化
    void optimize_cache();
    void preload_common_dependencies();
    
    // 调试和诊断
    std::string get_cache_info() const;
    std::string get_performance_report() const;
    bool validate_cache_integrity() const;
};

// 智能解析策略
class SmartParseStrategy {
private:
    std::map<std::string, std::vector<std::string>> package_patterns_;
    std::map<std::string, double> package_frequencies_;
    std::map<std::string, std::chrono::system_clock::time_point> last_used_;
    
public:
    // 策略选择
    enum class Strategy {
        FULL_PARSE,      // 完整解析
        INCREMENTAL,     // 增量解析
        PREDICTIVE,      // 预测解析
        CACHED_ONLY      // 仅缓存
    };
    
    Strategy select_strategy(const std::string& package, const std::string& version) const;
    
    // 模式学习
    void learn_pattern(const std::string& package, const std::vector<std::string>& dependencies);
    void update_frequency(const std::string& package);
    
    // 预测
    std::vector<std::string> predict_dependencies(const std::string& package) const;
    double get_confidence(const std::string& package) const;
    
    // 优化建议
    std::vector<std::string> get_optimization_suggestions() const;
};

// 全局增量解析器实例
extern std::unique_ptr<IncrementalParser> g_incremental_parser;

// 初始化函数
bool initialize_incremental_parser(const std::string& cache_directory = ".paker/cache");
void cleanup_incremental_parser();

// 便捷访问函数
IncrementalParser* get_incremental_parser();

} // namespace Paker
