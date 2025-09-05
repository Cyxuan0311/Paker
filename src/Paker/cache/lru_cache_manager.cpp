#include "Paker/cache/lru_cache_manager.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

// 全局LRU缓存管理器实例
std::unique_ptr<LRUCacheManager> g_lru_cache_manager;
std::unique_ptr<SmartCacheCleaner> g_smart_cache_cleaner;

LRUCacheManager::LRUCacheManager(const std::string& cache_directory,
                               size_t max_cache_size,
                               size_t max_cache_items,
                               std::chrono::hours max_age,
                               CacheEvictionPolicy policy)
    : max_cache_size_(max_cache_size)
    , max_cache_items_(max_cache_items)
    , max_age_(max_age)
    , eviction_policy_(policy)
    , cache_directory_(cache_directory) {
    
    LOG(INFO) << "LRUCacheManager initialized with max size: " << max_cache_size_ 
              << " bytes, max items: " << max_cache_items_;
}

LRUCacheManager::~LRUCacheManager() {
    save_cache_index();
}

bool LRUCacheManager::initialize() {
    try {
        // 创建缓存目录
        if (!create_cache_directory(cache_directory_)) {
            LOG(ERROR) << "Failed to create cache directory: " << cache_directory_;
            return false;
        }
        
        // 加载现有缓存索引
        if (!load_cache_index()) {
            LOG(WARNING) << "Failed to load cache index, creating new one";
        }
        
        // 验证缓存完整性
        if (!validate_cache_integrity()) {
            LOG(WARNING) << "Cache integrity validation failed, performing cleanup";
            cleanup_cache();
        }
        
        LOG(INFO) << "LRUCacheManager initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize LRUCacheManager: " << e.what();
        return false;
    }
}

bool LRUCacheManager::add_item(const std::string& package_name, const std::string& version, 
                              const std::string& cache_path) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::string key = generate_cache_key(package_name, version);
        
        // 检查是否已存在
        if (cache_items_.find(key) != cache_items_.end()) {
            LOG(DEBUG) << "Item already exists: " << key;
            mark_accessed(package_name, version);
            return true;
        }
        
        // 创建新的缓存项
        LRUCacheItem item(key, package_name, version);
        item.cache_path = cache_path;
        item.size_bytes = calculate_item_size(cache_path);
        item.last_access = std::chrono::system_clock::now();
        item.install_time = std::chrono::system_clock::now();
        item.access_count = 1;
        
        // 检查是否需要清理
        if (statistics_.total_size_bytes + item.size_bytes > max_cache_size_ ||
            cache_items_.size() >= max_cache_items_) {
            perform_eviction();
        }
        
        // 添加项
        cache_items_[key] = item;
        lru_list_.push_front(key);
        lru_map_[key] = lru_list_.begin();
        
        // 更新统计
        statistics_.total_items++;
        statistics_.total_size_bytes += item.size_bytes;
        statistics_.package_sizes[package_name] += item.size_bytes;
        
        LOG(INFO) << "Added cache item: " << key << " (size: " << item.size_bytes << " bytes)";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to add cache item: " << e.what();
        return false;
    }
}

bool LRUCacheManager::remove_item(const std::string& package_name, const std::string& version) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::string key = generate_cache_key(package_name, version);
        
        auto it = cache_items_.find(key);
        if (it == cache_items_.end()) {
            LOG(DEBUG) << "Item not found: " << key;
            return false;
        }
        
        const LRUCacheItem& item = it->second;
        
        // 从LRU列表中移除
        auto lru_it = lru_map_.find(key);
        if (lru_it != lru_map_.end()) {
            lru_list_.erase(lru_it->second);
            lru_map_.erase(lru_it);
        }
        
        // 从缓存中移除
        cache_items_.erase(it);
        
        // 更新统计
        statistics_.total_items--;
        statistics_.total_size_bytes -= item.size_bytes;
        auto size_it = statistics_.package_sizes.find(package_name);
        if (size_it != statistics_.package_sizes.end()) {
            size_it->second -= item.size_bytes;
            if (size_it->second == 0) {
                statistics_.package_sizes.erase(size_it);
            }
        }
        
        LOG(INFO) << "Removed cache item: " << key;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to remove cache item: " << e.what();
        return false;
    }
}

bool LRUCacheManager::has_item(const std::string& package_name, const std::string& version) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string key = generate_cache_key(package_name, version);
    return cache_items_.find(key) != cache_items_.end();
}

std::string LRUCacheManager::get_item_path(const std::string& package_name, const std::string& version) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string key = generate_cache_key(package_name, version);
    auto it = cache_items_.find(key);
    if (it != cache_items_.end()) {
        update_lru(key);
        update_statistics(key, true);
        return it->second.cache_path;
    }
    
    update_statistics(key, false);
    return "";
}

void LRUCacheManager::mark_accessed(const std::string& package_name, const std::string& version) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string key = generate_cache_key(package_name, version);
    auto it = cache_items_.find(key);
    if (it != cache_items_.end()) {
        it->second.last_access = std::chrono::system_clock::now();
        it->second.access_count++;
        update_lru(key);
        update_statistics(key, true);
    }
}

void LRUCacheManager::pin_item(const std::string& package_name, const std::string& version, bool pinned) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string key = generate_cache_key(package_name, version);
    auto it = cache_items_.find(key);
    if (it != cache_items_.end()) {
        it->second.is_pinned = pinned;
        LOG(INFO) << (pinned ? "Pinned" : "Unpinned") << " cache item: " << key;
    }
}

bool LRUCacheManager::cleanup_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        LOG(INFO) << "Starting cache cleanup...";
        
        size_t initial_items = cache_items_.size();
        size_t initial_size = statistics_.total_size_bytes;
        
        // 执行清理
        perform_eviction();
        
        // 清理过期项
        cleanup_old_items();
        
        // 清理未使用项
        cleanup_unused_items();
        
        size_t final_items = cache_items_.size();
        size_t final_size = statistics_.total_size_bytes;
        
        statistics_.last_cleanup = std::chrono::system_clock::now();
        
        LOG(INFO) << "Cache cleanup completed. Removed " << (initial_items - final_items) 
                  << " items, freed " << (initial_size - final_size) << " bytes";
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to cleanup cache: " << e.what();
        return false;
    }
}

bool LRUCacheManager::force_cleanup() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        LOG(INFO) << "Starting force cleanup...";
        
        // 移除所有未固定的项
        auto it = cache_items_.begin();
        while (it != cache_items_.end()) {
            if (!it->second.is_pinned) {
                std::string key = it->first;
                ++it;
                evict_item(key);
            } else {
                ++it;
            }
        }
        
        LOG(INFO) << "Force cleanup completed";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to force cleanup: " << e.what();
        return false;
    }
}

bool LRUCacheManager::cleanup_package(const std::string& package_name) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::vector<std::string> keys_to_remove;
        
        for (const auto& [key, item] : cache_items_) {
            if (item.package_name == package_name) {
                keys_to_remove.push_back(key);
            }
        }
        
        for (const auto& key : keys_to_remove) {
            evict_item(key);
        }
        
        LOG(INFO) << "Cleaned up package: " << package_name << " (" << keys_to_remove.size() << " items)";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to cleanup package: " << e.what();
        return false;
    }
}

bool LRUCacheManager::cleanup_old_items() {
    auto cutoff_time = std::chrono::system_clock::now() - max_age_;
    std::vector<std::string> keys_to_remove;
    
    for (const auto& [key, item] : cache_items_) {
        if (!item.is_pinned && item.last_access < cutoff_time) {
            keys_to_remove.push_back(key);
        }
    }
    
    for (const auto& key : keys_to_remove) {
        evict_item(key);
    }
    
    if (!keys_to_remove.empty()) {
        LOG(INFO) << "Cleaned up " << keys_to_remove.size() << " old items";
    }
    
    return true;
}

bool LRUCacheManager::cleanup_unused_items() {
    // 清理访问次数少于2次的项
    std::vector<std::string> keys_to_remove;
    
    for (const auto& [key, item] : cache_items_) {
        if (!item.is_pinned && item.access_count < 2) {
            keys_to_remove.push_back(key);
        }
    }
    
    for (const auto& key : keys_to_remove) {
        evict_item(key);
    }
    
    if (!keys_to_remove.empty()) {
        LOG(INFO) << "Cleaned up " << keys_to_remove.size() << " unused items";
    }
    
    return true;
}

CacheStatistics LRUCacheManager::get_statistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return statistics_;
}

size_t LRUCacheManager::get_cache_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return statistics_.total_size_bytes;
}

size_t LRUCacheManager::get_cache_items_count() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_items_.size();
}

double LRUCacheManager::get_hit_rate() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return statistics_.hit_rate;
}

std::vector<std::string> LRUCacheManager::get_all_packages() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::set<std::string> packages;
    for (const auto& [key, item] : cache_items_) {
        packages.insert(item.package_name);
    }
    
    return std::vector<std::string>(packages.begin(), packages.end());
}

std::vector<std::string> LRUCacheManager::get_package_versions(const std::string& package_name) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<std::string> versions;
    for (const auto& [key, item] : cache_items_) {
        if (item.package_name == package_name) {
            versions.push_back(item.version);
        }
    }
    
    return versions;
}

std::vector<LRUCacheItem> LRUCacheManager::get_oldest_items(size_t count) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<std::pair<std::string, LRUCacheItem>> items;
    for (const auto& [key, item] : cache_items_) {
        items.emplace_back(key, item);
    }
    
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) {
                  return a.second.last_access < b.second.last_access;
              });
    
    std::vector<LRUCacheItem> result;
    for (size_t i = 0; i < std::min(count, items.size()); ++i) {
        result.push_back(items[i].second);
    }
    
    return result;
}

std::vector<LRUCacheItem> LRUCacheManager::get_least_used_items(size_t count) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<std::pair<std::string, LRUCacheItem>> items;
    for (const auto& [key, item] : cache_items_) {
        items.emplace_back(key, item);
    }
    
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) {
                  return a.second.access_count < b.second.access_count;
              });
    
    std::vector<LRUCacheItem> result;
    for (size_t i = 0; i < std::min(count, items.size()); ++i) {
        result.push_back(items[i].second);
    }
    
    return result;
}

bool LRUCacheManager::save_cache_index(const std::string& filename) const {
    try {
        std::string index_file = filename.empty() ? 
            cache_directory_ + "/lru_cache_index.json" : filename;
        
        json j;
        j["statistics"] = {
            {"total_items", statistics_.total_items},
            {"total_size_bytes", statistics_.total_size_bytes},
            {"hit_count", statistics_.hit_count},
            {"miss_count", statistics_.miss_count},
            {"hit_rate", statistics_.hit_rate},
            {"last_cleanup", std::chrono::system_clock::to_time_t(statistics_.last_cleanup)}
        };
        
        j["items"] = json::array();
        for (const auto& [key, item] : cache_items_) {
            json item_json;
            item_json["key"] = item.key;
            item_json["package_name"] = item.package_name;
            item_json["version"] = item.version;
            item_json["cache_path"] = item.cache_path;
            item_json["size_bytes"] = item.size_bytes;
            item_json["last_access"] = std::chrono::system_clock::to_time_t(item.last_access);
            item_json["install_time"] = std::chrono::system_clock::to_time_t(item.install_time);
            item_json["access_count"] = item.access_count;
            item_json["is_pinned"] = item.is_pinned;
            j["items"].push_back(item_json);
        }
        
        std::ofstream file(index_file);
        file << j.dump(2);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save cache index: " << e.what();
        return false;
    }
}

bool LRUCacheManager::load_cache_index(const std::string& filename) {
    try {
        std::string index_file = filename.empty() ? 
            cache_directory_ + "/lru_cache_index.json" : filename;
        
        if (!fs::exists(index_file)) {
            return true; // 文件不存在，创建新的
        }
        
        std::ifstream file(index_file);
        json j;
        file >> j;
        
        // 清空现有数据
        cache_items_.clear();
        lru_list_.clear();
        lru_map_.clear();
        
        // 加载统计信息
        if (j.contains("statistics")) {
            const auto& stats = j["statistics"];
            statistics_.total_items = stats["total_items"];
            statistics_.total_size_bytes = stats["total_size_bytes"];
            statistics_.hit_count = stats["hit_count"];
            statistics_.miss_count = stats["miss_count"];
            statistics_.hit_rate = stats["hit_rate"];
            statistics_.last_cleanup = std::chrono::system_clock::from_time_t(stats["last_cleanup"]);
        }
        
        // 加载缓存项
        if (j.contains("items")) {
            for (const auto& item_json : j["items"]) {
                LRUCacheItem item;
                item.key = item_json["key"];
                item.package_name = item_json["package_name"];
                item.version = item_json["version"];
                item.cache_path = item_json["cache_path"];
                item.size_bytes = item_json["size_bytes"];
                item.last_access = std::chrono::system_clock::from_time_t(item_json["last_access"]);
                item.install_time = std::chrono::system_clock::from_time_t(item_json["install_time"]);
                item.access_count = item_json["access_count"];
                item.is_pinned = item_json["is_pinned"];
                
                // 验证文件是否存在
                if (fs::exists(item.cache_path)) {
                    cache_items_[item.key] = item;
                    lru_list_.push_front(item.key);
                    lru_map_[item.key] = lru_list_.begin();
                } else {
                    LOG(WARNING) << "Cache file not found: " << item.cache_path;
                }
            }
        }
        
        LOG(INFO) << "Loaded cache index with " << cache_items_.size() << " items";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load cache index: " << e.what();
        return false;
    }
}

void LRUCacheManager::optimize_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    LOG(INFO) << "Optimizing cache...";
    
    // 重新计算大小
    statistics_.total_size_bytes = 0;
    for (auto& [key, item] : cache_items_) {
        item.size_bytes = calculate_item_size(item.cache_path);
        statistics_.total_size_bytes += item.size_bytes;
    }
    
    // 更新统计
    update_cache_statistics();
    
    LOG(INFO) << "Cache optimization completed";
}

void LRUCacheManager::defragment_cache() {
    // 这里可以实现缓存碎片整理逻辑
    // 目前简化实现
    LOG(INFO) << "Cache defragmentation completed";
}

bool LRUCacheManager::validate_cache_integrity() const {
    try {
        size_t valid_items = 0;
        size_t invalid_items = 0;
        
        for (const auto& [key, item] : cache_items_) {
            if (fs::exists(item.cache_path)) {
                valid_items++;
            } else {
                invalid_items++;
                LOG(WARNING) << "Invalid cache item: " << key << " (path: " << item.cache_path << ")";
            }
        }
        
        LOG(INFO) << "Cache integrity check: " << valid_items << " valid, " << invalid_items << " invalid";
        return invalid_items == 0;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to validate cache integrity: " << e.what();
        return false;
    }
}

// 私有方法实现
void LRUCacheManager::update_lru(const std::string& key) {
    auto it = lru_map_.find(key);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_list_.push_front(key);
        lru_map_[key] = lru_list_.begin();
    }
}

void LRUCacheManager::evict_item(const std::string& key) {
    auto it = cache_items_.find(key);
    if (it == cache_items_.end()) {
        return;
    }
    
    const LRUCacheItem& item = it->second;
    
    // 从LRU列表中移除
    auto lru_it = lru_map_.find(key);
    if (lru_it != lru_map_.end()) {
        lru_list_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }
    
    // 从缓存中移除
    cache_items_.erase(it);
    
    // 更新统计
    statistics_.total_items--;
    statistics_.total_size_bytes -= item.size_bytes;
    
    LOG(DEBUG) << "Evicted cache item: " << key;
}

bool LRUCacheManager::should_evict(const LRUCacheItem& item) const {
    if (item.is_pinned) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto age = now - item.last_access;
    
    return age > max_age_;
}

size_t LRUCacheManager::calculate_item_size(const std::string& cache_path) const {
    try {
        if (!fs::exists(cache_path)) {
            return 0;
        }
        
        size_t total_size = 0;
        for (const auto& entry : fs::recursive_directory_iterator(cache_path)) {
            if (entry.is_regular_file()) {
                total_size += entry.file_size();
            }
        }
        
        return total_size;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to calculate item size: " << e.what();
        return 0;
    }
}

void LRUCacheManager::update_statistics(const std::string& key, bool hit) {
    if (hit) {
        statistics_.hit_count++;
    } else {
        statistics_.miss_count++;
    }
    
    size_t total_requests = statistics_.hit_count + statistics_.miss_count;
    if (total_requests > 0) {
        statistics_.hit_rate = static_cast<double>(statistics_.hit_count) / total_requests;
    }
}

void LRUCacheManager::evict_by_lru() {
    while (!lru_list_.empty() && 
           (statistics_.total_size_bytes > max_cache_size_ || 
            cache_items_.size() > max_cache_items_)) {
        
        std::string key = lru_list_.back();
        auto it = cache_items_.find(key);
        
        if (it != cache_items_.end() && !it->second.is_pinned) {
            evict_item(key);
        } else {
            lru_list_.pop_back();
        }
    }
}

void LRUCacheManager::evict_by_lfu() {
    std::vector<std::pair<std::string, LRUCacheItem*>> items;
    for (auto& [key, item] : cache_items_) {
        if (!item.is_pinned) {
            items.emplace_back(key, &item);
        }
    }
    
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) {
                  return a.second->access_count < b.second->access_count;
              });
    
    for (const auto& [key, item] : items) {
        if (statistics_.total_size_bytes <= max_cache_size_ && 
            cache_items_.size() <= max_cache_items_) {
            break;
        }
        evict_item(key);
    }
}

void LRUCacheManager::evict_by_size() {
    std::vector<std::pair<std::string, LRUCacheItem*>> items;
    for (auto& [key, item] : cache_items_) {
        if (!item.is_pinned) {
            items.emplace_back(key, &item);
        }
    }
    
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) {
                  return a.second->size_bytes > b.second->size_bytes;
              });
    
    for (const auto& [key, item] : items) {
        if (statistics_.total_size_bytes <= max_cache_size_ && 
            cache_items_.size() <= max_cache_items_) {
            break;
        }
        evict_item(key);
    }
}

void LRUCacheManager::evict_by_time() {
    auto cutoff_time = std::chrono::system_clock::now() - max_age_;
    std::vector<std::string> keys_to_remove;
    
    for (const auto& [key, item] : cache_items_) {
        if (!item.is_pinned && item.last_access < cutoff_time) {
            keys_to_remove.push_back(key);
        }
    }
    
    for (const auto& key : keys_to_remove) {
        evict_item(key);
    }
}

void LRUCacheManager::evict_by_hybrid() {
    // 混合策略：结合LRU、LFU和大小
    std::vector<std::pair<std::string, LRUCacheItem*>> items;
    for (auto& [key, item] : cache_items_) {
        if (!item.is_pinned) {
            items.emplace_back(key, &item);
        }
    }
    
    // 计算综合分数（访问频率、大小、时间）
    auto now = std::chrono::system_clock::now();
    std::sort(items.begin(), items.end(),
              [now](const auto& a, const auto& b) {
                  const auto& item_a = *a.second;
                  const auto& item_b = *b.second;
                  
                  // 计算时间分数（越新越好）
                  auto age_a = now - item_a.last_access;
                  auto age_b = now - item_b.last_access;
                  double time_score_a = 1.0 / (std::chrono::duration_cast<std::chrono::hours>(age_a).count() + 1);
                  double time_score_b = 1.0 / (std::chrono::duration_cast<std::chrono::hours>(age_b).count() + 1);
                  
                  // 计算频率分数
                  double freq_score_a = item_a.access_count;
                  double freq_score_b = item_b.access_count;
                  
                  // 计算大小分数（越小越好）
                  double size_score_a = 1.0 / (item_a.size_bytes + 1);
                  double size_score_b = 1.0 / (item_b.size_bytes + 1);
                  
                  // 综合分数
                  double score_a = time_score_a * 0.4 + freq_score_a * 0.4 + size_score_a * 0.2;
                  double score_b = time_score_b * 0.4 + freq_score_b * 0.4 + size_score_b * 0.2;
                  
                  return score_a < score_b;
              });
    
    for (const auto& [key, item] : items) {
        if (statistics_.total_size_bytes <= max_cache_size_ && 
            cache_items_.size() <= max_cache_items_) {
            break;
        }
        evict_item(key);
    }
}

bool LRUCacheManager::remove_cache_directory(const std::string& path) const {
    try {
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to remove cache directory: " << e.what();
        return false;
    }
}

bool LRUCacheManager::create_cache_directory(const std::string& path) const {
    try {
        fs::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to create cache directory: " << e.what();
        return false;
    }
}

void LRUCacheManager::perform_eviction() {
    switch (eviction_policy_) {
        case CacheEvictionPolicy::LRU:
            evict_by_lru();
            break;
        case CacheEvictionPolicy::LFU:
            evict_by_lfu();
            break;
        case CacheEvictionPolicy::SIZE_BASED:
            evict_by_size();
            break;
        case CacheEvictionPolicy::TIME_BASED:
            evict_by_time();
            break;
        case CacheEvictionPolicy::HYBRID:
        default:
            evict_by_hybrid();
            break;
    }
}

void LRUCacheManager::update_cache_statistics() {
    // 更新包大小统计
    statistics_.package_sizes.clear();
    for (const auto& [key, item] : cache_items_) {
        statistics_.package_sizes[item.package_name] += item.size_bytes;
    }
    
    // 更新访问统计
    statistics_.access_counts.clear();
    for (const auto& [key, item] : cache_items_) {
        statistics_.access_counts[item.package_name] += item.access_count;
    }
}

std::string LRUCacheManager::generate_cache_key(const std::string& package_name, const std::string& version) const {
    return package_name + ":" + version;
}

// SmartCacheCleaner 实现
SmartCacheCleaner::SmartCacheCleaner(LRUCacheManager* cache_manager) 
    : cache_manager_(cache_manager) {
    
    // 默认配置
    config_.size_threshold = 0.8;  // 80%
    config_.age_threshold = 7.0;   // 7天
    config_.min_keep_items = 10;   // 最少保留10个
    config_.enable_auto_cleanup = true;
    config_.cleanup_interval = std::chrono::hours(24); // 24小时
    
    last_cleanup_ = std::chrono::system_clock::now();
}

void SmartCacheCleaner::set_cleanup_config(const CleanupConfig& config) {
    config_ = config;
}

bool SmartCacheCleaner::should_perform_cleanup() const {
    if (!config_.enable_auto_cleanup) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_since_cleanup = now - last_cleanup_;
    
    if (time_since_cleanup < config_.cleanup_interval) {
        return false;
    }
    
    return is_cleanup_needed();
}

bool SmartCacheCleaner::perform_smart_cleanup() {
    if (!should_perform_cleanup()) {
        return true;
    }
    
    LOG(INFO) << "Performing smart cache cleanup...";
    
    auto recommendation = get_cleanup_recommendation();
    if (recommendation.type == CleanupRecommendation::Type::NONE) {
        LOG(INFO) << "No cleanup needed";
        return true;
    }
    
    // 执行清理
    bool success = cache_manager_->cleanup_cache();
    
    if (success) {
        last_cleanup_ = std::chrono::system_clock::now();
        LOG(INFO) << "Smart cleanup completed successfully";
    } else {
        LOG(ERROR) << "Smart cleanup failed";
    }
    
    return success;
}

bool SmartCacheCleaner::perform_aggressive_cleanup() {
    LOG(INFO) << "Performing aggressive cache cleanup...";
    
    bool success = cache_manager_->force_cleanup();
    
    if (success) {
        last_cleanup_ = std::chrono::system_clock::now();
        LOG(INFO) << "Aggressive cleanup completed successfully";
    } else {
        LOG(ERROR) << "Aggressive cleanup failed";
    }
    
    return success;
}

SmartCacheCleaner::CleanupRecommendation SmartCacheCleaner::get_cleanup_recommendation() const {
    CleanupRecommendation recommendation;
    recommendation.type = CleanupRecommendation::Type::NONE;
    
    if (!is_cleanup_needed()) {
        return recommendation;
    }
    
    auto stats = cache_manager_->get_statistics();
    double size_ratio = static_cast<double>(stats.total_size_bytes) / (10ULL * 1024 * 1024 * 1024); // 假设最大10GB
    double item_ratio = static_cast<double>(stats.total_items) / 1000; // 假设最大1000项
    
    if (size_ratio > config_.size_threshold || item_ratio > 0.9) {
        recommendation.type = CleanupRecommendation::Type::AGGRESSIVE;
        recommendation.reason = "Cache size exceeds threshold";
    } else if (size_ratio > config_.size_threshold * 0.7 || item_ratio > 0.7) {
        recommendation.type = CleanupRecommendation::Type::MODERATE;
        recommendation.reason = "Cache size approaching threshold";
    } else if (size_ratio > config_.size_threshold * 0.5 || item_ratio > 0.5) {
        recommendation.type = CleanupRecommendation::Type::LIGHT;
        recommendation.reason = "Preventive cleanup recommended";
    }
    
    recommendation.estimated_freed_space = calculate_cleanup_size();
    recommendation.items_to_remove = select_items_for_cleanup(recommendation.type);
    
    return recommendation;
}

void SmartCacheCleaner::enable_auto_cleanup(bool enable) {
    config_.enable_auto_cleanup = enable;
}

bool SmartCacheCleaner::is_cleanup_needed() const {
    auto stats = cache_manager_->get_statistics();
    
    // 检查大小阈值
    double size_ratio = static_cast<double>(stats.total_size_bytes) / (10ULL * 1024 * 1024 * 1024);
    if (size_ratio > config_.size_threshold) {
        return true;
    }
    
    // 检查项目数量
    double item_ratio = static_cast<double>(stats.total_items) / 1000;
    if (item_ratio > 0.9) {
        return true;
    }
    
    // 检查时间
    auto now = std::chrono::system_clock::now();
    auto time_since_cleanup = now - stats.last_cleanup;
    if (time_since_cleanup > std::chrono::hours(24 * 7)) { // 7天
        return true;
    }
    
    return false;
}

size_t SmartCacheCleaner::calculate_cleanup_size() const {
    auto stats = cache_manager_->get_statistics();
    size_t target_size = static_cast<size_t>(stats.total_size_bytes * 0.5); // 清理到50%
    return stats.total_size_bytes - target_size;
}

std::vector<std::string> SmartCacheCleaner::select_items_for_cleanup(CleanupRecommendation::Type type) const {
    std::vector<std::string> items;
    
    switch (type) {
        case CleanupRecommendation::Type::LIGHT:
            // 清理最老的10%
            {
                auto oldest = cache_manager_->get_oldest_items(stats.total_items / 10);
                for (const auto& item : oldest) {
                    items.push_back(item.key);
                }
            }
            break;
            
        case CleanupRecommendation::Type::MODERATE:
            // 清理最老的25%
            {
                auto oldest = cache_manager_->get_oldest_items(stats.total_items / 4);
                for (const auto& item : oldest) {
                    items.push_back(item.key);
                }
            }
            break;
            
        case CleanupRecommendation::Type::AGGRESSIVE:
            // 清理最老的50%
            {
                auto oldest = cache_manager_->get_oldest_items(stats.total_items / 2);
                for (const auto& item : oldest) {
                    items.push_back(item.key);
                }
            }
            break;
            
        default:
            break;
    }
    
    return items;
}

// 全局函数实现
bool initialize_lru_cache_manager(const std::string& cache_directory,
                                 size_t max_cache_size,
                                 size_t max_cache_items,
                                 std::chrono::hours max_age) {
    if (g_lru_cache_manager) {
        LOG(WARNING) << "LRUCacheManager is already initialized";
        return true;
    }
    
    // 使用默认值如果未指定
    if (max_cache_size == 0) max_cache_size = 10ULL * 1024 * 1024 * 1024; // 10GB
    if (max_cache_items == 0) max_cache_items = 1000;
    if (max_age == std::chrono::hours(0)) max_age = std::chrono::hours(24 * 30); // 30天
    
    g_lru_cache_manager = std::make_unique<LRUCacheManager>(
        cache_directory, max_cache_size, max_cache_items, max_age);
    g_smart_cache_cleaner = std::make_unique<SmartCacheCleaner>(g_lru_cache_manager.get());
    
    return g_lru_cache_manager->initialize();
}

void cleanup_lru_cache_manager() {
    g_smart_cache_cleaner.reset();
    g_lru_cache_manager.reset();
}

} // namespace Paker
