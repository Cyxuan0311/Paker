#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <filesystem>

namespace Paker {

// LRU缓存项
struct LRUCacheItem {
    std::string key;
    std::string package_name;
    std::string version;
    std::string cache_path;
    size_t size_bytes;
    std::chrono::system_clock::time_point last_access;
    std::chrono::system_clock::time_point install_time;
    size_t access_count;
    bool is_pinned; // 是否被固定（不会被清理）
    
    LRUCacheItem() : size_bytes(0), access_count(0), is_pinned(false) {}
    LRUCacheItem(const std::string& key, const std::string& package_name, const std::string& version)
        : key(key), package_name(package_name), version(version), size_bytes(0), access_count(0), is_pinned(false) {}
};

// 缓存策略
enum class CacheEvictionPolicy {
    LRU,                    // 最近最少使用
    LFU,                    // 最少使用频率
    SIZE_BASED,             // 基于大小
    TIME_BASED,             // 基于时间
    HYBRID,                 // 混合策略
    ADAPTIVE                // 自适应策略
};

// 访问模式分析
struct AccessPattern {
    std::string package_name;
    size_t access_count;
    std::chrono::steady_clock::time_point first_access;
    std::chrono::steady_clock::time_point last_access;
    double access_frequency;  // 访问频率（次/小时）
    size_t total_size;
    bool is_hot;             // 是否为热点数据
    double priority_score;   // 优先级分数
    
    AccessPattern() : access_count(0), access_frequency(0.0), total_size(0), 
                     is_hot(false), priority_score(0.0) {}
};

// 缓存碎片信息
struct FragmentationInfo {
    double fragmentation_ratio;  // 碎片化比例 (0.0-1.0)
    size_t total_files;          // 总文件数
    size_t fragmented_files;     // 碎片化文件数
    size_t total_size;           // 总大小
    size_t fragmented_size;      // 碎片化文件大小
    
    FragmentationInfo() : fragmentation_ratio(0.0), total_files(0), fragmented_files(0), 
                         total_size(0), fragmented_size(0) {}
};

// 自适应缓存策略
class AdaptiveCacheStrategy {
private:
    std::map<std::string, AccessPattern> access_patterns_;
    mutable std::mutex patterns_mutex_;
    std::chrono::steady_clock::time_point last_analysis_;
    std::chrono::milliseconds analysis_interval_;
    
    // 策略参数
    double hot_threshold_;      // 热点数据阈值
    double cold_threshold_;     // 冷数据阈值
    size_t min_retention_size_; // 最小保留大小
    double size_weight_;        // 大小权重
    double frequency_weight_;  // 频率权重
    double recency_weight_;    // 最近性权重
    
public:
    AdaptiveCacheStrategy(double hot_threshold = 0.8, double cold_threshold = 0.2,
                         size_t min_retention = 100, double size_w = 0.3,
                         double freq_w = 0.4, double rec_w = 0.3);
    
    void record_access(const std::string& package_name, size_t size);
    void analyze_patterns();
    double calculate_priority(const std::string& package_name) const;
    bool should_evict(const std::string& package_name) const;
    std::vector<std::string> get_eviction_candidates() const;
    void update_strategy_parameters();
};

// 缓存统计
struct CacheStatistics {
    size_t total_items;
    size_t total_size_bytes;
    size_t hit_count;
    size_t miss_count;
    double hit_rate;
    std::chrono::system_clock::time_point last_cleanup;
    std::map<std::string, size_t> package_sizes;
    std::map<std::string, size_t> access_counts;
    
    // 新增的碎片整理统计
    size_t defragmentation_count;
    std::chrono::system_clock::time_point last_defragmentation;
    
    CacheStatistics() : total_items(0), total_size_bytes(0), hit_count(0), miss_count(0), hit_rate(0.0),
                       defragmentation_count(0) {}
};

// LRU缓存管理器
class LRUCacheManager {
private:
    // LRU链表：最近使用的在头部，最少使用的在尾部
    mutable std::list<std::string> lru_list_;
    mutable std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;
    
    // 缓存项存储
    std::unordered_map<std::string, LRUCacheItem> cache_items_;
    
    // 配置
    size_t max_cache_size_;
    size_t max_cache_items_;
    std::chrono::hours max_age_;
    CacheEvictionPolicy eviction_policy_;
    
    // 自适应缓存策略
    std::unique_ptr<AdaptiveCacheStrategy> adaptive_strategy_;
    
    // 统计信息
    mutable CacheStatistics statistics_;
    
    // 线程安全
    mutable std::mutex cache_mutex_;
    
    // 缓存目录
    std::string cache_directory_;
    
    // 内部方法
    void update_lru(const std::string& key) const;
    void evict_item(const std::string& key);
    bool should_evict(const LRUCacheItem& item) const;
    size_t calculate_item_size(const std::string& cache_path) const;
    void update_statistics(const std::string& key, bool hit) const;
    
    // 清理策略
    void evict_by_lru();
    void evict_by_lfu();
    void evict_by_size();
    void evict_by_time();
    void evict_by_hybrid();
    
    // 文件系统操作
    bool remove_cache_directory(const std::string& path) const;
    bool create_cache_directory(const std::string& path) const;
    
    // 缓存碎片整理辅助方法
    FragmentationInfo analyze_cache_fragmentation() const;
    std::vector<LRUCacheItem> get_sorted_cache_items_for_defragmentation() const;
    bool consolidate_cache_item(const LRUCacheItem& item, const std::string& temp_dir);
    void update_cache_index_after_defragmentation();
    
public:
    LRUCacheManager(const std::string& cache_directory,
                   size_t max_cache_size = 10ULL * 1024 * 1024 * 1024, // 10GB
                   size_t max_cache_items = 1000,
                   std::chrono::hours max_age = std::chrono::hours(24 * 30), // 30天
                   CacheEvictionPolicy policy = CacheEvictionPolicy::HYBRID);
    
    ~LRUCacheManager();
    
    // 初始化
    bool initialize();
    
    // 缓存操作
    bool add_item(const std::string& package_name, const std::string& version, 
                 const std::string& cache_path);
    bool remove_item(const std::string& package_name, const std::string& version);
    bool has_item(const std::string& package_name, const std::string& version) const;
    std::string get_item_path(const std::string& package_name, const std::string& version) const;
    
    // 访问管理
    void mark_accessed(const std::string& package_name, const std::string& version);
    void pin_item(const std::string& package_name, const std::string& version, bool pinned = true);
    
    // 清理操作
    bool cleanup_cache();
    bool force_cleanup();
    bool cleanup_package(const std::string& package_name);
    bool cleanup_old_items();
    bool cleanup_unused_items();
    
    // 配置管理
    void set_max_cache_size(size_t max_size) { max_cache_size_ = max_size; }
    void set_max_cache_items(size_t max_items) { max_cache_items_ = max_items; }
    void set_max_age(std::chrono::hours max_age) { max_age_ = max_age; }
    void set_eviction_policy(CacheEvictionPolicy policy) { eviction_policy_ = policy; }
    
    // 自适应缓存管理
    void enable_adaptive_caching(bool enable = true);
    void update_access_pattern(const std::string& package_name, size_t size);
    void optimize_cache_strategy();
    
    // 统计信息
    CacheStatistics get_statistics() const;
    size_t get_cache_size() const;
    size_t get_cache_items_count() const;
    double get_hit_rate() const;
    
    // 查询操作
    std::vector<std::string> get_all_packages() const;
    std::vector<std::string> get_package_versions(const std::string& package_name) const;
    std::vector<LRUCacheItem> get_oldest_items(size_t count = 10) const;
    std::vector<LRUCacheItem> get_least_used_items(size_t count = 10) const;
    
    // 持久化
    bool save_cache_index(const std::string& filename = "") const;
    bool load_cache_index(const std::string& filename = "");
    
    // 维护操作
    void optimize_cache();
    void defragment_cache();
    bool validate_cache_integrity() const;
    
private:
    std::string generate_cache_key(const std::string& package_name, const std::string& version) const;
    void perform_eviction();
    void update_cache_statistics();
};

// 智能缓存清理器
class SmartCacheCleaner {
private:
    LRUCacheManager* cache_manager_;
    
    // 清理策略配置
    struct CleanupConfig {
        double size_threshold;        // 大小阈值（百分比）
        double age_threshold;         // 年龄阈值（天数）
        size_t min_keep_items;        // 最少保留项目数
        bool enable_auto_cleanup;     // 是否启用自动清理
        std::chrono::hours cleanup_interval; // 清理间隔
    };
    
    CleanupConfig config_;
    std::chrono::system_clock::time_point last_cleanup_;
    
public:
    SmartCacheCleaner(LRUCacheManager* cache_manager);
    
    // 配置管理
    void set_cleanup_config(const CleanupConfig& config);
    CleanupConfig get_cleanup_config() const { return config_; }
    
    // 智能清理
    bool should_perform_cleanup() const;
    bool perform_smart_cleanup();
    bool perform_aggressive_cleanup();
    
    // 清理建议
    struct CleanupRecommendation {
        enum class Type {
            NONE,
            LIGHT,      // 轻度清理
            MODERATE,   // 中度清理
            AGGRESSIVE  // 激进清理
        };
        
        Type type;
        size_t estimated_freed_space;
        std::vector<std::string> items_to_remove;
        std::string reason;
    };
    
    CleanupRecommendation get_cleanup_recommendation() const;
    
    // 自动清理
    void enable_auto_cleanup(bool enable = true);
    bool is_auto_cleanup_enabled() const { return config_.enable_auto_cleanup; }
    
private:
    bool is_cleanup_needed() const;
    size_t calculate_cleanup_size() const;
    std::vector<std::string> select_items_for_cleanup(CleanupRecommendation::Type type) const;
};

// 全局LRU缓存管理器实例
extern std::unique_ptr<LRUCacheManager> g_lru_cache_manager;
extern std::unique_ptr<SmartCacheCleaner> g_smart_cache_cleaner;

// 初始化LRU缓存管理器
bool initialize_lru_cache_manager(const std::string& cache_directory,
                                 size_t max_cache_size = 0,
                                 size_t max_cache_items = 0,
                                 std::chrono::hours max_age = std::chrono::hours(0));

// 清理LRU缓存管理器
void cleanup_lru_cache_manager();

} // namespace Paker
