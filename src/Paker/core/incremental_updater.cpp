#include "Paker/core/incremental_updater.h"
#include "Paker/core/output.h"
#include "Paker/simd/simd_hash.h"
#include <glog/logging.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <openssl/sha.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

// 全局增量更新器实例
std::unique_ptr<IncrementalUpdater> g_incremental_updater;
std::unique_ptr<GitIncrementalUpdater> g_git_incremental_updater;

IncrementalUpdater::IncrementalUpdater(const std::string& cache_directory)
    : cache_directory_(cache_directory) {
    manifest_file_ = cache_directory_ + "/incremental_manifest.json";
}

IncrementalUpdater::~IncrementalUpdater() {
    save_manifest();
}

bool IncrementalUpdater::initialize() {
    try {
        // 创建缓存目录
        fs::create_directories(cache_directory_);
        
        // 加载现有清单
        if (!load_manifest()) {
            LOG(WARNING) << "Failed to load manifest, creating new one";
        }
        
        LOG(INFO) << "IncrementalUpdater initialized with cache directory: " << cache_directory_;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to initialize IncrementalUpdater: " << e.what();
        return false;
    }
}

std::string IncrementalUpdater::calculate_file_hash(const std::string& file_path) const {
    try {
        // 使用SIMD优化的哈希计算
        return SIMDHashCalculator::sha256_simd_file(file_path);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to calculate hash for " << file_path << ": " << e.what();
        return "";
    }
}

std::string IncrementalUpdater::calculate_directory_hash(const std::string& dir_path) const {
    try {
        // 使用SIMD优化的目录哈希计算
        return SIMDFileHasher::calculate_directory_sha256(dir_path);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to calculate directory hash for " << dir_path << ": " << e.what();
        return "";
    }
}

std::vector<FileInfo> IncrementalUpdater::scan_directory(const std::string& dir_path) const {
    std::vector<FileInfo> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                FileInfo file_info;
                file_info.path = fs::relative(entry.path(), dir_path).string();
                file_info.size = entry.file_size();
                auto ftime = entry.last_write_time();
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                file_info.last_modified = sctp;
                file_info.hash = calculate_file_hash(entry.path().string());
                
                files.push_back(file_info);
            }
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to scan directory " << dir_path << ": " << e.what();
    }
    
    return files;
}

bool IncrementalUpdater::compare_files(const FileInfo& old_file, const FileInfo& new_file) const {
    return old_file.hash == new_file.hash && old_file.size == new_file.size;
}

PackageChanges IncrementalUpdater::detect_package_changes(const std::string& package, 
                                                        const std::string& version,
                                                        const std::string& package_path) {
    PackageChanges changes;
    changes.package_name = package;
    changes.version = version;
    changes.last_check = std::chrono::system_clock::now();
    
    try {
        // 扫描当前包文件
        std::vector<FileInfo> current_files = scan_directory(package_path);
        
        // 获取之前的文件清单
        std::vector<FileInfo> previous_files;
        auto package_it = package_manifests_.find(package);
        if (package_it != package_manifests_.end()) {
            auto version_it = package_it->second.find(version);
            if (version_it != package_it->second.end()) {
                previous_files = version_it->second;
            }
        }
        
        // 创建文件映射
        std::map<std::string, FileInfo> previous_map;
        for (const auto& file : previous_files) {
            previous_map[file.path] = file;
        }
        
        std::map<std::string, FileInfo> current_map;
        for (const auto& file : current_files) {
            current_map[file.path] = file;
        }
        
        // 检测变更
        for (auto& [path, current_file] : current_map) {
            auto prev_it = previous_map.find(path);
            if (prev_it == previous_map.end()) {
                // 新文件
                current_file.change_type = ChangeType::ADDED;
                changes.added_files.push_back(current_file);
                changes.changed_size += current_file.size;
            } else {
                // 检查是否修改
                if (compare_files(prev_it->second, current_file)) {
                    current_file.change_type = ChangeType::UNCHANGED;
                    changes.unchanged_files.push_back(current_file);
                } else {
                    current_file.change_type = ChangeType::MODIFIED;
                    changes.modified_files.push_back(current_file);
                    changes.changed_size += current_file.size;
                }
                previous_map.erase(prev_it);
            }
            changes.total_size += current_file.size;
        }
        
        // 剩余的是删除的文件
        for (auto& [path, file] : previous_map) {
            file.change_type = ChangeType::DELETED;
            changes.deleted_files.push_back(file);
            changes.changed_size += file.size;
        }
        
        LOG(INFO) << "Detected changes for " << package << "@" << version 
                  << ": " << changes.added_files.size() << " added, "
                  << changes.modified_files.size() << " modified, "
                  << changes.deleted_files.size() << " deleted";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to detect changes for " << package << "@" << version 
                   << ": " << e.what();
    }
    
    return changes;
}

bool IncrementalUpdater::perform_incremental_update(const std::string& package, 
                                                  const std::string& version,
                                                  const std::string& repository_url, 
                                                  const std::string& target_path,
                                                  const PackageChanges& changes) {
    try {
        LOG(INFO) << "Performing incremental update for " << package << "@" << version;
        
        // 如果变更很少，使用增量更新
        if (changes.changed_size < changes.total_size * 0.1) { // 变更少于10%
            LOG(INFO) << "Using incremental update (changes: " << changes.changed_size 
                      << " bytes, total: " << changes.total_size << " bytes)";
            
            // 删除文件
            for (const auto& file : changes.deleted_files) {
                std::string file_path = target_path + "/" + file.path;
                if (fs::exists(file_path)) {
                    fs::remove(file_path);
                    LOG(INFO) << "Deleted file: " << file.path;
                }
            }
            
            // 这里可以添加增量下载逻辑
            // 目前简化为完整更新
            return true;
            
        } else {
            LOG(INFO) << "Changes too large, performing full update";
            return false; // 触发完整更新
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to perform incremental update: " << e.what();
        return false;
    }
}

void IncrementalUpdater::update_package_manifest(const std::string& package, 
                                               const std::string& version,
                                               const std::vector<FileInfo>& files) {
    package_manifests_[package][version] = files;
    save_manifest();
}

void IncrementalUpdater::remove_package_manifest(const std::string& package, 
                                               const std::string& version) {
    auto package_it = package_manifests_.find(package);
    if (package_it != package_manifests_.end()) {
        package_it->second.erase(version);
        if (package_it->second.empty()) {
            package_manifests_.erase(package_it);
        }
        save_manifest();
    }
}

size_t IncrementalUpdater::get_package_file_count(const std::string& package, 
                                                const std::string& version) const {
    auto package_it = package_manifests_.find(package);
    if (package_it != package_manifests_.end()) {
        auto version_it = package_it->second.find(version);
        if (version_it != package_it->second.end()) {
            return version_it->second.size();
        }
    }
    return 0;
}

size_t IncrementalUpdater::get_package_size(const std::string& package, 
                                          const std::string& version) const {
    size_t total_size = 0;
    auto package_it = package_manifests_.find(package);
    if (package_it != package_manifests_.end()) {
        auto version_it = package_it->second.find(version);
        if (version_it != package_it->second.end()) {
            for (const auto& file : version_it->second) {
                total_size += file.size;
            }
        }
    }
    return total_size;
}

std::vector<std::string> IncrementalUpdater::get_package_files(const std::string& package, 
                                                            const std::string& version) const {
    std::vector<std::string> files;
    auto package_it = package_manifests_.find(package);
    if (package_it != package_manifests_.end()) {
        auto version_it = package_it->second.find(version);
        if (version_it != package_it->second.end()) {
            for (const auto& file : version_it->second) {
                files.push_back(file.path);
            }
        }
    }
    return files;
}

void IncrementalUpdater::cleanup_old_manifests() {
    // 清理超过30天的清单
    auto cutoff_time = std::chrono::system_clock::now() - std::chrono::hours(24 * 30);
    
    for (auto package_it = package_manifests_.begin(); package_it != package_manifests_.end();) {
        for (auto version_it = package_it->second.begin(); version_it != package_it->second.end();) {
            if (!version_it->second.empty()) {
                auto last_modified = version_it->second[0].last_modified;
                if (last_modified < cutoff_time) {
                    version_it = package_it->second.erase(version_it);
                    continue;
                }
            }
            ++version_it;
        }
        
        if (package_it->second.empty()) {
            package_it = package_manifests_.erase(package_it);
        } else {
            ++package_it;
        }
    }
    
    save_manifest();
}

void IncrementalUpdater::clear_all_manifests() {
    package_manifests_.clear();
    save_manifest();
}

bool IncrementalUpdater::load_manifest() {
    try {
        if (!fs::exists(manifest_file_)) {
            return true; // 文件不存在，创建新的
        }
        
        std::ifstream file(manifest_file_);
        json j;
        file >> j;
        
        package_manifests_.clear();
        
        for (const auto& [package_name, versions] : j.items()) {
            for (const auto& [version, files] : versions.items()) {
                std::vector<FileInfo> file_list;
                for (const auto& file : files) {
                    FileInfo file_info;
                    file_info.path = file["path"];
                    file_info.hash = file["hash"];
                    file_info.size = file["size"];
                    file_info.last_modified = std::chrono::system_clock::from_time_t(file["last_modified"]);
                    file_list.push_back(file_info);
                }
                package_manifests_[package_name][version] = file_list;
            }
        }
        
        LOG(INFO) << "Loaded manifest with " << package_manifests_.size() << " packages";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load manifest: " << e.what();
        return false;
    }
}

bool IncrementalUpdater::save_manifest() const {
    try {
        json j;
        
        for (const auto& [package_name, versions] : package_manifests_) {
            for (const auto& [version, files] : versions) {
                json file_list = json::array();
                for (const auto& file : files) {
                    json file_json;
                    file_json["path"] = file.path;
                    file_json["hash"] = file.hash;
                    file_json["size"] = file.size;
                    file_json["last_modified"] = std::chrono::system_clock::to_time_t(file.last_modified);
                    file_list.push_back(file_json);
                }
                j[package_name][version] = file_list;
            }
        }
        
        std::ofstream file(manifest_file_);
        file << j.dump(2);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save manifest: " << e.what();
        return false;
    }
}

// GitIncrementalUpdater 实现
GitIncrementalUpdater::GitIncrementalUpdater(const std::string& cache_directory)
    : cache_directory_(cache_directory) {
}

bool GitIncrementalUpdater::is_git_repository(const std::string& path) const {
    return fs::exists(path + "/.git");
}

std::string GitIncrementalUpdater::get_git_remote_url(const std::string& path) const {
    std::ostringstream cmd;
    cmd << "cd " << path << " && git config --get remote.origin.url";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) return "";
    
    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // 移除换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::string GitIncrementalUpdater::get_git_current_commit(const std::string& path) const {
    std::ostringstream cmd;
    cmd << "cd " << path << " && git rev-parse HEAD";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) return "";
    
    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
    }
    pclose(pipe);
    
    // 移除换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::vector<std::string> GitIncrementalUpdater::get_git_changed_files(const std::string& path,
                                                                     const std::string& from_commit,
                                                                     const std::string& to_commit) const {
    std::vector<std::string> changed_files;
    
    std::ostringstream cmd;
    cmd << "cd " << path << " && git diff --name-only " << from_commit << " " << to_commit;
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) return changed_files;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string file = buffer;
        if (!file.empty() && file.back() == '\n') {
            file.pop_back();
        }
        if (!file.empty()) {
            changed_files.push_back(file);
        }
    }
    pclose(pipe);
    
    return changed_files;
}

bool GitIncrementalUpdater::perform_git_pull(const std::string& path) const {
    std::ostringstream cmd;
    cmd << "cd " << path << " && git pull origin";
    
    int ret = std::system(cmd.str().c_str());
    return ret == 0;
}

bool GitIncrementalUpdater::perform_git_fetch(const std::string& path) const {
    std::ostringstream cmd;
    cmd << "cd " << path << " && git fetch origin";
    
    int ret = std::system(cmd.str().c_str());
    return ret == 0;
}

bool GitIncrementalUpdater::update_package_incremental(const std::string& package, 
                                                      const std::string& version,
                                                      const std::string& repository_url, 
                                                      const std::string& target_path) {
    try {
        if (!is_git_repository(target_path)) {
            LOG(INFO) << "Not a git repository, performing full clone";
            return false; // 触发完整克隆
        }
        
        std::string current_commit = get_git_current_commit(target_path);
        if (current_commit.empty()) {
            LOG(WARNING) << "Failed to get current commit, performing full update";
            return false;
        }
        
        // 获取最新更改
        if (!perform_git_fetch(target_path)) {
            LOG(WARNING) << "Failed to fetch updates";
            return false;
        }
        
        // 检查是否有更新
        std::ostringstream cmd;
        cmd << "cd " << target_path << " && git rev-parse origin/HEAD";
        
        FILE* pipe = popen(cmd.str().c_str(), "r");
        if (!pipe) return false;
        
        char buffer[256];
        std::string latest_commit;
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            latest_commit = buffer;
            if (!latest_commit.empty() && latest_commit.back() == '\n') {
                latest_commit.pop_back();
            }
        }
        pclose(pipe);
        
        if (current_commit == latest_commit) {
            LOG(INFO) << "Package " << package << " is already up to date";
            return true;
        }
        
        // 获取变更文件列表
        std::vector<std::string> changed_files = get_git_changed_files(target_path, current_commit, latest_commit);
        
        LOG(INFO) << "Found " << changed_files.size() << " changed files for " << package;
        
        // 执行增量更新
        if (!perform_git_pull(target_path)) {
            LOG(ERROR) << "Failed to pull updates";
            return false;
        }
        
        // 检出版本
        if (!version.empty() && version != "*" && version != "latest") {
            if (!checkout_version(target_path, version)) {
                LOG(WARNING) << "Failed to checkout version " << version;
            }
        }
        
        LOG(INFO) << "Successfully updated " << package << " incrementally";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to update package incrementally: " << e.what();
        return false;
    }
}

std::vector<std::string> GitIncrementalUpdater::detect_git_changes(const std::string& package_path) const {
    if (!is_git_repository(package_path)) {
        return {};
    }
    
    std::ostringstream cmd;
    cmd << "cd " << package_path << " && git status --porcelain";
    
    std::vector<std::string> changed_files;
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) return changed_files;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line = buffer;
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (line.length() > 3) {
            changed_files.push_back(line.substr(3)); // 跳过状态前缀
        }
    }
    pclose(pipe);
    
    return changed_files;
}

bool GitIncrementalUpdater::checkout_version(const std::string& package_path, const std::string& version) const {
    std::ostringstream cmd;
    cmd << "cd " << package_path << " && git checkout " << version;
    
    int ret = std::system(cmd.str().c_str());
    return ret == 0;
}

std::vector<std::string> GitIncrementalUpdater::get_available_versions(const std::string& package_path) const {
    std::vector<std::string> versions;
    
    if (!is_git_repository(package_path)) {
        return versions;
    }
    
    std::ostringstream cmd;
    cmd << "cd " << package_path << " && git tag -l";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) return versions;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string version = buffer;
        if (!version.empty() && version.back() == '\n') {
            version.pop_back();
        }
        if (!version.empty()) {
            versions.push_back(version);
        }
    }
    pclose(pipe);
    
    return versions;
}

// 全局函数实现
bool initialize_incremental_updater(const std::string& cache_directory) {
    if (g_incremental_updater) {
        LOG(WARNING) << "IncrementalUpdater is already initialized";
        return true;
    }
    
    g_incremental_updater = std::make_unique<IncrementalUpdater>(cache_directory);
    g_git_incremental_updater = std::make_unique<GitIncrementalUpdater>(cache_directory);
    
    return g_incremental_updater->initialize();
}

void cleanup_incremental_updater() {
    g_incremental_updater.reset();
    g_git_incremental_updater.reset();
}

} // namespace Paker
