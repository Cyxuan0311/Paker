#pragma once

#include <string>
#include <vector>

namespace Paker {

// 预热命令接口
void pm_warmup();
void pm_warmup_analyze();
void pm_warmup_stats();
void pm_warmup_config();

// 预热配置管理
bool configure_warmup_settings();
bool show_warmup_configuration();
bool reset_warmup_configuration();

// 预热分析工具
bool analyze_project_dependencies();
bool generate_warmup_recommendations();
bool optimize_warmup_strategy();

} // namespace Paker
