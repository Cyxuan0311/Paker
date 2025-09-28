#include "Paker/core/memory_pool.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Paker {

// SmartMemoryPool 实现
SmartMemoryPool::SmartMemoryPool(size_t max_pool_size, size_t small_size, 
                                size_t medium_size, size_t large_size, size_t huge_size)
    : max_pool_size_(max_pool_size)
    , small_block_size_(small_size)
    , medium_block_size_(medium_size)
    , large_block_size_(large_size)
    , huge_block_size_(huge_size)
    , cleanup_enabled_(true)
    , preallocation_enabled_(true) {
    
    LOG(INFO) << "SmartMemoryPool initialized with max size: " << max_pool_size_ 
              << " bytes";
}

SmartMemoryPool::~SmartMemoryPool() {
    shutdown();
}

bool SmartMemoryPool::initialize() {
    try {
        // 预分配内存块
        if (preallocation_enabled_) {
            preallocate_blocks();
        }
        
        // 启动清理线程
        if (cleanup_enabled_) {
            cleanup_thread_ = std::thread(&SmartMemoryPool::cleanup_thread_function, this);
        }
        
        LOG(INFO) << "SmartMemoryPool initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize SmartMemoryPool: " << e.what();
        return false;
    }
}

void SmartMemoryPool::shutdown() {
    cleanup_enabled_ = false;
    
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    // 释放所有内存块
    for (auto& block : memory_blocks_) {
        if (block.ptr && !block.is_free) {
            std::free(block.ptr);
        }
    }
    
    memory_blocks_.clear();
    free_blocks_by_size_.clear();
    free_blocks_by_type_.clear();
    
    LOG(INFO) << "SmartMemoryPool shutdown completed";
}

void* SmartMemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    if (size == 0) {
        return nullptr;
    }
    
    MemoryBlockType type = get_block_type(size);
    size_t block_index = find_free_block(size, type);
    
    if (block_index == SIZE_MAX) {
        // 没有找到空闲块，分配新块
        void* ptr = allocate_block(size, type);
        if (!ptr) {
            LOG(ERROR) << "Failed to allocate memory block of size: " << size;
            return nullptr;
        }
        
        update_statistics(size, true);
        return ptr;
    }
    
    // 使用现有空闲块
    MemoryBlock& block = memory_blocks_[block_index];
    block.is_free = false;
    block.last_used = std::chrono::steady_clock::now();
    
    update_statistics(size, true);
    return block.ptr;
}

void SmartMemoryPool::deallocate(void* ptr) {
    if (!ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // 查找对应的内存块
    for (size_t i = 0; i < memory_blocks_.size(); ++i) {
        MemoryBlock& block = memory_blocks_[i];
        if (block.ptr == ptr && !block.is_free) {
            block.is_free = true;
            block.last_used = std::chrono::steady_clock::now();
            
            // 添加到空闲块列表
            free_blocks_by_size_[block.size].push_back(i);
            free_blocks_by_type_[block.type].push_back(i);
            
            update_statistics(block.size, false);
            return;
        }
    }
    
    LOG(WARNING) << "Attempted to deallocate unknown pointer: " << ptr;
}

void* SmartMemoryPool::reallocate(void* ptr, size_t new_size) {
    if (!ptr) {
        return allocate(new_size);
    }
    
    if (new_size == 0) {
        deallocate(ptr);
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // 查找现有块
    for (auto& block : memory_blocks_) {
        if (block.ptr == ptr && !block.is_free) {
            if (new_size <= block.size) {
                // 新大小小于等于现有大小，直接返回
                return ptr;
            } else {
                // 需要更大的块，分配新块并复制数据
                void* new_ptr = allocate(new_size);
                if (new_ptr) {
                    std::memcpy(new_ptr, ptr, block.size);
                    deallocate(ptr);
                }
                return new_ptr;
            }
        }
    }
    
    LOG(WARNING) << "Attempted to reallocate unknown pointer: " << ptr;
    return nullptr;
}

MemoryBlockType SmartMemoryPool::get_block_type(size_t size) const {
    if (size <= small_block_size_) {
        return MemoryBlockType::SMALL;
    } else if (size <= medium_block_size_) {
        return MemoryBlockType::MEDIUM;
    } else if (size <= large_block_size_) {
        return MemoryBlockType::LARGE;
    } else {
        return MemoryBlockType::HUGE;
    }
}

size_t SmartMemoryPool::find_free_block(size_t size, MemoryBlockType type) {
    // 首先在相同类型的空闲块中查找
    auto& free_blocks = free_blocks_by_type_[type];
    for (size_t i = 0; i < free_blocks.size(); ++i) {
        size_t block_index = free_blocks[i];
        if (memory_blocks_[block_index].size >= size) {
            // 找到合适的块，从列表中移除
            free_blocks.erase(free_blocks.begin() + i);
            return block_index;
        }
    }
    
    // 在相同大小的空闲块中查找
    auto& size_blocks = free_blocks_by_size_[size];
    if (!size_blocks.empty()) {
        size_t block_index = size_blocks.back();
        size_blocks.pop_back();
        return block_index;
    }
    
    return SIZE_MAX;
}

void* SmartMemoryPool::allocate_block(size_t size, MemoryBlockType type) {
    void* ptr = std::malloc(size);
    if (!ptr) {
        return nullptr;
    }
    
    MemoryBlock block;
    block.ptr = ptr;
    block.size = size;
    block.is_free = false;
    block.last_used = std::chrono::steady_clock::now();
    block.type = type;
    
    memory_blocks_.push_back(block);
    
    return ptr;
}

void SmartMemoryPool::update_statistics(size_t size, bool is_allocation) {
    if (is_allocation) {
        stats_.total_allocated += size;
        stats_.current_usage += size;
        stats_.allocation_count++;
        
        if (stats_.current_usage > stats_.peak_usage) {
            stats_.peak_usage = stats_.current_usage;
        }
    } else {
        stats_.total_freed += size;
        stats_.current_usage -= size;
        stats_.free_count++;
    }
    
    // 计算碎片化比率
    if (stats_.total_allocated > 0) {
        stats_.fragmentation_ratio = static_cast<double>(stats_.total_freed) / stats_.total_allocated;
    }
}

void SmartMemoryPool::cleanup_unused_blocks() {
    auto now = std::chrono::steady_clock::now();
    auto cleanup_threshold = std::chrono::minutes(5); // 5分钟未使用的块
    
    std::vector<size_t> blocks_to_remove;
    
    for (size_t i = 0; i < memory_blocks_.size(); ++i) {
        MemoryBlock& block = memory_blocks_[i];
        if (block.is_free && (now - block.last_used) > cleanup_threshold) {
            blocks_to_remove.push_back(i);
        }
    }
    
    // 从后往前删除，避免索引变化
    for (auto it = blocks_to_remove.rbegin(); it != blocks_to_remove.rend(); ++it) {
        size_t index = *it;
        MemoryBlock& block = memory_blocks_[index];
        
        if (block.ptr) {
            std::free(block.ptr);
        }
        
        memory_blocks_.erase(memory_blocks_.begin() + index);
    }
    
    if (!blocks_to_remove.empty()) {
        LOG(INFO) << "Cleaned up " << blocks_to_remove.size() << " unused memory blocks";
    }
}

void SmartMemoryPool::cleanup() {
    cleanup_unused_blocks();
}

void SmartMemoryPool::optimize() {
    LOG(INFO) << "Optimizing memory pool...";
    
    // 清理未使用的块
    cleanup_unused_blocks();
    
    // 压缩内存块向量
    memory_blocks_.shrink_to_fit();
    
    // 清理空闲块映射
    for (auto& [size, blocks] : free_blocks_by_size_) {
        blocks.shrink_to_fit();
    }
    
    for (auto& [type, blocks] : free_blocks_by_type_) {
        blocks.shrink_to_fit();
    }
    
    LOG(INFO) << "Memory pool optimization completed";
}

void SmartMemoryPool::enable_preallocation(bool enable) {
    preallocation_enabled_ = enable;
    if (enable) {
        preallocate_blocks();
    }
}

size_t SmartMemoryPool::get_current_usage() const {
    return stats_.current_usage;
}

MemoryPoolStats SmartMemoryPool::get_statistics() const {
    return stats_;
}

void SmartMemoryPool::preallocate_blocks() {
    LOG(INFO) << "Preallocating memory blocks...";
    
    // 预分配小对象块
    for (size_t i = 0; i < preallocated_blocks_[MemoryBlockType::SMALL]; ++i) {
        void* ptr = std::malloc(small_block_size_);
        if (ptr) {
            MemoryBlock block;
            block.ptr = ptr;
            block.size = small_block_size_;
            block.is_free = true;
            block.type = MemoryBlockType::SMALL;
            memory_blocks_.push_back(block);
        }
    }
    
    // 预分配中等对象块
    for (size_t i = 0; i < preallocated_blocks_[MemoryBlockType::MEDIUM]; ++i) {
        void* ptr = std::malloc(medium_block_size_);
        if (ptr) {
            MemoryBlock block;
            block.ptr = ptr;
            block.size = medium_block_size_;
            block.is_free = true;
            block.type = MemoryBlockType::MEDIUM;
            memory_blocks_.push_back(block);
        }
    }
    
    LOG(INFO) << "Preallocation completed";
}

void SmartMemoryPool::cleanup_thread_function() {
    while (cleanup_enabled_) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        if (cleanup_enabled_) {
            cleanup_unused_blocks();
            stats_.last_cleanup = std::chrono::steady_clock::now();
        }
    }
}

// StringMemoryPool 实现
StringMemoryPool::StringMemoryPool(size_t pool_size, size_t max_length)
    : string_pool_size_(pool_size)
    , max_string_length_(max_length)
    , string_compression_enabled_(true) {
    
    pool_ = std::make_unique<SmartMemoryPool>(pool_size, 64, 1024, 64*1024, 1024*1024);
}

StringMemoryPool::~StringMemoryPool() {
    if (pool_) {
        pool_->shutdown();
    }
}

bool StringMemoryPool::initialize() {
    return pool_ && pool_->initialize();
}

char* StringMemoryPool::allocate_string(size_t length) {
    if (length > max_string_length_) {
        LOG(WARNING) << "String length " << length << " exceeds maximum " << max_string_length_;
        return nullptr;
    }
    
    char* str = static_cast<char*>(pool_->allocate(length + 1)); // +1 for null terminator
    if (str) {
        str[length] = '\0'; // 确保以null结尾
    }
    
    return str;
}

void StringMemoryPool::deallocate_string(char* str) {
    if (str) {
        pool_->deallocate(str);
    }
}

char* StringMemoryPool::reallocate_string(char* str, size_t new_length) {
    if (new_length > max_string_length_) {
        LOG(WARNING) << "New string length " << new_length << " exceeds maximum " << max_string_length_;
        return nullptr;
    }
    
    return static_cast<char*>(pool_->reallocate(str, new_length + 1));
}

// ConfigMemoryPool 实现
ConfigMemoryPool::ConfigMemoryPool(size_t pool_size)
    : config_count_(0)
    , config_memory_usage_(0) {
    
    pool_ = std::make_unique<SmartMemoryPool>(pool_size, 256, 4096, 64*1024, 1024*1024);
}

ConfigMemoryPool::~ConfigMemoryPool() {
    if (pool_) {
        pool_->shutdown();
    }
}

bool ConfigMemoryPool::initialize() {
    return pool_ && pool_->initialize();
}

void* ConfigMemoryPool::allocate_config(size_t size) {
    void* ptr = pool_->allocate(size);
    if (ptr) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_count_++;
        config_memory_usage_ += size;
    }
    return ptr;
}

void ConfigMemoryPool::deallocate_config(void* ptr) {
    if (ptr) {
        pool_->deallocate(ptr);
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_count_--;
    }
}

// GlobalMemoryManager 实现
std::unique_ptr<SmartMemoryPool> GlobalMemoryManager::global_pool_;
std::unique_ptr<StringMemoryPool> GlobalMemoryManager::string_pool_;
std::unique_ptr<ConfigMemoryPool> GlobalMemoryManager::config_pool_;
std::mutex GlobalMemoryManager::manager_mutex_;
std::atomic<bool> GlobalMemoryManager::initialized_{false};

bool GlobalMemoryManager::initialize_global_pools() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (initialized_) {
        return true;
    }
    
    try {
        // 初始化全局内存池
        global_pool_ = std::make_unique<SmartMemoryPool>(1024 * 1024 * 1024); // 1GB
        if (!global_pool_->initialize()) {
            LOG(ERROR) << "Failed to initialize global memory pool";
            return false;
        }
        
        // 初始化字符串内存池
        string_pool_ = std::make_unique<StringMemoryPool>(64 * 1024 * 1024); // 64MB
        if (!string_pool_->initialize()) {
            LOG(ERROR) << "Failed to initialize string memory pool";
            return false;
        }
        
        // 初始化配置内存池
        config_pool_ = std::make_unique<ConfigMemoryPool>(16 * 1024 * 1024); // 16MB
        if (!config_pool_->initialize()) {
            LOG(ERROR) << "Failed to initialize config memory pool";
            return false;
        }
        
        initialized_ = true;
        LOG(INFO) << "Global memory pools initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize global memory pools: " << e.what();
        return false;
    }
}

void GlobalMemoryManager::shutdown_global_pools() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (!initialized_) {
        return;
    }
    
    if (config_pool_) {
        config_pool_.reset();
    }
    
    if (string_pool_) {
        string_pool_.reset();
    }
    
    if (global_pool_) {
        global_pool_.reset();
    }
    
    initialized_ = false;
    LOG(INFO) << "Global memory pools shutdown completed";
}

void* GlobalMemoryManager::global_allocate(size_t size) {
    if (!initialized_) {
        return std::malloc(size);
    }
    
    return global_pool_->allocate(size);
}

void GlobalMemoryManager::global_deallocate(void* ptr) {
    if (!initialized_) {
        std::free(ptr);
        return;
    }
    
    global_pool_->deallocate(ptr);
}

char* GlobalMemoryManager::allocate_string(size_t length) {
    if (!initialized_) {
        return static_cast<char*>(std::malloc(length + 1));
    }
    
    return string_pool_->allocate_string(length);
}

void GlobalMemoryManager::deallocate_string(char* str) {
    if (!initialized_) {
        std::free(str);
        return;
    }
    
    string_pool_->deallocate_string(str);
}

void* GlobalMemoryManager::allocate_config(size_t size) {
    if (!initialized_) {
        return std::malloc(size);
    }
    
    return config_pool_->allocate_config(size);
}

void GlobalMemoryManager::deallocate_config(void* ptr) {
    if (!initialized_) {
        std::free(ptr);
        return;
    }
    
    config_pool_->deallocate_config(ptr);
}

MemoryPoolStats GlobalMemoryManager::get_global_stats() {
    if (!initialized_ || !global_pool_) {
        return MemoryPoolStats();
    }
    
    return global_pool_->get_statistics();
}

void GlobalMemoryManager::print_memory_report() {
    if (!initialized_) {
        Output::info("Memory pools not initialized");
        return;
    }
    
    auto stats = get_global_stats();
    
    Output::info("=== Memory Pool Report ===");
    Output::info("Total allocated: " + std::to_string(stats.total_allocated) + " bytes");
    Output::info("Total freed: " + std::to_string(stats.total_freed) + " bytes");
    Output::info("Current usage: " + std::to_string(stats.current_usage) + " bytes");
    Output::info("Peak usage: " + std::to_string(stats.peak_usage) + " bytes");
    Output::info("Allocation count: " + std::to_string(stats.allocation_count));
    Output::info("Free count: " + std::to_string(stats.free_count));
    Output::info("Fragmentation ratio: " + std::to_string(stats.fragmentation_ratio));
}

} // namespace Paker
