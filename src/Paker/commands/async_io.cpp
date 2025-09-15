#include "Paker/commands/async_io.h"
#include "Paker/core/async_io.h"
#include "Paker/cache/async_cache_manager.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <chrono>
#include <random>
#include <fstream>
#include <filesystem>

namespace Paker {

void pm_async_io_stats() {
    LOG(INFO) << "Displaying async I/O statistics";
    
    auto* async_io_manager = get_async_io_manager();
    auto* async_cache_manager = get_async_cache_manager();
    
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("⚡ 异步I/O统计信息");
        Output::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // 异步I/O统计
        Output::info("📊 异步I/O统计:");
        Output::info("  总操作数: " + std::to_string(async_io_manager->get_total_operations()));
        Output::info("  已完成操作: " + std::to_string(async_io_manager->get_completed_operations()));
        Output::info("  失败操作: " + std::to_string(async_io_manager->get_failed_operations()));
        Output::info("  活跃操作: " + std::to_string(async_io_manager->get_active_operations()));
        Output::info("  队列大小: " + std::to_string(async_io_manager->get_queue_size()));
        Output::info("  成功率: " + std::to_string(async_io_manager->get_success_rate()) + "%");
        Output::info("  平均操作时间: " + std::to_string(async_io_manager->get_average_operation_time()) + "ms");
        
        // 异步缓存统计
        if (async_cache_manager) {
            Output::info("💾 异步缓存统计:");
            Output::info("  总读取: " + std::to_string(async_cache_manager->get_total_reads()));
            Output::info("  总写入: " + std::to_string(async_cache_manager->get_total_writes()));
            Output::info("  缓存命中: " + std::to_string(async_cache_manager->get_cache_hits()));
            Output::info("  缓存未命中: " + std::to_string(async_cache_manager->get_cache_misses()));
            Output::info("  缓存命中率: " + std::to_string(async_cache_manager->get_cache_hit_rate()) + "%");
            Output::info("  异步操作: " + std::to_string(async_cache_manager->get_async_operations()));
            Output::info("  平均读取时间: " + std::to_string(async_cache_manager->get_average_read_time()) + "ms");
            Output::info("  平均写入时间: " + std::to_string(async_cache_manager->get_average_write_time()) + "ms");
        }
        
        // 显示详细报告
        std::string performance_report = async_io_manager->get_performance_report();
        std::istringstream report_stream(performance_report);
        std::string line;
        while (std::getline(report_stream, line)) {
            if (!line.empty()) {
                Output::info("  " + line);
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to get async I/O statistics: " << e.what();
        Output::error("获取统计信息失败: " + std::string(e.what()));
    }
}

void pm_async_io_config() {
    LOG(INFO) << "Displaying async I/O configuration";
    
    auto* async_io_manager = get_async_io_manager();
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("⚙️ 异步I/O配置");
        Output::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        Output::info("🔧 线程配置:");
        Output::info("  最大并发操作: " + std::to_string(async_io_manager->get_max_concurrent_operations()));
        Output::info("  硬件并发数: " + std::to_string(std::thread::hardware_concurrency()));
        
        Output::info("📊 性能配置:");
        Output::info("  当前队列大小: " + std::to_string(async_io_manager->get_queue_size()));
        Output::info("  活跃操作数: " + std::to_string(async_io_manager->get_active_operations()));
        
        Output::info("💡 优化建议:");
        if (async_io_manager->get_queue_size() > 100) {
            Output::info("  ⚠️ 队列积压较多，建议增加工作线程");
        }
        if (async_io_manager->get_success_rate() < 90.0) {
            Output::info("  ⚠️ 成功率较低，建议检查I/O操作");
        }
        if (async_io_manager->get_average_operation_time() > 1000) {
            Output::info("  ⚠️ 平均操作时间较长，建议优化I/O性能");
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to get async I/O configuration: " << e.what();
        Output::error("获取配置信息失败: " + std::string(e.what()));
    }
}

void pm_async_io_test() {
    LOG(INFO) << "Running async I/O test";
    
    auto* async_io_manager = get_async_io_manager();
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("🧪 开始异步I/O测试...");
        
        // 创建测试文件
        std::string test_content = "This is a test file for async I/O operations.\n";
        test_content += "Testing async file read and write operations.\n";
        test_content += "Performance should be significantly improved with async I/O.\n";
        
        std::string test_file = "/tmp/paker_async_test.txt";
        
        // 测试异步写入
        Output::info("📝 测试异步文件写入...");
        auto write_future = async_io_manager->write_file_async(test_file, test_content);
        auto write_result = write_future.get();
        
        if (write_result && write_result->status == IOOperationStatus::COMPLETED) {
            Output::success("✅ 异步写入测试通过");
            Output::info("  写入字节数: " + std::to_string(write_result->bytes_written));
            Output::info("  写入时间: " + std::to_string(write_result->duration.count()) + "ms");
        } else {
            Output::error("❌ 异步写入测试失败");
            return;
        }
        
        // 测试异步读取
        Output::info("📖 测试异步文件读取...");
        auto read_future = async_io_manager->read_file_async(test_file, true);
        auto read_result = read_future.get();
        
        if (read_result && read_result->status == IOOperationStatus::COMPLETED) {
            Output::success("✅ 异步读取测试通过");
            Output::info("  读取字节数: " + std::to_string(read_result->bytes_processed));
            Output::info("  读取时间: " + std::to_string(read_result->duration.count()) + "ms");
            Output::info("  内容匹配: " + std::string(read_result->content == test_content ? "✅ 是" : "❌ 否"));
        } else {
            Output::error("❌ 异步读取测试失败");
            return;
        }
        
        // 测试批量操作
        Output::info("📚 测试批量异步操作...");
        std::vector<std::string> test_files;
        std::vector<std::pair<std::string, std::string>> test_data;
        
        for (int i = 0; i < 5; ++i) {
            std::string filename = "/tmp/paker_async_test_" + std::to_string(i) + ".txt";
            std::string content = "Test file " + std::to_string(i) + " content.\n";
            test_files.push_back(filename);
            test_data.emplace_back(filename, content);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 批量写入
        auto write_futures = async_io_manager->write_files_async(test_data);
        for (auto& future : write_futures) {
            auto result = future.get();
            if (!result || result->status != IOOperationStatus::COMPLETED) {
                Output::error("❌ 批量写入测试失败");
                return;
            }
        }
        
        // 批量读取
        auto read_futures = async_io_manager->read_files_async(test_files, true);
        for (auto& future : read_futures) {
            auto result = future.get();
            if (!result || result->status != IOOperationStatus::COMPLETED) {
                Output::error("❌ 批量读取测试失败");
                return;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        Output::success("✅ 批量操作测试通过");
        Output::info("  批量操作时间: " + std::to_string(total_time.count()) + "ms");
        Output::info("  平均每文件: " + std::to_string(total_time.count() / test_files.size()) + "ms");
        
        // 清理测试文件
        for (const auto& file : test_files) {
            std::filesystem::remove(file);
        }
        std::filesystem::remove(test_file);
        
        Output::success("🎉 异步I/O测试全部通过！");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O test failed: " << e.what();
        Output::error("异步I/O测试失败: " + std::string(e.what()));
    }
}

void pm_async_io_benchmark() {
    LOG(INFO) << "Running async I/O benchmark";
    
    auto* async_io_manager = get_async_io_manager();
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("🏃 开始异步I/O性能基准测试...");
        
        const int num_files = 100;
        const int file_size = 1024; // 1KB per file
        
        // 生成测试数据
        std::string test_content(file_size, 'A');
        std::vector<std::string> test_files;
        std::vector<std::pair<std::string, std::string>> test_data;
        
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_benchmark_" + std::to_string(i) + ".txt";
            test_files.push_back(filename);
            test_data.emplace_back(filename, test_content);
        }
        
        // 异步I/O基准测试
        Output::info("⚡ 异步I/O基准测试 (" + std::to_string(num_files) + " 文件)...");
        auto async_start = std::chrono::high_resolution_clock::now();
        
        auto write_futures = async_io_manager->write_files_async(test_data);
        for (auto& future : write_futures) {
            future.get();
        }
        
        auto read_futures = async_io_manager->read_files_async(test_files, true);
        for (auto& future : read_futures) {
            future.get();
        }
        
        auto async_end = std::chrono::high_resolution_clock::now();
        auto async_time = std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
        
        // 同步I/O基准测试（对比）
        Output::info("🐌 同步I/O基准测试 (" + std::to_string(num_files) + " 文件)...");
        auto sync_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_sync_benchmark_" + std::to_string(i) + ".txt";
            
            // 同步写入
            std::ofstream file(filename);
            file << test_content;
            file.close();
            
            // 同步读取
            std::ifstream read_file(filename);
            std::string content((std::istreambuf_iterator<char>(read_file)),
                               std::istreambuf_iterator<char>());
            read_file.close();
        }
        
        auto sync_end = std::chrono::high_resolution_clock::now();
        auto sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(sync_end - sync_start);
        
        // 显示结果
        Output::info("📊 基准测试结果:");
        Output::info("  异步I/O时间: " + std::to_string(async_time.count()) + "ms");
        Output::info("  同步I/O时间: " + std::to_string(sync_time.count()) + "ms");
        
        if (sync_time.count() > 0) {
            double speedup = static_cast<double>(sync_time.count()) / async_time.count();
            Output::info("  性能提升: " + std::to_string(speedup) + "x");
            Output::info("  时间节省: " + std::to_string(sync_time.count() - async_time.count()) + "ms");
        }
        
        // 清理测试文件
        for (const auto& file : test_files) {
            std::filesystem::remove(file);
        }
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_sync_benchmark_" + std::to_string(i) + ".txt";
            std::filesystem::remove(filename);
        }
        
        Output::success("🎉 基准测试完成！");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O benchmark failed: " << e.what();
        Output::error("异步I/O基准测试失败: " + std::string(e.what()));
    }
}

void pm_async_io_optimize() {
    LOG(INFO) << "Optimizing async I/O performance";
    
    auto* async_io_manager = get_async_io_manager();
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("🔧 开始异步I/O性能优化...");
        
        // 清理队列
        size_t queue_size = async_io_manager->get_queue_size();
        if (queue_size > 0) {
            Output::info("🧹 清理队列中的 " + std::to_string(queue_size) + " 个待处理操作...");
            async_io_manager->clear_queue();
        }
        
        // 取消所有操作
        Output::info("⏹️ 取消所有进行中的操作...");
        async_io_manager->cancel_all_operations();
        
        // 等待操作完成
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 显示优化后的状态
        Output::info("📊 优化后状态:");
        Output::info("  队列大小: " + std::to_string(async_io_manager->get_queue_size()));
        Output::info("  活跃操作: " + std::to_string(async_io_manager->get_active_operations()));
        Output::info("  成功率: " + std::to_string(async_io_manager->get_success_rate()) + "%");
        
        // 优化建议
        Output::info("💡 优化建议:");
        if (async_io_manager->get_success_rate() < 95.0) {
            Output::info("  ⚠️ 成功率较低，建议检查I/O操作和错误处理");
        }
        if (async_io_manager->get_average_operation_time() > 500) {
            Output::info("  ⚠️ 平均操作时间较长，建议优化文件系统或网络连接");
        }
        if (async_io_manager->get_max_concurrent_operations() < std::thread::hardware_concurrency()) {
            Output::info("  💡 可以增加最大并发操作数以提升性能");
        }
        
        Output::success("✅ 异步I/O性能优化完成！");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O optimization failed: " << e.what();
        Output::error("异步I/O优化失败: " + std::string(e.what()));
    }
}

} // namespace Paker
