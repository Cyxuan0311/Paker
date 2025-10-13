#include "Paker/cache/async_cache_manager.h"
#include "Paker/core/async_io.h"
#include <filesystem>
#include <glog/logging.h>
#include <fstream>
#include <sstream>
#include <functional>

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
            
            // 验证缓存目录是否存在
            if (!std::filesystem::exists(cache_directory_)) {
                LOG(WARNING) << "Cache directory does not exist: " << cache_directory_;
                return false;
            }
            
            // 检查缓存目录权限
            if (!std::filesystem::is_directory(cache_directory_)) {
                LOG(ERROR) << "Cache path is not a directory: " << cache_directory_;
                return false;
            }
            
            // 验证缓存文件完整性
            size_t valid_files = 0;
            size_t total_files = 0;
            
            for (const auto& entry : std::filesystem::directory_iterator(cache_directory_)) {
                if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                    total_files++;
                    
                    // 检查文件大小是否合理（大于0字节）
                    if (entry.file_size() > 0) {
                        valid_files++;
                    } else {
                        LOG(WARNING) << "Empty cache file found: " << entry.path();
                    }
                }
            }
            
            // 计算缓存健康度
            double health_ratio = total_files > 0 ? static_cast<double>(valid_files) / total_files : 1.0;
            
            LOG(INFO) << "Cache validation completed - Valid files: " << valid_files 
                      << "/" << total_files << " (Health: " << (health_ratio * 100) << "%)";
            
            return health_ratio >= 0.8; // 80%以上的文件有效才认为缓存健康
            
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
            
            // 检查缓存目录是否存在
            if (!std::filesystem::exists(cache_directory_)) {
                LOG(WARNING) << "Cache directory does not exist: " << cache_directory_;
                return corrupted_keys;
            }
            
            // 遍历所有缓存文件，检测损坏的文件
            for (const auto& entry : std::filesystem::recursive_directory_iterator(cache_directory_)) {
                if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                    try {
                        // 检查文件是否可读
                        std::ifstream file(entry.path(), std::ios::binary);
                        if (!file.is_open()) {
                            LOG(WARNING) << "Cannot open cache file: " << entry.path();
                            corrupted_keys.push_back(entry.path().filename().string());
                            continue;
                        }
                        
                        // 检查文件大小
                        file.seekg(0, std::ios::end);
                        size_t file_size = file.tellg();
                        file.seekg(0, std::ios::beg);
                        
                        if (file_size == 0) {
                            LOG(WARNING) << "Empty cache file detected: " << entry.path();
                            corrupted_keys.push_back(entry.path().filename().string());
                            continue;
                        }
                        
                        // 尝试读取文件头部，检查是否包含有效的缓存头信息
                        char header[16];
                        file.read(header, sizeof(header));
                        if (static_cast<size_t>(file.gcount()) < sizeof(header)) {
                            LOG(WARNING) << "Cache file too small or corrupted: " << entry.path();
                            corrupted_keys.push_back(entry.path().filename().string());
                            continue;
                        }
                        
                        // 简单的校验和检查（这里可以添加更复杂的验证逻辑）
                        bool is_valid = true;
                        for (size_t i = 0; i < sizeof(header); ++i) {
                            if (header[i] == 0 && i < 8) { // 前8字节不应该全为0
                                is_valid = false;
                                break;
                            }
                        }
                        
                        if (!is_valid) {
                            LOG(WARNING) << "Invalid cache file header: " << entry.path();
                            corrupted_keys.push_back(entry.path().filename().string());
                        }
                        
                    } catch (const std::exception& file_error) {
                        LOG(ERROR) << "Error checking cache file " << entry.path() 
                                  << ": " << file_error.what();
                        corrupted_keys.push_back(entry.path().filename().string());
                    }
                }
            }
            
            if (!corrupted_keys.empty()) {
                LOG(WARNING) << "Found " << corrupted_keys.size() << " corrupted cache files";
            } else {
                LOG(INFO) << "No corrupted cache files found";
            }
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during corrupted cache detection: " << e.what();
            LOG(ERROR) << "Cache directory: " << cache_directory_;
            LOG(ERROR) << "Error type: " << typeid(e).name();
        }
        
        return corrupted_keys;
    });
}

double AsyncCacheManager::get_cache_hit_rate() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return cache_hit_rate_; // 直接返回已计算的命中率，避免重复计算
}

double AsyncCacheManager::get_average_read_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return average_read_time_; // 直接返回已计算的平均读取时间
}

double AsyncCacheManager::get_average_write_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return average_write_time_; // 直接返回已计算的平均写入时间
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
    // 生成缓存键的哈希值，避免文件名过长和特殊字符问题
    std::hash<std::string> hasher;
    size_t hash_value = hasher(cache_key);
    
    // 将哈希值转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << hash_value;
    std::string hash_str = ss.str();
    
    // 创建分层目录结构，避免单个目录下文件过多
    // 使用哈希值的前2位作为第一级目录，接下来2位作为第二级目录
    std::string level1 = hash_str.substr(0, 2);
    std::string level2 = hash_str.length() > 2 ? hash_str.substr(2, 2) : "00";
    
    // 构建完整路径
    std::string cache_path = cache_directory_ + "/" + level1 + "/" + level2 + "/" + hash_str + ".cache";
    
    // 确保目录存在
    std::string dir_path = std::filesystem::path(cache_path).parent_path();
    if (!std::filesystem::exists(dir_path)) {
        try {
            std::filesystem::create_directories(dir_path);
        } catch (const std::filesystem::filesystem_error& e) {
            LOG(ERROR) << "Failed to create cache directory: " << dir_path << " - " << e.what();
            // 回退到简单路径
            return cache_directory_ + "/" + hash_str + ".cache";
        }
    }
    
    return cache_path;
}

void AsyncCacheManager::update_read_stats(const std::chrono::milliseconds& duration, bool hit) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_read_time_ += duration;
    total_read_operations_++;
    
    if (hit) {
        cache_hits_++;
    } else {
        cache_misses_++;
    }
    
    // 更新平均读取时间
    average_read_time_ = total_read_operations_ > 0 ? 
        total_read_time_.count() / total_read_operations_ : 0;
    
    // 更新缓存命中率
    size_t total_requests = cache_hits_ + cache_misses_;
    cache_hit_rate_ = total_requests > 0 ? 
        static_cast<double>(cache_hits_) / total_requests * 100.0 : 0.0;
}

void AsyncCacheManager::update_write_stats(const std::chrono::milliseconds& duration) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_write_time_ += duration;
    total_write_operations_++;
    
    // 更新平均写入时间
    average_write_time_ = total_write_operations_ > 0 ? 
        total_write_time_.count() / total_write_operations_ : 0;
    
    // 记录最大写入时间
    if (duration > max_write_time_) {
        max_write_time_ = duration;
    }
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
