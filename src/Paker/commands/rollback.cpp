#include "Paker/commands/rollback.h"
#include "Paker/core/version_history.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/version_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <glog/logging.h>

namespace fs = std::filesystem;

namespace Paker {

void pm_rollback_to_version(const std::string& package_name, const std::string& target_version, bool force) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    LOG(INFO) << "Rolling back " << package_name << " to version " << target_version;
    Output::info("Rolling back " + package_name + " to version " + target_version);
    
    // 设置回滚选项
    RollbackOptions options;
    options.strategy = RollbackStrategy::SINGLE_PACKAGE;
    options.create_backup = true;
    options.validate_dependencies = !force;
    options.force = force;
    options.reason = "Manual rollback to version " + target_version;
    
    // 执行回滚
    auto result = history_manager->rollback_to_version(package_name, target_version, options);
    
    // 显示结果
    if (result.success) {
        Output::success("Rollback completed successfully");
        Output::info("Duration: " + std::to_string(result.duration.count()) + "ms");
        
        if (!result.rolled_back_packages.empty()) {
            Output::info("Rolled back packages:");
            for (const auto& pkg : result.rolled_back_packages) {
                Output::info("  - " + pkg);
            }
        }
        
        // 显示详细报告
        std::string report = RollbackUtils::generate_rollback_report(result);
        Output::info(report);
        
    } else {
        Output::error("Rollback failed: " + result.message);
        
        if (!result.failed_packages.empty()) {
            Output::warning("Failed packages:");
            for (const auto& pkg : result.failed_packages) {
                Output::warning("  - " + pkg);
            }
        }
    }
}

void pm_rollback_to_previous(const std::string& package_name, bool force) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    LOG(INFO) << "Rolling back " << package_name << " to previous version";
    Output::info("Rolling back " + package_name + " to previous version");
    
    // 设置回滚选项
    RollbackOptions options;
    options.strategy = RollbackStrategy::SINGLE_PACKAGE;
    options.create_backup = true;
    options.validate_dependencies = !force;
    options.force = force;
    options.reason = "Manual rollback to previous version";
    
    // 执行回滚
    auto result = history_manager->rollback_to_previous(package_name, options);
    
    // 显示结果
    if (result.success) {
        Output::success("Rollback to previous version completed successfully");
        Output::info("Duration: " + std::to_string(result.duration.count()) + "ms");
        
        if (!result.rolled_back_packages.empty()) {
            Output::info("Rolled back packages:");
            for (const auto& pkg : result.rolled_back_packages) {
                Output::info("  - " + pkg);
            }
        }
        
    } else {
        Output::error("Rollback to previous version failed: " + result.message);
    }
}

void pm_rollback_to_timestamp(const std::string& timestamp, bool force) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    LOG(INFO) << "Rolling back to timestamp: " << timestamp;
    Output::info("Rolling back to timestamp: " + timestamp);
    
    // 解析时间戳
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) {
        Output::error("Invalid timestamp format. Use: YYYY-MM-DD HH:MM:SS");
        return;
    }
    
    auto target_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    // 设置回滚选项
    RollbackOptions options;
    options.strategy = RollbackStrategy::ALL_PACKAGES;
    options.create_backup = true;
    options.validate_dependencies = !force;
    options.force = force;
    options.reason = "Manual rollback to timestamp " + timestamp;
    
    // 执行回滚
    auto result = history_manager->rollback_to_timestamp(target_time, options);
    
    // 显示结果
    if (result.success) {
        Output::success("Rollback to timestamp completed successfully");
        Output::info("Duration: " + std::to_string(result.duration.count()) + "ms");
        
        if (!result.rolled_back_packages.empty()) {
            Output::info("Rolled back packages:");
            for (const auto& pkg : result.rolled_back_packages) {
                Output::info("  - " + pkg);
            }
        }
        
    } else {
        Output::error("Rollback to timestamp failed: " + result.message);
    }
}

void pm_history_show(const std::string& package_name) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    std::vector<VersionHistoryEntry> history;
    if (package_name.empty()) {
        history = history_manager->get_recent_history(20);
        Output::info("Recent version history (last 20 entries):");
    } else {
        history = history_manager->get_package_history(package_name);
        Output::info("Version history for " + package_name + ":");
    }
    
    if (history.empty()) {
        Output::info("No history entries found");
        return;
    }
    
    // 显示历史记录表格
    Output::info("┌─────────────────┬─────────────┬─────────────┬─────────────────────┬─────────────┐");
    Output::info("│ Package         │ Old Version │ New Version │ Timestamp           │ Operation   │");
    Output::info("├─────────────────┼─────────────┼─────────────┼─────────────────────┼─────────────┤");
    
    for (const auto& entry : history) {
        std::ostringstream row;
        row << "│ " << std::setw(15) << std::left << entry.package_name << " │ "
            << std::setw(11) << std::left << entry.old_version << " │ "
            << std::setw(11) << std::left << entry.new_version << " │ ";
        
        // 格式化时间戳
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::stringstream time_ss;
        time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M");
        row << std::setw(19) << std::left << time_ss.str() << " │ ";
        
        // 操作类型
        std::string operation = entry.is_rollback ? "Rollback" : "Update";
        row << std::setw(11) << std::left << operation << " │";
        
        Output::info(row.str());
    }
    
    Output::info("└─────────────────┴─────────────┴─────────────┴─────────────────────┴─────────────┘");
}

void pm_rollback_list(const std::string& package_name) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    if (package_name.empty()) {
        Output::error("Package name is required");
        return;
    }
    
    auto versions = history_manager->get_rollbackable_versions(package_name);
    
    if (versions.empty()) {
        Output::info("No rollbackable versions found for " + package_name);
        return;
    }
    
    Output::info("Rollbackable versions for " + package_name + ":");
    
    // 按版本排序
    std::sort(versions.begin(), versions.end(), [](const std::string& a, const std::string& b) {
        SemanticVersion va(a);
        SemanticVersion vb(b);
        return va > vb; // 最新版本在前
    });
    
    for (size_t i = 0; i < versions.size(); ++i) {
        std::string marker = (i == 0) ? " (current)" : "";
        Output::info("  " + std::to_string(i + 1) + ". " + versions[i] + marker);
    }
    
    Output::info("\nUse: paker rollback-to-version " + package_name + " <version>");
}

void pm_rollback_check(const std::string& package_name, const std::string& target_version) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    if (package_name.empty() || target_version.empty()) {
        Output::error("Package name and target version are required");
        return;
    }
    
    Output::info("Checking rollback safety for " + package_name + " to " + target_version);
    
    bool is_safe = history_manager->can_safely_rollback(package_name, target_version);
    
    if (is_safe) {
        Output::success("✅ Rollback is safe");
        Output::info("All safety checks passed");
    } else {
        Output::warning("⚠️  Rollback may not be safe");
        Output::info("Some safety checks failed");
        Output::info("Use --force flag to override safety checks");
    }
    
    // 详细检查
    Output::info("\nDetailed safety check:");
    
    // 检查版本是否存在
    auto versions = history_manager->get_rollbackable_versions(package_name);
    bool version_exists = std::find(versions.begin(), versions.end(), target_version) != versions.end();
    Output::info("  Version exists: " + std::string(version_exists ? "✅" : "❌"));
    
    // 检查版本兼容性
    bool compatible = VersionManager::is_version_compatible(target_version, "current");
    Output::info("  Version compatible: " + std::string(compatible ? "✅" : "❌"));
    
    // 检查备份可用性
    auto history = history_manager->get_package_history(package_name);
    bool backup_available = false;
    for (const auto& entry : history) {
        if (entry.new_version == target_version && !entry.backup_path.empty()) {
            backup_available = fs::exists(entry.backup_path);
            break;
        }
    }
    Output::info("  Backup available: " + std::string(backup_available ? "✅" : "❌"));
}

void pm_history_cleanup(size_t max_entries) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    Output::info("Cleaning up history records (keeping " + std::to_string(max_entries) + " entries)");
    
    bool success = history_manager->cleanup_old_history(max_entries);
    
    if (success) {
        Output::success("History cleanup completed successfully");
    } else {
        Output::error("History cleanup failed");
    }
}

void pm_history_export(const std::string& export_path) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    if (export_path.empty()) {
        Output::error("Export path is required");
        return;
    }
    
    Output::info("Exporting history to: " + export_path);
    
    bool success = history_manager->export_history(export_path);
    
    if (success) {
        Output::success("History exported successfully");
    } else {
        Output::error("History export failed");
    }
}

void pm_history_import(const std::string& import_path) {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    if (import_path.empty()) {
        Output::error("Import path is required");
        return;
    }
    
    Output::info("Importing history from: " + import_path);
    
    bool success = history_manager->import_history(import_path);
    
    if (success) {
        Output::success("History imported successfully");
    } else {
        Output::error("History import failed");
    }
}

void pm_rollback_stats() {
    auto* history_manager = get_history_manager();
    if (!history_manager) {
        LOG(ERROR) << "History manager not initialized";
        Output::error("History manager not initialized");
        return;
    }
    
    auto stats = history_manager->get_statistics();
    
    Output::info("📊 Rollback Statistics");
    Output::info("=====================");
    Output::info("Total entries: " + std::to_string(stats.total_entries));
    Output::info("Total packages: " + std::to_string(stats.total_packages));
    Output::info("Total rollbacks: " + std::to_string(stats.total_rollbacks));
    Output::info("Total backup size: " + std::to_string(stats.total_backup_size_bytes / 1024 / 1024) + " MB");
    
    if (stats.total_entries > 0) {
        auto first_time = std::chrono::system_clock::to_time_t(stats.first_entry);
        auto last_time = std::chrono::system_clock::to_time_t(stats.last_entry);
        
        std::stringstream first_ss, last_ss;
        first_ss << std::put_time(std::localtime(&first_time), "%Y-%m-%d %H:%M");
        last_ss << std::put_time(std::localtime(&last_time), "%Y-%m-%d %H:%M");
        
        Output::info("First entry: " + first_ss.str());
        Output::info("Last entry: " + last_ss.str());
    }
}

} // namespace Paker 