#include "Paker/core/openmp_io.h"
#include "Paker/simd/simd_hash.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <glog/logging.h>

using namespace Paker;

/**
 * @brief OpenMP优化示例程序
 * 
 * 演示如何使用OpenMP优化密集I/O操作
 */
class OpenMPOptimizationExample {
public:
    /**
     * @brief 运行所有优化示例
     */
    static void run_all_examples() {
        std::cout << "=== OpenMP I/O 优化示例 ===" << std::endl;
        
        // 创建测试文件
        create_test_files();
        
        // 示例1：并行文件读取
        example_parallel_file_reading();
        
        // 示例2：并行文件写入
        example_parallel_file_writing();
        
        // 示例3：并行哈希计算
        example_parallel_hash_calculation();
        
        // 示例4：并行文件操作
        example_parallel_file_operations();
        
        // 示例5：性能对比测试
        performance_comparison_test();
        
        // 清理测试文件
        cleanup_test_files();
        
        std::cout << "=== 所有示例完成 ===" << std::endl;
    }

private:
    static const std::string TEST_DIR;
    static const size_t TEST_FILE_COUNT;
    static const size_t TEST_FILE_SIZE;
    
    /**
     * @brief 创建测试文件
     */
    static void create_test_files() {
        std::cout << "\n--- 创建测试文件 ---" << std::endl;
        
        std::filesystem::create_directories(TEST_DIR);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建测试文件
        for (size_t i = 0; i < TEST_FILE_COUNT; ++i) {
            std::string file_path = TEST_DIR + "/test_file_" + std::to_string(i) + ".txt";
            std::ofstream file(file_path);
            
            // 写入测试数据
            for (size_t j = 0; j < TEST_FILE_SIZE; ++j) {
                file << "Line " << j << " in file " << i << " - Test data for OpenMP optimization\n";
            }
            file.close();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        std::cout << "创建了 " << TEST_FILE_COUNT << " 个测试文件，耗时: " << duration_ms << "ms" << std::endl;
    }
    
    /**
     * @brief 示例1：并行文件读取
     */
    static void example_parallel_file_reading() {
        std::cout << "\n--- 示例1：并行文件读取 ---" << std::endl;
        
        // 准备文件路径
        std::vector<std::string> file_paths;
        for (size_t i = 0; i < TEST_FILE_COUNT; ++i) {
            file_paths.push_back(TEST_DIR + "/test_file_" + std::to_string(i) + ".txt");
        }
        
        // 使用OpenMP并行读取
        OpenMPIOManager io_manager(4); // 使用4个线程
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto contents = io_manager.read_text_files_parallel(file_paths);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        std::cout << "并行读取 " << contents.size() << " 个文件，耗时: " << duration_ms << "ms" << std::endl;
        std::cout << "平均每个文件: " << duration_ms / contents.size() << "ms" << std::endl;
        
        // 显示性能统计
        auto stats = io_manager.get_performance_stats();
        std::cout << "性能统计:" << std::endl;
        std::cout << "  总操作数: " << stats.total_operations << std::endl;
        std::cout << "  成功操作: " << stats.successful_operations << std::endl;
        std::cout << "  失败操作: " << stats.failed_operations << std::endl;
        std::cout << "  平均时间: " << stats.average_time_ms << "ms" << std::endl;
    }
    
    /**
     * @brief 示例2：并行文件写入
     */
    static void example_parallel_file_writing() {
        std::cout << "\n--- 示例2：并行文件写入 ---" << std::endl;
        
        // 准备文件内容
        std::vector<std::pair<std::string, std::string>> file_contents;
        for (size_t i = 0; i < TEST_FILE_COUNT; ++i) {
            std::string content = "OpenMP optimized file " + std::to_string(i) + "\n";
            for (size_t j = 0; j < 1000; ++j) {
                content += "Line " + std::to_string(j) + " - Parallel write test\n";
            }
            file_contents.push_back({TEST_DIR + "/output_" + std::to_string(i) + ".txt", content});
        }
        
        // 使用OpenMP并行写入
        OpenMPIOManager io_manager(4);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto results = io_manager.write_text_files_parallel(file_contents);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        size_t success_count = std::count(results.begin(), results.end(), true);
        std::cout << "并行写入 " << success_count << "/" << results.size() << " 个文件，耗时: " << duration_ms << "ms" << std::endl;
    }
    
    /**
     * @brief 示例3：并行哈希计算
     */
    static void example_parallel_hash_calculation() {
        std::cout << "\n--- 示例3：并行哈希计算 ---" << std::endl;
        
        // 准备文件路径
        std::vector<std::string> file_paths;
        for (size_t i = 0; i < TEST_FILE_COUNT; ++i) {
            file_paths.push_back(TEST_DIR + "/test_file_" + std::to_string(i) + ".txt");
        }
        
        // 使用OpenMP并行计算SHA256
        OpenMPIOManager io_manager(4);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto hashes = io_manager.calculate_file_hashes_parallel(file_paths, "sha256");
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        std::cout << "并行计算 " << hashes.size() << " 个文件的SHA256，耗时: " << duration_ms << "ms" << std::endl;
        
        // 显示前几个哈希值
        for (size_t i = 0; i < std::min(size_t(3), hashes.size()); ++i) {
            std::cout << "文件 " << i << " 哈希: " << hashes[i].substr(0, 16) << "..." << std::endl;
        }
    }
    
    /**
     * @brief 示例4：并行文件操作
     */
    static void example_parallel_file_operations() {
        std::cout << "\n--- 示例4：并行文件操作 ---" << std::endl;
        
        OpenMPIOManager io_manager(4);
        
        // 并行创建目录
        std::vector<std::string> dirs = {
            TEST_DIR + "/subdir1",
            TEST_DIR + "/subdir2", 
            TEST_DIR + "/subdir3"
        };
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto create_results = io_manager.create_directories_parallel(dirs);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        size_t success_count = std::count(create_results.begin(), create_results.end(), true);
        std::cout << "并行创建 " << success_count << "/" << create_results.size() << " 个目录，耗时: " << duration_ms << "ms" << std::endl;
        
        // 并行列出目录
        auto list_results = io_manager.list_directories_parallel(dirs);
        std::cout << "并行列出目录内容完成" << std::endl;
    }
    
    /**
     * @brief 示例5：性能对比测试
     */
    static void performance_comparison_test() {
        std::cout << "\n--- 示例5：性能对比测试 ---" << std::endl;
        
        // 准备测试数据
        std::vector<std::string> file_paths;
        for (size_t i = 0; i < TEST_FILE_COUNT; ++i) {
            file_paths.push_back(TEST_DIR + "/test_file_" + std::to_string(i) + ".txt");
        }
        
        // 测试串行读取
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<std::string> serial_contents;
        for (const auto& file_path : file_paths) {
            std::ifstream file(file_path);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            serial_contents.push_back(content);
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        double serial_duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 测试并行读取
        OpenMPIOManager io_manager(4);
        start_time = std::chrono::high_resolution_clock::now();
        auto parallel_contents = io_manager.read_text_files_parallel(file_paths);
        end_time = std::chrono::high_resolution_clock::now();
        double parallel_duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 计算加速比
        double speedup = serial_duration / parallel_duration;
        
        std::cout << "性能对比结果:" << std::endl;
        std::cout << "  串行读取耗时: " << serial_duration << "ms" << std::endl;
        std::cout << "  并行读取耗时: " << parallel_duration << "ms" << std::endl;
        std::cout << "  加速比: " << speedup << "x" << std::endl;
        std::cout << "  性能提升: " << ((speedup - 1.0) * 100) << "%" << std::endl;
    }
    
    /**
     * @brief 清理测试文件
     */
    static void cleanup_test_files() {
        std::cout << "\n--- 清理测试文件 ---" << std::endl;
        
        try {
            std::filesystem::remove_all(TEST_DIR);
            std::cout << "测试文件清理完成" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "清理测试文件时出错: " << e.what() << std::endl;
        }
    }
};

// 静态成员定义
const std::string OpenMPOptimizationExample::TEST_DIR = "./openmp_test_files";
const size_t OpenMPOptimizationExample::TEST_FILE_COUNT = 20;
const size_t OpenMPOptimizationExample::TEST_FILE_SIZE = 1000;

/**
 * @brief 主函数
 */
int main() {
    // 初始化日志
    google::InitGoogleLogging("OpenMPOptimizationExample");
    
    try {
        // 运行所有示例
        OpenMPOptimizationExample::run_all_examples();
        
        std::cout << "\n=== OpenMP优化示例程序运行完成 ===" << std::endl;
        std::cout << "OpenMP并行化显著提升了I/O操作性能！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "程序运行出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
