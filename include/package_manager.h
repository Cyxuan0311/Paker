#include <string>
#include <map>
#pragma once

void pm_init();
void pm_add(const std::string& pkg);
void pm_remove(const std::string& pkg);
void pm_list();
void pm_add_desc(const std::string& desc);
void pm_add_version(const std::string& vers);

// 递归安装依赖
void pm_add_recursive(const std::string& pkg);
// 依赖树展示
void pm_tree();

// 获取内置仓库映射表
typedef std::map<std::string, std::string> RepoMap;
const RepoMap& get_builtin_repos(); 

// 生成/更新 Paker.lock 文件，记录实际安装的依赖版本
void pm_lock();
// 根据 Paker.lock 文件安装依赖
void pm_install_lock();
// 升级所有依赖到最新或指定版本
void pm_upgrade(const std::string& pkg = ""); 

// 搜索可用依赖包
void pm_search(const std::string& keyword);
// 显示依赖包详细信息
void pm_info(const std::string& pkg);
// 同步/刷新本地依赖信息
void pm_update();
// 清理未使用或损坏的依赖包
void pm_clean(); 