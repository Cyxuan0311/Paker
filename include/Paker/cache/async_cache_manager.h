#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "Paker/core/async_io.h"
#include "Paker/cache/cache_manager.h"

namespace Paker {

// 异步缓存操作结果
struct AsyncCacheResult {
    bool success;
    std::string error_message;
    size_t bytes_processed;
    std::chrono::milliseconds duration;
    
    AsyncCacheResult() : success(false), bytes_processed(0), duration(0) {}
};

// 异步缓存读取结果
struct AsyncCacheReadResult : public AsyncCacheResult {
    std::vector<char> data;
    std::string content;
    size_t cache_size;
    std::chrono::system_clock::time_point last_modified;
    
    AsyncCacheReadResult() : cache_size(0) {}
};

// 异步缓存写入结果
struct AsyncCacheWriteResult : public AsyncCacheResult {
    std::string cache_key;
    size_t bytes_written;
    std::string cache_path;
    
    AsyncCacheWriteResult() : bytes_written(0) {}
};

// 异步缓存管理器
class AsyncCacheManager {
private:
    std::unique_ptr<CacheManager> cache_manager_;
    AsyncIOManager* async_io_manager_;
    
    // 缓存统计
    std::atomic<size_t> total_reads_{0};
    std::atomic<size_t> total_writes_{0};
    std::atomic<size_t> cache_hits_{0};
    std::atomic<size_t> cache_misses_{0};
    std::atomic<size_t> async_operations_{0};
    
    // 性能监控
    std::chrono::milliseconds total_read_time_{0};
    std::chrono::milliseconds total_write_time_{0};
    mutable std::mutex stats_mutex_;
    
public:
    AsyncCacheManager(AsyncIOManager* async_io_manager);
    ~AsyncCacheManager();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 异步缓存读取
    std::future<std::shared_ptr<AsyncCacheReadResult>> read_cache_async(
        const std::string& cache_key, bool read_as_text = true);
    
    // 异步缓存写入
    std::future<std::shared_ptr<AsyncCacheWriteResult>> write_cache_async(
        const std::string& cache_key, const std::vector<char>& data);
    
    std::future<std::shared_ptr<AsyncCacheWriteResult>> write_cache_async(
        const std::string& cache_key, const std::string& content);
    
    // 批量异步操作
    std::vector<std::future<std::shared_ptr<AsyncCacheReadResult>>> read_multiple_cache_async(
        const std::vector<std::string>& cache_keys, bool read_as_text = true);
    
    std::vector<std::future<std::shared_ptr<AsyncCacheWriteResult>>> write_multiple_cache_async(
        const std::vector<std::pair<std::string, std::string>>& cache_data);
    
    // 异步缓存清理
    std::future<bool> clear_cache_async();
    std::future<bool> remove_cache_async(const std::string& cache_key);
    
    // 异步缓存预加载
    std::future<bool> preload_cache_async(const std::vector<std::string>& cache_keys);
    
    // 异步缓存验证
    std::future<bool> validate_cache_async();
    std::future<std::vector<std::string>> get_corrupted_cache_async();
    
    // 统计信息
    size_t get_total_reads() const { return total_reads_; }
    size_t get_total_writes() const { return total_writes_; }
    size_t get_cache_hits() const { return cache_hits_; }
    size_t get_cache_misses() const { return cache_misses_; }
    size_t get_async_operations() const { return async_operations_; }
    
    double get_cache_hit_rate() const;
    double get_average_read_time() const;
    double get_average_write_time() const;
    
    std::string get_performance_report() const;
    
    // 缓存管理器访问
    CacheManager* get_cache_manager() const { return cache_manager_.get(); }
    
private:
    // 内部方法
    std::string get_cache_path(const std::string& cache_key) const;
    void update_read_stats(const std::chrono::milliseconds& duration, bool hit);
    void update_write_stats(const std::chrono::milliseconds& duration);
};

// 异步缓存工具类
class AsyncCacheTools {
public:
    // 便捷方法
    static std::future<std::string> read_cache_text_async(
        AsyncCacheManager& manager, const std::string& cache_key);
    
    static std::future<std::vector<char>> read_cache_binary_async(
        AsyncCacheManager& manager, const std::string& cache_key);
    
    static std::future<bool> write_cache_text_async(
        AsyncCacheManager& manager, const std::string& cache_key, const std::string& content);
    
    static std::future<bool> write_cache_binary_async(
        AsyncCacheManager& manager, const std::string& cache_key, const std::vector<char>& data);
    
    // 批量操作
    static std::future<std::vector<std::string>> read_multiple_cache_text_async(
        AsyncCacheManager& manager, const std::vector<std::string>& cache_keys);
    
    static std::future<bool> write_multiple_cache_text_async(
        AsyncCacheManager& manager, 
        const std::vector<std::pair<std::string, std::string>>& cache_data);
    
    // 缓存管理
    static std::future<size_t> get_cache_size_async(AsyncCacheManager& manager);
    static std::future<std::vector<std::string>> list_cache_keys_async(AsyncCacheManager& manager);
    static std::future<bool> cache_exists_async(AsyncCacheManager& manager, const std::string& cache_key);
    
    // 性能优化
    static std::future<bool> optimize_cache_async(AsyncCacheManager& manager);
    static std::future<bool> defragment_cache_async(AsyncCacheManager& manager);
};

// 全局异步缓存管理器实例
extern std::unique_ptr<AsyncCacheManager> g_async_cache_manager;

// 初始化函数
bool initialize_async_cache_manager(AsyncIOManager* async_io_manager = nullptr);
void cleanup_async_cache_manager();

// 便捷访问函数
AsyncCacheManager* get_async_cache_manager();

} // namespace Paker
