#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <filesystem>
#include <memory>

namespace Paker {

// 文件变更类型
enum class ChangeType {
    ADDED,      // 新增文件
    MODIFIED,   // 修改文件
    DELETED,    // 删除文件
    UNCHANGED   // 未变更
};

// 文件信息
struct FileInfo {
    std::string path;
    std::string hash;
    size_t size;
    std::chrono::system_clock::time_point last_modified;
    ChangeType change_type;
    
    FileInfo() : size(0), change_type(ChangeType::UNCHANGED) {}
    FileInfo(const std::string& path, const std::string& hash, size_t size)
        : path(path), hash(hash), size(size), change_type(ChangeType::UNCHANGED) {}
};

// 包变更信息
struct PackageChanges {
    std::string package_name;
    std::string version;
    std::vector<FileInfo> added_files;
    std::vector<FileInfo> modified_files;
    std::vector<FileInfo> deleted_files;
    std::vector<FileInfo> unchanged_files;
    size_t total_size;
    size_t changed_size;
    std::chrono::system_clock::time_point last_check;
    
    PackageChanges() : total_size(0), changed_size(0) {}
};

// 增量更新器
class IncrementalUpdater {
private:
    std::string cache_directory_;
    std::string manifest_file_;
    std::map<std::string, std::map<std::string, std::vector<FileInfo>>> package_manifests_;
    
    // 哈希计算
    std::string calculate_file_hash(const std::string& file_path) const;
    std::string calculate_directory_hash(const std::string& dir_path) const;
    
    // 文件系统操作
    std::vector<FileInfo> scan_directory(const std::string& dir_path) const;
    bool compare_files(const FileInfo& old_file, const FileInfo& new_file) const;
    
    // 清单管理
    bool load_manifest();
    bool save_manifest() const;
    void update_package_manifest(const std::string& package, const std::string& version, 
                                const std::vector<FileInfo>& files);
    
public:
    IncrementalUpdater(const std::string& cache_directory);
    ~IncrementalUpdater();
    
    // 初始化
    bool initialize();
    
    // 包变更检测
    PackageChanges detect_package_changes(const std::string& package, const std::string& version,
                                        const std::string& package_path);
    
    // 增量下载
    bool perform_incremental_update(const std::string& package, const std::string& version,
                                  const std::string& repository_url, const std::string& target_path,
                                  const PackageChanges& changes);
    
    // 清单管理
    void update_package_manifest(const std::string& package, const std::string& version,
                                const std::string& package_path);
    void remove_package_manifest(const std::string& package, const std::string& version);
    
    // 统计信息
    size_t get_package_file_count(const std::string& package, const std::string& version) const;
    size_t get_package_size(const std::string& package, const std::string& version) const;
    std::vector<std::string> get_package_files(const std::string& package, const std::string& version) const;
    
    // 清理
    void cleanup_old_manifests();
    void clear_all_manifests();
};

// Git增量更新器
class GitIncrementalUpdater {
private:
    std::string cache_directory_;
    
    // Git操作
    bool is_git_repository(const std::string& path) const;
    std::string get_git_remote_url(const std::string& path) const;
    std::string get_git_current_commit(const std::string& path) const;
    std::vector<std::string> get_git_changed_files(const std::string& path, 
                                                  const std::string& from_commit,
                                                  const std::string& to_commit) const;
    bool perform_git_pull(const std::string& path) const;
    bool perform_git_fetch(const std::string& path) const;
    
public:
    GitIncrementalUpdater(const std::string& cache_directory);
    
    // 增量更新
    bool update_package_incremental(const std::string& package, const std::string& version,
                                  const std::string& repository_url, const std::string& target_path);
    
    // 变更检测
    std::vector<std::string> detect_git_changes(const std::string& package_path) const;
    
    // 版本管理
    bool checkout_version(const std::string& package_path, const std::string& version) const;
    std::vector<std::string> get_available_versions(const std::string& package_path) const;
};

// 全局增量更新器实例
extern std::unique_ptr<IncrementalUpdater> g_incremental_updater;
extern std::unique_ptr<GitIncrementalUpdater> g_git_incremental_updater;

// 初始化增量更新器
bool initialize_incremental_updater(const std::string& cache_directory);

// 清理增量更新器
void cleanup_incremental_updater();

} // namespace Paker
