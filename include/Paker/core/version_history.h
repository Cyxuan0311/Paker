#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace Paker {

// 版本历史记录
struct VersionHistoryEntry {
    std::string package_name;
    std::string old_version;
    std::string new_version;
    std::string repository_url;
    std::chrono::system_clock::time_point timestamp;
    std::string reason;  // 回滚原因
    std::string user;    // 执行用户
    std::string commit_hash;  // Git commit hash
    bool is_rollback;    // 是否为回滚操作
    
    // 备份信息
    std::string backup_path;
    std::vector<std::string> affected_files;
    size_t backup_size_bytes;
    
    VersionHistoryEntry() : backup_size_bytes(0), is_rollback(false) {}
};

// 回滚策略
enum class RollbackStrategy {
    SINGLE_PACKAGE,      // 单个包回滚
    ALL_PACKAGES,        // 所有包回滚
    DEPENDENCY_AWARE,    // 依赖感知回滚
    SELECTIVE           // 选择性回滚
};

// 回滚选项
struct RollbackOptions {
    RollbackStrategy strategy = RollbackStrategy::SINGLE_PACKAGE;
    bool create_backup = true;           // 是否创建备份
    bool validate_dependencies = true;   // 是否验证依赖
    bool interactive = false;            // 是否交互式
    std::string reason;                  // 回滚原因
    bool force = false;                  // 强制回滚
    size_t max_history_entries = 100;    // 最大历史记录数
    
    RollbackOptions() = default;
};

// 回滚结果
struct RollbackResult {
    bool success;
    std::string message;
    std::vector<std::string> rolled_back_packages;
    std::vector<std::string> failed_packages;
    std::string backup_location;
    size_t total_files_affected;
    std::chrono::milliseconds duration;
    
    RollbackResult() : success(false), total_files_affected(0), duration(0) {}
};

// 版本历史管理器
class VersionHistoryManager {
private:
    std::string history_file_path_;
    std::string backup_dir_;
    std::vector<VersionHistoryEntry> history_;
    std::map<std::string, std::vector<VersionHistoryEntry>> package_history_;
    
    // 私有方法
    bool load_history();
    bool save_history();
    bool create_backup(const std::string& package_name, const std::string& version);
    bool restore_backup(const std::string& backup_path, const std::string& target_path);
    std::string generate_backup_path(const std::string& package_name, const std::string& version);
    bool validate_rollback_safety(const std::string& package_name, const std::string& target_version);
    std::vector<std::string> get_dependent_packages(const std::string& package_name);
    
public:
    explicit VersionHistoryManager(const std::string& project_path = "");
    
    // 记录版本变更
    bool record_version_change(const std::string& package_name, 
                             const std::string& old_version,
                             const std::string& new_version,
                             const std::string& repository_url,
                             const std::string& reason = "");
    
    // 回滚到指定版本
    RollbackResult rollback_to_version(const std::string& package_name, 
                                      const std::string& target_version,
                                      const RollbackOptions& options = RollbackOptions{});
    
    // 回滚到指定时间点
    RollbackResult rollback_to_timestamp(const std::chrono::system_clock::time_point& timestamp,
                                        const RollbackOptions& options = RollbackOptions{});
    
    // 回滚到上一个版本
    RollbackResult rollback_to_previous(const std::string& package_name,
                                       const RollbackOptions& options = RollbackOptions{});
    
    // 获取版本历史
    std::vector<VersionHistoryEntry> get_package_history(const std::string& package_name) const;
    std::vector<VersionHistoryEntry> get_recent_history(size_t count = 10) const;
    
    // 获取可回滚的版本列表
    std::vector<std::string> get_rollbackable_versions(const std::string& package_name) const;
    
    // 检查是否可以安全回滚
    bool can_safely_rollback(const std::string& package_name, const std::string& target_version) const;
    
    // 清理历史记录
    bool cleanup_old_history(size_t max_entries = 50);
    
    // 导出历史记录
    bool export_history(const std::string& export_path) const;
    
    // 导入历史记录
    bool import_history(const std::string& import_path);
    
    // 获取统计信息
    struct HistoryStats {
        size_t total_entries;
        size_t total_packages;
        size_t total_rollbacks;
        std::chrono::system_clock::time_point first_entry;
        std::chrono::system_clock::time_point last_entry;
        size_t total_backup_size_bytes;
    };
    
    HistoryStats get_statistics() const;
    
    // 验证历史记录完整性
    bool validate_history_integrity() const;
    
    // 紧急回滚（跳过验证）
    RollbackResult emergency_rollback(const std::string& package_name, 
                                     const std::string& target_version);
};

// 版本回滚工具类
class RollbackUtils {
public:
    // 检查回滚安全性
    static bool check_rollback_safety(const std::string& package_name, 
                                     const std::string& target_version);
    
    // 生成回滚报告
    static std::string generate_rollback_report(const RollbackResult& result);
    
    // 验证备份完整性
    static bool validate_backup_integrity(const std::string& backup_path);
    
    // 计算文件差异
    static std::vector<std::string> calculate_file_differences(const std::string& path1, 
                                                              const std::string& path2);
    
    // 创建差异备份
    static bool create_differential_backup(const std::string& source_path, 
                                         const std::string& backup_path);
    
    // 应用差异备份
    static bool apply_differential_backup(const std::string& backup_path, 
                                        const std::string& target_path);
};

} // namespace Paker 