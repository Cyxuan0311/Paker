#include <gtest/gtest.h>
#include "Paker/core/async_io.h"
#include "Paker/cache/async_cache_manager.h"
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>

namespace Paker {

class AsyncIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = "/tmp/paker_test_async_io";
        std::filesystem::create_directories(test_dir_);
        
        // 初始化异步I/O管理器
        async_io_manager_ = std::make_unique<AsyncIOManager>(4, 10);
        async_io_manager_->initialize();
        
        // 初始化异步缓存管理器
        async_cache_manager_ = std::make_unique<AsyncCacheManager>(async_io_manager_.get());
        async_cache_manager_->initialize();
    }
    
    void TearDown() override {
        if (async_cache_manager_) {
            async_cache_manager_->shutdown();
        }
        
        if (async_io_manager_) {
            async_io_manager_->shutdown();
        }
        
        // 清理测试目录
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
    std::unique_ptr<AsyncIOManager> async_io_manager_;
    std::unique_ptr<AsyncCacheManager> async_cache_manager_;
};

TEST_F(AsyncIOTest, InitializationAndShutdown) {
    ASSERT_NE(async_io_manager_, nullptr);
    ASSERT_TRUE(async_io_manager_->is_running());
    
    ASSERT_NE(async_cache_manager_, nullptr);
}

TEST_F(AsyncIOTest, AsyncFileReadWrite) {
    std::string test_file = test_dir_ + "/test_file.txt";
    std::string test_content = "This is a test file for async I/O operations.\n";
    test_content += "Testing async file read and write operations.\n";
    
    // 测试异步写入
    auto write_future = async_io_manager_->write_file_async(test_file, test_content);
    auto write_result = write_future.get();
    
    ASSERT_TRUE(write_result);
    ASSERT_EQ(write_result->status, IOOperationStatus::COMPLETED);
    ASSERT_EQ(write_result->bytes_written, test_content.size());
    
    // 测试异步读取
    auto read_future = async_io_manager_->read_file_async(test_file, true);
    auto read_result = read_future.get();
    
    ASSERT_TRUE(read_result);
    ASSERT_EQ(read_result->status, IOOperationStatus::COMPLETED);
    ASSERT_EQ(read_result->content, test_content);
    ASSERT_EQ(read_result->bytes_processed, test_content.size());
}

TEST_F(AsyncIOTest, AsyncBinaryFileOperations) {
    std::string test_file = test_dir_ + "/test_binary.bin";
    std::vector<char> test_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    
    // 测试异步二进制写入
    auto write_future = async_io_manager_->write_file_async(test_file, test_data);
    auto write_result = write_future.get();
    
    ASSERT_TRUE(write_result);
    ASSERT_EQ(write_result->status, IOOperationStatus::COMPLETED);
    ASSERT_EQ(write_result->bytes_written, test_data.size());
    
    // 测试异步二进制读取
    auto read_future = async_io_manager_->read_file_async(test_file, false);
    auto read_result = read_future.get();
    
    ASSERT_TRUE(read_result);
    ASSERT_EQ(read_result->status, IOOperationStatus::COMPLETED);
    ASSERT_EQ(read_result->data, test_data);
    ASSERT_EQ(read_result->bytes_processed, test_data.size());
}

TEST_F(AsyncIOTest, BatchAsyncOperations) {
    const int num_files = 10;
    std::vector<std::string> test_files;
    std::vector<std::pair<std::string, std::string>> test_data;
    
    // 准备测试数据
    for (int i = 0; i < num_files; ++i) {
        std::string filename = test_dir_ + "/batch_test_" + std::to_string(i) + ".txt";
        std::string content = "Batch test file " + std::to_string(i) + " content.\n";
        test_files.push_back(filename);
        test_data.emplace_back(filename, content);
    }
    
    // 批量异步写入
    auto write_futures = async_io_manager_->write_files_async(test_data);
    for (auto& future : write_futures) {
        auto result = future.get();
        ASSERT_TRUE(result);
        ASSERT_EQ(result->status, IOOperationStatus::COMPLETED);
    }
    
    // 批量异步读取
    auto read_futures = async_io_manager_->read_files_async(test_files, true);
    for (size_t i = 0; i < read_futures.size(); ++i) {
        auto result = read_futures[i].get();
        ASSERT_TRUE(result);
        ASSERT_EQ(result->status, IOOperationStatus::COMPLETED);
        ASSERT_EQ(result->content, test_data[i].second);
    }
}

TEST_F(AsyncIOTest, AsyncCacheOperations) {
    std::string cache_key = "test_cache_key";
    std::string cache_content = "This is cached content for testing.\n";
    
    // 测试异步缓存写入
    auto write_future = async_cache_manager_->write_cache_async(cache_key, cache_content);
    auto write_result = write_future.get();
    
    ASSERT_TRUE(write_result);
    ASSERT_TRUE(write_result->success);
    ASSERT_EQ(write_result->cache_key, cache_key);
    
    // 测试异步缓存读取
    auto read_future = async_cache_manager_->read_cache_async(cache_key, true);
    auto read_result = read_future.get();
    
    ASSERT_TRUE(read_result);
    ASSERT_TRUE(read_result->success);
    ASSERT_EQ(read_result->content, cache_content);
}

TEST_F(AsyncIOTest, AsyncCacheBatchOperations) {
    const int num_entries = 5;
    std::vector<std::string> cache_keys;
    std::vector<std::pair<std::string, std::string>> cache_data;
    
    // 准备缓存数据
    for (int i = 0; i < num_entries; ++i) {
        std::string key = "batch_cache_" + std::to_string(i);
        std::string content = "Batch cache content " + std::to_string(i) + ".\n";
        cache_keys.push_back(key);
        cache_data.emplace_back(key, content);
    }
    
    // 批量异步缓存写入
    auto write_futures = async_cache_manager_->write_multiple_cache_async(cache_data);
    for (auto& future : write_futures) {
        auto result = future.get();
        ASSERT_TRUE(result);
        ASSERT_TRUE(result->success);
    }
    
    // 批量异步缓存读取
    auto read_futures = async_cache_manager_->read_multiple_cache_async(cache_keys, true);
    for (size_t i = 0; i < read_futures.size(); ++i) {
        auto result = read_futures[i].get();
        ASSERT_TRUE(result);
        ASSERT_TRUE(result->success);
        ASSERT_EQ(result->content, cache_data[i].second);
    }
}

TEST_F(AsyncIOTest, PerformanceStatistics) {
    // 执行一些操作来生成统计信息
    std::string test_file = test_dir_ + "/stats_test.txt";
    std::string test_content = "Performance statistics test content.\n";
    
    auto write_future = async_io_manager_->write_file_async(test_file, test_content);
    write_future.get();
    
    auto read_future = async_io_manager_->read_file_async(test_file, true);
    read_future.get();
    
    // 检查统计信息
    ASSERT_GT(async_io_manager_->get_total_operations(), 0);
    ASSERT_GT(async_io_manager_->get_completed_operations(), 0);
    ASSERT_GE(async_io_manager_->get_success_rate(), 0.0);
    ASSERT_LE(async_io_manager_->get_success_rate(), 100.0);
    
    // 检查性能报告
    std::string report = async_io_manager_->get_performance_report();
    ASSERT_FALSE(report.empty());
    ASSERT_NE(report.find("AsyncIO Performance Report:"), std::string::npos);
}

TEST_F(AsyncIOTest, CacheStatistics) {
    // 执行一些缓存操作来生成统计信息
    std::string cache_key = "stats_cache_key";
    std::string cache_content = "Cache statistics test content.\n";
    
    auto write_future = async_cache_manager_->write_cache_async(cache_key, cache_content);
    write_future.get();
    
    auto read_future = async_cache_manager_->read_cache_async(cache_key, true);
    read_future.get();
    
    // 检查缓存统计信息
    ASSERT_GT(async_cache_manager_->get_total_reads(), 0);
    ASSERT_GT(async_cache_manager_->get_total_writes(), 0);
    ASSERT_GT(async_cache_manager_->get_cache_hits(), 0);
    ASSERT_GE(async_cache_manager_->get_cache_hit_rate(), 0.0);
    ASSERT_LE(async_cache_manager_->get_cache_hit_rate(), 100.0);
    
    // 检查性能报告
    std::string report = async_cache_manager_->get_performance_report();
    ASSERT_FALSE(report.empty());
    ASSERT_NE(report.find("AsyncCache Performance Report:"), std::string::npos);
}

TEST_F(AsyncIOTest, ConcurrentOperations) {
    const int num_threads = 10;
    const int operations_per_thread = 5;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // 创建多个线程同时执行异步操作
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, operations_per_thread, &success_count]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string filename = test_dir_ + "/concurrent_" + std::to_string(i) + "_" + std::to_string(j) + ".txt";
                std::string content = "Concurrent test content " + std::to_string(i) + "_" + std::to_string(j) + ".\n";
                
                auto write_future = async_io_manager_->write_file_async(filename, content);
                auto write_result = write_future.get();
                
                if (write_result && write_result->status == IOOperationStatus::COMPLETED) {
                    success_count++;
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有操作都成功
    ASSERT_EQ(success_count.load(), num_threads * operations_per_thread);
}

TEST_F(AsyncIOTest, ErrorHandling) {
    // 测试不存在的文件读取
    std::string non_existent_file = test_dir_ + "/non_existent.txt";
    auto read_future = async_io_manager_->read_file_async(non_existent_file, true);
    auto read_result = read_future.get();
    
    ASSERT_TRUE(read_result);
    ASSERT_EQ(read_result->status, IOOperationStatus::FAILED);
    ASSERT_FALSE(read_result->content.empty()); // 应该有错误信息
}

// 性能测试
TEST(AsyncIOPerformanceTest, AsyncVsSyncComparison) {
    const int num_files = 50;
    const std::string test_content(1024, 'A'); // 1KB content
    
    // 创建临时目录
    std::string test_dir = "/tmp/paker_perf_test";
    std::filesystem::create_directories(test_dir);
    
    // 异步I/O测试
    auto async_manager = std::make_unique<AsyncIOManager>(4, 10);
    async_manager->initialize();
    
    std::vector<std::pair<std::string, std::string>> test_data;
    for (int i = 0; i < num_files; ++i) {
        std::string filename = test_dir + "/async_" + std::to_string(i) + ".txt";
        test_data.emplace_back(filename, test_content);
    }
    
    auto async_start = std::chrono::high_resolution_clock::now();
    auto write_futures = async_manager->write_files_async(test_data);
    for (auto& future : write_futures) {
        future.get();
    }
    auto async_end = std::chrono::high_resolution_clock::now();
    auto async_time = std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
    
    // 同步I/O测试
    auto sync_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_files; ++i) {
        std::string filename = test_dir + "/sync_" + std::to_string(i) + ".txt";
        std::ofstream file(filename);
        file << test_content;
        file.close();
    }
    auto sync_end = std::chrono::high_resolution_clock::now();
    auto sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(sync_end - sync_start);
    
    // 验证异步I/O性能更好（或至少不差）
    ASSERT_LE(async_time.count(), sync_time.count() * 2); // 允许一些误差
    
    async_manager->shutdown();
    std::filesystem::remove_all(test_dir);
}

} // namespace Paker
