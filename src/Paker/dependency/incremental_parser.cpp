#include "Paker/dependency/incremental_parser.h"
#include "Paker/core/utils.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/sources.h"
#include "Paker/simd/simd_hash.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <iomanip>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include <openssl/sha.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

// 全局实例
std::unique_ptr<IncrementalParser> g_incremental_parser = nullptr;

IncrementalParser::IncrementalParser(const std::string& cache_directory)
    : cache_file_path_(cache_directory + "/parse_cache.json"), 
      active_tasks_(0) {
    resolver_ = std::make_unique<DependencyResolver>();
}

IncrementalParser::~IncrementalParser() {
    shutdown();
}

bool IncrementalParser::initialize() {
    LOG(INFO) << "Initializing incremental parser";
    
    // 创建缓存目录
    fs::create_directories(fs::path(cache_file_path_).parent_path());
    
    // 加载缓存
    if (config_.enable_caching) {
        if (!load_cache_from_disk()) {
            LOG(WARNING) << "Failed to load parse cache, starting with empty cache";
        }
    }
    
    // 初始化解析器
    if (!resolver_) {
        resolver_ = std::make_unique<DependencyResolver>();
    }
    
    LOG(INFO) << "Incremental parser initialized successfully";
    return true;
}

void IncrementalParser::shutdown() {
    LOG(INFO) << "Shutting down incremental parser";
    
    // 等待所有并行任务完成
    wait_for_parallel_tasks();
    
    // 保存缓存
    if (config_.enable_caching) {
        save_cache_to_disk();
    }
    
    LOG(INFO) << "Incremental parser shutdown complete";
}

void IncrementalParser::set_config(const ParseConfig& config) {
    config_ = config;
    LOG(INFO) << "Parse configuration updated";
}

ParseConfig IncrementalParser::get_config() const {
    return config_;
}

bool IncrementalParser::parse_package(const std::string& package, const std::string& version) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    LOG(INFO) << "Parsing package: " << package << (version.empty() ? "" : "@" + version);
    
    // 检查缓存
    std::string cache_key = package + "@" + version;
    if (config_.enable_caching) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = parse_cache_.find(cache_key);
        if (it != parse_cache_.end() && is_cache_valid(it->second)) {
            // 缓存命中
            it->second.last_accessed = std::chrono::system_clock::now();
            it->second.access_count++;
            update_cache_stats(true);
            
            LOG(INFO) << "Package " << package << " found in cache";
            return true;
        }
    }
    
    // 缓存未命中，进行解析
    update_cache_stats(false);
    
    bool success = resolver_->resolve_package(package, version);
    if (success && config_.enable_caching) {
        // 更新缓存
        std::lock_guard<std::mutex> lock(cache_mutex_);
        ParseCacheEntry entry;
        entry.package_name = package;
        entry.version = version;
        entry.hash = calculate_package_hash(package, version);
        entry.last_parsed = std::chrono::system_clock::now();
        entry.last_accessed = entry.last_parsed;
        entry.access_count = 1;
        entry.is_valid = true;
        
        // 获取依赖信息
        const auto& graph = resolver_->get_dependency_graph();
        if (graph.has_node(package)) {
            const auto& node = graph.get_node(package);
            // 将 set 转换为 vector
            entry.dependencies.assign(node->dependencies.begin(), node->dependencies.end());
        }
        
        parse_cache_[cache_key] = entry;
        
        // 检查缓存大小限制
        if (parse_cache_.size() > config_.max_cache_size) {
            evict_old_cache_entries();
        }
    }
    
    // 更新统计信息
    auto end_time = std::chrono::high_resolution_clock::now();
    auto parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_packages_parsed++;
        stats_.total_parse_time += parse_time;
        stats_.avg_parse_time = std::chrono::milliseconds(
            stats_.total_parse_time.count() / stats_.total_packages_parsed);
    }
    
    LOG(INFO) << "Package " << package << " parsed in " << parse_time.count() << "ms";
    return success;
}

bool IncrementalParser::parse_packages(const std::vector<std::string>& packages) {
    LOG(INFO) << "Parsing " << packages.size() << " packages";
    
    if (config_.enable_parallel && packages.size() > 1) {
        // 并行解析
        for (const auto& package : packages) {
            if (active_tasks_.load() < config_.max_parallel_tasks) {
                parallel_tasks_.emplace_back(
                    std::async(std::launch::async, 
                              [this, package]() { parse_package_parallel(package, ""); }));
                active_tasks_++;
            } else {
                // 如果并行任务已满，直接解析
                parse_package(package);
            }
        }
        
        // 等待所有并行任务完成
        wait_for_parallel_tasks();
    } else {
        // 串行解析
        for (const auto& package : packages) {
            if (!parse_package(package)) {
                LOG(WARNING) << "Failed to parse package: " << package;
            }
        }
    }
    
    LOG(INFO) << "Finished parsing " << packages.size() << " packages";
    return true;
}

bool IncrementalParser::parse_project_dependencies() {
    LOG(INFO) << "Parsing project dependencies";
    
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Project JSON file not found: " << json_file;
        return false;
    }
    
    try {
        std::ifstream ifs(json_file);
        json j;
        ifs >> j;
        
        std::vector<std::string> packages;
        if (j.contains("dependencies")) {
            for (const auto& [package, version] : j["dependencies"].items()) {
                packages.push_back(package);
            }
        }
        
        return parse_packages(packages);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to parse project JSON: " << e.what();
        return false;
    }
}

bool IncrementalParser::incremental_parse(const std::vector<std::string>& packages) {
    LOG(INFO) << "Starting incremental parse for " << packages.size() << " packages";
    
    // 检测变更
    ChangeDetectionResult changes = detect_changes(packages);
    
    if (!changes.has_changes) {
        LOG(INFO) << "No changes detected, using cached results";
        return true;
    }
    
    LOG(INFO) << "Changes detected: " << changes.changed_packages.size() 
              << " changed, " << changes.new_packages.size() << " new, "
              << changes.removed_packages.size() << " removed";
    
    // 解析变更的包
    std::vector<std::string> packages_to_parse;
    packages_to_parse.insert(packages_to_parse.end(), 
                           changes.changed_packages.begin(), 
                           changes.changed_packages.end());
    packages_to_parse.insert(packages_to_parse.end(), 
                           changes.new_packages.begin(), 
                           changes.new_packages.end());
    
    bool success = parse_packages(packages_to_parse);
    
    // 更新统计
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.incremental_updates++;
    }
    
    return success;
}

ChangeDetectionResult IncrementalParser::detect_changes(const std::vector<std::string>& packages) const {
    ChangeDetectionResult result;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    for (const auto& package : packages) {
        std::string cache_key = package + "@*";  // 检查所有版本
        
        // 检查包是否在缓存中
        bool found_in_cache = false;
        for (const auto& [key, entry] : parse_cache_) {
            if (entry.package_name == package) {
                found_in_cache = true;
                
                // 检查是否有变更
                if (has_package_changed(package, entry.version)) {
                    result.changed_packages.insert(package);
                    result.has_changes = true;
                }
                break;
            }
        }
        
        if (!found_in_cache) {
            result.new_packages.insert(package);
            result.has_changes = true;
        }
    }
    
    return result;
}

std::string IncrementalParser::calculate_dependency_hash(const std::string& package_path) const {
    try {
        std::string manifest_path = package_path + "/paker.json";
        if (!fs::exists(manifest_path)) {
            manifest_path = package_path + "/package.json";
        }
        
        if (!fs::exists(manifest_path)) {
            return "";
        }
        
        // 使用SIMD优化的哈希计算
        return SIMDHashCalculator::sha256_simd_file(manifest_path);
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to calculate dependency hash: " << e.what();
        return "";
    }
}

std::string IncrementalParser::calculate_package_hash(const std::string& package, const std::string& version) const {
    // 使用简单的包名作为路径，实际项目中应该有更复杂的路径解析
    std::string install_path = "packages/" + package;
    return calculate_dependency_hash(install_path);
}

bool IncrementalParser::is_cache_valid(const ParseCacheEntry& entry) const {
    auto now = std::chrono::system_clock::now();
    auto age = now - entry.last_parsed;
    
    return entry.is_valid && (age < config_.cache_ttl);
}

void IncrementalParser::update_cache_entry(ParseCacheEntry& entry) {
    entry.last_accessed = std::chrono::system_clock::now();
    entry.access_count++;
}

void IncrementalParser::evict_old_cache_entries() {
    if (parse_cache_.size() <= config_.max_cache_size) {
        return;
    }
    
    // 按最后访问时间和访问次数排序
    std::vector<std::pair<std::string, ParseCacheEntry*>> entries;
    for (auto& [key, entry] : parse_cache_) {
        entries.emplace_back(key, &entry);
    }
    
    std::sort(entries.begin(), entries.end(), 
              [](const auto& a, const auto& b) {
                  // 优先保留最近访问和访问次数多的条目
                  if (a.second->last_accessed != b.second->last_accessed) {
                      return a.second->last_accessed > b.second->last_accessed;
                  }
                  return a.second->access_count > b.second->access_count;
              });
    
    // 删除最旧的条目
    size_t to_remove = parse_cache_.size() - config_.max_cache_size + 10; // 多删除一些避免频繁清理
    for (size_t i = entries.size() - to_remove; i < entries.size(); ++i) {
        parse_cache_.erase(entries[i].first);
    }
    
    LOG(INFO) << "Evicted " << to_remove << " cache entries";
}

bool IncrementalParser::load_cache_from_disk() {
    try {
        if (!fs::exists(cache_file_path_)) {
            return true; // 文件不存在不算错误
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::ifstream ifs(cache_file_path_);
        json j;
        ifs >> j;
        
        parse_cache_.clear();
        for (const auto& [key, value] : j.items()) {
            ParseCacheEntry entry;
            entry.package_name = value["package_name"];
            entry.version = value["version"];
            entry.hash = value["hash"];
            entry.access_count = value["access_count"];
            entry.is_valid = value["is_valid"];
            
            // 解析依赖列表
            if (value.contains("dependencies")) {
                for (const auto& dep : value["dependencies"]) {
                    entry.dependencies.push_back(dep);
                }
            }
            
            // 解析时间戳
            if (value.contains("last_parsed")) {
                auto timestamp = std::chrono::system_clock::from_time_t(value["last_parsed"]);
                entry.last_parsed = timestamp;
            }
            
            if (value.contains("last_accessed")) {
                auto timestamp = std::chrono::system_clock::from_time_t(value["last_accessed"]);
                entry.last_accessed = timestamp;
            }
            
            parse_cache_[key] = entry;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.cache_load_time = load_time;
        }
        
        LOG(INFO) << "Loaded " << parse_cache_.size() << " cache entries in " 
                  << load_time.count() << "ms";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load cache from disk: " << e.what();
        return false;
    }
}

bool IncrementalParser::save_cache_to_disk() const {
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        json j;
        for (const auto& [key, entry] : parse_cache_) {
            json entry_json;
            entry_json["package_name"] = entry.package_name;
            entry_json["version"] = entry.version;
            entry_json["hash"] = entry.hash;
            entry_json["access_count"] = entry.access_count;
            entry_json["is_valid"] = entry.is_valid;
            entry_json["dependencies"] = entry.dependencies;
            entry_json["last_parsed"] = std::chrono::system_clock::to_time_t(entry.last_parsed);
            entry_json["last_accessed"] = std::chrono::system_clock::to_time_t(entry.last_accessed);
            
            j[key] = entry_json;
        }
        
        std::ofstream ofs(cache_file_path_);
        ofs << j.dump(4);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto save_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            const_cast<ParseStats&>(stats_).cache_save_time = save_time;
        }
        
        LOG(INFO) << "Saved " << parse_cache_.size() << " cache entries in " 
                  << save_time.count() << "ms";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save cache to disk: " << e.what();
        return false;
    }
}

void IncrementalParser::update_cache_stats(bool hit) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (hit) {
        stats_.cache_hits++;
    } else {
        stats_.cache_misses++;
    }
}

bool IncrementalParser::has_package_changed(const std::string& package, const std::string& version) const {
    std::string current_hash = calculate_package_hash(package, version);
    std::string cache_key = package + "@" + version;
    
    auto it = parse_cache_.find(cache_key);
    if (it == parse_cache_.end()) {
        return true; // 不在缓存中，认为有变更
    }
    
    return it->second.hash != current_hash;
}

void IncrementalParser::parse_package_parallel(const std::string& package, const std::string& version) {
    try {
        parse_package(package, version);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Parallel parse failed for " << package << ": " << e.what();
    }
    
    active_tasks_--;
}

void IncrementalParser::wait_for_parallel_tasks() {
    for (auto& task : parallel_tasks_) {
        if (task.valid()) {
            task.wait();
        }
    }
    parallel_tasks_.clear();
    active_tasks_ = 0;
}

void IncrementalParser::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    parse_cache_.clear();
    LOG(INFO) << "Parse cache cleared";
}

void IncrementalParser::invalidate_package_cache(const std::string& package) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = parse_cache_.begin();
    while (it != parse_cache_.end()) {
        if (it->second.package_name == package) {
            it = parse_cache_.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG(INFO) << "Cache invalidated for package: " << package;
}

void IncrementalParser::invalidate_all_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (auto& [key, entry] : parse_cache_) {
        entry.is_valid = false;
    }
    LOG(INFO) << "All cache entries invalidated";
}

size_t IncrementalParser::get_cache_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return parse_cache_.size();
}

ParseStats IncrementalParser::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void IncrementalParser::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = ParseStats();
    LOG(INFO) << "Parse statistics reset";
}

const DependencyGraph& IncrementalParser::get_dependency_graph() const {
    return resolver_->get_dependency_graph();
}

DependencyGraph& IncrementalParser::get_dependency_graph() {
    return resolver_->get_dependency_graph();
}

std::string IncrementalParser::get_cache_info() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::stringstream ss;
    ss << "Cache Info:\n";
    ss << "  Total entries: " << parse_cache_.size() << "\n";
    ss << "  Max size: " << config_.max_cache_size << "\n";
    ss << "  TTL: " << config_.cache_ttl.count() << " minutes\n";
    
    // 统计有效条目
    size_t valid_entries = 0;
    for (const auto& [key, entry] : parse_cache_) {
        if (entry.is_valid) {
            valid_entries++;
        }
    }
    ss << "  Valid entries: " << valid_entries << "\n";
    
    return ss.str();
}

std::string IncrementalParser::get_performance_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Performance Report:\n";
    ss << "  Total packages parsed: " << stats_.total_packages_parsed << "\n";
    ss << "  Cache hits: " << stats_.cache_hits << "\n";
    ss << "  Cache misses: " << stats_.cache_misses << "\n";
    ss << "  Cache hit rate: " << (stats_.cache_hits + stats_.cache_misses > 0 ? 
        (double)stats_.cache_hits / (stats_.cache_hits + stats_.cache_misses) * 100 : 0) << "%\n";
    ss << "  Incremental updates: " << stats_.incremental_updates << "\n";
    ss << "  Full parses: " << stats_.full_parses << "\n";
    ss << "  Average parse time: " << stats_.avg_parse_time.count() << "ms\n";
    ss << "  Total parse time: " << stats_.total_parse_time.count() << "ms\n";
    ss << "  Cache load time: " << stats_.cache_load_time.count() << "ms\n";
    ss << "  Cache save time: " << stats_.cache_save_time.count() << "ms\n";
    
    return ss.str();
}

bool IncrementalParser::validate_cache_integrity() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    size_t invalid_entries = 0;
    for (const auto& [key, entry] : parse_cache_) {
        if (!entry.is_valid) {
            invalid_entries++;
        }
    }
    
    LOG(INFO) << "Cache integrity check: " << invalid_entries 
              << " invalid entries out of " << parse_cache_.size();
    
    return invalid_entries == 0;
}

void IncrementalParser::optimize_cache() {
    LOG(INFO) << "Optimizing cache";
    
    // 清理过期条目
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = parse_cache_.begin();
    while (it != parse_cache_.end()) {
        if (!is_cache_valid(it->second)) {
            it = parse_cache_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 如果缓存仍然过大，执行LRU清理
    if (parse_cache_.size() > config_.max_cache_size) {
        evict_old_cache_entries();
    }
    
    LOG(INFO) << "Cache optimization completed";
}

void IncrementalParser::preload_common_dependencies() {
    LOG(INFO) << "Preloading common dependencies";
    
    // 预加载一些常见的依赖包
    std::vector<std::string> common_packages = {
        "fmt", "spdlog", "nlohmann-json", "glog", "openssl"
    };
    
    for (const auto& package : common_packages) {
        try {
            parse_package(package);
        } catch (const std::exception& e) {
            LOG(WARNING) << "Failed to preload package " << package << ": " << e.what();
        }
    }
    
    LOG(INFO) << "Common dependencies preloading completed";
}

// 全局函数实现
bool initialize_incremental_parser(const std::string& cache_directory) {
    if (g_incremental_parser) {
        LOG(WARNING) << "Incremental parser already initialized";
        return true;
    }
    
    g_incremental_parser = std::make_unique<IncrementalParser>(cache_directory);
    return g_incremental_parser->initialize();
}

void cleanup_incremental_parser() {
    if (g_incremental_parser) {
        g_incremental_parser->shutdown();
        g_incremental_parser.reset();
    }
}

IncrementalParser* get_incremental_parser() {
    return g_incremental_parser.get();
}

} // namespace Paker
