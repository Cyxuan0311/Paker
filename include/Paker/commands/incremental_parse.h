#pragma once

#include <string>
#include <vector>

namespace Paker {

// 增量解析命令接口
void pm_incremental_parse(const std::vector<std::string>& packages = {});
void pm_incremental_parse_stats();
void pm_incremental_parse_config();
void pm_incremental_parse_clear_cache();
void pm_incremental_parse_optimize();
void pm_incremental_parse_validate();

} // namespace Paker
