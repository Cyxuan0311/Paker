#pragma once

#include <string>

namespace Paker {

/**
 * 智能包推荐命令
 * @param category_filter 类别过滤器
 * @param performance_filter 性能过滤器
 * @param security_filter 安全过滤器
 * @param detailed 是否显示详细信息
 * @param auto_install 是否自动安装推荐包
 * @param export_path 导出路径
 */
void pm_smart_suggestion(
    const std::string& category_filter = "",
    const std::string& performance_filter = "",
    const std::string& security_filter = "",
    bool detailed = false,
    bool auto_install = false,
    const std::string& export_path = ""
);

} // namespace Paker
