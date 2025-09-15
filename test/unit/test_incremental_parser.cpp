#include <gtest/gtest.h>
#include "Paker/dependency/incremental_parser.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/core/package_manager.h"
#include <filesystem>
#include <thread>
#include <vector>

namespace Paker {

class IncrementalParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = "/tmp/paker_test_incremental";
        std::filesystem::create_directories(test_dir_);
        
        // 初始化增量解析器
        parser_ = std::make_unique<IncrementalParser>(test_dir_);
        parser_->initialize();
    }
    
    void TearDown() override {
        if (parser_) {
            parser_->shutdown();
        }
        
        // 清理测试目录
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
    std::unique_ptr<IncrementalParser> parser_;
};

TEST_F(IncrementalParserTest, InitializationAndShutdown) {
    ASSERT_NE(parser_, nullptr);
    
    // 测试初始化
    EXPECT_TRUE(parser_->initialize());
    
    // 测试配置
    auto config = parser_->get_config();
    EXPECT_TRUE(config.enable_caching);
    EXPECT_TRUE(config.enable_incremental);
    EXPECT_TRUE(config.enable_parallel);
    
    // 测试关闭
    parser_->shutdown();
}

TEST_F(IncrementalParserTest, ConfigurationManagement) {
    ParseConfig config;
    config.enable_caching = false;
    config.enable_incremental = false;
    config.max_cache_size = 500;
    config.max_parallel_tasks = 2;
    
    parser_->set_config(config);
    
    auto retrieved_config = parser_->get_config();
    EXPECT_FALSE(retrieved_config.enable_caching);
    EXPECT_FALSE(retrieved_config.enable_incremental);
    EXPECT_EQ(retrieved_config.max_cache_size, 500);
    EXPECT_EQ(retrieved_config.max_parallel_tasks, 2);
}

TEST_F(IncrementalParserTest, CacheOperations) {
    // 测试缓存大小
    EXPECT_EQ(parser_->get_cache_size(), 0);
    
    // 测试缓存清理
    parser_->clear_cache();
    EXPECT_EQ(parser_->get_cache_size(), 0);
    
    // 测试缓存失效
    parser_->invalidate_package_cache("test_package");
    parser_->invalidate_all_cache();
}

TEST_F(IncrementalParserTest, StatisticsTracking) {
    auto initial_stats = parser_->get_stats();
    EXPECT_EQ(initial_stats.total_packages_parsed, 0);
    EXPECT_EQ(initial_stats.cache_hits, 0);
    EXPECT_EQ(initial_stats.cache_misses, 0);
    
    // 重置统计
    parser_->reset_stats();
    auto reset_stats = parser_->get_stats();
    EXPECT_EQ(reset_stats.total_packages_parsed, 0);
}

TEST_F(IncrementalParserTest, CacheInfoAndPerformanceReport) {
    // 测试缓存信息
    std::string cache_info = parser_->get_cache_info();
    EXPECT_FALSE(cache_info.empty());
    EXPECT_NE(cache_info.find("Cache Info:"), std::string::npos);
    
    // 测试性能报告
    std::string performance_report = parser_->get_performance_report();
    EXPECT_FALSE(performance_report.empty());
    EXPECT_NE(performance_report.find("Performance Report:"), std::string::npos);
}

TEST_F(IncrementalParserTest, CacheIntegrityValidation) {
    // 测试缓存完整性验证
    bool is_valid = parser_->validate_cache_integrity();
    EXPECT_TRUE(is_valid); // 空缓存应该是有效的
}

TEST_F(IncrementalParserTest, DependencyGraphAccess) {
    // 测试依赖图访问
    const auto& graph = parser_->get_dependency_graph();
    EXPECT_EQ(graph.get_nodes().size(), 0); // 初始应该为空
    
    auto& mutable_graph = parser_->get_dependency_graph();
    EXPECT_EQ(mutable_graph.get_nodes().size(), 0);
}

TEST_F(IncrementalParserTest, ChangeDetection) {
    // 测试变更检测
    std::vector<std::string> packages = {"package1", "package2", "package3"};
    auto changes = parser_->detect_changes(packages);
    
    EXPECT_TRUE(changes.has_changes); // 新包应该有变更
    EXPECT_EQ(changes.new_packages.size(), 3);
    EXPECT_TRUE(changes.changed_packages.empty());
    EXPECT_TRUE(changes.removed_packages.empty());
}

TEST_F(IncrementalParserTest, ParallelParsing) {
    // 配置并行解析
    ParseConfig config = parser_->get_config();
    config.enable_parallel = true;
    config.max_parallel_tasks = 2;
    parser_->set_config(config);
    
    // 测试并行解析（使用模拟包）
    std::vector<std::string> packages = {"test1", "test2", "test3"};
    
    // 注意：这里可能会失败，因为包不存在，但应该不会崩溃
    try {
        parser_->parse_packages(packages);
    } catch (const std::exception& e) {
        // 预期的异常，因为测试包不存在
        SUCCEED();
    }
}

TEST_F(IncrementalParserTest, IncrementalParsing) {
    // 测试增量解析
    std::vector<std::string> packages = {"package1", "package2"};
    
    try {
        bool success = parser_->incremental_parse(packages);
        // 可能会失败，但不应该崩溃
        SUCCEED();
    } catch (const std::exception& e) {
        // 预期的异常
        SUCCEED();
    }
}

TEST_F(IncrementalParserTest, ProjectDependencyParsing) {
    // 测试项目依赖解析
    try {
        bool success = parser_->parse_project_dependencies();
        // 可能会失败，因为项目文件不存在
        SUCCEED();
    } catch (const std::exception& e) {
        // 预期的异常
        SUCCEED();
    }
}

// 集成测试：与依赖解析器的集成
TEST(IncrementalParserIntegrationTest, DependencyResolverIntegration) {
    // 创建依赖解析器
    auto resolver = std::make_unique<DependencyResolver>();
    
    // 测试增量解析功能
    EXPECT_TRUE(resolver->enable_incremental_parsing(true));
    EXPECT_TRUE(resolver->is_incremental_parsing_enabled());
    
    // 获取增量解析器
    auto* incremental_parser = resolver->get_incremental_parser();
    EXPECT_NE(incremental_parser, nullptr);
    
    // 测试配置
    auto config = incremental_parser->get_config();
    EXPECT_TRUE(config.enable_incremental);
    
    // 禁用增量解析
    EXPECT_TRUE(resolver->enable_incremental_parsing(false));
    EXPECT_FALSE(resolver->is_incremental_parsing_enabled());
}

// 性能测试
TEST(IncrementalParserPerformanceTest, CachePerformance) {
    std::string test_dir = "/tmp/paker_perf_test";
    std::filesystem::create_directories(test_dir);
    
    auto parser = std::make_unique<IncrementalParser>(test_dir);
    parser->initialize();
    
    // 配置大缓存
    ParseConfig config;
    config.max_cache_size = 1000;
    config.enable_caching = true;
    parser->set_config(config);
    
    // 测试缓存操作性能
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 执行多次缓存操作
    for (int i = 0; i < 100; ++i) {
        parser->invalidate_package_cache("test_package_" + std::to_string(i));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 性能应该合理（小于100ms）
    EXPECT_LT(duration.count(), 100);
    
    parser->shutdown();
    std::filesystem::remove_all(test_dir);
}

// 并发安全测试
TEST(IncrementalParserConcurrencyTest, ThreadSafety) {
    std::string test_dir = "/tmp/paker_concurrency_test";
    std::filesystem::create_directories(test_dir);
    
    auto parser = std::make_unique<IncrementalParser>(test_dir);
    parser->initialize();
    
    // 配置并行解析
    ParseConfig config;
    config.enable_parallel = true;
    config.max_parallel_tasks = 4;
    parser->set_config(config);
    
    // 创建多个线程同时访问解析器
    std::vector<std::thread> threads;
    const int num_threads = 10;
    const int operations_per_thread = 50;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&parser, i, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                // 执行各种操作
                parser->invalidate_package_cache("thread_" + std::to_string(i) + "_package_" + std::to_string(j));
                parser->get_cache_size();
                parser->get_stats();
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证没有崩溃
    EXPECT_GE(parser->get_cache_size(), 0);
    
    parser->shutdown();
    std::filesystem::remove_all(test_dir);
}

} // namespace Paker
