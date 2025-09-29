#include "Paker/simd/simd_utils.h"
#include "Paker/simd/simd_hash.h"
#include "Paker/core/output.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>

using namespace Paker;

// 性能测试函数
template<typename Func>
double measure_time(Func func, const std::string& operation_name) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double time_ms = duration.count() / 1000.0;
    
    std::cout << operation_name << " 耗时: " << time_ms << " ms" << std::endl;
    return time_ms;
}

// 测试字符串操作性能
void test_string_operations() {
    std::cout << "\n=== 字符串操作性能测试 ===" << std::endl;
    
    // 生成测试数据
    std::vector<std::string> test_strings;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100, 1000);
    
    for (int i = 0; i < 1000; ++i) {
        std::string str(dis(gen), 'a' + (i % 26));
        test_strings.push_back(str);
    }
    
    // 测试字符串比较
    double simd_time = measure_time([&]() {
        for (size_t i = 0; i < test_strings.size() - 1; ++i) {
            SIMDStringUtils::string_equals_simd(test_strings[i], test_strings[i + 1]);
        }
    }, "SIMD字符串比较");
    
    double standard_time = measure_time([&]() {
        for (size_t i = 0; i < test_strings.size() - 1; ++i) {
            test_strings[i] == test_strings[i + 1];
        }
    }, "标准字符串比较");
    
    std::cout << "SIMD加速比: " << (standard_time / simd_time) << "x" << std::endl;
    
    // 测试字符串哈希
    std::vector<uint32_t> simd_hashes, standard_hashes;
    
    simd_time = measure_time([&]() {
        for (const auto& str : test_strings) {
            simd_hashes.push_back(SIMDStringUtils::string_hash_simd(str));
        }
    }, "SIMD字符串哈希");
    
    standard_time = measure_time([&]() {
        for (const auto& str : test_strings) {
            uint32_t hash = 0;
            for (char c : str) {
                hash = hash * 31 + c;
            }
            standard_hashes.push_back(hash);
        }
    }, "标准字符串哈希");
    
    std::cout << "SIMD哈希加速比: " << (standard_time / simd_time) << "x" << std::endl;
}

// 测试内存操作性能
void test_memory_operations() {
    std::cout << "\n=== 内存操作性能测试 ===" << std::endl;
    
    const size_t data_size = 1024 * 1024; // 1MB
    std::vector<char> source_data(data_size, 'A');
    std::vector<char> dest_data(data_size);
    
    // 测试内存拷贝
    double simd_time = measure_time([&]() {
        SIMMemoryUtils::memcpy_simd(dest_data.data(), source_data.data(), data_size);
    }, "SIMD内存拷贝");
    
    double standard_time = measure_time([&]() {
        std::memcpy(dest_data.data(), source_data.data(), data_size);
    }, "标准内存拷贝");
    
    std::cout << "SIMD内存拷贝加速比: " << (standard_time / simd_time) << "x" << std::endl;
    
    // 测试内存比较
    std::vector<char> compare_data(data_size, 'B');
    
    simd_time = measure_time([&]() {
        SIMMemoryUtils::memcmp_simd(source_data.data(), compare_data.data(), data_size);
    }, "SIMD内存比较");
    
    standard_time = measure_time([&]() {
        std::memcmp(source_data.data(), compare_data.data(), data_size);
    }, "标准内存比较");
    
    std::cout << "SIMD内存比较加速比: " << (standard_time / simd_time) << "x" << std::endl;
}

// 测试哈希计算性能
void test_hash_operations() {
    std::cout << "\n=== 哈希计算性能测试 ===" << std::endl;
    
    // 生成测试数据
    std::vector<std::string> test_data;
    for (int i = 0; i < 100; ++i) {
        test_data.push_back("test_data_" + std::to_string(i) + "_" + std::string(1000, 'x'));
    }
    
    // 测试SHA256计算
    double simd_time = measure_time([&]() {
        for (const auto& data : test_data) {
            SIMDHashCalculator::sha256_simd(data);
        }
    }, "SIMD SHA256计算");
    
    double standard_time = measure_time([&]() {
        for (const auto& data : test_data) {
            // 标准SHA256实现
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, data.c_str(), data.length());
            SHA256_Final(hash, &ctx);
        }
    }, "标准SHA256计算");
    
    std::cout << "SIMD SHA256加速比: " << (standard_time / simd_time) << "x" << std::endl;
    
    // 测试CRC32计算
    simd_time = measure_time([&]() {
        for (const auto& data : test_data) {
            SIMDHashCalculator::crc32_simd(data);
        }
    }, "SIMD CRC32计算");
    
    standard_time = measure_time([&]() {
        for (const auto& data : test_data) {
            uint32_t crc = 0xFFFFFFFF;
            for (char c : data) {
                crc = _mm_crc32_u8(crc, c);
            }
            crc ^= 0xFFFFFFFF;
        }
    }, "标准CRC32计算");
    
    std::cout << "SIMD CRC32加速比: " << (standard_time / simd_time) << "x" << std::endl;
}

// 测试数组操作性能
void test_array_operations() {
    std::cout << "\n=== 数组操作性能测试 ===" << std::endl;
    
    const size_t array_size = 1000000;
    std::vector<int32_t> test_array(array_size);
    
    // 生成随机数据
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (size_t i = 0; i < array_size; ++i) {
        test_array[i] = dis(gen);
    }
    
    // 测试数组求和
    int32_t simd_sum, standard_sum;
    
    double simd_time = measure_time([&]() {
        simd_sum = SIMDArrayUtils::sum_int32_simd(test_array.data(), array_size);
    }, "SIMD数组求和");
    
    double standard_time = measure_time([&]() {
        standard_sum = 0;
        for (int32_t value : test_array) {
            standard_sum += value;
        }
    }, "标准数组求和");
    
    std::cout << "SIMD数组求和加速比: " << (standard_time / simd_time) << "x" << std::endl;
    std::cout << "结果验证: " << (simd_sum == standard_sum ? "通过" : "失败") << std::endl;
    
    // 测试数组查找
    int32_t target_value = test_array[array_size / 2];
    size_t simd_index, standard_index;
    
    simd_time = measure_time([&]() {
        simd_index = SIMDArrayUtils::find_int32_simd(test_array.data(), array_size, target_value);
    }, "SIMD数组查找");
    
    standard_time = measure_time([&]() {
        standard_index = SIZE_MAX;
        for (size_t i = 0; i < array_size; ++i) {
            if (test_array[i] == target_value) {
                standard_index = i;
                break;
            }
        }
    }, "标准数组查找");
    
    std::cout << "SIMD数组查找加速比: " << (standard_time / simd_time) << "x" << std::endl;
    std::cout << "结果验证: " << (simd_index == standard_index ? "通过" : "失败") << std::endl;
}

// 测试文件哈希性能
void test_file_hash_operations() {
    std::cout << "\n=== 文件哈希性能测试 ===" << std::endl;
    
    // 创建测试文件
    std::string test_file = "test_file.txt";
    std::ofstream file(test_file);
    for (int i = 0; i < 10000; ++i) {
        file << "This is test line " << i << " with some data to make it larger.\n";
    }
    file.close();
    
    // 测试文件SHA256计算
    double simd_time = measure_time([&]() {
        SIMDFileHasher::calculate_file_sha256(test_file);
    }, "SIMD文件SHA256计算");
    
    double standard_time = measure_time([&]() {
        std::ifstream file_stream(test_file, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file_stream)),
                           std::istreambuf_iterator<char>());
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, content.c_str(), content.length());
        SHA256_Final(hash, &ctx);
    }, "标准文件SHA256计算");
    
    std::cout << "SIMD文件SHA256加速比: " << (standard_time / simd_time) << "x" << std::endl;
    
    // 清理测试文件
    std::remove(test_file.c_str());
}

// 显示SIMD支持信息
void display_simd_info() {
    std::cout << "\n=== SIMD支持信息 ===" << std::endl;
    
    SIMDDetector::initialize();
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    std::cout << "检测到的SIMD指令集: ";
    switch (instruction_set) {
        case SIMDInstructionSet::NONE:
            std::cout << "无SIMD支持";
            break;
        case SIMDInstructionSet::SSE2:
            std::cout << "SSE2";
            break;
        case SIMDInstructionSet::SSE3:
            std::cout << "SSE3";
            break;
        case SIMDInstructionSet::SSSE3:
            std::cout << "SSSE3";
            break;
        case SIMDInstructionSet::SSE4_1:
            std::cout << "SSE4.1";
            break;
        case SIMDInstructionSet::SSE4_2:
            std::cout << "SSE4.2";
            break;
        case SIMDInstructionSet::AVX:
            std::cout << "AVX";
            break;
        case SIMDInstructionSet::AVX2:
            std::cout << "AVX2";
            break;
        case SIMDInstructionSet::AVX512:
            std::cout << "AVX512";
            break;
    }
    std::cout << std::endl;
    
    std::cout << "SSE2支持: " << (SIMDDetector::has_sse2() ? "是" : "否") << std::endl;
    std::cout << "SSE4.2支持: " << (SIMDDetector::has_sse4_2() ? "是" : "否") << std::endl;
    std::cout << "AVX2支持: " << (SIMDDetector::has_avx2() ? "是" : "否") << std::endl;
    std::cout << "AVX512支持: " << (SIMDDetector::has_avx512() ? "是" : "否") << std::endl;
}

// 显示性能统计
void display_performance_stats() {
    std::cout << "\n=== 性能统计 ===" << std::endl;
    
    auto simd_stats = SIMDPerformanceMonitor::get_performance_stats();
    auto file_stats = SIMDFileHasher::get_performance_stats();
    
    std::cout << "SIMD操作次数: " << simd_stats.simd_operations_count_ << std::endl;
    std::cout << "回退操作次数: " << simd_stats.fallback_operations_count_ << std::endl;
    std::cout << "总SIMD时间: " << simd_stats.total_simd_time_.count() << " ms" << std::endl;
    std::cout << "总回退时间: " << simd_stats.total_fallback_time_.count() << " ms" << std::endl;
    std::cout << "SIMD加速比: " << SIMDPerformanceMonitor::get_speedup_factor() << "x" << std::endl;
    
    std::cout << "文件处理次数: " << file_stats.total_files_processed_ << std::endl;
    std::cout << "缓存命中次数: " << file_stats.cache_hits_ << std::endl;
    std::cout << "缓存未命中次数: " << file_stats.cache_misses_ << std::endl;
    std::cout << "缓存命中率: " << (file_stats.cache_hit_rate_ * 100) << "%" << std::endl;
    std::cout << "平均处理时间: " << file_stats.avg_processing_time_.count() << " ms" << std::endl;
}

int main() {
    std::cout << "=== Paker SIMD优化性能测试 ===" << std::endl;
    
    // 初始化SIMD管理器
    if (!SIMDHashManager::initialize()) {
        std::cerr << "Failed to initialize SIMDHashManager" << std::endl;
        return 1;
    }
    
    try {
        // 显示SIMD支持信息
        display_simd_info();
        
        // 运行性能测试
        test_string_operations();
        test_memory_operations();
        test_hash_operations();
        test_array_operations();
        test_file_hash_operations();
        
        // 显示性能统计
        display_performance_stats();
        
        std::cout << "\n=== 测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    // 清理
    SIMDHashManager::shutdown();
    
    return 0;
}
