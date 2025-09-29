#pragma once

#include "Paker/common.h"
#include <immintrin.h>
#include <nmmintrin.h>
#include <smmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

namespace Paker {

// SIMD指令集检测
enum class SIMDInstructionSet {
    NONE = 0,
    SSE2 = 1,
    SSE3 = 2,
    SSSE3 = 3,
    SSE4_1 = 4,
    SSE4_2 = 5,
    AVX = 6,
    AVX2 = 7,
    AVX512 = 8
};

// SIMD功能检测器
class SIMDDetector {
private:
    static SIMDInstructionSet detected_instruction_set_;
    static bool detection_completed_;
    
public:
    static SIMDInstructionSet detect_instruction_set();
    static bool has_sse2();
    static bool has_sse3();
    static bool has_ssse3();
    static bool has_sse4_1();
    static bool has_sse4_2();
    static bool has_avx();
    static bool has_avx2();
    static bool has_avx512();
    
    static void initialize();
    static SIMDInstructionSet get_current_instruction_set();
};

// SIMD字符串处理工具
class SIMDStringUtils {
public:
    // 快速字符串比较
    static bool string_equals_simd(const char* str1, const char* str2, size_t len);
    static bool string_equals_simd(const std::string& str1, const std::string& str2);
    
    // 快速字符串搜索
    static size_t string_find_simd(const char* haystack, size_t haystack_len, 
                                  const char* needle, size_t needle_len);
    static size_t string_find_simd(const std::string& haystack, const std::string& needle);
    
    // 快速字符串哈希
    static uint32_t string_hash_simd(const char* str, size_t len);
    static uint32_t string_hash_simd(const std::string& str);
    
    // 快速字符串转换
    static void to_lowercase_simd(char* str, size_t len);
    static void to_uppercase_simd(char* str, size_t len);
    static std::string to_lowercase_simd(const std::string& str);
    static std::string to_uppercase_simd(const std::string& str);
    
    // 快速字符串分割
    static std::vector<std::string> split_simd(const std::string& str, char delimiter);
    static std::vector<std::string> split_simd(const std::string& str, const std::string& delimiter);
    
private:
    // SSE2实现
    static bool string_equals_sse2(const char* str1, const char* str2, size_t len);
    static size_t string_find_sse2(const char* haystack, size_t haystack_len, 
                                  const char* needle, size_t needle_len);
    static uint32_t string_hash_sse2(const char* str, size_t len);
    
    // AVX2实现
    static bool string_equals_avx2(const char* str1, const char* str2, size_t len);
    static size_t string_find_avx2(const char* haystack, size_t haystack_len, 
                                  const char* needle, size_t needle_len);
    static uint32_t string_hash_avx2(const char* str, size_t len);
};

// SIMD内存操作工具
class SIMMemoryUtils {
public:
    // 快速内存拷贝
    static void* memcpy_simd(void* dest, const void* src, size_t n);
    static void* memmove_simd(void* dest, const void* src, size_t n);
    
    // 快速内存比较
    static int memcmp_simd(const void* ptr1, const void* ptr2, size_t n);
    
    // 快速内存设置
    static void* memset_simd(void* ptr, int value, size_t n);
    
    // 快速内存搜索
    static void* memchr_simd(const void* ptr, int value, size_t n);
    
    // 快速内存对齐检查
    static bool is_aligned(const void* ptr, size_t alignment);
    static void* align_pointer(void* ptr, size_t alignment);
    
private:
    // SSE2实现
    static void* memcpy_sse2(void* dest, const void* src, size_t n);
    static int memcmp_sse2(const void* ptr1, const void* ptr2, size_t n);
    static void* memset_sse2(void* ptr, int value, size_t n);
    
    // AVX2实现
    static void* memcpy_avx2(void* dest, const void* src, size_t n);
    static int memcmp_avx2(const void* ptr1, const void* ptr2, size_t n);
    static void* memset_avx2(void* ptr, int value, size_t n);
};

// SIMD哈希计算工具
class SIMDHashUtils {
public:
    // 快速CRC32计算
    static uint32_t crc32_simd(const void* data, size_t len);
    static uint32_t crc32_simd(const std::string& str);
    
    // 快速MD5计算（使用SIMD优化）
    static std::string md5_simd(const void* data, size_t len);
    static std::string md5_simd(const std::string& str);
    
    // 快速SHA256计算（使用SIMD优化）
    static std::string sha256_simd(const void* data, size_t len);
    static std::string sha256_simd(const std::string& str);
    
    // 快速哈希组合
    static uint64_t hash_combine_simd(uint64_t seed, const void* data, size_t len);
    
private:
    // SSE4.2 CRC32实现
    static uint32_t crc32_sse42(const void* data, size_t len);
    
    // AVX2哈希实现
    static uint32_t crc32_avx2(const void* data, size_t len);
};

// SIMD数组操作工具
class SIMDArrayUtils {
public:
    // 快速数组求和
    static int32_t sum_int32_simd(const int32_t* array, size_t count);
    static int64_t sum_int64_simd(const int64_t* array, size_t count);
    static float sum_float_simd(const float* array, size_t count);
    static double sum_double_simd(const double* array, size_t count);
    
    // 快速数组查找
    static size_t find_int32_simd(const int32_t* array, size_t count, int32_t value);
    static size_t find_int64_simd(const int64_t* array, size_t count, int64_t value);
    static size_t find_float_simd(const float* array, size_t count, float value);
    
    // 快速数组排序（基数排序）
    static void radix_sort_int32_simd(int32_t* array, size_t count);
    static void radix_sort_int64_simd(int64_t* array, size_t count);
    
    // 快速数组去重
    static size_t unique_int32_simd(int32_t* array, size_t count);
    static size_t unique_int64_simd(int64_t* array, size_t count);
    
private:
    // SSE2实现
    static int32_t sum_int32_sse2(const int32_t* array, size_t count);
    static size_t find_int32_sse2(const int32_t* array, size_t count, int32_t value);
    
    // AVX2实现
    static int32_t sum_int32_avx2(const int32_t* array, size_t count);
    static size_t find_int32_avx2(const int32_t* array, size_t count, int32_t value);
};

// SIMD性能监控
class SIMDPerformanceMonitor {
public:
    struct PerformanceStats {
        size_t simd_operations_count_;
        size_t fallback_operations_count_;
        std::chrono::milliseconds total_simd_time_;
        std::chrono::milliseconds total_fallback_time_;
        double simd_speedup_factor_;
        
        PerformanceStats() 
            : simd_operations_count_(0)
            , fallback_operations_count_(0)
            , total_simd_time_(0)
            , total_fallback_time_(0)
            , simd_speedup_factor_(1.0) {}
    };
    
    static void record_simd_operation(std::chrono::milliseconds duration);
    static void record_fallback_operation(std::chrono::milliseconds duration);
    static PerformanceStats get_performance_stats();
    static void reset_stats();
    static double get_speedup_factor();
    
private:
    static PerformanceStats stats_;
    static std::mutex stats_mutex_;
};

// SIMD优化配置
struct SIMDConfig {
    bool enable_simd_ = true;
    bool enable_auto_detection_ = true;
    bool enable_performance_monitoring_ = true;
    SIMDInstructionSet preferred_instruction_set_ = SIMDInstructionSet::AVX2;
    size_t min_data_size_for_simd_ = 64;  // 小于此大小的数据不使用SIMD
    bool enable_fallback_ = true;          // 是否启用回退到标准实现
};

// SIMD管理器
class SIMManager {
private:
    static SIMDConfig config_;
    static bool initialized_;
    
public:
    static bool initialize(const SIMDConfig& config = SIMDConfig{});
    static void shutdown();
    static SIMDConfig get_config();
    static void configure(const SIMDConfig& config);
    static bool is_initialized();
    
    // 自动选择最优实现
    template<typename Func>
    static auto auto_select(const Func& simd_func, const Func& fallback_func) {
        if (config_.enable_simd_ && SIMDDetector::get_current_instruction_set() != SIMDInstructionSet::NONE) {
            return simd_func;
        } else {
            return fallback_func;
        }
    }
};

} // namespace Paker
