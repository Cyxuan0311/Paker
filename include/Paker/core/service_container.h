#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <mutex>
#include <functional>
#include <string>
#include <glog/logging.h>

namespace Paker {

// 服务容器接口
class IServiceContainer {
public:
    virtual ~IServiceContainer() = default;
    virtual void register_singleton(const std::type_index& type, std::shared_ptr<void> instance) = 0;
    virtual void register_factory(const std::type_index& type, std::function<std::shared_ptr<void>()> factory) = 0;
    virtual std::shared_ptr<void> get(const std::type_index& type) = 0;
    virtual bool has(const std::type_index& type) const = 0;
    virtual void clear() = 0;
};

// 服务容器实现
class ServiceContainer : public IServiceContainer {
private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;
    mutable std::mutex mutex_;

public:
    void register_singleton(const std::type_index& type, std::shared_ptr<void> instance) override {
        std::lock_guard<std::mutex> lock(mutex_);
        singletons_[type] = instance;
        LOG(INFO) << "Registered singleton service: " << type.name();
    }

    void register_factory(const std::type_index& type, std::function<std::shared_ptr<void>()> factory) override {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_[type] = factory;
        LOG(INFO) << "Registered factory service: " << type.name();
    }

    std::shared_ptr<void> get(const std::type_index& type) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 首先检查单例
        auto singleton_it = singletons_.find(type);
        if (singleton_it != singletons_.end()) {
            return singleton_it->second;
        }
        
        // 然后检查工厂
        auto factory_it = factories_.find(type);
        if (factory_it != factories_.end()) {
            auto instance = factory_it->second();
            LOG(INFO) << "Created service instance via factory: " << type.name();
            return instance;
        }
        
        LOG(WARNING) << "Service not found: " << type.name();
        return nullptr;
    }

    bool has(const std::type_index& type) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return singletons_.find(type) != singletons_.end() || 
               factories_.find(type) != factories_.end();
    }

    void clear() override {
        std::lock_guard<std::mutex> lock(mutex_);
        singletons_.clear();
        factories_.clear();
        LOG(INFO) << "Service container cleared";
    }
};

// 服务定位器 - 全局访问点
class ServiceLocator {
private:
    static std::unique_ptr<IServiceContainer> container_;
    static std::mutex container_mutex_;

public:
    // 设置容器
    static void set_container(std::unique_ptr<IServiceContainer> container) {
        std::lock_guard<std::mutex> lock(container_mutex_);
        container_ = std::move(container);
    }

    // 获取容器
    static IServiceContainer* get_container() {
        std::lock_guard<std::mutex> lock(container_mutex_);
        if (!container_) {
            container_ = std::make_unique<ServiceContainer>();
            LOG(INFO) << "Created default service container";
        }
        return container_.get();
    }

    // 注册服务
    template<typename T>
    static void register_singleton(std::shared_ptr<T> instance) {
        auto container = get_container();
        container->register_singleton(std::type_index(typeid(T)), instance);
    }

    template<typename T>
    static void register_factory(std::function<std::shared_ptr<T>()> factory) {
        auto container = get_container();
        container->register_factory(std::type_index(typeid(T)), 
            [factory]() -> std::shared_ptr<void> { return factory(); });
    }

    // 获取服务
    template<typename T>
    static std::shared_ptr<T> get() {
        auto container = get_container();
        auto instance = container->get(std::type_index(typeid(T)));
        return std::static_pointer_cast<T>(instance);
    }

    // 检查服务是否存在
    template<typename T>
    static bool has() {
        auto container = get_container();
        return container->has(std::type_index(typeid(T)));
    }

    // 清理
    static void clear() {
        std::lock_guard<std::mutex> lock(container_mutex_);
        if (container_) {
            container_->clear();
        }
    }
};

// 服务基类
class IService {
public:
    virtual ~IService() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual std::string get_name() const = 0;
};

// 服务管理器
class ServiceManager {
private:
    std::vector<std::shared_ptr<IService>> services_;
    mutable std::mutex services_mutex_;

public:
    template<typename T>
    void register_service(std::shared_ptr<T> service) {
        static_assert(std::is_base_of_v<IService, T>, "Service must inherit from IService");
        
        std::lock_guard<std::mutex> lock(services_mutex_);
        services_.push_back(service);
        
        // 注册到服务容器
        ServiceLocator::register_singleton<T>(service);
        
        LOG(INFO) << "Registered service: " << service->get_name();
    }

    bool initialize_all() {
        std::lock_guard<std::mutex> lock(services_mutex_);
        
        for (auto& service : services_) {
            if (!service->initialize()) {
                LOG(ERROR) << "Failed to initialize service: " << service->get_name();
                return false;
            }
        }
        
        LOG(INFO) << "All services initialized successfully";
        return true;
    }

    void shutdown_all() {
        std::lock_guard<std::mutex> lock(services_mutex_);
        
        // 逆序关闭服务
        for (auto it = services_.rbegin(); it != services_.rend(); ++it) {
            try {
                (*it)->shutdown();
            } catch (const std::exception& e) {
                LOG(ERROR) << "Exception during service shutdown: " << e.what();
            }
        }
        
        services_.clear();
        LOG(INFO) << "All services shut down";
    }

    std::vector<std::string> get_service_names() const {
        std::lock_guard<std::mutex> lock(services_mutex_);
        std::vector<std::string> names;
        for (const auto& service : services_) {
            names.push_back(service->get_name());
        }
        return names;
    }
};

// 全局服务管理器实例
extern std::unique_ptr<ServiceManager> g_service_manager;

// 初始化服务管理器
bool initialize_service_manager();

// 清理服务管理器
void cleanup_service_manager();

} // namespace Paker
