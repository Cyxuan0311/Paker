#include "Paker/core/service_container.h"
#include <glog/logging.h>

namespace Paker {

// 静态成员定义
std::unique_ptr<IServiceContainer> ServiceLocator::container_ = nullptr;
std::mutex ServiceLocator::container_mutex_;

// 全局服务管理器实例
std::unique_ptr<ServiceManager> g_service_manager = nullptr;

bool initialize_service_manager() {
    if (g_service_manager) {
        LOG(WARNING) << "Service manager already initialized";
        return true;
    }
    
    g_service_manager = std::make_unique<ServiceManager>();
    LOG(INFO) << "Service manager initialized";
    return true;
}

void cleanup_service_manager() {
    if (g_service_manager) {
        g_service_manager->shutdown_all();
        g_service_manager.reset();
        ServiceLocator::clear();
        LOG(INFO) << "Service manager cleaned up";
    }
}

} // namespace Paker
