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
        
        // 显示增强功能状态
        Output::info("🚀 增强功能状态:");
        Output::info("  自适应缓冲区: " + std::string(async_io_manager->is_adaptive_buffering_enabled() ? "✅ 启用" : "❌ 禁用"));
        Output::info("  智能预读: " + std::string(async_io_manager->is_smart_pre_read_enabled() ? "✅ 启用" : "❌ 禁用"));
        Output::info("  网络重试: " + std::string(async_io_manager->is_network_retry_enabled() ? "✅ 启用" : "❌ 禁用"));
        Output::info("  批量优化: " + std::string(async_io_manager->is_batch_optimization_enabled() ? "✅ 启用" : "❌ 禁用"));
        Output::info("  内存使用: " + std::to_string(async_io_manager->get_memory_usage() / 1024 / 1024) + " MB");
        
        // 获取优化建议
        auto suggestions = async_io_manager->get_optimization_suggestions();
        if (!suggestions.empty()) {
            Output::info("💡 优化建议:");
            for (const auto& suggestion : suggestions) {
                Output::info("  • " + suggestion);
            }
        }
        
        // 应用优化建议
        if (!suggestions.empty()) {
            Output::info("🔧 应用优化建议...");
            async_io_manager->apply_optimization_suggestions();
        }
        
        // 触发预读分析
        if (async_io_manager->is_smart_pre_read_enabled()) {
            Output::info("📖 执行智能预读分析...");
            async_io_manager->trigger_pre_read_analysis();
            
            auto candidates = async_io_manager->get_pre_read_candidates();
            if (!candidates.empty()) {
                Output::info("  发现 " + std::to_string(candidates.size()) + " 个预读候选文件");
            }
        }
        
        // 处理批量操作
        if (async_io_manager->is_batch_optimization_enabled()) {
            Output::info("📦 处理批量操作优化...");
            async_io_manager->process_pending_batches();
        }
        
        // 显示详细性能报告
        std::string detailed_report = async_io_manager->get_detailed_performance_report();
        std::istringstream report_stream(detailed_report);
        std::string line;
        while (std::getline(report_stream, line)) {
            if (!line.empty()) {
                Output::info("  " + line);
            }
        }
        
        Output::success("✅ 异步I/O性能优化完成！");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O optimization failed: " << e.what();
        Output::error("异步I/O优化失败: " + std::string(e.what()));
    }
}

void pm_async_io_enhanced_features() {
    LOG(INFO) << "Displaying enhanced async I/O features";
    
    auto* async_io_manager = get_async_io_manager();
    if (!async_io_manager) {
        Output::error("异步I/O管理器未初始化");
        return;
    }
    
    try {
        Output::info("🚀 增强异步I/O功能展示");
        Output::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // 动态缓冲区管理
        Output::info("📊 动态缓冲区管理:");
        Output::info("  自适应缓冲区: " + std::string(async_io_manager->is_adaptive_buffering_enabled() ? "✅ 启用" : "❌ 禁用"));
        Output::info("  内存使用: " + std::to_string(async_io_manager->get_memory_usage() / 1024 / 1024) + " MB");
        
        // 显示各种缓冲区配置
        for (int i = 0; i < 4; ++i) {
            BufferType type = static_cast<BufferType>(i);
            auto config = async_io_manager->get_buffer_config(type);
            std::string type_name;
            switch (type) {
                case BufferType::FILE_READ: type_name = "文件读取"; break;
                case BufferType::FILE_WRITE: type_name = "文件写入"; break;
                case BufferType::NETWORK_DOWNLOAD: type_name = "网络下载"; break;
                case BufferType::NETWORK_UPLOAD: type_name = "网络上传"; break;
            }
            Output::info("  " + type_name + ": " + std::to_string(config.initial_size / 1024) + "KB");
        }
        
        // 智能预读策略
        Output::info("📖 智能预读策略:");
        Output::info("  智能预读: " + std::string(async_io_manager->is_smart_pre_read_enabled() ? "✅ 启用" : "❌ 禁用"));
        
        auto candidates = async_io_manager->get_pre_read_candidates();
        if (!candidates.empty()) {
            Output::info("  预读候选: " + std::to_string(candidates.size()) + " 个文件");
            for (size_t i = 0; i < std::min(candidates.size(), static_cast<size_t>(5)); ++i) {
                Output::info("    • " + candidates[i]);
            }
            if (candidates.size() > 5) {
                Output::info("    ... 还有 " + std::to_string(candidates.size() - 5) + " 个文件");
            }
        } else {
            Output::info("  暂无预读候选文件");
        }
        
        // 网络重试策略
        Output::info("🌐 网络重试策略:");
        Output::info("  网络重试: " + std::string(async_io_manager->is_network_retry_enabled() ? "✅ 启用" : "❌ 禁用"));
        
        auto retry_config = async_io_manager->get_retry_config();
        Output::info("  最大重试次数: " + std::to_string(retry_config.max_retries));
        Output::info("  初始延迟: " + std::to_string(retry_config.initial_delay.count()) + "ms");
        Output::info("  退避因子: " + std::to_string(retry_config.backoff_factor));
        Output::info("  最大延迟: " + std::to_string(retry_config.max_delay.count()) + "ms");
        
        // 批量处理优化
        Output::info("📦 批量处理优化:");
        Output::info("  批量优化: " + std::string(async_io_manager->is_batch_optimization_enabled() ? "✅ 启用" : "❌ 禁用"));
        
        // 性能统计
        Output::info("📈 性能统计:");
        Output::info("  平均吞吐量: " + std::to_string(async_io_manager->get_average_throughput()) + " MB/s");
        Output::info("  缓存命中率: " + std::to_string(async_io_manager->get_cache_hit_rate()) + "%");
        Output::info("  总处理字节: " + std::to_string(async_io_manager->get_total_bytes_processed() / 1024 / 1024) + " MB");
        
        // 优化建议
        auto suggestions = async_io_manager->get_optimization_suggestions();
        if (!suggestions.empty()) {
            Output::info("💡 优化建议:");
            for (const auto& suggestion : suggestions) {
                Output::info("  • " + suggestion);
            }
        } else {
            Output::info("  ✅ 当前配置已优化");
        }
        
        Output::success("🎉 增强功能展示完成！");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to display enhanced features: " << e.what();
        Output::error("显示增强功能失败: " + std::string(e.what()));
    }
}

} // namespace Paker
