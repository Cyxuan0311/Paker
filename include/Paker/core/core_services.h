#pragma once

#include "Paker/core/service_container.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/cache/cache_manager.h"
#include "Paker/cache/cache_warmup.h"
#include "Paker/core/parallel_executor.h"
#include "Paker/monitor/performance_monitor.h"
#include "Paker/core/incremental_updater.h"
#include <memory>

namespace Paker {

// 依赖解析服务
class DependencyResolverService : public IService {
private:
    std::unique_ptr<DependencyResolver> resolver_;
    std::mutex resolver_mutex_;

public:
    DependencyResolverService();
    ~DependencyResolverService() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "DependencyResolverService"; }

    DependencyResolver* get_resolver();
    DependencyGraph* get_dependency_graph();
};

// 缓存管理服务
class CacheManagerService : public IService {
private:
    std::unique_ptr<CacheManager> cache_manager_;
    std::mutex cache_mutex_;

public:
    CacheManagerService();
    ~CacheManagerService() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "CacheManagerService"; }

    CacheManager* get_cache_manager();
};

// 并行执行服务
class ParallelExecutorService : public IService {
private:
    std::unique_ptr<ParallelExecutor> executor_;
    std::mutex executor_mutex_;

public:
    ParallelExecutorService();
    ~ParallelExecutorService() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "ParallelExecutorService"; }

    ParallelExecutor* get_executor();
};

// 性能监控服务
class PerformanceMonitorService : public IService {
private:
    std::unique_ptr<PerformanceMonitor> monitor_;
    std::mutex monitor_mutex_;

public:
    PerformanceMonitorService();
    ~PerformanceMonitorService() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "PerformanceMonitorService"; }

    PerformanceMonitor* get_monitor();
};

// 增量更新服务
class IncrementalUpdaterService : public IService {
private:
    std::unique_ptr<IncrementalUpdater> updater_;
    std::mutex updater_mutex_;

public:
    IncrementalUpdaterService();
    ~IncrementalUpdaterService() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "IncrementalUpdaterService"; }

    IncrementalUpdater* get_updater();
};

// 缓存预热服务
class CacheWarmupServiceWrapper : public IService {
private:
    std::shared_ptr<CacheWarmupService> warmup_service_;
    std::mutex warmup_mutex_;

public:
    CacheWarmupServiceWrapper();
    ~CacheWarmupServiceWrapper() override = default;

    bool initialize() override;
    void shutdown() override;
    std::string get_name() const override { return "CacheWarmupServiceWrapper"; }

    CacheWarmupService* get_warmup_service();
};

// 服务工厂
class ServiceFactory {
public:
    static std::shared_ptr<DependencyResolverService> create_dependency_resolver_service();
    static std::shared_ptr<CacheManagerService> create_cache_manager_service();
    static std::shared_ptr<ParallelExecutorService> create_parallel_executor_service();
    static std::shared_ptr<PerformanceMonitorService> create_performance_monitor_service();
    static std::shared_ptr<IncrementalUpdaterService> create_incremental_updater_service();
    static std::shared_ptr<CacheWarmupServiceWrapper> create_cache_warmup_service();
    
    // 注册所有核心服务
    static bool register_all_core_services();
};

// 便捷的访问函数
DependencyResolver* get_dependency_resolver();
DependencyGraph* get_dependency_graph();
CacheManager* get_cache_manager();
ParallelExecutor* get_parallel_executor();
PerformanceMonitor* get_performance_monitor();
IncrementalUpdater* get_incremental_updater();
CacheWarmupService* get_cache_warmup_service();

} // namespace Paker
