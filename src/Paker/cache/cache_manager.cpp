#include "Paker/cache/cache_manager.h"
#include "Paker/cache/cache_path_resolver.h"
#include "Paker/core/output.h"
#include "Paker/core/utils.h"
#include "Paker/core/memory_pool.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <zlib.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

// 全局缓存管理器实例
std::unique_ptr<CacheManager> g_cache_manager;

CacheManager::CacheManager() 
    : strategy_(CacheStrategy::HYBRID)  // 默认使用混合模式
    , version_storage_(VersionStorage::SHALLOW_CLONE)
    , max_cache_size_(10ULL * 1024 * 1024 * 1024)  // 10GB
    , max_versions_per_package_(3)
    , cleanup_interval_(std::chrono::hours(24 * 7))  // 7天
    , memory_pool_(std::make_unique<SmartMemoryPool>(256 * 1024 * 1024))  // 256MB内存池
    , compression_enabled_(true)
    , preallocation_enabled_(true)
    , initialized_(false) {
    
    // 初始化内存池
    if (memory_pool_) {
        memory_pool_->initialize();
        memory_pool_->enable_preallocation(true);
    }
}

CacheManager::~CacheManager() {
    save_cache_index();
}

bool CacheManager::initialize(const std::string& config_path) {
    try {
        // 设置默认缓存路径
        const char* home_dir = std::getenv("HOME");
        if (home_dir) {
            user_cache_path_ = std::string(home_dir) + "/.paker/cache";
        } else {
            user_cache_path_ = "./.paker/cache";
        }
        
        global_cache_path_ = "/usr/local/share/paker/cache";
        project_cache_path_ = ".paker/cache";
        
        // 尝试创建全局缓存目录（可能因权限问题失败）
        try {
            fs::create_directories(global_cache_path_);
            LOG(INFO) << "Global cache directory created: " << global_cache_path_;
        } catch (const std::exception& e) {
            LOG(WARNING) << "Failed to create global cache directory (permission denied), will use user cache only";
            global_cache_path_ = "";  // 禁用全局缓存
        }
        
        // 创建用户和项目缓存目录（这些应该总是成功的）
        try {
            fs::create_directories(user_cache_path_);
            fs::create_directories(project_cache_path_);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to create cache directories: " << e.what();
            return false;
        }
        
        // 加载配置
        if (!config_path.empty()) {
            load_configuration(config_path);
        }
        
        // 加载缓存索引
        if (!load_cache_index()) {
            LOG(WARNING) << "Failed to load cache index, creating new one";
        }
        
        // 扫描已安装的包并添加到缓存索引
        scan_installed_packages();
        
        // 标记为已初始化
        initialized_ = true;
        
        LOG(INFO) << "Cache manager initialized successfully";
        if (!global_cache_path_.empty()) {
            LOG(INFO) << "Global cache: " << global_cache_path_;
        }
        LOG(INFO) << "User cache: " << user_cache_path_;
        LOG(INFO) << "Project cache: " << project_cache_path_;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize cache manager: " << e.what();
        initialized_ = false;
        return false;
    }
}

bool CacheManager::install_package_to_cache(const std::string& package, const std::string& version, 
                                          const std::string& repository_url) {
    try {
        // 检查是否已缓存
        if (is_package_cached(package, version)) {
            LOG(INFO) << "Package " << package << "@" << version << " already cached";
            return true;
        }
        
        // 确定缓存路径
        std::string cache_path = resolve_cache_path(package, version);
        fs::path pkg_cache_dir(cache_path);
        
        // 创建包目录
        fs::create_directories(pkg_cache_dir.parent_path());
        
        // 根据版本存储策略选择安装方法
        bool success = false;
        switch (version_storage_) {
            case VersionStorage::SHALLOW_CLONE:
                success = install_shallow_clone(repository_url, pkg_cache_dir.string(), version);
                break;
            case VersionStorage::ARCHIVE_ONLY:
                success = install_archive_only(repository_url, pkg_cache_dir.string(), version);
                break;
            case VersionStorage::COMPRESSED:
                success = install_compressed(repository_url, pkg_cache_dir.string(), version);
                break;
            default:
                success = install_shallow_clone(repository_url, pkg_cache_dir.string(), version);
                break;
        }
        
        if (!success) {
            LOG(ERROR) << "Failed to install package " << package << "@" << version << " to cache";
            return false;
        }
        
        // 更新缓存索引
        PackageCacheInfo info;
        info.package_name = package;
        info.version = version;
        info.cache_path = cache_path;
        info.repository_url = repository_url;
        info.install_time = std::chrono::system_clock::now();
        info.last_access = std::chrono::system_clock::now();
        info.size_bytes = calculate_directory_size(cache_path);
        info.access_count = 1;
        info.is_active = true;
        
        package_index_[package][version] = info;
        
        // 保存索引
        save_cache_index();
        
        LOG(INFO) << "Successfully installed " << package << "@" << version << " to cache";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error installing package to cache: " << e.what();
        return false;
    }
}

bool CacheManager::is_package_cached(const std::string& package, const std::string& version) const {
    auto pkg_it = package_index_.find(package);
    if (pkg_it == package_index_.end()) {
        return false;
    }
    
    if (version.empty()) {
        // 检查是否有任何版本
        return !pkg_it->second.empty();
    }
    
    auto ver_it = pkg_it->second.find(version);
    if (ver_it == pkg_it->second.end()) {
        return false;
    }
    
    // 检查文件是否实际存在
    return fs::exists(ver_it->second.cache_path);
}

std::string CacheManager::get_cached_package_path(const std::string& package, const std::string& version) const {
    auto pkg_it = package_index_.find(package);
    if (pkg_it == package_index_.end()) {
        return "";
    }
    
    if (version.empty()) {
        // 返回最新版本
        if (pkg_it->second.empty()) {
            return "";
        }
        return pkg_it->second.rbegin()->second.cache_path;
    }
    
    auto ver_it = pkg_it->second.find(version);
    if (ver_it == pkg_it->second.end()) {
        return "";
    }
    
    return ver_it->second.cache_path;
}

bool CacheManager::create_project_link(const std::string& package, const std::string& version, 
                                     const std::string& project_path) {
    try {
        // 获取缓存的包路径
        std::string cached_path = get_cached_package_path(package, version);
        if (cached_path.empty()) {
            LOG(ERROR) << "Package " << package << "@" << version << " not found in cache";
            return false;
        }
        
        // 创建项目链接目录
        fs::path project_links_dir = fs::path(project_path) / ".paker" / "links";
        fs::create_directories(project_links_dir);
        
        // 创建符号链接
        fs::path link_path = project_links_dir / package;
        if (fs::exists(link_path)) {
            fs::remove(link_path);
        }
        
        return create_symbolic_link(cached_path, link_path.string());
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error creating project link: " << e.what();
        return false;
    }
}

bool CacheManager::remove_project_link(const std::string& package, const std::string& project_path) {
    try {
        fs::path link_path = fs::path(project_path) / ".paker" / "links" / package;
        if (fs::exists(link_path)) {
            return remove_symbolic_link(link_path.string());
        }
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error removing project link: " << e.what();
        return false;
    }
}

std::string CacheManager::get_project_package_path(const std::string& package, const std::string& project_path) const {
    fs::path link_path = fs::path(project_path) / ".paker" / "links" / package;
    if (fs::exists(link_path) && fs::is_symlink(link_path)) {
        return fs::read_symlink(link_path).string();
    }
    return "";
}

bool CacheManager::cleanup_unused_packages() {
    try {
        std::vector<std::string> packages_to_remove;
        
        for (const auto& [package, versions] : package_index_) {
            for (const auto& [version, info] : versions) {
                // 检查是否长时间未使用
                auto now = std::chrono::system_clock::now();
                auto days_since_access = std::chrono::duration_cast<std::chrono::hours>(
                    now - info.last_access).count() / 24;
                
                if (days_since_access > 30 && info.access_count < 5) {  // 30天未使用且访问次数少于5次
                    packages_to_remove.push_back(package + "@" + version);
                }
            }
        }
        
        for (const auto& pkg_ver : packages_to_remove) {
            auto pos = pkg_ver.find('@');
            std::string package = pkg_ver.substr(0, pos);
            std::string version = pkg_ver.substr(pos + 1);
            remove_package_from_cache(package, version);
        }
        
        LOG(INFO) << "Cleaned up " << packages_to_remove.size() << " unused packages";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error cleaning up unused packages: " << e.what();
        return false;
    }
}

bool CacheManager::cleanup_old_versions() {
    try {
        for (auto& [package, versions] : package_index_) {
            if (versions.size() <= max_versions_per_package_) {
                continue;
            }
            
            // 按安装时间排序，保留最新的版本
            std::vector<std::pair<std::string, PackageCacheInfo>> sorted_versions;
            for (const auto& [version, info] : versions) {
                sorted_versions.emplace_back(version, info);
            }
            
            std::sort(sorted_versions.begin(), sorted_versions.end(),
                     [](const auto& a, const auto& b) {
                         return a.second.install_time > b.second.install_time;
                     });
            
            // 删除旧版本
            for (size_t i = max_versions_per_package_; i < sorted_versions.size(); ++i) {
                remove_package_from_cache(package, sorted_versions[i].first);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error cleaning up old versions: " << e.what();
        return false;
    }
}

CacheStats CacheManager::get_cache_statistics() const {
    CacheStats stats;
    
    for (const auto& [package, versions] : package_index_) {
        stats.total_packages += versions.size();
        
        for (const auto& [version, info] : versions) {
            stats.total_size_bytes += info.size_bytes;
            
            // 检查是否未使用
            auto now = std::chrono::system_clock::now();
            auto days_since_access = std::chrono::duration_cast<std::chrono::hours>(
                now - info.last_access).count() / 24;
            
            if (days_since_access > 30 && info.access_count < 5) {
                stats.unused_packages++;
            }
        }
    }
    
    return stats;
}

bool CacheManager::migrate_from_legacy_mode(const std::string& project_path) {
    try {
        fs::path legacy_packages_dir = fs::path(project_path) / "packages";
        if (!fs::exists(legacy_packages_dir)) {
            LOG(INFO) << "No legacy packages found for migration";
            return true;
        }
        
        for (const auto& entry : fs::directory_iterator(legacy_packages_dir)) {
            if (entry.is_directory()) {
                std::string package_name = entry.path().filename().string();
                
                // 确定版本
                std::string version = "unknown";
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            version = head_line.substr(head_line.find_last_of('/') + 1);
                        } else {
                            version = head_line.substr(0, 8);  // 取前8位commit hash
                        }
                    }
                }
                
                // 安装到缓存
                std::string repo_url = "";  // 需要从配置中获取
                if (install_package_to_cache(package_name, version, repo_url)) {
                    // 创建项目链接
                    create_project_link(package_name, version, project_path);
                    
                    // 删除原始目录
                    fs::remove_all(entry.path());
                }
            }
        }
        
        LOG(INFO) << "Successfully migrated project from legacy mode";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error migrating from legacy mode: " << e.what();
        return false;
    }
}

// 私有方法实现

bool CacheManager::install_shallow_clone(const std::string& repo_url, const std::string& cache_path, 
                                       const std::string& version) {
    std::ostringstream cmd;
    cmd << "git clone --quiet --depth 1 " << repo_url << " " << cache_path << " 2>/dev/null";
    int ret = std::system(cmd.str().c_str());
    
    if (ret != 0) {
        return false;
    }
    
    if (!version.empty() && version != "*") {
        std::ostringstream checkout_cmd;
        checkout_cmd << "cd " << cache_path << " && git fetch --tags --quiet && git checkout --quiet " << version << " 2>/dev/null";
        ret = std::system(checkout_cmd.str().c_str());
        if (ret != 0) {
            LOG(WARNING) << "Failed to checkout version " << version;
        }
    }
    
    return true;
}

bool CacheManager::install_archive_only(const std::string& repo_url, const std::string& cache_path, 
                                      const std::string& version) {
    // 简化为浅克隆，但移除.git目录
    if (!install_shallow_clone(repo_url, cache_path, version)) {
        return false;
    }
    
    // 移除Git信息以节省空间
    fs::path git_dir = fs::path(cache_path) / ".git";
    if (fs::exists(git_dir)) {
        fs::remove_all(git_dir);
    }
    
    return true;
}

bool CacheManager::install_compressed(const std::string& repo_url, const std::string& cache_path, 
                                    const std::string& version) {
    // 先安装到临时目录
    std::string temp_path = cache_path + ".tmp";
    if (!install_shallow_clone(repo_url, temp_path, version)) {
        return false;
    }
    
    // 创建压缩归档
    std::ostringstream tar_cmd;
    tar_cmd << "cd " << temp_path << " && tar -czf " << cache_path << ".tar.gz .";
    int ret = std::system(tar_cmd.str().c_str());
    
    // 清理临时目录
    fs::remove_all(temp_path);
    
    return ret == 0;
}

bool CacheManager::load_cache_index() {
    try {
        std::string index_path = user_cache_path_ + "/cache_index.json";
        if (!fs::exists(index_path)) {
            return true;  // 索引文件不存在是正常的
        }
        
        std::ifstream file(index_path);
        json j;
        file >> j;
        
        package_index_.clear();
        for (const auto& [package, versions] : j.items()) {
            for (const auto& [version, info] : versions.items()) {
                PackageCacheInfo pkg_info;
                pkg_info.package_name = package;
                pkg_info.version = version;
                pkg_info.cache_path = info["cache_path"];
                pkg_info.repository_url = info["repository_url"];
                pkg_info.size_bytes = info["size_bytes"];
                pkg_info.access_count = info["access_count"];
                pkg_info.is_active = info["is_active"];
                
                // 解析时间戳
                if (info.contains("install_time")) {
                    pkg_info.install_time = std::chrono::system_clock::from_time_t(info["install_time"]);
                }
                if (info.contains("last_access")) {
                    pkg_info.last_access = std::chrono::system_clock::from_time_t(info["last_access"]);
                }
                
                package_index_[package][version] = pkg_info;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error loading cache index: " << e.what();
        return false;
    }
}

void CacheManager::scan_installed_packages() {
    try {
        fs::path packages_dir = "packages";
        if (!fs::exists(packages_dir) || !fs::is_directory(packages_dir)) {
            LOG(INFO) << "No packages directory found, skipping scan";
            return;
        }
        
        LOG(INFO) << "Scanning installed packages in " << packages_dir.string();
        
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
                
                // 计算包大小
                size_t package_size = 0;
                try {
                    for (const auto& file_entry : fs::recursive_directory_iterator(entry.path())) {
                        if (file_entry.is_regular_file()) {
                            package_size += fs::file_size(file_entry.path());
                        }
                    }
                } catch (const std::exception& e) {
                    LOG(WARNING) << "Failed to calculate size for package " << package_name << ": " << e.what();
                }
                
                // 创建缓存信息
                PackageCacheInfo pkg_info;
                pkg_info.package_name = package_name;
                pkg_info.version = version;
                pkg_info.cache_path = entry.path().string();
                pkg_info.repository_url = ""; // 从packages目录安装的包没有仓库URL
                pkg_info.size_bytes = package_size;
                pkg_info.access_count = 1; // 默认访问次数
                pkg_info.is_active = true;
                pkg_info.install_time = std::chrono::system_clock::now();
                pkg_info.last_access = std::chrono::system_clock::now();
                
                // 添加到缓存索引
                package_index_[package_name][version] = pkg_info;
                
                LOG(INFO) << "Scanned package: " << package_name << "@" << version 
                         << " (size: " << package_size << " bytes)";
            }
        }
        
        // 保存更新后的缓存索引
        save_cache_index();
        
        LOG(INFO) << "Scanned " << package_index_.size() << " packages";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error scanning installed packages: " << e.what();
    }
}

bool CacheManager::save_cache_index() {
    try {
        std::string index_path = user_cache_path_ + "/cache_index.json";
        fs::create_directories(fs::path(index_path).parent_path());
        
        json j;
        for (const auto& [package, versions] : package_index_) {
            for (const auto& [version, info] : versions) {
                j[package][version] = {
                    {"cache_path", info.cache_path},
                    {"repository_url", info.repository_url},
                    {"size_bytes", info.size_bytes},
                    {"access_count", info.access_count},
                    {"is_active", info.is_active},
                    {"install_time", std::chrono::system_clock::to_time_t(info.install_time)},
                    {"last_access", std::chrono::system_clock::to_time_t(info.last_access)}
                };
            }
        }
        
        std::ofstream file(index_path);
        file << j.dump(2);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error saving cache index: " << e.what();
        return false;
    }
}

std::string CacheManager::resolve_cache_path(const std::string& package, const std::string& version) const {
    std::string cache_dir;
    
    switch (strategy_) {
        case CacheStrategy::GLOBAL_ONLY:
            cache_dir = global_cache_path_;
            break;
        case CacheStrategy::USER_ONLY:
            cache_dir = user_cache_path_;
            break;
        case CacheStrategy::HYBRID:
            cache_dir = user_cache_path_;  // 优先使用用户缓存
            break;
        case CacheStrategy::PROJECT_LOCAL:
            cache_dir = project_cache_path_;
            break;
    }
    
    std::string version_suffix = version.empty() ? "latest" : version;
    return cache_dir + "/" + package + "/" + version_suffix;
}

bool CacheManager::create_symbolic_link(const std::string& target, const std::string& link_path) {
    try {
        if (fs::exists(link_path)) {
            fs::remove(link_path);
        }
        fs::create_symlink(target, link_path);
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error creating symbolic link: " << e.what();
        return false;
    }
}

bool CacheManager::remove_symbolic_link(const std::string& link_path) {
    try {
        if (fs::exists(link_path)) {
            fs::remove(link_path);
        }
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error removing symbolic link: " << e.what();
        return false;
    }
}

size_t CacheManager::calculate_directory_size(const std::string& path) const {
    size_t total_size = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                total_size += fs::file_size(entry.path());
            }
        }
    } catch (const std::exception& e) {
        LOG(WARNING) << "Error calculating directory size: " << e.what();
    }
    return total_size;
}

bool CacheManager::remove_package_from_cache(const std::string& package, const std::string& version) {
    try {
        auto pkg_it = package_index_.find(package);
        if (pkg_it == package_index_.end()) {
            return false;
        }
        
        if (version.empty()) {
            // 删除所有版本
            for (const auto& [ver, info] : pkg_it->second) {
                if (fs::exists(info.cache_path)) {
                    fs::remove_all(info.cache_path);
                }
            }
            package_index_.erase(pkg_it);
        } else {
            // 删除特定版本
            auto ver_it = pkg_it->second.find(version);
            if (ver_it != pkg_it->second.end()) {
                if (fs::exists(ver_it->second.cache_path)) {
                    fs::remove_all(ver_it->second.cache_path);
                }
                pkg_it->second.erase(ver_it);
                
                if (pkg_it->second.empty()) {
                    package_index_.erase(pkg_it);
                }
            }
        }
        
        save_cache_index();
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error removing package from cache: " << e.what();
        return false;
    }
}

// 全局函数实现
bool initialize_cache_manager() {
    g_cache_manager = std::make_unique<CacheManager>();
    return g_cache_manager->initialize();
}

void cleanup_cache_manager() {
    if (g_cache_manager) {
        g_cache_manager->save_cache_index();
        g_cache_manager.reset();
    }
}

// 添加缺失的函数实现
std::vector<PackageCacheInfo> CacheManager::get_package_list() const {
    std::vector<PackageCacheInfo> packages;
    
    // 检查全局缓存路径
    if (fs::exists(global_cache_path_)) {
        try {
            for (const auto& entry : fs::directory_iterator(global_cache_path_)) {
                if (entry.is_directory()) {
                    std::string package_name = entry.path().filename().string();
                    
                    // 查找版本目录
                    for (const auto& version_entry : fs::directory_iterator(entry.path())) {
                        if (version_entry.is_directory()) {
                            std::string version = version_entry.path().filename().string();
                            
                            PackageCacheInfo info;
                            info.package_name = package_name;
                            info.version = version;
                            info.cache_path = version_entry.path().string();
                            info.size_bytes = calculate_directory_size(version_entry.path().string());
                            info.last_access = std::chrono::system_clock::from_time_t(
                                std::chrono::duration_cast<std::chrono::seconds>(
                                    fs::last_write_time(version_entry.path()).time_since_epoch()).count());
                            
                            packages.push_back(info);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << "Error getting package list from global cache: " << e.what();
        }
    }
    
    return packages;
}

bool CacheManager::load_configuration(const std::string& config_path) {
    try {
        if (!fs::exists(config_path)) {
            LOG(WARNING) << "Configuration file does not exist: " << config_path;
            return false;
        }
        
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            LOG(ERROR) << "Failed to open configuration file: " << config_path;
            return false;
        }
        
        // 简单的配置加载逻辑
        std::string line;
        while (std::getline(config_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // 处理配置项
                if (key == "global_cache_dir") {
                    global_cache_path_ = value;
                } else if (key == "user_cache_dir") {
                    user_cache_path_ = value;
                } else if (key == "project_cache_dir") {
                    project_cache_path_ = value;
                } else if (key == "max_cache_size") {
                    max_cache_size_ = std::stoull(value);
                } else if (key == "version_storage") {
                    if (value == "shallow") {
                        version_storage_ = VersionStorage::SHALLOW_CLONE;
                    } else if (value == "archive") {
                        version_storage_ = VersionStorage::ARCHIVE_ONLY;
                    } else if (value == "compressed") {
                        version_storage_ = VersionStorage::COMPRESSED;
                    }
                }
            }
        }
        
        LOG(INFO) << "Configuration loaded from: " << config_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error loading configuration: " << e.what();
        return false;
    }
}

// 内存管理方法实现
void CacheManager::enable_compression(bool enable) {
    compression_enabled_ = enable;
    LOG(INFO) << "Cache compression " << (enable ? "enabled" : "disabled");
}

void CacheManager::enable_preallocation(bool enable) {
    preallocation_enabled_ = enable;
    if (memory_pool_) {
        memory_pool_->enable_preallocation(enable);
    }
    LOG(INFO) << "Memory preallocation " << (enable ? "enabled" : "disabled");
}

void CacheManager::optimize_memory_usage() {
    LOG(INFO) << "Optimizing memory usage...";
    
    if (memory_pool_) {
        memory_pool_->optimize();
        memory_pool_->cleanup();
    }
    
    // 压缩缓存数据
    if (compression_enabled_) {
        compress_cache_data();
    }
    
    LOG(INFO) << "Memory optimization completed";
}

void CacheManager::compress_cache_data() {
    if (!compression_enabled_) {
        return;
    }
    
    LOG(INFO) << "Compressing cache data...";
    
    size_t compressed_count = 0;
    size_t total_savings = 0;
    
    for (auto& [package_name, versions] : package_index_) {
        for (auto& [version, cache_info] : versions) {
            if (cache_info.is_active && !cache_info.cache_path.empty()) {
                std::string compressed_path = cache_info.cache_path + ".compressed";
                
                // 使用zlib压缩
                if (compress_file_zlib(cache_info.cache_path, compressed_path)) {
                    // 计算压缩率
                    size_t original_size = cache_info.size_bytes;
                    size_t compressed_size = calculate_file_size(compressed_path);
                    
                    if (compressed_size < original_size) {
                        // 替换原文件
                        std::remove(cache_info.cache_path.c_str());
                        std::rename(compressed_path.c_str(), cache_info.cache_path.c_str());
                        
                        cache_info.size_bytes = compressed_size;
                        compressed_count++;
                        total_savings += (original_size - compressed_size);
                        
                        LOG(INFO) << "Compressed " << package_name << "@" << version 
                                  << ": " << original_size << " -> " << compressed_size << " bytes";
                    } else {
                        // 压缩效果不好，删除压缩文件
                        std::remove(compressed_path.c_str());
                    }
                }
            }
        }
    }
    
    LOG(INFO) << "Compression completed: " << compressed_count << " files, " 
              << total_savings << " bytes saved";
}

size_t CacheManager::get_memory_usage() const {
    size_t usage = 0;
    
    if (memory_pool_) {
        usage += memory_pool_->get_current_usage();
    }
    
    // 计算缓存数据内存使用
    for (const auto& [package_name, versions] : package_index_) {
        for (const auto& [version, cache_info] : versions) {
            usage += cache_info.size_bytes;
        }
    }
    
    return usage;
}

std::string CacheManager::get_memory_report() const {
    std::ostringstream ss;
    
    ss << "=== Cache Memory Report ===\n";
    ss << "Total cache size: " << get_cache_size() << " bytes\n";
    ss << "Memory pool usage: ";
    
    if (memory_pool_) {
        auto stats = memory_pool_->get_statistics();
        ss << stats.current_usage << " bytes\n";
        ss << "Memory pool peak: " << stats.peak_usage << " bytes\n";
        ss << "Allocations: " << stats.allocation_count << "\n";
        ss << "Fragmentation: " << stats.fragmentation_ratio << "\n";
    } else {
        ss << "Not initialized\n";
    }
    
    ss << "Compression: " << (compression_enabled_ ? "enabled" : "disabled") << "\n";
    ss << "Preallocation: " << (preallocation_enabled_ ? "enabled" : "disabled") << "\n";
    
    return ss.str();
}

// 辅助方法实现
bool CacheManager::compress_file_zlib(const std::string& input_path, const std::string& output_path) {
    try {
        std::ifstream input(input_path, std::ios::binary);
        if (!input.is_open()) {
            return false;
        }
        
        // 读取文件内容
        input.seekg(0, std::ios::end);
        size_t file_size = input.tellg();
        input.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(file_size);
        input.read(buffer.data(), file_size);
        input.close();
        
        // 计算压缩后大小
        uLong compressed_size = compressBound(file_size);
        std::vector<char> compressed_buffer(compressed_size);
        
        // 执行压缩
        int result = compress2(
            reinterpret_cast<Bytef*>(compressed_buffer.data()), &compressed_size,
            reinterpret_cast<const Bytef*>(buffer.data()), file_size,
            Z_BEST_COMPRESSION);
        
        if (result != Z_OK) {
            return false;
        }
        
        // 写入压缩文件
        std::ofstream output(output_path, std::ios::binary);
        if (!output.is_open()) {
            return false;
        }
        
        // 写入原始大小和压缩大小
        output.write(reinterpret_cast<const char*>(&file_size), sizeof(file_size));
        output.write(reinterpret_cast<const char*>(&compressed_size), sizeof(compressed_size));
        output.write(compressed_buffer.data(), compressed_size);
        output.close();
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Compression failed: " << e.what();
        return false;
    }
}

bool CacheManager::decompress_file_zlib(const std::string& input_path, const std::string& output_path) {
    try {
        std::ifstream input(input_path, std::ios::binary);
        if (!input.is_open()) {
            return false;
        }
        
        // 读取原始大小和压缩大小
        size_t original_size, compressed_size;
        input.read(reinterpret_cast<char*>(&original_size), sizeof(original_size));
        input.read(reinterpret_cast<char*>(&compressed_size), sizeof(compressed_size));
        
        // 读取压缩数据
        std::vector<char> compressed_buffer(compressed_size);
        input.read(compressed_buffer.data(), compressed_size);
        input.close();
        
        // 解压缩
        std::vector<char> decompressed_buffer(original_size);
        uLong decompressed_size = original_size;
        int result = uncompress(
            reinterpret_cast<Bytef*>(decompressed_buffer.data()), &decompressed_size,
            reinterpret_cast<const Bytef*>(compressed_buffer.data()), compressed_size);
        
        if (result != Z_OK || decompressed_size != original_size) {
            return false;
        }
        
        // 写入解压缩文件
        std::ofstream output(output_path, std::ios::binary);
        if (!output.is_open()) {
            return false;
        }
        
        output.write(decompressed_buffer.data(), original_size);
        output.close();
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Decompression failed: " << e.what();
        return false;
    }
}

size_t CacheManager::calculate_file_size(const std::string& file_path) const {
    try {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return 0;
        }
        return file.tellg();
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to calculate file size: " << e.what();
        return 0;
    }
}

size_t CacheManager::get_cache_size() const {
    size_t total_size = 0;
    
    // 计算所有缓存目录的总大小
    std::vector<std::string> cache_paths = {global_cache_path_, user_cache_path_, project_cache_path_};
    
    for (const auto& cache_path : cache_paths) {
        if (!cache_path.empty() && std::filesystem::exists(cache_path)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(cache_path)) {
                if (entry.is_regular_file()) {
                    total_size += entry.file_size();
                }
            }
        }
    }
    
    return total_size;
}

} // namespace Paker 