#include "Paker/cache/cache_warmup.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/core/service_container.h"
#include "Paker/core/core_services.h"
#include "Paker/core/utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <chrono>
#include <thread>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

// 全局预热服务实例
std::unique_ptr<CacheWarmupService> g_cache_warmup_service;

CacheWarmupService::CacheWarmupService()
    : max_concurrent_preloads_(4)
    , max_preload_size_(1024ULL * 1024 * 1024)  // 1GB
    , preload_timeout_(std::chrono::seconds(300))  // 5分钟
    , default_strategy_(WarmupStrategy::ASYNC)
    , is_preloading_(false)
    , should_stop_(false)
    , current_preload_count_(0)
    , total_preload_count_(0)
    , cache_manager_(nullptr)
    , dependency_resolver_(nullptr) {
}

CacheWarmupService::~CacheWarmupService() {
    stop_preload();
}

bool CacheWarmupService::initialize() {
    try {
        // 获取依赖服务
        cache_manager_ = get_cache_manager();
        dependency_resolver_ = get_dependency_resolver();
        
        if (!cache_manager_) {
            LOG(ERROR) << "Cache manager not available for warmup service";
            return false;
        }
        
        // 加载默认配置
        load_default_config();
        
        // 分析使用模式
        analyze_usage_patterns();
        
        // 更新流行度分数
        update_popularity_scores();
        
        // 优化预热顺序
        optimize_preload_order();
        
        LOG(INFO) << "Cache warmup service initialized successfully";
        LOG(INFO) << "Max concurrent preloads: " << max_concurrent_preloads_;
        LOG(INFO) << "Max preload size: " << (max_preload_size_ / (1024 * 1024)) << " MB";
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize cache warmup service: " << e.what();
        return false;
    }
}

void CacheWarmupService::shutdown() {
    try {
        stop_preload();
        
        // 保存配置
        save_preload_config(".paker/warmup_config.json");
        
        LOG(INFO) << "Cache warmup service shut down successfully";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error during cache warmup service shutdown: " << e.what();
    }
}

bool CacheWarmupService::register_package(const std::string& package, const std::string& version,
                                        const std::string& repository_url, WarmupPriority priority) {
    try {
        std::lock_guard<std::mutex> lock(preload_mutex_);
        
        PackageWarmupInfo info;
        info.package_name = package;
        info.version = version;
        info.repository_url = repository_url;
        info.priority = priority;
        info.is_essential = is_package_essential(package);
        info.popularity_score = calculate_popularity_score(info);
        
        // 生成唯一键
        std::string key = package + "@" + version;
        
        // 检查是否已存在
        if (package_registry_.find(key) != package_registry_.end()) {
            LOG(WARNING) << "Package " << key << " already registered for warmup";
            return false;
        }
        
        package_registry_[key] = info;
        packages_to_preload_.push_back(info);
        
        // 重建优先级队列
        rebuild_priority_queues();
        
        LOG(INFO) << "Package registered for warmup: " << key << " (priority: " << static_cast<int>(priority) << ")";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to register package for warmup: " << e.what();
        return false;
    }
}

bool CacheWarmupService::unregister_package(const std::string& package, const std::string& version) {
    try {
        std::lock_guard<std::mutex> lock(preload_mutex_);
        
        std::string key = package + "@" + (version.empty() ? "*" : version);
        
        if (version.empty()) {
            // 移除所有版本
            auto it = package_registry_.begin();
            while (it != package_registry_.end()) {
                if (it->second.package_name == package) {
                    it = package_registry_.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            // 移除特定版本
            package_registry_.erase(key);
        }
        
        // 从预加载列表中移除
        packages_to_preload_.erase(
            std::remove_if(packages_to_preload_.begin(), packages_to_preload_.end(),
                [&](const PackageWarmupInfo& info) {
                    return info.package_name == package && 
                           (version.empty() || info.version == version);
                }),
            packages_to_preload_.end()
        );
        
        // 重建优先级队列
        rebuild_priority_queues();
        
        LOG(INFO) << "Package unregistered from warmup: " << key;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to unregister package from warmup: " << e.what();
        return false;
    }
}

bool CacheWarmupService::start_preload(WarmupStrategy strategy) {
    try {
        if (is_preloading_.load()) {
            LOG(WARNING) << "Preload already in progress";
            return false;
        }
        
        if (packages_to_preload_.empty()) {
            LOG(INFO) << "No packages to preload";
            return true;
        }
        
        // 设置预热参数
        is_preloading_.store(true);
        should_stop_.store(false);
        current_preload_count_.store(0);
        total_preload_count_.store(packages_to_preload_.size());
        
        // 更新统计信息
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_packages = packages_to_preload_.size();
            stats_.preloaded_packages = 0;
            stats_.failed_packages = 0;
            stats_.skipped_packages = 0;
            stats_.total_time = std::chrono::milliseconds(0);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (strategy == WarmupStrategy::IMMEDIATE) {
            // 同步预热
            LOG(INFO) << "Starting immediate preload of " << packages_to_preload_.size() << " packages";
            
            for (const auto& package : packages_to_preload_) {
                if (should_stop_.load()) break;
                
                bool success = preload_single_package(package);
                update_preload_progress(package.package_name, package.version, success);
            }
            
        } else {
            // 异步预热
            LOG(INFO) << "Starting async preload of " << packages_to_preload_.size() << " packages";
            
            // 启动工作线程
            preload_threads_.clear();
            for (size_t i = 0; i < max_concurrent_preloads_; ++i) {
                preload_threads_.emplace_back(&CacheWarmupService::preload_worker_thread, this);
            }
            
            // 等待所有线程完成
            for (auto& thread : preload_threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 更新统计信息
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_time = duration;
            if (stats_.total_packages > 0) {
                stats_.average_time_per_package = std::chrono::milliseconds(
                    duration.count() / stats_.total_packages
                );
                stats_.success_rate = static_cast<double>(stats_.preloaded_packages) / stats_.total_packages;
            }
        }
        
        is_preloading_.store(false);
        
        LOG(INFO) << "Preload completed in " << duration.count() << "ms";
        LOG(INFO) << "Success rate: " << (stats_.success_rate * 100) << "%";
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to start preload: " << e.what();
        is_preloading_.store(false);
        return false;
    }
}

bool CacheWarmupService::stop_preload() {
    try {
        if (!is_preloading_.load()) {
            return true;
        }
        
        should_stop_.store(true);
        
        // 等待所有线程完成
        for (auto& thread : preload_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        preload_threads_.clear();
        is_preloading_.store(false);
        
        LOG(INFO) << "Preload stopped";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to stop preload: " << e.what();
        return false;
    }
}

bool CacheWarmupService::start_smart_preload(const std::vector<std::string>& project_dependencies) {
    try {
        LOG(INFO) << "Starting smart preload analysis";
        
        // 分析项目依赖
        std::vector<std::string> dependencies = project_dependencies;
        if (dependencies.empty()) {
            dependencies = analyze_project_dependencies(".paker/paker.json");
        }
        
        // 注册项目依赖包
        for (const auto& dep : dependencies) {
            register_package(dep, "latest", "", WarmupPriority::CRITICAL);
        }
        
        // 注册流行包
        preload_popular_packages(20);
        
        // 注册系统核心包
        preload_essential_packages();
        
        // 开始预热
        return start_preload(WarmupStrategy::ASYNC);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to start smart preload: " << e.what();
        return false;
    }
}

bool CacheWarmupService::preload_essential_packages() {
    try {
        std::vector<std::string> essential_packages = {
            "glog", "nlohmann-json", "CLI11", "OpenSSL", "zlib", "boost"
        };
        
        for (const auto& package : essential_packages) {
            register_package(package, "latest", "", WarmupPriority::CRITICAL);
        }
        
        LOG(INFO) << "Essential packages registered for preload";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to preload essential packages: " << e.what();
        return false;
    }
}

bool CacheWarmupService::preload_popular_packages(size_t count) {
    try {
        // 这里应该从使用统计中获取流行包
        // 暂时使用一些常见的包作为示例
        std::vector<std::string> popular_packages = {
            "fmt", "spdlog", "catch2", "gtest", "benchmark", "eigen", "opencv"
        };
        
        size_t registered = 0;
        for (const auto& package : popular_packages) {
            if (registered >= count) break;
            
            if (register_package(package, "latest", "", WarmupPriority::HIGH)) {
                registered++;
            }
        }
        
        LOG(INFO) << "Popular packages registered for preload: " << registered;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to preload popular packages: " << e.what();
        return false;
    }
}

double CacheWarmupService::get_progress_percentage() const {
    size_t current = current_preload_count_.load();
    size_t total = total_preload_count_.load();
    
    if (total == 0) return 100.0;
    return static_cast<double>(current) / total * 100.0;
}

WarmupStats CacheWarmupService::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // 如果统计信息为空，尝试从已安装的包中获取
    if (stats_.total_packages == 0) {
        WarmupStats current_stats = stats_;
        
        // 扫描已安装的包
        fs::path packages_dir = "packages";
        if (fs::exists(packages_dir) && fs::is_directory(packages_dir)) {
            size_t installed_count = 0;
            for (const auto& entry : fs::directory_iterator(packages_dir)) {
                if (entry.is_directory()) {
                    installed_count++;
                }
            }
            current_stats.total_packages = installed_count;
        }
        
        return current_stats;
    }
    
    return stats_;
}

void CacheWarmupService::preload_worker_thread() {
    try {
        while (!should_stop_.load()) {
            PackageWarmupInfo package_info;
            bool found_package = false;
            
            // 获取下一个要预热的包
            {
                std::lock_guard<std::mutex> lock(preload_mutex_);
                for (auto& package : packages_to_preload_) {
                    if (!package.is_preloaded) {
                        package_info = package;
                        package.is_preloaded = true;
                        found_package = true;
                        break;
                    }
                }
            }
            
            if (!found_package) {
                break;  // 没有更多包需要预热
            }
            
            // 预热单个包
            bool success = preload_single_package(package_info);
            update_preload_progress(package_info.package_name, package_info.version, success);
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in preload worker thread: " << e.what();
    }
}

bool CacheWarmupService::preload_single_package(const PackageWarmupInfo& package_info) {
    try {
        // 检查资源限制
        if (!check_preload_resources(package_info)) {
            LOG(WARNING) << "Insufficient resources for preloading " << package_info.package_name;
            return false;
        }
        
        // 检查是否已缓存
        if (cache_manager_->is_package_cached(package_info.package_name, package_info.version)) {
            LOG(INFO) << "Package already cached: " << package_info.package_name << "@" << package_info.version;
            return true;
        }
        
        // 安装到缓存
        bool success = cache_manager_->install_package_to_cache(
            package_info.package_name,
            package_info.version,
            package_info.repository_url
        );
        
        if (success) {
            LOG(INFO) << "Successfully preloaded: " << package_info.package_name << "@" << package_info.version;
        } else {
            LOG(WARNING) << "Failed to preload: " << package_info.package_name << "@" << package_info.version;
        }
        
        return success;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error preloading package " << package_info.package_name << ": " << e.what();
        return false;
    }
}

void CacheWarmupService::update_preload_progress(const std::string& package, const std::string& version, bool success) {
    current_preload_count_.fetch_add(1);
    
    // 更新统计信息
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (success) {
            stats_.preloaded_packages++;
        } else {
            stats_.failed_packages++;
        }
    }
    
    // 调用进度回调
    if (progress_callback_) {
        progress_callback_(package, version, current_preload_count_.load(), total_preload_count_.load(), success);
    }
}

void CacheWarmupService::rebuild_priority_queues() {
    priority_queues_.clear();
    
    for (const auto& package : packages_to_preload_) {
        priority_queues_[package.priority].push_back(package.package_name + "@" + package.version);
    }
}

bool CacheWarmupService::analyze_usage_patterns(const std::string& project_path) {
    try {
        // 分析项目配置文件
        std::string config_file = project_path.empty() ? ".paker/paker.json" : project_path + "/.paker/paker.json";
        
        if (!fs::exists(config_file)) {
            LOG(WARNING) << "Project config not found: " << config_file;
            return false;
        }
        
        std::ifstream file(config_file);
        json config;
        file >> config;
        
        // 分析依赖使用模式
        if (config.contains("dependencies")) {
            for (const auto& [package, version] : config["dependencies"].items()) {
                register_package(package, version.get<std::string>(), "", WarmupPriority::HIGH);
            }
        }
        
        LOG(INFO) << "Usage patterns analyzed from: " << config_file;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to analyze usage patterns: " << e.what();
        return false;
    }
}

bool CacheWarmupService::update_popularity_scores() {
    try {
        for (auto& package : packages_to_preload_) {
            package.popularity_score = calculate_popularity_score(package);
        }
        
        LOG(INFO) << "Popularity scores updated";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to update popularity scores: " << e.what();
        return false;
    }
}

bool CacheWarmupService::optimize_preload_order() {
    try {
        // 按优先级和流行度排序
        std::sort(packages_to_preload_.begin(), packages_to_preload_.end(),
            [](const PackageWarmupInfo& a, const PackageWarmupInfo& b) {
                if (a.priority != b.priority) {
                    return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                }
                return a.popularity_score > b.popularity_score;
            });
        
        LOG(INFO) << "Preload order optimized";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to optimize preload order: " << e.what();
        return false;
    }
}

double CacheWarmupService::calculate_popularity_score(const PackageWarmupInfo& package) const {
    double score = 0.0;
    
    // 基于访问频率
    score += package.access_frequency * 0.4;
    
    // 基于是否为核心包
    if (package.is_essential) {
        score += 10.0;
    }
    
    // 基于包大小（较小的包优先）
    if (package.estimated_size > 0) {
        score += 1000.0 / (package.estimated_size / (1024 * 1024));  // MB
    }
    
    return score;
}

bool CacheWarmupService::is_package_essential(const std::string& package) const {
    std::vector<std::string> essential_packages = {
        "glog", "nlohmann-json", "CLI11", "OpenSSL", "zlib", "boost"
    };
    
    return std::find(essential_packages.begin(), essential_packages.end(), package) != essential_packages.end();
}

std::vector<std::string> CacheWarmupService::analyze_project_dependencies(const std::string& project_path) const {
    std::vector<std::string> dependencies;
    
    try {
        if (!fs::exists(project_path)) {
            return dependencies;
        }
        
        std::ifstream file(project_path);
        json config;
        file >> config;
        
        if (config.contains("dependencies")) {
            for (const auto& [package, version] : config["dependencies"].items()) {
                dependencies.push_back(package);
            }
        }
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to analyze project dependencies: " << e.what();
    }
    
    return dependencies;
}

void CacheWarmupService::scan_installed_packages_for_warmup() {
    try {
        fs::path packages_dir = "packages";
        if (!fs::exists(packages_dir) || !fs::is_directory(packages_dir)) {
            LOG(INFO) << "No packages directory found for warmup scan";
            return;
        }
        
        LOG(INFO) << "Scanning installed packages for warmup in " << packages_dir.string();
        
        packages_to_preload_.clear();
        package_registry_.clear();
        
        for (const auto& entry : fs::directory_iterator(packages_dir)) {
            if (entry.is_directory()) {
                std::string package_name = entry.path().filename().string();
                std::string version = "unknown";
                
                // 获取Git版本信息
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            version = head_line.substr(head_line.find_last_of('/') + 1);
                        } else {
                            version = head_line.substr(0, 8);
                        }
                    }
                }
                
                // 创建预热信息
                PackageWarmupInfo info;
                info.package_name = package_name;
                info.version = version;
                info.repository_url = ""; // 从packages目录安装的包没有仓库URL
                info.priority = WarmupPriority::NORMAL; // 默认优先级
                info.is_essential = false; // 默认非必需
                info.is_preloaded = false;
                info.popularity_score = 0.0;
                info.estimated_size = 0; // 可以后续计算
                
                packages_to_preload_.push_back(info);
                package_registry_[package_name + "@" + version] = info;
                
                LOG(INFO) << "Scanned package for warmup: " << package_name << "@" << version;
            }
        }
        
        rebuild_priority_queues();
        
        LOG(INFO) << "Scanned " << packages_to_preload_.size() << " packages for warmup";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error scanning installed packages for warmup: " << e.what();
    }
}

std::vector<PackageWarmupInfo> CacheWarmupService::get_preload_queue() const {
    std::lock_guard<std::mutex> lock(preload_mutex_);
    return packages_to_preload_;
}

std::vector<PackageWarmupInfo> CacheWarmupService::get_preloaded_packages() const {
    std::lock_guard<std::mutex> lock(preload_mutex_);
    std::vector<PackageWarmupInfo> preloaded;
    for (const auto& package : packages_to_preload_) {
        if (package.is_preloaded) {
            preloaded.push_back(package);
        }
    }
    return preloaded;
}

bool CacheWarmupService::check_preload_resources(const PackageWarmupInfo& package) const {
    // 检查包大小限制
    if (package.estimated_size > max_preload_size_) {
        return false;
    }
    
    // 检查并发限制
    if (current_preload_count_.load() >= max_concurrent_preloads_) {
        return false;
    }
    
    return true;
}

bool CacheWarmupService::load_default_config() {
    try {
        // 设置默认配置
        max_concurrent_preloads_ = 4;
        max_preload_size_ = 1024ULL * 1024 * 1024;  // 1GB
        preload_timeout_ = std::chrono::seconds(300);  // 5分钟
        default_strategy_ = WarmupStrategy::ASYNC;
        
        LOG(INFO) << "Default warmup configuration loaded";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load default config: " << e.what();
        return false;
    }
}

bool CacheWarmupService::save_preload_config(const std::string& config_path) const {
    try {
        json config;
        config["max_concurrent_preloads"] = max_concurrent_preloads_;
        config["max_preload_size_mb"] = max_preload_size_ / (1024 * 1024);
        config["preload_timeout_seconds"] = preload_timeout_.count();
        config["default_strategy"] = static_cast<int>(default_strategy_);
        
        // 保存包注册信息
        json packages = json::array();
        for (const auto& package : packages_to_preload_) {
            json pkg;
            pkg["package_name"] = package.package_name;
            pkg["version"] = package.version;
            pkg["repository_url"] = package.repository_url;
            pkg["priority"] = static_cast<int>(package.priority);
            pkg["is_essential"] = package.is_essential;
            packages.push_back(pkg);
        }
        config["packages"] = packages;
        
        // 创建目录
        fs::create_directories(fs::path(config_path).parent_path());
        
        // 保存文件
        std::ofstream file(config_path);
        file << config.dump(4);
        
        LOG(INFO) << "Preload configuration saved to: " << config_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save preload config: " << e.what();
        return false;
    }
}

bool CacheWarmupService::load_preload_config(const std::string& config_path) {
    try {
        if (!fs::exists(config_path)) {
            LOG(WARNING) << "Preload config not found: " << config_path;
            return false;
        }
        
        std::ifstream file(config_path);
        json config;
        file >> config;
        
        // 加载配置
        if (config.contains("max_concurrent_preloads")) {
            max_concurrent_preloads_ = config["max_concurrent_preloads"];
        }
        if (config.contains("max_preload_size_mb")) {
            max_preload_size_ = config["max_preload_size_mb"].get<size_t>() * 1024 * 1024;
        }
        if (config.contains("preload_timeout_seconds")) {
            preload_timeout_ = std::chrono::seconds(config["preload_timeout_seconds"]);
        }
        if (config.contains("default_strategy")) {
            default_strategy_ = static_cast<WarmupStrategy>(config["default_strategy"]);
        }
        
        // 加载包信息
        if (config.contains("packages")) {
            packages_to_preload_.clear();
            package_registry_.clear();
            
            for (const auto& pkg : config["packages"]) {
                PackageWarmupInfo info;
                info.package_name = pkg["package_name"];
                info.version = pkg["version"];
                info.repository_url = pkg["repository_url"];
                info.priority = static_cast<WarmupPriority>(pkg["priority"]);
                info.is_essential = pkg.value("is_essential", false);
                
                packages_to_preload_.push_back(info);
                package_registry_[info.package_name + "@" + info.version] = info;
            }
            
            rebuild_priority_queues();
        } else {
            // 如果没有配置文件，扫描已安装的包
            scan_installed_packages_for_warmup();
        }
        
        LOG(INFO) << "Preload configuration loaded from: " << config_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load preload config: " << e.what();
        return false;
    }
}

// 全局函数实现
bool initialize_cache_warmup_service() {
    try {
        if (g_cache_warmup_service) {
            LOG(WARNING) << "Cache warmup service already initialized";
            return true;
        }
        
        g_cache_warmup_service = std::make_unique<CacheWarmupService>();
        g_cache_warmup_service->initialize();
        
        LOG(INFO) << "Cache warmup service initialized globally";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize cache warmup service: " << e.what();
        return false;
    }
}

void cleanup_cache_warmup_service() {
    try {
        if (g_cache_warmup_service) {
            g_cache_warmup_service->shutdown();
            g_cache_warmup_service.reset();
        }
        
        LOG(INFO) << "Cache warmup service cleaned up";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error during cache warmup service cleanup: " << e.what();
    }
}

// 这个函数已经在core_services.h中声明，不需要在这里重复定义

} // namespace Paker
