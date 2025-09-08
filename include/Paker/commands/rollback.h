#pragma once

#include <string>

namespace Paker {

// 回滚到指定版本
void pm_rollback_to_version(const std::string& package_name, const std::string& target_version, bool force = false);

// 回滚到上一个版本
void pm_rollback_to_previous(const std::string& package_name, bool force = false);

// 回滚到指定时间点
void pm_rollback_to_timestamp(const std::string& timestamp, bool force = false);

// 显示版本历史
void pm_history_show(const std::string& package_name = "");

// 显示可回滚的版本列表
void pm_rollback_list(const std::string& package_name);

// 验证回滚安全性
void pm_rollback_check(const std::string& package_name, const std::string& target_version);

// 清理历史记录
void pm_history_cleanup(size_t max_entries = 50);

// 导出历史记录
void pm_history_export(const std::string& export_path);

// 导入历史记录
void pm_history_import(const std::string& import_path);

// 显示回滚统计信息
void pm_rollback_stats();

} // namespace Paker 