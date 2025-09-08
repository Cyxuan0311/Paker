#pragma once

#include <string>

namespace Paker {

// 性能监控命令
int pm_performance_report(const std::string& output_file = "");

// 依赖分析命令
int pm_analyze_dependencies(const std::string& output_file = "");

// 诊断命令
int pm_diagnose(const std::string& output_file = "");

// 监控设置命令
int pm_monitor_enable(bool enable = true);
int pm_monitor_clear();

} // namespace Paker 