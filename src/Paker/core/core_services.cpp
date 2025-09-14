#include "Paker/core/core_services.h"
#include "Paker/core/output.h"
#include "Paker/cache/cache_warmup.h"
#include <glog/logging.h>

namespace Paker {

// ==================== DependencyResolverService ====================

DependencyResolverService::DependencyResolverService() {
    resolver_ = std::make_unique<DependencyResolver>();
}

bool DependencyResolverService::initialize() {
    std::lock_guard<std::mutex> lock(resolver_mutex_);
    
    if (!resolver_) {
        resolver_ = std::make_unique<DependencyResolver>();
    }
    
    LOG(INFO) << "DependencyResolverService initialized";
    return true;
}

void DependencyResolverService::shutdown() {
    std::lock_guard<std::mutex> lock(resolver_mutex_);
    resolver_.reset();
    LOG(INFO) << "DependencyResolverService shut down";
}

DependencyResolver* DependencyResolverService::get_resolver() {
    std::lock_guard<std::mutex> lock(resolver_mutex_);
    return resolver_.get();
}

DependencyGraph* DependencyResolverService::get_dependency_graph() {
    std::lock_guard<std::mutex> lock(resolver_mutex_);
    return resolver_ ? &resolver_->get_dependency_graph() : nullptr;
}

// ==================== CacheManagerService ====================

CacheManagerService::CacheManagerService() {
    cache_manager_ = std::make_unique<CacheManager>();
}

bool CacheManagerService::initialize() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (!cache_manager_) {
        cache_manager_ = std::make_unique<CacheManager>();
    }
    
    if (!cache_manager_->initialize()) {
        LOG(ERROR) << "Failed to initialize cache manager";
        return false;
    }
    
    LOG(INFO) << "CacheManagerService initialized";
    return true;
}

void CacheManagerService::shutdown() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_manager_.reset();
    LOG(INFO) << "CacheManagerService shut down";
}

CacheManager* CacheManagerService::get_cache_manager() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_manager_.get();
}

// ==================== ParallelExecutorService ====================

ParallelExecutorService::ParallelExecutorService() {
    executor_ = std::make_unique<ParallelExecutor>();
}

bool ParallelExecutorService::initialize() {
    std::lock_guard<std::mutex> lock(executor_mutex_);
    
    if (!executor_) {
        executor_ = std::make_unique<ParallelExecutor>();
    }
    
    if (!executor_->start()) {
        LOG(ERROR) << "Failed to start parallel executor";
        return false;
    }
    
    LOG(INFO) << "ParallelExecutorService initialized";
    return true;
}

void ParallelExecutorService::shutdown() {
    std::lock_guard<std::mutex> lock(executor_mutex_);
    if (executor_) {
        executor_->stop();
        executor_.reset();
    }
    LOG(INFO) << "ParallelExecutorService shut down";
}

ParallelExecutor* ParallelExecutorService::get_executor() {
    std::lock_guard<std::mutex> lock(executor_mutex_);
    return executor_.get();
}

// ==================== PerformanceMonitorService ====================

PerformanceMonitorService::PerformanceMonitorService() {
    monitor_ = std::make_unique<PerformanceMonitor>();
}

bool PerformanceMonitorService::initialize() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    if (!monitor_) {
        monitor_ = std::make_unique<PerformanceMonitor>();
    }
    
    monitor_->enable(true);
    
    LOG(INFO) << "PerformanceMonitorService initialized";
    return true;
}

void PerformanceMonitorService::shutdown() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    if (monitor_) {
        monitor_->enable(false);
        monitor_.reset();
    }
    LOG(INFO) << "PerformanceMonitorService shut down";
}

PerformanceMonitor* PerformanceMonitorService::get_monitor() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return monitor_.get();
}

// ==================== IncrementalUpdaterService ====================

IncrementalUpdaterService::IncrementalUpdaterService() {
    // 需要缓存目录路径，这里使用默认路径
    std::string cache_dir = std::getenv("HOME") ? std::getenv("HOME") + std::string("/.paker/cache") : "/tmp/paker_cache";
    updater_ = std::make_unique<IncrementalUpdater>(cache_dir);
}

bool IncrementalUpdaterService::initialize() {
    std::lock_guard<std::mutex> lock(updater_mutex_);
    
    if (!updater_) {
        std::string cache_dir = std::getenv("HOME") ? std::getenv("HOME") + std::string("/.paker/cache") : "/tmp/paker_cache";
        updater_ = std::make_unique<IncrementalUpdater>(cache_dir);
    }
    
    if (!updater_->initialize()) {
        LOG(ERROR) << "Failed to initialize incremental updater";
        return false;
    }
    
    LOG(INFO) << "IncrementalUpdaterService initialized";
    return true;
}

void IncrementalUpdaterService::shutdown() {
    std::lock_guard<std::mutex> lock(updater_mutex_);
    updater_.reset();
    LOG(INFO) << "IncrementalUpdaterService shut down";
}

IncrementalUpdater* IncrementalUpdaterService::get_updater() {
    std::lock_guard<std::mutex> lock(updater_mutex_);
    return updater_.get();
}

// ==================== CacheWarmupServiceWrapper ====================

CacheWarmupServiceWrapper::CacheWarmupServiceWrapper() {
    warmup_service_ = std::make_shared<CacheWarmupService>();
}

bool CacheWarmupServiceWrapper::initialize() {
    std::lock_guard<std::mutex> lock(warmup_mutex_);
    
    if (!warmup_service_) {
        warmup_service_ = std::make_shared<CacheWarmupService>();
    }
    
    warmup_service_->initialize();
    LOG(INFO) << "CacheWarmupServiceWrapper initialized";
    return true;
}

void CacheWarmupServiceWrapper::shutdown() {
    std::lock_guard<std::mutex> lock(warmup_mutex_);
    if (warmup_service_) {
        warmup_service_->shutdown();
    }
    warmup_service_.reset();
    LOG(INFO) << "CacheWarmupServiceWrapper shut down";
}

CacheWarmupService* CacheWarmupServiceWrapper::get_warmup_service() {
    std::lock_guard<std::mutex> lock(warmup_mutex_);
    return warmup_service_.get();
}

// ==================== ServiceFactory ====================

std::shared_ptr<DependencyResolverService> ServiceFactory::create_dependency_resolver_service() {
    return std::make_shared<DependencyResolverService>();
}

std::shared_ptr<CacheManagerService> ServiceFactory::create_cache_manager_service() {
    return std::make_shared<CacheManagerService>();
}

std::shared_ptr<ParallelExecutorService> ServiceFactory::create_parallel_executor_service() {
    return std::make_shared<ParallelExecutorService>();
}

std::shared_ptr<PerformanceMonitorService> ServiceFactory::create_performance_monitor_service() {
    return std::make_shared<PerformanceMonitorService>();
}

std::shared_ptr<IncrementalUpdaterService> ServiceFactory::create_incremental_updater_service() {
    return std::make_shared<IncrementalUpdaterService>();
}

std::shared_ptr<CacheWarmupServiceWrapper> ServiceFactory::create_cache_warmup_service() {
    return std::make_shared<CacheWarmupServiceWrapper>();
}

bool ServiceFactory::register_all_core_services() {
    if (!g_service_manager) {
        LOG(ERROR) << "Service manager not initialized";
        return false;
    }
    
    // 注册所有核心服务
    g_service_manager->register_service(create_dependency_resolver_service());
    g_service_manager->register_service(create_cache_manager_service());
    g_service_manager->register_service(create_parallel_executor_service());
    g_service_manager->register_service(create_performance_monitor_service());
    g_service_manager->register_service(create_incremental_updater_service());
    g_service_manager->register_service(create_cache_warmup_service());
    
    // 初始化所有服务
    if (!g_service_manager->initialize_all()) {
        LOG(ERROR) << "Failed to initialize all core services";
        return false;
    }
    
    LOG(INFO) << "All core services registered and initialized";
    return true;
}

// ==================== 便捷访问函数 ====================

DependencyResolver* get_dependency_resolver() {
    auto service = ServiceLocator::get<DependencyResolverService>();
    return service ? service->get_resolver() : nullptr;
}

DependencyGraph* get_dependency_graph() {
    auto service = ServiceLocator::get<DependencyResolverService>();
    return service ? service->get_dependency_graph() : nullptr;
}

CacheManager* get_cache_manager() {
    auto service = ServiceLocator::get<CacheManagerService>();
    return service ? service->get_cache_manager() : nullptr;
}

ParallelExecutor* get_parallel_executor() {
    auto service = ServiceLocator::get<ParallelExecutorService>();
    return service ? service->get_executor() : nullptr;
}

PerformanceMonitor* get_performance_monitor() {
    auto service = ServiceLocator::get<PerformanceMonitorService>();
    return service ? service->get_monitor() : nullptr;
}

IncrementalUpdater* get_incremental_updater() {
    auto service = ServiceLocator::get<IncrementalUpdaterService>();
    return service ? service->get_updater() : nullptr;
}

CacheWarmupService* get_cache_warmup_service() {
    auto service = ServiceLocator::get<CacheWarmupServiceWrapper>();
    return service ? service->get_warmup_service() : nullptr;
}

} // namespace Paker
