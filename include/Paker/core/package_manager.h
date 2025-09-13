#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>

// 前向声明
namespace Paker {
    class DependencyResolver;
    class DependencyGraph;
    class CacheManager;
    class ParallelExecutor;
    class PerformanceMonitor;
    class IncrementalUpdater;
}

void pm_init();
void pm_add(const std::string& pkg);
void pm_add_parallel(const std::vector<std::string>& packages);
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

// 依赖冲突检测与解决相关函数
void pm_resolve_dependencies();                    // 解析依赖
void pm_check_conflicts();                         // 检查冲突
void pm_resolve_conflicts();                       // 解决冲突
void pm_validate_dependencies();                   // 验证依赖

// 版本回滚相关函数
void pm_record_version_change(const std::string& package_name, 
                             const std::string& old_version,
                             const std::string& new_version,
                             const std::string& repository_url,
                             const std::string& reason = "");  // 记录版本变更

// 服务架构相关函数
namespace Paker {
    // 初始化服务管理器
    bool initialize_service_manager();
    
    // 清理服务管理器
    void cleanup_service_manager();
    
    // 便捷访问函数（通过服务架构）
    DependencyResolver* get_dependency_resolver();
    DependencyGraph* get_dependency_graph();
    CacheManager* get_cache_manager();
    ParallelExecutor* get_parallel_executor();
    PerformanceMonitor* get_performance_monitor();
    IncrementalUpdater* get_incremental_updater();
} 