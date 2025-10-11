#pragma once

#include "Paker/common.h"
#include <unordered_map>

namespace Paker {

// 内存块类型
enum class MemoryBlockType {
    SMALL,      // 小对象 (< 1KB)
    MEDIUM,     // 中等对象 (1KB - 64KB)
    LARGE,      // 大对象 (64KB - 1MB)
    HUGE        // 超大对象 (> 1MB)
};

// 内存块信息
struct MemoryBlock {
    void* ptr;
    size_t size;
    bool is_free;
    std::chrono::steady_clock::time_point last_used;
    MemoryBlockType type;
    
    MemoryBlock() : ptr(nullptr), size(0), is_free(true), type(MemoryBlockType::SMALL) {}
};

// 内存池统计信息
struct MemoryPoolStats {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
    double fragmentation_ratio;
    std::chrono::steady_clock::time_point last_cleanup;
    
    MemoryPoolStats() : total_allocated(0), total_freed(0), current_usage(0), 
                       peak_usage(0), allocation_count(0), free_count(0), 
                       fragmentation_ratio(0.0) {}
};

// 智能内存池管理器
class SmartMemoryPool {
private:
    // 内存池配置
    size_t max_pool_size_;
    size_t small_block_size_;
    size_t medium_block_size_;
    size_t large_block_size_;
    size_t huge_block_size_;
    
    // 内存块管理
    std::vector<MemoryBlock> memory_blocks_;
    std::unordered_map<size_t, std::vector<size_t>> free_blocks_by_size_;
    std::unordered_map<MemoryBlockType, std::vector<size_t>> free_blocks_by_type_;
    
    // 统计信息
    MemoryPoolStats stats_;
    
    // 线程安全
    mutable std::mutex pool_mutex_;
    std::atomic<bool> cleanup_enabled_;
    std::thread cleanup_thread_;
    
    // 内存预分配
    std::unordered_map<MemoryBlockType, size_t> preallocated_blocks_;
    std::atomic<bool> preallocation_enabled_;
    
    // 内部方法
    MemoryBlockType get_block_type(size_t size) const;
    size_t find_free_block(size_t size, MemoryBlockType type);
    void* allocate_block(size_t size, MemoryBlockType type);
    void deallocate_block(void* ptr);
    void cleanup_unused_blocks();
    void preallocate_blocks();
    void update_statistics(size_t allocated_size, bool is_allocation);
    
public:
    SmartMemoryPool(size_t max_pool_size = 1024 * 1024 * 1024,  // 1GB
                   size_t small_size = 1024,                     // 1KB
                   size_t medium_size = 64 * 1024,              // 64KB
                   size_t large_size = 1024 * 1024,            // 1MB
                   size_t huge_size = 16 * 1024 * 1024);       // 16MB
    
    ~SmartMemoryPool();
    
    // 内存分配和释放
    void* allocate(size_t size);
    void deallocate(void* ptr);
    void* reallocate(void* ptr, size_t new_size);
    
    // 内存池管理
    bool initialize();
    void shutdown();
    void cleanup();
    void optimize();
    
    // 预分配管理
    void enable_preallocation(bool enable = true);
    void set_preallocation_blocks(MemoryBlockType type, size_t count);
    void preallocate_all_blocks();
    
    // 统计信息
    MemoryPoolStats get_statistics() const;
    size_t get_current_usage() const;
    size_t get_peak_usage() const;
    double get_fragmentation_ratio() const;
    size_t get_free_blocks_count() const;
    
    // 配置管理
    void set_max_pool_size(size_t size);
    void set_block_sizes(size_t small, size_t medium, size_t large, size_t huge);
    void enable_cleanup(bool enable = true);
    
private:
    void cleanup_thread_function();
};

// 字符串内存池（专门用于字符串优化）
class StringMemoryPool {
private:
    std::unique_ptr<SmartMemoryPool> pool_;
    std::unordered_map<std::string, size_t> string_frequency_;
    mutable std::mutex string_mutex_;
    
    // 字符串池配置
    size_t string_pool_size_;
    size_t max_string_length_;
    std::atomic<bool> string_compression_enabled_;
    
public:
    StringMemoryPool(size_t pool_size = 64 * 1024 * 1024,  // 64MB
                    size_t max_length = 1024 * 1024);      // 1MB
    
    ~StringMemoryPool();
    
    // 字符串分配
    char* allocate_string(size_t length);
    void deallocate_string(char* str);
    char* reallocate_string(char* str, size_t new_length);
    
    // 字符串池管理
    bool initialize();
    void optimize_string_pool();
    void compress_strings();
    
    // 统计信息
    size_t get_string_count() const;
    size_t get_string_memory_usage() const;
    double get_string_compression_ratio() const;
    
private:
    void update_string_frequency(const std::string& str);
    void compress_frequent_strings();
};

// 配置项内存池（专门用于配置优化）
class ConfigMemoryPool {
private:
    std::unique_ptr<SmartMemoryPool> pool_;
    std::unordered_map<std::string, void*> config_entries_;
    mutable std::mutex config_mutex_;
    
    // 配置池统计
    size_t config_count_;
    size_t config_memory_usage_;
    
public:
    ConfigMemoryPool(size_t pool_size = 16 * 1024 * 1024);  // 16MB
    
    ~ConfigMemoryPool();
    
    // 配置项管理
    void* allocate_config(size_t size);
    void deallocate_config(void* ptr);
    void* reallocate_config(void* ptr, size_t new_size);
    
    // 配置池管理
    bool initialize();
    void cleanup_config_pool();
    
    // 统计信息
    size_t get_config_count() const;
    size_t get_config_memory_usage() const;
};

// 全局内存池管理器
class GlobalMemoryManager {
private:
    static std::unique_ptr<SmartMemoryPool> global_pool_;
    static std::unique_ptr<StringMemoryPool> string_pool_;
    static std::unique_ptr<ConfigMemoryPool> config_pool_;
    static std::mutex manager_mutex_;
    static std::atomic<bool> initialized_;
    
public:
    // 全局内存池管理
    static bool initialize_global_pools();
    static void shutdown_global_pools();
    
    // 全局分配器
    static void* global_allocate(size_t size);
    static void global_deallocate(void* ptr);
    static void* global_reallocate(void* ptr, size_t new_size);
    
    // 字符串分配器
    static char* allocate_string(size_t length);
    static void deallocate_string(char* str);
    static char* reallocate_string(char* str, size_t new_length);
    
    // 配置分配器
    static void* allocate_config(size_t size);
    static void deallocate_config(void* ptr);
    static void* reallocate_config(void* ptr, size_t new_size);
    
    // 统计信息
    static MemoryPoolStats get_global_stats();
    static size_t get_total_memory_usage();
    static void print_memory_report();
};

} // namespace Paker
