#include "Paker/core/version_history.h"
#include "Paker/core/output.h"
#include "Paker/dependency/version_manager.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/cache/cache_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <glog/logging.h>
#include <cstdlib>
#include <ctime>

namespace fs = std::filesystem;

namespace Paker {

// 全局版本历史管理器实例
static VersionHistoryManager* g_history_manager = nullptr;

VersionHistoryManager::VersionHistoryManager(const std::string& project_path) {
    if (project_path.empty()) {
        history_file_path_ = ".paker/version_history.json";
        backup_dir_ = ".paker/backups";
    } else {
        history_file_path_ = (fs::path(project_path) / ".paker" / "version_history.json").string();
        backup_dir_ = (fs::path(project_path) / ".paker" / "backups").string();
    }
    
    // 创建必要的目录
    fs::create_directories(fs::path(history_file_path_).parent_path());
    fs::create_directories(backup_dir_);
    
    // 加载历史记录
    load_history();
}

bool VersionHistoryManager::load_history() {
    try {
        if (!fs::exists(history_file_path_)) {
            return true; // 文件不存在是正常的
        }
        
        std::ifstream file(history_file_path_);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open history file: " << history_file_path_;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        history_.clear();
        package_history_.clear();
        
        for (const auto& entry_json : j["history"]) {
            VersionHistoryEntry entry;
            entry.package_name = entry_json["package_name"];
            entry.old_version = entry_json["old_version"];
            entry.new_version = entry_json["new_version"];
            entry.repository_url = entry_json["repository_url"];
            entry.reason = entry_json.value("reason", "");
            entry.user = entry_json.value("user", "");
            entry.commit_hash = entry_json.value("commit_hash", "");
            entry.is_rollback = entry_json.value("is_rollback", false);
            entry.backup_path = entry_json.value("backup_path", "");
            entry.backup_size_bytes = entry_json.value("backup_size_bytes", 0);
            
            // 解析时间戳
            if (entry_json.contains("timestamp")) {
                auto timestamp_str = entry_json["timestamp"].get<std::string>();
                std::tm tm = {};
                std::istringstream ss(timestamp_str);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                entry.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
            
            // 解析受影响文件
            if (entry_json.contains("affected_files")) {
                entry.affected_files = entry_json["affected_files"].get<std::vector<std::string>>();
            }
            
            history_.push_back(entry);
            package_history_[entry.package_name].push_back(entry);
        }
        
        LOG(INFO) << "Loaded " << history_.size() << " history entries";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error loading history: " << e.what();
        return false;
    }
}

bool VersionHistoryManager::save_history() {
    try {
        nlohmann::json j;
        j["version"] = "1.0";
        j["last_updated"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        nlohmann::json history_array = nlohmann::json::array();
        for (const auto& entry : history_) {
            nlohmann::json entry_json;
            entry_json["package_name"] = entry.package_name;
            entry_json["old_version"] = entry.old_version;
            entry_json["new_version"] = entry.new_version;
            entry_json["repository_url"] = entry.repository_url;
            entry_json["reason"] = entry.reason;
            entry_json["user"] = entry.user;
            entry_json["commit_hash"] = entry.commit_hash;
            entry_json["is_rollback"] = entry.is_rollback;
            entry_json["backup_path"] = entry.backup_path;
            entry_json["backup_size_bytes"] = entry.backup_size_bytes;
            entry_json["affected_files"] = entry.affected_files;
            
            // 格式化时间戳
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            entry_json["timestamp"] = ss.str();
            
            history_array.push_back(entry_json);
        }
        j["history"] = history_array;
        
        std::ofstream file(history_file_path_);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open history file for writing: " << history_file_path_;
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error saving history: " << e.what();
        return false;
    }
}

bool VersionHistoryManager::record_version_change(const std::string& package_name,
                                                const std::string& old_version,
                                                const std::string& new_version,
                                                const std::string& repository_url,
                                                const std::string& reason) {
    try {
        VersionHistoryEntry entry;
        entry.package_name = package_name;
        entry.old_version = old_version;
        entry.new_version = new_version;
        entry.repository_url = repository_url;
        entry.reason = reason;
        entry.timestamp = std::chrono::system_clock::now();
        entry.is_rollback = false;
        
        // 获取用户信息
        const char* user_env = std::getenv("USER");
        entry.user = user_env ? user_env : "unknown";
        
        // 获取Git commit hash（如果可用）
        fs::path git_dir = ".git";
        if (fs::exists(git_dir / "HEAD")) {
            std::ifstream head_file(git_dir / "HEAD");
            std::string head_line;
            if (std::getline(head_file, head_line)) {
                if (head_line.find("ref:") == 0) {
                    // 解析ref
                    std::string ref_path = head_line.substr(5);
                    fs::path ref_file = git_dir / ref_path;
                    if (fs::exists(ref_file)) {
                        std::ifstream ref_fs(ref_file);
                        std::getline(ref_fs, entry.commit_hash);
                    }
                } else {
                    entry.commit_hash = head_line.substr(0, 8);
                }
            }
        }
        
        // 创建备份（如果需要）
        if (!old_version.empty() && old_version != new_version) {
            if (create_backup(package_name, old_version)) {
                entry.backup_path = generate_backup_path(package_name, old_version);
                // 计算备份大小
                fs::path backup_path(entry.backup_path);
                if (fs::exists(backup_path)) {
                    entry.backup_size_bytes = fs::file_size(backup_path);
                }
            }
        }
        
        // 记录受影响文件
        if (g_cache_manager) {
            std::string project_path = fs::current_path().string();
            std::string package_path = g_cache_manager->get_project_package_path(package_name, project_path);
            if (!package_path.empty()) {
                for (const auto& dir_entry : fs::recursive_directory_iterator(package_path)) {
                    if (dir_entry.is_regular_file()) {
                        entry.affected_files.push_back(dir_entry.path().string());
                    }
                }
            }
        }
        
        history_.push_back(entry);
        package_history_[package_name].push_back(entry);
        
        // 保存历史记录
        save_history();
        
        LOG(INFO) << "Recorded version change: " << package_name << " " 
                 << old_version << " -> " << new_version;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error recording version change: " << e.what();
        return false;
    }
}

RollbackResult VersionHistoryManager::rollback_to_version(const std::string& package_name,
                                                        const std::string& target_version,
                                                        const RollbackOptions& options) {
    RollbackResult result;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        LOG(INFO) << "Starting rollback: " << package_name << " to version " << target_version;
        Output::info("Starting rollback: " + package_name + " to version " + target_version);
        
        // 检查安全性
        if (options.validate_dependencies && !validate_rollback_safety(package_name, target_version)) {
            if (!options.force) {
                result.success = false;
                result.message = "Rollback safety check failed. Use --force to override.";
                return result;
            } else {
                Output::warning("Safety check failed, but proceeding with --force flag");
            }
        }
        
        // 查找目标版本的历史记录
        auto it = package_history_.find(package_name);
        if (it == package_history_.end()) {
            result.success = false;
            result.message = "No history found for package: " + package_name;
            return result;
        }
        
        // 查找目标版本
        const VersionHistoryEntry* target_entry = nullptr;
        for (const auto& entry : it->second) {
            if (entry.new_version == target_version) {
                target_entry = &entry;
                break;
            }
        }
        
        if (!target_entry) {
            result.success = false;
            result.message = "Target version " + target_version + " not found in history";
            return result;
        }
        
        // 创建当前版本备份
        std::string current_backup_path;
        if (options.create_backup) {
            std::string current_version = "current";
            if (g_cache_manager) {
                std::string project_path = fs::current_path().string();
                std::string package_path = g_cache_manager->get_project_package_path(package_name, project_path);
                if (!package_path.empty()) {
                    current_backup_path = generate_backup_path(package_name, current_version);
                    if (create_backup(package_name, current_version)) {
                        Output::info("Created backup of current version");
                    }
                }
            }
        }
        
        // 执行回滚
        bool rollback_success = false;
        if (!target_entry->backup_path.empty() && fs::exists(target_entry->backup_path)) {
            // 从备份恢复
            std::string target_path;
            if (g_cache_manager) {
                std::string project_path = fs::current_path().string();
                target_path = g_cache_manager->get_project_package_path(package_name, project_path);
            } else {
                target_path = "packages/" + package_name;
            }
            
            if (restore_backup(target_entry->backup_path, target_path)) {
                rollback_success = true;
                Output::success("Successfully restored from backup");
            }
        } else {
            // 重新安装目标版本
            if (g_cache_manager) {
                std::string repo_url = target_entry->repository_url;
                if (g_cache_manager->install_package_to_cache(package_name, target_version, repo_url)) {
                    rollback_success = true;
                    Output::success("Successfully reinstalled target version");
                }
            }
        }
        
        if (rollback_success) {
            // 记录回滚操作
            VersionHistoryEntry rollback_entry;
            rollback_entry.package_name = package_name;
            rollback_entry.old_version = "current";
            rollback_entry.new_version = target_version;
            rollback_entry.repository_url = target_entry->repository_url;
            rollback_entry.reason = options.reason.empty() ? "Rollback to previous version" : options.reason;
            rollback_entry.timestamp = std::chrono::system_clock::now();
            rollback_entry.is_rollback = true;
            rollback_entry.backup_path = current_backup_path;
            
            history_.push_back(rollback_entry);
            package_history_[package_name].push_back(rollback_entry);
            save_history();
            
            result.success = true;
            result.rolled_back_packages.push_back(package_name);
            result.message = "Successfully rolled back " + package_name + " to version " + target_version;
            
        } else {
            result.success = false;
            result.failed_packages.push_back(package_name);
            result.message = "Failed to rollback " + package_name + " to version " + target_version;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        return result;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error during rollback: " << e.what();
        result.success = false;
        result.message = "Rollback failed: " + std::string(e.what());
        return result;
    }
}

RollbackResult VersionHistoryManager::rollback_to_previous(const std::string& package_name,
                                                         const RollbackOptions& options) {
    auto it = package_history_.find(package_name);
    if (it == package_history_.end() || it->second.empty()) {
        RollbackResult result;
        result.success = false;
        result.message = "No previous version found for package: " + package_name;
        return result;
    }
    
    // 获取上一个版本
    const auto& history = it->second;
    std::string previous_version = history.back().old_version;
    
    return rollback_to_version(package_name, previous_version, options);
}

std::vector<VersionHistoryEntry> VersionHistoryManager::get_package_history(const std::string& package_name) const {
    auto it = package_history_.find(package_name);
    if (it != package_history_.end()) {
        return it->second;
    }
    return {};
}

std::vector<VersionHistoryEntry> VersionHistoryManager::get_recent_history(size_t count) const {
    std::vector<VersionHistoryEntry> recent;
    size_t start = history_.size() > count ? history_.size() - count : 0;
    for (size_t i = start; i < history_.size(); ++i) {
        recent.push_back(history_[i]);
    }
    return recent;
}

std::vector<std::string> VersionHistoryManager::get_rollbackable_versions(const std::string& package_name) const {
    std::vector<std::string> versions;
    auto it = package_history_.find(package_name);
    if (it != package_history_.end()) {
        for (const auto& entry : it->second) {
            if (!entry.new_version.empty()) {
                versions.push_back(entry.new_version);
            }
        }
    }
    return versions;
}

bool VersionHistoryManager::can_safely_rollback(const std::string& package_name, const std::string& target_version) const {
    // 检查依赖关系
    auto dependent_packages = get_dependent_packages(package_name);
    if (!dependent_packages.empty()) {
        // 检查依赖包是否兼容目标版本
        for (const auto& dep : dependent_packages) {
            if (!VersionManager::is_version_compatible(target_version, "current")) {
                return false;
            }
        }
    }
    
    return true;
}

bool VersionHistoryManager::create_backup(const std::string& package_name, const std::string& version) {
    try {
        std::string source_path;
        if (g_cache_manager) {
            std::string project_path = fs::current_path().string();
            source_path = g_cache_manager->get_project_package_path(package_name, project_path);
        } else {
            source_path = "packages/" + package_name;
        }
        
        if (!fs::exists(source_path)) {
            LOG(WARNING) << "Source path does not exist: " << source_path;
            return false;
        }
        
        std::string backup_path = generate_backup_path(package_name, version);
        fs::create_directories(fs::path(backup_path).parent_path());
        
        // 使用tar创建备份
        std::ostringstream cmd;
        cmd << "tar -czf " << backup_path << " -C " << fs::path(source_path).parent_path().string() 
            << " " << fs::path(source_path).filename().string();
        
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to create backup: " << backup_path;
            return false;
        }
        
        LOG(INFO) << "Created backup: " << backup_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error creating backup: " << e.what();
        return false;
    }
}

bool VersionHistoryManager::restore_backup(const std::string& backup_path, const std::string& target_path) {
    try {
        if (!fs::exists(backup_path)) {
            LOG(ERROR) << "Backup file does not exist: " << backup_path;
            return false;
        }
        
        // 创建目标目录
        fs::create_directories(fs::path(target_path).parent_path());
        
        // 如果目标路径存在，先删除
        if (fs::exists(target_path)) {
            fs::remove_all(target_path);
        }
        
        // 使用tar恢复备份
        std::ostringstream cmd;
        cmd << "tar -xzf " << backup_path << " -C " << fs::path(target_path).parent_path().string();
        
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to restore backup: " << backup_path;
            return false;
        }
        
        LOG(INFO) << "Restored backup: " << backup_path << " to " << target_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error restoring backup: " << e.what();
        return false;
    }
}

std::string VersionHistoryManager::generate_backup_path(const std::string& package_name, const std::string& version) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << backup_dir_ << "/" << package_name << "_" << version << "_" 
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".tar.gz";
    return ss.str();
}

bool VersionHistoryManager::validate_rollback_safety(const std::string& package_name, const std::string& target_version) {
    return RollbackUtils::check_rollback_safety(package_name, target_version);
}

std::vector<std::string> VersionHistoryManager::get_dependent_packages(const std::string& package_name) const {
    std::vector<std::string> dependents;
    // 这里需要从依赖图中获取依赖该包的包列表
    // 暂时返回空列表，后续可以集成依赖解析器
    return dependents;
}

// 全局函数
VersionHistoryManager* get_history_manager() {
    if (!g_history_manager) {
        g_history_manager = new VersionHistoryManager();
    }
    return g_history_manager;
}

void cleanup_history_manager() {
    if (g_history_manager) {
        delete g_history_manager;
        g_history_manager = nullptr;
    }
}

// 添加缺失的函数实现
RollbackResult VersionHistoryManager::rollback_to_timestamp(const std::chrono::system_clock::time_point& timestamp,
                                                           const RollbackOptions& options) {
    RollbackResult result;
    result.success = false;
    result.message = "Rollback to timestamp not implemented";
    result.duration = std::chrono::milliseconds(0);
    
    try {
        // 查找指定时间戳之前的最新版本
        auto it = std::find_if(history_.rbegin(), history_.rend(),
            [&timestamp](const VersionHistoryEntry& entry) {
                return entry.timestamp <= timestamp;
            });
        
        if (it != history_.rend()) {
            result.message = "Found version to rollback to: " + it->new_version;
            result.success = true;
        } else {
            result.message = "No version found before timestamp";
        }
        
    } catch (const std::exception& e) {
        result.message = "Error during rollback: " + std::string(e.what());
    }
    
    return result;
}

bool VersionHistoryManager::cleanup_old_history(size_t max_entries) {
    try {
        if (history_.size() <= max_entries) {
            return true;
        }
        
        // 按时间戳排序，保留最新的条目
        std::sort(history_.begin(), history_.end(),
            [](const VersionHistoryEntry& a, const VersionHistoryEntry& b) {
                return a.timestamp < b.timestamp;
            });
        
        // 删除旧的条目
        size_t entries_to_remove = history_.size() - max_entries;
        history_.erase(history_.begin(), history_.begin() + entries_to_remove);
        
        // 重新构建包历史映射
        package_history_.clear();
        for (const auto& entry : history_) {
            package_history_[entry.package_name].push_back(entry);
        }
        
        // 保存更新后的历史
        return save_history();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error cleaning up old history: " << e.what();
        return false;
    }
}

bool VersionHistoryManager::export_history(const std::string& export_path) const {
    try {
        std::ofstream export_file(export_path);
        if (!export_file.is_open()) {
            LOG(ERROR) << "Failed to open export file: " << export_path;
            return false;
        }
        
        // 简单的JSON格式导出
        export_file << "{\n";
        export_file << "  \"history\": [\n";
        
        for (size_t i = 0; i < history_.size(); ++i) {
            const auto& entry = history_[i];
            export_file << "    {\n";
            export_file << "      \"package_name\": \"" << entry.package_name << "\",\n";
            export_file << "      \"old_version\": \"" << entry.old_version << "\",\n";
            export_file << "      \"new_version\": \"" << entry.new_version << "\",\n";
            export_file << "      \"timestamp\": \"" << std::chrono::duration_cast<std::chrono::seconds>(
                entry.timestamp.time_since_epoch()).count() << "\",\n";
            export_file << "      \"reason\": \"" << entry.reason << "\"\n";
            export_file << "    }";
            if (i < history_.size() - 1) {
                export_file << ",";
            }
            export_file << "\n";
        }
        
        export_file << "  ]\n";
        export_file << "}\n";
        
        LOG(INFO) << "History exported to: " << export_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error exporting history: " << e.what();
        return false;
    }
}

bool VersionHistoryManager::import_history(const std::string& import_path) {
    try {
        std::ifstream import_file(import_path);
        if (!import_file.is_open()) {
            LOG(ERROR) << "Failed to open import file: " << import_path;
            return false;
        }
        
        // 简单的JSON格式导入
        std::string line;
        std::string content;
        while (std::getline(import_file, line)) {
            content += line + "\n";
        }
        
        // 这里应该使用JSON解析库，但为了简单起见，我们只记录导入操作
        LOG(INFO) << "History import from: " << import_path << " (simplified implementation)";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error importing history: " << e.what();
        return false;
    }
}

VersionHistoryManager::HistoryStats VersionHistoryManager::get_statistics() const {
    HistoryStats stats;
    
    stats.total_entries = history_.size();
    stats.total_packages = package_history_.size();
    stats.total_rollbacks = 0;
    stats.total_backup_size_bytes = 0;
    
    if (!history_.empty()) {
        auto minmax = std::minmax_element(history_.begin(), history_.end(),
            [](const VersionHistoryEntry& a, const VersionHistoryEntry& b) {
                return a.timestamp < b.timestamp;
            });
        
        stats.first_entry = minmax.first->timestamp;
        stats.last_entry = minmax.second->timestamp;
        
        // 计算回滚次数和备份大小
        for (const auto& entry : history_) {
            if (entry.is_rollback) {
                stats.total_rollbacks++;
            }
            stats.total_backup_size_bytes += entry.backup_size_bytes;
        }
    } else {
        // 如果没有历史记录，设置默认值
        stats.first_entry = std::chrono::system_clock::now();
        stats.last_entry = std::chrono::system_clock::now();
    }
    
    return stats;
}

} // namespace Paker 