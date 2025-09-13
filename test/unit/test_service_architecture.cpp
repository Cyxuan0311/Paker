#include <gtest/gtest.h>
#include "Paker/core/service_container.h"
#include "Paker/core/core_services.h"
#include <memory>
#include <thread>
#include <vector>

using namespace Paker;

class ServiceArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保测试环境干净
        cleanup_service_manager();
    }
    
    void TearDown() override {
        // 清理测试环境
        cleanup_service_manager();
    }
};

// 测试服务容器基本功能
TEST_F(ServiceArchitectureTest, ServiceContainerBasicFunctionality) {
    auto container = std::make_unique<ServiceContainer>();
    
    // 注册单例服务
    auto test_service = std::make_shared<std::string>("test_value");
    container->register_singleton(std::type_index(typeid(std::string)), test_service);
    
    // 获取服务
    auto retrieved_service = container->get(std::type_index(typeid(std::string)));
    ASSERT_NE(retrieved_service, nullptr);
    
    auto* string_ptr = static_cast<std::string*>(retrieved_service.get());
    ASSERT_EQ(*string_ptr, "test_value");
}

// 测试服务工厂
TEST_F(ServiceArchitectureTest, ServiceFactory) {
    auto container = std::make_unique<ServiceContainer>();
    
    // 注册工厂
    container->register_factory(std::type_index(typeid(std::string)), 
        []() -> std::shared_ptr<void> {
            return std::make_shared<std::string>("factory_created");
        });
    
    // 通过工厂创建服务
    auto service1 = container->get(std::type_index(typeid(std::string)));
    auto service2 = container->get(std::type_index(typeid(std::string)));
    
    ASSERT_NE(service1, nullptr);
    ASSERT_NE(service2, nullptr);
    
    // 每次调用工厂都应该创建新实例
    auto* str1 = static_cast<std::string*>(service1.get());
    auto* str2 = static_cast<std::string*>(service2.get());
    
    ASSERT_EQ(*str1, "factory_created");
    ASSERT_EQ(*str2, "factory_created");
    ASSERT_NE(str1, str2); // 不同的实例
}

// 测试服务定位器
TEST_F(ServiceArchitectureTest, ServiceLocator) {
    // 设置自定义容器
    auto container = std::make_unique<ServiceContainer>();
    ServiceLocator::set_container(std::move(container));
    
    // 注册服务
    auto test_service = std::make_shared<std::string>("locator_test");
    ServiceLocator::register_singleton<std::string>(test_service);
    
    // 获取服务
    auto retrieved = ServiceLocator::get<std::string>();
    ASSERT_NE(retrieved, nullptr);
    ASSERT_EQ(*retrieved, "locator_test");
    
    // 检查服务是否存在
    ASSERT_TRUE(ServiceLocator::has<std::string>());
    ASSERT_FALSE(ServiceLocator::has<int>());
}

// 测试核心服务
TEST_F(ServiceArchitectureTest, CoreServices) {
    // 初始化服务管理器
    ASSERT_TRUE(initialize_service_manager());
    
    // 注册核心服务
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    // 测试依赖解析服务
    auto* resolver = get_dependency_resolver();
    ASSERT_NE(resolver, nullptr);
    
    auto* graph = get_dependency_graph();
    ASSERT_NE(graph, nullptr);
    
    // 测试缓存管理服务
    auto* cache_manager = get_cache_manager();
    ASSERT_NE(cache_manager, nullptr);
    
    // 测试并行执行服务
    auto* executor = get_parallel_executor();
    ASSERT_NE(executor, nullptr);
    
    // 测试性能监控服务
    auto* monitor = get_performance_monitor();
    ASSERT_NE(monitor, nullptr);
    
    // 测试增量更新服务
    auto* updater = get_incremental_updater();
    ASSERT_NE(updater, nullptr);
}

// 测试服务生命周期
TEST_F(ServiceArchitectureTest, ServiceLifecycle) {
    // 初始化服务
    ASSERT_TRUE(initialize_service_manager());
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    // 验证服务可用
    ASSERT_NE(get_dependency_resolver(), nullptr);
    ASSERT_NE(get_cache_manager(), nullptr);
    
    // 清理服务
    cleanup_service_manager();
    
    // 验证服务已清理
    ASSERT_EQ(get_dependency_resolver(), nullptr);
    ASSERT_EQ(get_cache_manager(), nullptr);
}

// 测试线程安全性
TEST_F(ServiceArchitectureTest, ThreadSafety) {
    ASSERT_TRUE(initialize_service_manager());
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<DependencyResolver*> resolvers(num_threads);
    std::vector<CacheManager*> caches(num_threads);
    
    // 启动多个线程同时访问服务
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&resolvers, &caches, i]() {
            resolvers[i] = get_dependency_resolver();
            caches[i] = get_cache_manager();
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有线程获取的是同一个实例
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(resolvers[0], resolvers[i]) << "All threads should get the same resolver instance";
        EXPECT_EQ(caches[0], caches[i]) << "All threads should get the same cache instance";
    }
    
    // 验证实例不为空
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_NE(resolvers[i], nullptr) << "Resolver should not be null";
        EXPECT_NE(caches[i], nullptr) << "Cache manager should not be null";
    }
}

// 测试服务依赖
TEST_F(ServiceArchitectureTest, ServiceDependencies) {
    ASSERT_TRUE(initialize_service_manager());
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    // 测试服务之间的依赖关系
    auto* resolver = get_dependency_resolver();
    auto* graph = get_dependency_graph();
    auto* cache = get_cache_manager();
    auto* executor = get_parallel_executor();
    auto* monitor = get_performance_monitor();
    
    // 所有服务都应该可用
    ASSERT_NE(resolver, nullptr);
    ASSERT_NE(graph, nullptr);
    ASSERT_NE(cache, nullptr);
    ASSERT_NE(executor, nullptr);
    ASSERT_NE(monitor, nullptr);
    
    // 验证服务状态
    ASSERT_TRUE(executor->is_running());
    ASSERT_TRUE(monitor->is_enabled());
}

// 测试异常安全性
TEST_F(ServiceArchitectureTest, ExceptionSafety) {
    // 测试在异常情况下服务的清理
    try {
        ASSERT_TRUE(initialize_service_manager());
        ASSERT_TRUE(ServiceFactory::register_all_core_services());
        
        // 模拟异常
        throw std::runtime_error("Test exception");
    } catch (const std::exception&) {
        // 异常被捕获，但服务应该仍然有效
        auto* resolver = get_dependency_resolver();
        ASSERT_NE(resolver, nullptr);
    }
    
    // 清理服务
    cleanup_service_manager();
}

// 测试服务重新初始化
TEST_F(ServiceArchitectureTest, ServiceReinitialization) {
    // 第一次初始化
    ASSERT_TRUE(initialize_service_manager());
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    auto* resolver1 = get_dependency_resolver();
    ASSERT_NE(resolver1, nullptr);
    
    // 清理
    cleanup_service_manager();
    
    // 重新初始化
    ASSERT_TRUE(initialize_service_manager());
    ASSERT_TRUE(ServiceFactory::register_all_core_services());
    
    auto* resolver2 = get_dependency_resolver();
    ASSERT_NE(resolver2, nullptr);
    
    // 应该是新的实例
    EXPECT_NE(resolver1, resolver2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
