#include "Paker/cache/async_cache_manager.h"
#include "Paker/core/async_io.h"
#include <filesystem>
#include <glog/logging.h>

namespace fs = std::filesystem;

namespace Paker {

// 全局实例
std::unique_ptr<AsyncCacheManager> g_async_cache_manager = nullptr;

AsyncCacheManager::AsyncCacheManager(AsyncIOManager* async_io_manager)
    : async_io_manager_(async_io_manager) {
    cache_manager_ = std::make_unique<CacheManager>();
}

AsyncCacheManager::~AsyncCacheManager() {
    shutdown();
}

bool AsyncCacheManager::initialize() {
    if (!async_io_manager_) {
        LOG(ERROR) << "AsyncIOManager not provided";
        return false;
    }
    
    if (!cache_manager_) {
        cache_manager_ = std::make_unique<CacheManager>();
    }
    
    LOG(INFO) << "AsyncCacheManager initialized";
    return true;
}

void AsyncCacheManager::shutdown() {
    LOG(INFO) << "AsyncCacheManager shutdown";
}

std::future<std::shared_ptr<AsyncCacheReadResult>> AsyncCacheManager::read_cache_async(
    const std::string& cache_key, bool read_as_text) {
    
    return std::async(std::launch::async, [this, cache_key, read_as_text]() -> std::shared_ptr<AsyncCacheReadResult> {
        auto result = std::make_shared<AsyncCacheReadResult>();
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            total_reads_++;
            async_operations_++;
            
            // 检查缓存是否存在
            std::string cache_path = get_cache_path(cache_key);
            if (!fs::exists(cache_path)) {
                cache_misses_++;
                result->success = false;
                result->error_message = "Cache key not found: " + cache_key;
                return result;
            }
            
            // 使用异步I/O读取缓存
            auto future = async_io_manager_->read_file_async(cache_path, read_as_text);
            auto io_result = future.get();
            
            if (!io_result || io_result->status != IOOperationStatus::COMPLETED) {
                cache_misses_++;
                result->success = false;
                result->error_message = "Failed to read cache file: " + cache_path;
                return result;
            }
            
            // 设置结果
            result->success = true;
            result->data = io_result->data;
            result->content = io_result->content;
            result->cache_size = io_result->file_size;
            result->bytes_processed = io_result->bytes_processed;
            result->last_modified = std::chrono::system_clock::now();
            
            cache_hits_++;
            
        } catch (const std::exception& e) {
            cache_misses_++;
            result->success = false;
            result->error_message = "Exception during cache read: " + std::string(e.what());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result->duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        update_read_stats(result->duration, result->success);
        return result;
    });
}

std::future<std::shared_ptr<AsyncCacheWriteResult>> AsyncCacheManager::write_cache_async(
    const std::string& cache_key, const std::vector<char>& data) {
    
    return std::async(std::launch::async, [this, cache_key, data]() -> std::shared_ptr<AsyncCacheWriteResult> {
        auto result = std::make_shared<AsyncCacheWriteResult>();
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            total_writes_++;
            async_operations_++;
            
            std::string cache_path = get_cache_path(cache_key);
            
            // 使用异步I/O写入缓存
            auto future = async_io_manager_->write_file_async(cache_path, data);
            auto io_result = future.get();
            
            if (!io_result || io_result->status != IOOperationStatus::COMPLETED) {
                result->success = false;
                result->error_message = "Failed to write cache file: " + cache_path;
                return result;
            }
            
            // 设置结果
            result->success = true;
            result->cache_key = cache_key;
            result->cache_path = cache_path;
            result->bytes_written = io_result->bytes_written;
            result->bytes_processed = io_result->bytes_written;
            
        } catch (const std::exception& e) {
            result->success = false;
            result->error_message = "Exception during cache write: " + std::string(e.what());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result->duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        update_write_stats(result->duration);
        return result;
    });
}

std::future<std::shared_ptr<AsyncCacheWriteResult>> AsyncCacheManager::write_cache_async(
    const std::string& cache_key, const std::string& content) {
    
    return std::async(std::launch::async, [this, cache_key, content]() -> std::shared_ptr<AsyncCacheWriteResult> {
        auto result = std::make_shared<AsyncCacheWriteResult>();
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            total_writes_++;
            async_operations_++;
            
            std::string cache_path = get_cache_path(cache_key);
            
            // 使用异步I/O写入缓存
            auto future = async_io_manager_->write_file_async(cache_path, content);
            auto io_result = future.get();
            
            if (!io_result || io_result->status != IOOperationStatus::COMPLETED) {
                result->success = false;
                result->error_message = "Failed to write cache file: " + cache_path;
                return result;
            }
            
            // 设置结果
            result->success = true;
            result->cache_key = cache_key;
            result->cache_path = cache_path;
            result->bytes_written = io_result->bytes_written;
            result->bytes_processed = io_result->bytes_written;
            
        } catch (const std::exception& e) {
            result->success = false;
            result->error_message = "Exception during cache write: " + std::string(e.what());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result->duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        update_write_stats(result->duration);
        return result;
    });
}

std::vector<std::future<std::shared_ptr<AsyncCacheReadResult>>> AsyncCacheManager::read_multiple_cache_async(
    const std::vector<std::string>& cache_keys, bool read_as_text) {
    
    std::vector<std::future<std::shared_ptr<AsyncCacheReadResult>>> futures;
    futures.reserve(cache_keys.size());
    
    for (const auto& cache_key : cache_keys) {
        futures.push_back(read_cache_async(cache_key, read_as_text));
    }
    
    return futures;
}

std::vector<std::future<std::shared_ptr<AsyncCacheWriteResult>>> AsyncCacheManager::write_multiple_cache_async(
    const std::vector<std::pair<std::string, std::string>>& cache_data) {
    
    std::vector<std::future<std::shared_ptr<AsyncCacheWriteResult>>> futures;
    futures.reserve(cache_data.size());
    
    for (const auto& [cache_key, content] : cache_data) {
        futures.push_back(write_cache_async(cache_key, content));
    }
    
    return futures;
}

std::future<bool> AsyncCacheManager::clear_cache_async() {
    return std::async(std::launch::async, [this]() -> bool {
        try {
            async_operations_++;
            
            if (cache_manager_) {
                // 清理所有未使用的包
                cache_manager_->cleanup_unused_packages();
            }
            
            return true;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during cache clear: " << e.what();
            return false;
        }
    });
}

std::future<bool> AsyncCacheManager::remove_cache_async(const std::string& cache_key) {
    return std::async(std::launch::async, [this, cache_key]() -> bool {
        try {
            async_operations_++;
            
            std::string cache_path = get_cache_path(cache_key);
            if (fs::exists(cache_path)) {
                fs::remove(cache_path);
                return true;
            }
            
            return false;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during cache remove: " << e.what();
            return false;
        }
    });
}

std::future<bool> AsyncCacheManager::preload_cache_async(const std::vector<std::string>& cache_keys) {
    return std::async(std::launch::async, [this, cache_keys]() -> bool {
        try {
            async_operations_++;
            
            auto futures = read_multiple_cache_async(cache_keys, true);
            bool all_success = true;
            
            for (auto& future : futures) {
                auto result = future.get();
                if (!result || !result->success) {
                    all_success = false;
                }
            }
            
            return all_success;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during cache preload: " << e.what();
            return false;
        }
    });
}

std::future<bool> AsyncCacheManager::validate_cache_async() {
    return std::async(std::launch::async, [this]() -> bool {
        try {
            async_operations_++;
            
            // 这里可以实现更复杂的缓存验证逻辑
            return true;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during cache validation: " << e.what();
            return false;
        }
    });
}

std::future<std::vector<std::string>> AsyncCacheManager::get_corrupted_cache_async() {
    return std::async(std::launch::async, [this]() -> std::vector<std::string> {
        std::vector<std::string> corrupted_keys;
        
        try {
            async_operations_++;
            
            // 这里可以实现缓存损坏检测逻辑
            // 暂时返回空列表
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during corrupted cache detection: " << e.what();
        }
        
        return corrupted_keys;
    });
}

double AsyncCacheManager::get_cache_hit_rate() const {
    size_t total = cache_hits_ + cache_misses_;
    if (total == 0) return 0.0;
    return static_cast<double>(cache_hits_) / total * 100.0;
}

double AsyncCacheManager::get_average_read_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    size_t reads = total_reads_.load();
    if (reads == 0) return 0.0;
    return static_cast<double>(total_read_time_.count()) / reads;
}

double AsyncCacheManager::get_average_write_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    size_t writes = total_writes_.load();
    if (writes == 0) return 0.0;
    return static_cast<double>(total_write_time_.count()) / writes;
}

std::string AsyncCacheManager::get_performance_report() const {
    std::stringstream ss;
    ss << "AsyncCache Performance Report:\n";
    ss << "  Total reads: " << total_reads_.load() << "\n";
    ss << "  Total writes: " << total_writes_.load() << "\n";
    ss << "  Cache hits: " << cache_hits_.load() << "\n";
    ss << "  Cache misses: " << cache_misses_.load() << "\n";
    ss << "  Cache hit rate: " << get_cache_hit_rate() << "%\n";
    ss << "  Async operations: " << async_operations_.load() << "\n";
    ss << "  Average read time: " << get_average_read_time() << "ms\n";
    ss << "  Average write time: " << get_average_write_time() << "ms\n";
    
    return ss.str();
}

std::string AsyncCacheManager::get_cache_path(const std::string& cache_key) const {
    // 简单的缓存路径生成，实际项目中应该有更复杂的逻辑
    return "cache/" + cache_key + ".cache";
}

void AsyncCacheManager::update_read_stats(const std::chrono::milliseconds& duration, bool hit) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_read_time_ += duration;
}

void AsyncCacheManager::update_write_stats(const std::chrono::milliseconds& duration) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_write_time_ += duration;
}

// 全局函数实现
bool initialize_async_cache_manager(AsyncIOManager* async_io_manager) {
    if (g_async_cache_manager) {
        LOG(WARNING) << "AsyncCache manager already initialized";
        return true;
    }
    
    if (!async_io_manager) {
        async_io_manager = get_async_io_manager();
        if (!async_io_manager) {
            LOG(ERROR) << "AsyncIOManager not available";
            return false;
        }
    }
    
    g_async_cache_manager = std::make_unique<AsyncCacheManager>(async_io_manager);
    return g_async_cache_manager->initialize();
}

void cleanup_async_cache_manager() {
    if (g_async_cache_manager) {
        g_async_cache_manager->shutdown();
        g_async_cache_manager.reset();
    }
}

AsyncCacheManager* get_async_cache_manager() {
    return g_async_cache_manager.get();
}

} // namespace Paker
