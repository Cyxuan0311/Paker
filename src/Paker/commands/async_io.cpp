#include "Paker/commands/async_io.h"
#include "Paker/core/async_io.h"
#include "Paker/cache/async_cache_manager.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include <glog/logging.h>
#include <chrono>
#include <random>
#include <fstream>
#include <filesystem>

namespace Paker {

// 确保异步I/O管理器已初始化的辅助函数
bool ensure_async_io_manager_initialized() {
    auto* manager = get_async_io_manager();
    if (manager) {
        return true;
    }
    
    // 尝试初始化服务
    Output::info("Initializing async I/O manager...");
    if (!initialize_paker_services()) {
        Output::error("Failed to initialize services");
        return false;
    }
    
    // 再次检查
    manager = get_async_io_manager();
    if (!manager) {
        Output::error("Async I/O manager service not available");
        return false;
    }
    
    return true;
}

void pm_async_io_stats() {
    LOG(INFO) << "Displaying async I/O statistics";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    auto* async_cache_manager = get_async_cache_manager();
    
    try {
        Output::info("\033[1;36m Async I/O Statistics\033[0m");
        Output::info("\033[0;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m");
        
        // Async I/O statistics
        Output::info("\033[1;32m Async I/O Statistics:\033[0m");
        Output::info("  Total operations: \033[1;33m" + std::to_string(async_io_manager->get_total_operations()) + "\033[0m");
        Output::info("  Completed operations: \033[1;32m" + std::to_string(async_io_manager->get_completed_operations()) + "\033[0m");
        Output::info("  Failed operations: \033[1;31m" + std::to_string(async_io_manager->get_failed_operations()) + "\033[0m");
        Output::info("  Active operations: \033[1;35m" + std::to_string(async_io_manager->get_active_operations()) + "\033[0m");
        Output::info("  Queue size: \033[1;34m" + std::to_string(async_io_manager->get_queue_size()) + "\033[0m");
        Output::info("  Success rate: \033[1;33m" + std::to_string(async_io_manager->get_success_rate()) + "%\033[0m");
        Output::info("  Average operation time: \033[1;36m" + std::to_string(async_io_manager->get_average_operation_time()) + "ms\033[0m");
        
        // Async cache statistics
        if (async_cache_manager) {
            Output::info("\033[1;35m Async Cache Statistics:\033[0m");
            Output::info("  Total reads: \033[1;33m" + std::to_string(async_cache_manager->get_total_reads()) + "\033[0m");
            Output::info("  Total writes: \033[1;33m" + std::to_string(async_cache_manager->get_total_writes()) + "\033[0m");
            Output::info("  Cache hits: \033[1;32m" + std::to_string(async_cache_manager->get_cache_hits()) + "\033[0m");
            Output::info("  Cache misses: \033[1;31m" + std::to_string(async_cache_manager->get_cache_misses()) + "\033[0m");
            Output::info("  Cache hit rate: \033[1;36m" + std::to_string(async_cache_manager->get_cache_hit_rate()) + "%\033[0m");
            Output::info("  Async operations: \033[1;35m" + std::to_string(async_cache_manager->get_async_operations()) + "\033[0m");
            Output::info("  Average read time: \033[1;34m" + std::to_string(async_cache_manager->get_average_read_time()) + "ms\033[0m");
            Output::info("  Average write time: \033[1;34m" + std::to_string(async_cache_manager->get_average_write_time()) + "ms\033[0m");
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
        Output::error("Failed to get statistics: " + std::string(e.what()));
    }
}

void pm_async_io_config() {
    LOG(INFO) << "Displaying async I/O configuration";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    
    try {
        Output::info("\033[1;36m Async I/O Configuration\033[0m");
        Output::info("\033[0;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m");
        
        Output::info("\033[1;32m Thread Configuration:\033[0m");
        Output::info("  Max concurrent operations: \033[1;33m" + std::to_string(async_io_manager->get_max_concurrent_operations()) + "\033[0m");
        Output::info("  Hardware concurrency: \033[1;33m" + std::to_string(std::thread::hardware_concurrency()) + "\033[0m");
        
        Output::info("\033[1;32m Performance Configuration:\033[0m");
        Output::info("  Current queue size: \033[1;34m" + std::to_string(async_io_manager->get_queue_size()) + "\033[0m");
        Output::info("  Active operations: \033[1;35m" + std::to_string(async_io_manager->get_active_operations()) + "\033[0m");
        
        Output::info("\033[1;33m Optimization Suggestions:\033[0m");
        if (async_io_manager->get_queue_size() > 100) {
            Output::info("  \033[1;33m[WARN]\033[0m Queue backlog is high, consider adding worker threads");
        }
        if (async_io_manager->get_success_rate() < 90.0) {
            Output::info("  \033[1;33m[WARN]\033[0m Low success rate, consider checking I/O operations");
        }
        if (async_io_manager->get_average_operation_time() > 1000) {
            Output::info("  \033[1;33m[WARN]\033[0m Average operation time is long, consider optimizing I/O performance");
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to get async I/O configuration: " << e.what();
        Output::error("Failed to get configuration: " + std::string(e.what()));
    }
}

void pm_async_io_test() {
    LOG(INFO) << "Running async I/O test";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    
    try {
        Output::info("\033[1;36mStarting async I/O test...\033[0m");
        
        // 创建测试文件
        std::string test_content = "This is a test file for async I/O operations.\n";
        test_content += "Testing async file read and write operations.\n";
        test_content += "Performance should be significantly improved with async I/O.\n";
        
        std::string test_file = "/tmp/paker_async_test.txt";
        
        // 测试异步写入
        Output::info("\033[1;34mTesting async file write...\033[0m");
        auto write_future = async_io_manager->write_file_async(test_file, test_content);
        
        // 设置超时机制（5秒）
        auto write_result = write_future.wait_for(std::chrono::seconds(5)) == std::future_status::ready 
            ? write_future.get() 
            : nullptr;
        
        if (write_result && write_result->status == IOOperationStatus::COMPLETED) {
            Output::success("\033[1;32m[OK]\033[0m Async write test passed");
            Output::info("  Bytes written: \033[1;33m" + std::to_string(write_result->bytes_written) + "\033[0m");
            Output::info("  Write time: \033[1;36m" + std::to_string(write_result->duration.count()) + "ms\033[0m");
        } else if (!write_result) {
            Output::error("\033[1;31m[FAIL]\033[0m Async write test timed out (5s)");
            return;
        } else {
            Output::error("\033[1;31m[FAIL]\033[0m Async write test failed");
            return;
        }
        
        // 测试异步读取
        Output::info("\033[1;34mTesting async file read...\033[0m");
        auto read_future = async_io_manager->read_file_async(test_file, true);
        
        // 设置超时机制（5秒）
        auto read_result = read_future.wait_for(std::chrono::seconds(5)) == std::future_status::ready 
            ? read_future.get() 
            : nullptr;
        
        if (read_result && read_result->status == IOOperationStatus::COMPLETED) {
            Output::success("\033[1;32m[OK]\033[0m Async read test passed");
            Output::info("  Bytes read: \033[1;33m" + std::to_string(read_result->bytes_processed) + "\033[0m");
            Output::info("  Read time: \033[1;36m" + std::to_string(read_result->duration.count()) + "ms\033[0m");
            Output::info("  Content match: " + std::string(read_result->content == test_content ? "\033[1;32m[OK] Yes\033[0m" : "\033[1;31m[FAIL] No\033[0m"));
        } else if (!read_result) {
            Output::error("\033[1;31m[FAIL]\033[0m Async read test timed out (5s)");
            return;
        } else {
            Output::error("\033[1;31m[FAIL]\033[0m Async read test failed");
            return;
        }
        
        // Test batch operations
        Output::info("\033[1;35mTesting batch async operations...\033[0m");
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
            auto result = future.wait_for(std::chrono::seconds(5)) == std::future_status::ready 
                ? future.get() 
                : nullptr;
            if (!result) {
                Output::error("\033[1;31m[FAIL]\033[0m Batch write test timed out (5s)");
                return;
            } else if (result->status != IOOperationStatus::COMPLETED) {
                Output::error("\033[1;31m[FAIL]\033[0m Batch write test failed");
                return;
            }
        }
        
        // Batch read
        auto read_futures = async_io_manager->read_files_async(test_files, true);
        for (auto& future : read_futures) {
            auto result = future.wait_for(std::chrono::seconds(5)) == std::future_status::ready 
                ? future.get() 
                : nullptr;
            if (!result) {
                Output::error("\033[1;31m[FAIL]\033[0m Batch read test timed out (5s)");
                return;
            } else if (result->status != IOOperationStatus::COMPLETED) {
                Output::error("\033[1;31m[FAIL]\033[0m Batch read test failed");
                return;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        Output::success("\033[1;32m[OK]\033[0m Batch operation test passed");
        Output::info("  Batch operation time: \033[1;36m" + std::to_string(total_time.count()) + "ms\033[0m");
        Output::info("  Average per file: \033[1;33m" + std::to_string(total_time.count() / test_files.size()) + "ms\033[0m");
        
        // Clean up test files
        for (const auto& file : test_files) {
            std::filesystem::remove(file);
        }
        std::filesystem::remove(test_file);
        
        Output::success("\033[1;32mAsync I/O test completed successfully!\033[0m");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O test failed: " << e.what();
        Output::error("Async I/O test failed: " + std::string(e.what()));
    }
}

void pm_async_io_benchmark() {
    LOG(INFO) << "Running async I/O benchmark";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    
    try {
        Output::info("\033[1;36mStarting async I/O performance benchmark...\033[0m");
        
        const int num_files = 100;
        const int file_size = 1024; // 1KB per file
        
        // Generate test data
        std::string test_content(file_size, 'A');
        std::vector<std::string> test_files;
        std::vector<std::pair<std::string, std::string>> test_data;
        
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_benchmark_" + std::to_string(i) + ".txt";
            test_files.push_back(filename);
            test_data.emplace_back(filename, test_content);
        }
        
        // Async I/O benchmark
        Output::info("\033[1;34mAsync I/O benchmark (\033[1;33m" + std::to_string(num_files) + "\033[1;34m files)...\033[0m");
        auto async_start = std::chrono::high_resolution_clock::now();
        
        auto write_futures = async_io_manager->write_files_async(test_data);
        for (auto& future : write_futures) {
            if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
                Output::error("\033[1;31m[FAIL]\033[0m Async write benchmark timed out (10s)");
                return;
            }
            future.get();
        }
        
        auto read_futures = async_io_manager->read_files_async(test_files, true);
        for (auto& future : read_futures) {
            if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
                Output::error("\033[1;31m[FAIL]\033[0m Async read benchmark timed out (10s)");
                return;
            }
            future.get();
        }
        
        auto async_end = std::chrono::high_resolution_clock::now();
        auto async_time = std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
        
        // Sync I/O benchmark (comparison)
        Output::info("\033[1;35mSync I/O benchmark (\033[1;33m" + std::to_string(num_files) + "\033[1;35m files)...\033[0m");
        auto sync_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_sync_benchmark_" + std::to_string(i) + ".txt";
            
            // Sync write
            std::ofstream file(filename);
            file << test_content;
            file.close();
            
            // Sync read
            std::ifstream read_file(filename);
            std::string content((std::istreambuf_iterator<char>(read_file)),
                               std::istreambuf_iterator<char>());
            read_file.close();
        }
        
        auto sync_end = std::chrono::high_resolution_clock::now();
        auto sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(sync_end - sync_start);
        
        // Display results
        Output::info("\033[1;32mBenchmark results:\033[0m");
        Output::info("  Async I/O time: \033[1;34m" + std::to_string(async_time.count()) + "ms\033[0m");
        Output::info("  Sync I/O time: \033[1;35m" + std::to_string(sync_time.count()) + "ms\033[0m");
        
        if (sync_time.count() > 0) {
            double speedup = static_cast<double>(sync_time.count()) / async_time.count();
            Output::info("  Performance improvement: \033[1;33m" + std::to_string(speedup) + "x\033[0m");
            Output::info("  Time saved: \033[1;36m" + std::to_string(sync_time.count() - async_time.count()) + "ms\033[0m");
        }
        
        // Clean up test files
        for (const auto& file : test_files) {
            std::filesystem::remove(file);
        }
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "/tmp/paker_sync_benchmark_" + std::to_string(i) + ".txt";
            std::filesystem::remove(filename);
        }
        
        Output::success("\033[1;32mBenchmark completed!\033[0m");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O benchmark failed: " << e.what();
        Output::error("Async I/O benchmark failed: " + std::string(e.what()));
    }
}

void pm_async_io_optimize() {
    LOG(INFO) << "Optimizing async I/O performance";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    
    try {
        Output::info("\033[1;36mStarting async I/O performance optimization...\033[0m");
        
        // Clear queue
        size_t queue_size = async_io_manager->get_queue_size();
        if (queue_size > 0) {
            Output::info("Clearing \033[1;33m" + std::to_string(queue_size) + "\033[0m pending operations in queue...");
            async_io_manager->clear_queue();
        }
        
        // Cancel all operations
        Output::info("Canceling all ongoing operations...");
        async_io_manager->cancel_all_operations();
        
        // Wait for operations to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Display optimized state
        Output::info("\033[1;32mOptimized state:\033[0m");
        Output::info("  Queue size: \033[1;34m" + std::to_string(async_io_manager->get_queue_size()) + "\033[0m");
        Output::info("  Active operations: \033[1;35m" + std::to_string(async_io_manager->get_active_operations()) + "\033[0m");
        Output::info("  Success rate: \033[1;33m" + std::to_string(async_io_manager->get_success_rate()) + "%\033[0m");
        
        // Display enhanced features status
        Output::info("\033[1;35mEnhanced features status:\033[0m");
        Output::info("  Adaptive buffering: " + std::string(async_io_manager->is_adaptive_buffering_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        Output::info("  Smart pre-read: " + std::string(async_io_manager->is_smart_pre_read_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        Output::info("  Network retry: " + std::string(async_io_manager->is_network_retry_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        Output::info("  Batch optimization: " + std::string(async_io_manager->is_batch_optimization_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        Output::info("  Memory usage: \033[1;33m" + std::to_string(async_io_manager->get_memory_usage() / 1024 / 1024) + " MB\033[0m");
        
        // Get optimization suggestions
        auto suggestions = async_io_manager->get_optimization_suggestions();
        if (!suggestions.empty()) {
            Output::info("\033[1;33mOptimization Suggestions:\033[0m");
            for (const auto& suggestion : suggestions) {
                Output::info("  \033[1;36m•\033[0m " + suggestion);
            }
        }
        
        // Apply optimization suggestions
        if (!suggestions.empty()) {
            Output::info("Applying optimization suggestions...");
            async_io_manager->apply_optimization_suggestions();
        }
        
        // Trigger pre-read analysis
        if (async_io_manager->is_smart_pre_read_enabled()) {
            Output::info("Executing smart pre-read analysis...");
            async_io_manager->trigger_pre_read_analysis();
            
            auto candidates = async_io_manager->get_pre_read_candidates();
            if (!candidates.empty()) {
                Output::info("  Found " + std::to_string(candidates.size()) + " pre-read candidate files");
            }
        }
        
        // Process batch operations
        if (async_io_manager->is_batch_optimization_enabled()) {
            Output::info("Processing batch operation optimization...");
            async_io_manager->process_pending_batches();
        }
        
        // Display detailed performance report
        std::string detailed_report = async_io_manager->get_detailed_performance_report();
        std::istringstream report_stream(detailed_report);
        std::string line;
        while (std::getline(report_stream, line)) {
            if (!line.empty()) {
                Output::info("  " + line);
            }
        }
        
        Output::success("\033[1;32m[OK]\033[0m Async I/O performance optimization completed!");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Async I/O optimization failed: " << e.what();
        Output::error("Async I/O optimization failed: " + std::string(e.what()));
    }
}

void pm_async_io_enhanced_features() {
    LOG(INFO) << "Displaying enhanced async I/O features";
    
    if (!ensure_async_io_manager_initialized()) {
        return;
    }
    
    auto* async_io_manager = get_async_io_manager();
    
    try {
        Output::info("\033[1;36mEnhanced Async I/O Features\033[0m");
        Output::info("\033[0;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m");
        
        // Dynamic buffer management
        Output::info("\033[1;32mDynamic buffer management:\033[0m");
        Output::info("  Adaptive buffering: " + std::string(async_io_manager->is_adaptive_buffering_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        Output::info("  Memory usage: \033[1;33m" + std::to_string(async_io_manager->get_memory_usage() / 1024 / 1024) + " MB\033[0m");
        
        // Display various buffer configurations
        for (int i = 0; i < 4; ++i) {
            BufferType type = static_cast<BufferType>(i);
            auto config = async_io_manager->get_buffer_config(type);
            std::string type_name;
            switch (type) {
                case BufferType::FILE_READ: type_name = "File read"; break;
                case BufferType::FILE_WRITE: type_name = "File write"; break;
                case BufferType::NETWORK_DOWNLOAD: type_name = "Network download"; break;
                case BufferType::NETWORK_UPLOAD: type_name = "Network upload"; break;
            }
            Output::info("  " + type_name + ": \033[1;34m" + std::to_string(config.initial_size / 1024) + "KB\033[0m");
        }
        
        // Smart pre-read strategy
        Output::info("\033[1;35mSmart pre-read strategy:\033[0m");
        Output::info("  Smart pre-read: " + std::string(async_io_manager->is_smart_pre_read_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        
        auto candidates = async_io_manager->get_pre_read_candidates();
        if (!candidates.empty()) {
            Output::info("  Pre-read candidates: \033[1;33m" + std::to_string(candidates.size()) + " files\033[0m");
            for (size_t i = 0; i < std::min(candidates.size(), static_cast<size_t>(5)); ++i) {
                Output::info("    \033[1;36m•\033[0m " + candidates[i]);
            }
            if (candidates.size() > 5) {
                Output::info("    ... and \033[1;33m" + std::to_string(candidates.size() - 5) + "\033[0m more files");
            }
        } else {
            Output::info("  No pre-read candidate files");
        }
        
        // Network retry strategy
        Output::info("\033[1;34mNetwork retry strategy:\033[0m");
        Output::info("  Network retry: " + std::string(async_io_manager->is_network_retry_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        
        auto retry_config = async_io_manager->get_retry_config();
        Output::info("  Max retry attempts: \033[1;33m" + std::to_string(retry_config.max_retries) + "\033[0m");
        Output::info("  Initial delay: \033[1;36m" + std::to_string(retry_config.initial_delay.count()) + "ms\033[0m");
        Output::info("  Backoff factor: \033[1;35m" + std::to_string(retry_config.backoff_factor) + "\033[0m");
        Output::info("  Max delay: \033[1;36m" + std::to_string(retry_config.max_delay.count()) + "ms\033[0m");
        
        // Batch processing optimization
        Output::info("\033[1;32mBatch processing optimization:\033[0m");
        Output::info("  Batch optimization: " + std::string(async_io_manager->is_batch_optimization_enabled() ? "\033[1;32m[OK] Enabled\033[0m" : "\033[1;31m[FAIL] Disabled\033[0m"));
        
        // Performance statistics
        Output::info("\033[1;33mPerformance statistics:\033[0m");
        Output::info("  Average throughput: \033[1;34m" + std::to_string(async_io_manager->get_average_throughput()) + " MB/s\033[0m");
        Output::info("  Cache hit rate: \033[1;36m" + std::to_string(async_io_manager->get_cache_hit_rate()) + "%\033[0m");
        Output::info("  Total bytes processed: \033[1;35m" + std::to_string(async_io_manager->get_total_bytes_processed() / 1024 / 1024) + " MB\033[0m");
        
        // Optimization suggestions
        auto suggestions = async_io_manager->get_optimization_suggestions();
        if (!suggestions.empty()) {
            Output::info("\033[1;33mOptimization Suggestions:\033[0m");
            for (const auto& suggestion : suggestions) {
                Output::info("  \033[1;36m•\033[0m " + suggestion);
            }
        } else {
            Output::info("  \033[1;32m[OK]\033[0m Current configuration is optimized");
        }
        
        Output::success("\033[1;32mEnhanced features demonstration completed!\033[0m");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to display enhanced features: " << e.what();
        Output::error("Failed to display enhanced features: " + std::string(e.what()));
    }
}

} // namespace Paker
