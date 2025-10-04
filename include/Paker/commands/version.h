#pragma once

#include <string>

namespace Paker {

/**
 * @brief 显示版本信息
 */
void pm_version();

/**
 * @brief 显示简短版本信息
 */
void pm_version_short();

/**
 * @brief 显示构建信息
 */
void pm_version_build();

/**
 * @brief 检查版本兼容性
 * @param required_version 要求的版本号
 */
void pm_version_check(const std::string& required_version);

/**
 * @brief 显示帮助信息
 */
void pm_version_help();

} // namespace Paker

