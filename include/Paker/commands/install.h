#pragma once
#include <string>
#include <vector>

// 原有的包管理函数
void pm_add(const std::string& pkg_input);
void pm_add_url(const std::string& url);
void pm_remove(const std::string& pkg);
void pm_add_recursive(const std::string& pkg);

// 新的install命令函数
void pm_install(const std::string& package);
void pm_install_parallel(const std::vector<std::string>& packages);
void pm_uninstall(const std::string& package);

// 构建系统检测和安装辅助函数
enum class BuildSystem {
    CMAKE,
    MAKE,
    NINJA,
    MESON,
    AUTOTOOLS,
    UNKNOWN
};

BuildSystem detect_build_system(const std::string& package_path);
bool build_and_install_package(const std::string& package_path, const std::string& package_name, BuildSystem build_system);
std::vector<std::string> install_to_system_and_get_files(const std::string& package_path, const std::string& package_name, const std::vector<std::string>& installed_files);
std::vector<std::string> collect_installed_files(const std::string& package_path);
void record_installation(const std::string& package_name, const std::string& install_path, const std::vector<std::string>& installed_files);
void remove_installation_record(const std::string& package_name); 