#include <gtest/gtest.h>
#include "Paker/core/package_manager.h"
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

using namespace Paker;

class MemoryManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保测试环境干净
        cleanup_resolver();
    }
    
    void TearDown() override {
        // 清理测试环境
        cleanup_resolver();
    }
};

// 测试智能指针的基本功能
TEST_F(MemoryManagementTest, SmartPointerBasicFunctionality) {
    // 测试获取解析器
    auto* resolver1 = get_resolver();
    ASSERT_NE(resolver1, nullptr);
    
    // 再次获取应该返回同一个实例
    auto* resolver2 = get_resolver();
    ASSERT_EQ(resolver1, resolver2);
    
    // 测试获取依赖图
    auto* graph1 = get_dependency_graph();
    ASSERT_NE(graph1, nullptr);
    
    auto* graph2 = get_dependency_graph();
    ASSERT_EQ(graph1, graph2);
}

// 测试线程安全性
TEST_F(MemoryManagementTest, ThreadSafety) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<DependencyResolver*> resolvers(num_threads);
    std::vector<DependencyGraph*> graphs(num_threads);
    
    // 启动多个线程同时获取解析器
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&resolvers, &graphs, i]() {
            resolvers[i] = get_resolver();
            graphs[i] = get_dependency_graph();
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有线程获取的是同一个实例
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(resolvers[0], resolvers[i]) << "All threads should get the same resolver instance";
        EXPECT_EQ(graphs[0], graphs[i]) << "All threads should get the same graph instance";
    }
    
    // 验证实例不为空
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_NE(resolvers[i], nullptr) << "Resolver should not be null";
        EXPECT_NE(graphs[i], nullptr) << "Graph should not be null";
    }
}

// 测试清理和重新初始化
TEST_F(MemoryManagementTest, CleanupAndReinitialization) {
    // 获取初始实例
    auto* resolver1 = get_resolver();
    auto* graph1 = get_dependency_graph();
    
    ASSERT_NE(resolver1, nullptr);
    ASSERT_NE(graph1, nullptr);
    
    // 清理
    cleanup_resolver();
    
    // 重新获取实例
    auto* resolver2 = get_resolver();
    auto* graph2 = get_dependency_graph();
    
    // 应该是新的实例（地址不同）
    EXPECT_NE(resolver1, resolver2) << "After cleanup, should get new resolver instance";
    EXPECT_NE(graph1, graph2) << "After cleanup, should get new graph instance";
    
    // 但实例不应为空
    EXPECT_NE(resolver2, nullptr) << "New resolver should not be null";
    EXPECT_NE(graph2, nullptr) << "New graph should not be null";
}

// 测试异常安全性
TEST_F(MemoryManagementTest, ExceptionSafety) {
    // 测试在异常情况下内存管理的安全性
    try {
        auto* resolver = get_resolver();
        ASSERT_NE(resolver, nullptr);
        
        // 模拟异常
        throw std::runtime_error("Test exception");
    } catch (const std::exception&) {
        // 异常被捕获，但解析器应该仍然有效
        auto* resolver_after_exception = get_resolver();
        EXPECT_NE(resolver_after_exception, nullptr) << "Resolver should still be valid after exception";
    }
}

// 测试多次初始化的幂等性
TEST_F(MemoryManagementTest, IdempotentInitialization) {
    // 多次调用应该不会创建多个实例
    auto* resolver1 = get_resolver();
    auto* resolver2 = get_resolver();
    auto* resolver3 = get_resolver();
    
    EXPECT_EQ(resolver1, resolver2) << "Multiple calls should return same instance";
    EXPECT_EQ(resolver2, resolver3) << "Multiple calls should return same instance";
    
    auto* graph1 = get_dependency_graph();
    auto* graph2 = get_dependency_graph();
    auto* graph3 = get_dependency_graph();
    
    EXPECT_EQ(graph1, graph2) << "Multiple calls should return same graph instance";
    EXPECT_EQ(graph2, graph3) << "Multiple calls should return same graph instance";
}

// 测试并发访问的性能
TEST_F(MemoryManagementTest, ConcurrentAccessPerformance) {
    const int num_threads = 8;
    const int iterations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::chrono::milliseconds> thread_times(num_threads);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&thread_times, i, iterations_per_thread]() {
            auto thread_start = std::chrono::high_resolution_clock::now();
            
            for (int j = 0; j < iterations_per_thread; ++j) {
                auto* resolver = get_resolver();
                auto* graph = get_dependency_graph();
                (void)resolver; // 避免未使用变量警告
                (void)graph;
            }
            
            auto thread_end = std::chrono::high_resolution_clock::now();
            thread_times[i] = std::chrono::duration_cast<std::chrono::milliseconds>(
                thread_end - thread_start);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // 验证性能（总时间应该合理）
    EXPECT_LT(total_time.count(), 1000) << "Concurrent access should be reasonably fast";
    
    // 验证所有线程都成功完成
    for (const auto& time : thread_times) {
        EXPECT_GT(time.count(), 0) << "Each thread should have taken some time";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
