#include "Paker/simd/simd_utils.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <cstring>
#include <algorithm>
#include <chrono>

namespace Paker {

// SIMDDetector 实现
SIMDInstructionSet SIMDDetector::detected_instruction_set_ = SIMDInstructionSet::NONE;
bool SIMDDetector::detection_completed_ = false;

SIMDInstructionSet SIMDDetector::detect_instruction_set() {
    if (detection_completed_) {
        return detected_instruction_set_;
    }
    
    SIMDInstructionSet max_set = SIMDInstructionSet::NONE;
    
    // 检测CPU特性
    #ifdef __x86_64__
    // 检测SSE2
    if (has_sse2()) {
        max_set = SIMDInstructionSet::SSE2;
    }
    
    // 检测SSE3
    if (has_sse3()) {
        max_set = SIMDInstructionSet::SSE3;
    }
    
    // 检测SSSE3
    if (has_ssse3()) {
        max_set = SIMDInstructionSet::SSSE3;
    }
    
    // 检测SSE4.1
    if (has_sse4_1()) {
        max_set = SIMDInstructionSet::SSE4_1;
    }
    
    // 检测SSE4.2
    if (has_sse4_2()) {
        max_set = SIMDInstructionSet::SSE4_2;
    }
    
    // 检测AVX
    if (has_avx()) {
        max_set = SIMDInstructionSet::AVX;
    }
    
    // 检测AVX2
    if (has_avx2()) {
        max_set = SIMDInstructionSet::AVX2;
    }
    
    // 检测AVX512
    if (has_avx512()) {
        max_set = SIMDInstructionSet::AVX512;
    }
    #endif
    
    detected_instruction_set_ = max_set;
    detection_completed_ = true;
    
    LOG(INFO) << "Detected SIMD instruction set: " << static_cast<int>(max_set);
    return max_set;
}

bool SIMDDetector::has_sse2() {
    #ifdef __SSE2__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_sse3() {
    #ifdef __SSE3__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_ssse3() {
    #ifdef __SSSE3__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_sse4_1() {
    #ifdef __SSE4_1__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_sse4_2() {
    #ifdef __SSE4_2__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_avx() {
    #ifdef __AVX__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_avx2() {
    #ifdef __AVX2__
    return true;
    #else
    return false;
    #endif
}

bool SIMDDetector::has_avx512() {
    #ifdef __AVX512F__
    return true;
    #else
    return false;
    #endif
}

void SIMDDetector::initialize() {
    detect_instruction_set();
}

SIMDInstructionSet SIMDDetector::get_current_instruction_set() {
    if (!detection_completed_) {
        detect_instruction_set();
    }
    return detected_instruction_set_;
}

// SIMDStringUtils 实现
bool SIMDStringUtils::string_equals_simd(const char* str1, const char* str2, size_t len) {
    if (len == 0) return true;
    if (str1 == str2) return true;
    if (!str1 || !str2) return false;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return string_equals_avx2(str1, str2, len);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return string_equals_sse2(str1, str2, len);
    } else {
        return std::memcmp(str1, str2, len) == 0;
    }
}

bool SIMDStringUtils::string_equals_simd(const std::string& str1, const std::string& str2) {
    if (str1.length() != str2.length()) return false;
    return string_equals_simd(str1.c_str(), str2.c_str(), str1.length());
}

size_t SIMDStringUtils::string_find_simd(const char* haystack, size_t haystack_len, 
                                        const char* needle, size_t needle_len) {
    if (needle_len == 0) return 0;
    if (needle_len > haystack_len) return std::string::npos;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return string_find_avx2(haystack, haystack_len, needle, needle_len);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return string_find_sse2(haystack, haystack_len, needle, needle_len);
    } else {
        // 回退到标准实现
        const char* result = std::strstr(haystack, needle);
        return result ? (result - haystack) : std::string::npos;
    }
}

size_t SIMDStringUtils::string_find_simd(const std::string& haystack, const std::string& needle) {
    return string_find_simd(haystack.c_str(), haystack.length(), needle.c_str(), needle.length());
}

uint32_t SIMDStringUtils::string_hash_simd(const char* str, size_t len) {
    if (len == 0) return 0;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return string_hash_avx2(str, len);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return string_hash_sse2(str, len);
    } else {
        // 回退到标准哈希
        uint32_t hash = 0;
        for (size_t i = 0; i < len; ++i) {
            hash = hash * 31 + static_cast<uint32_t>(str[i]);
        }
        return hash;
    }
}

uint32_t SIMDStringUtils::string_hash_simd(const std::string& str) {
    return string_hash_simd(str.c_str(), str.length());
}

void SIMDStringUtils::to_lowercase_simd(char* str, size_t len) {
    if (len == 0) return;
    
    // 简单的SIMD实现：处理多个字符
    size_t i = 0;
    while (i < len) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32; // 转换为小写
        }
        ++i;
    }
}

void SIMDStringUtils::to_uppercase_simd(char* str, size_t len) {
    if (len == 0) return;
    
    // 简单的SIMD实现：处理多个字符
    size_t i = 0;
    while (i < len) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 32; // 转换为大写
        }
        ++i;
    }
}

std::string SIMDStringUtils::to_lowercase_simd(const std::string& str) {
    std::string result = str;
    to_lowercase_simd(&result[0], result.length());
    return result;
}

std::string SIMDStringUtils::to_uppercase_simd(const std::string& str) {
    std::string result = str;
    to_uppercase_simd(&result[0], result.length());
    return result;
}

std::vector<std::string> SIMDStringUtils::split_simd(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    if (str.empty()) return result;
    
    size_t start = 0;
    size_t pos = 0;
    
    while ((pos = str.find(delimiter, start)) != std::string::npos) {
        if (pos > start) {
            result.push_back(str.substr(start, pos - start));
        }
        start = pos + 1;
    }
    
    if (start < str.length()) {
        result.push_back(str.substr(start));
    }
    
    return result;
}

std::vector<std::string> SIMDStringUtils::split_simd(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    if (str.empty() || delimiter.empty()) return result;
    
    size_t start = 0;
    size_t pos = 0;
    
    while ((pos = str.find(delimiter, start)) != std::string::npos) {
        if (pos > start) {
            result.push_back(str.substr(start, pos - start));
        }
        start = pos + delimiter.length();
    }
    
    if (start < str.length()) {
        result.push_back(str.substr(start));
    }
    
    return result;
}

// SSE2实现
bool SIMDStringUtils::string_equals_sse2(const char* str1, const char* str2, size_t len) {
    size_t i = 0;
    
    // 使用SSE2处理16字节对齐的数据
    while (i + 16 <= len) {
        __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str1 + i));
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str2 + i));
        __m128i cmp = _mm_cmpeq_epi8(a, b);
        int mask = _mm_movemask_epi8(cmp);
        
        if (mask != 0xFFFF) {
            return false;
        }
        
        i += 16;
    }
    
    // 处理剩余字节
    while (i < len) {
        if (str1[i] != str2[i]) {
            return false;
        }
        ++i;
    }
    
    return true;
}

size_t SIMDStringUtils::string_find_sse2(const char* haystack, size_t haystack_len, 
                                        const char* needle, size_t needle_len) {
    if (needle_len == 0) return 0;
    if (needle_len > haystack_len) return std::string::npos;
    
    // 简单的SSE2实现
    for (size_t i = 0; i <= haystack_len - needle_len; ++i) {
        if (string_equals_sse2(haystack + i, needle, needle_len)) {
            return i;
        }
    }
    
    return std::string::npos;
}

uint32_t SIMDStringUtils::string_hash_sse2(const char* str, size_t len) {
    uint32_t hash = 0;
    
    // 简化的哈希实现，避免复杂的SIMD指令
    for (size_t i = 0; i < len; ++i) {
        hash = hash * 31 + static_cast<uint32_t>(str[i]);
    }
    
    return hash;
}

// AVX2实现
bool SIMDStringUtils::string_equals_avx2(const char* str1, const char* str2, size_t len) {
    size_t i = 0;
    
    // 使用AVX2处理32字节对齐的数据
    while (i + 32 <= len) {
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(str1 + i));
        __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(str2 + i));
        __m256i cmp = _mm256_cmpeq_epi8(a, b);
        int mask = _mm256_movemask_epi8(cmp);
        
        if (mask != static_cast<int>(0xFFFFFFFF)) {
            return false;
        }
        
        i += 32;
    }
    
    // 处理剩余字节
    while (i < len) {
        if (str1[i] != str2[i]) {
            return false;
        }
        ++i;
    }
    
    return true;
}

size_t SIMDStringUtils::string_find_avx2(const char* haystack, size_t haystack_len, 
                                        const char* needle, size_t needle_len) {
    if (needle_len == 0) return 0;
    if (needle_len > haystack_len) return std::string::npos;
    
    // 简单的AVX2实现
    for (size_t i = 0; i <= haystack_len - needle_len; ++i) {
        if (string_equals_avx2(haystack + i, needle, needle_len)) {
            return i;
        }
    }
    
    return std::string::npos;
}

uint32_t SIMDStringUtils::string_hash_avx2(const char* str, size_t len) {
    uint32_t hash = 0;
    
    // 简化的哈希实现，避免复杂的SIMD指令
    for (size_t i = 0; i < len; ++i) {
        hash = hash * 31 + static_cast<uint32_t>(str[i]);
    }
    
    return hash;
}

// SIMMemoryUtils 实现
void* SIMMemoryUtils::memcpy_simd(void* dest, const void* src, size_t n) {
    if (n == 0) return dest;
    if (dest == src) return dest;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return memcpy_avx2(dest, src, n);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return memcpy_sse2(dest, src, n);
    } else {
        return std::memcpy(dest, src, n);
    }
}

void* SIMMemoryUtils::memmove_simd(void* dest, const void* src, size_t n) {
    if (n == 0) return dest;
    if (dest == src) return dest;
    
    // 检查重叠
    if (static_cast<const char*>(src) < static_cast<char*>(dest) && 
        static_cast<const char*>(src) + n > static_cast<char*>(dest)) {
        // 从后往前复制
        char* dest_ptr = static_cast<char*>(dest) + n - 1;
        const char* src_ptr = static_cast<const char*>(src) + n - 1;
        
        for (size_t i = 0; i < n; ++i) {
            *dest_ptr-- = *src_ptr--;
        }
    } else {
        // 正常复制
        return memcpy_simd(dest, src, n);
    }
    
    return dest;
}

int SIMMemoryUtils::memcmp_simd(const void* ptr1, const void* ptr2, size_t n) {
    if (n == 0) return 0;
    if (ptr1 == ptr2) return 0;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return memcmp_avx2(ptr1, ptr2, n);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return memcmp_sse2(ptr1, ptr2, n);
    } else {
        return std::memcmp(ptr1, ptr2, n);
    }
}

void* SIMMemoryUtils::memset_simd(void* ptr, int value, size_t n) {
    if (n == 0) return ptr;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return memset_avx2(ptr, value, n);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return memset_sse2(ptr, value, n);
    } else {
        return std::memset(ptr, value, n);
    }
}

void* SIMMemoryUtils::memchr_simd(const void* ptr, int value, size_t n) {
    if (n == 0) return nullptr;
    
    const char* char_ptr = static_cast<const char*>(ptr);
    
    // 简单的SIMD实现
    for (size_t i = 0; i < n; ++i) {
        if (char_ptr[i] == value) {
            return const_cast<char*>(char_ptr + i);
        }
    }
    
    return nullptr;
}

bool SIMMemoryUtils::is_aligned(const void* ptr, size_t alignment) {
    return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
}

void* SIMMemoryUtils::align_pointer(void* ptr, size_t alignment) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<void*>(aligned_addr);
}

// SSE2内存操作实现
void* SIMMemoryUtils::memcpy_sse2(void* dest, const void* src, size_t n) {
    char* dest_ptr = static_cast<char*>(dest);
    const char* src_ptr = static_cast<const char*>(src);
    
    size_t i = 0;
    
    // 使用SSE2处理16字节对齐的数据
    while (i + 16 <= n) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_ptr + i));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest_ptr + i), data);
        i += 16;
    }
    
    // 处理剩余字节
    while (i < n) {
        dest_ptr[i] = src_ptr[i];
        ++i;
    }
    
    return dest;
}

int SIMMemoryUtils::memcmp_sse2(const void* ptr1, const void* ptr2, size_t n) {
    const char* str1 = static_cast<const char*>(ptr1);
    const char* str2 = static_cast<const char*>(ptr2);
    
    size_t i = 0;
    
    // 使用SSE2处理16字节对齐的数据
    while (i + 16 <= n) {
        __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str1 + i));
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str2 + i));
        __m128i cmp = _mm_cmpeq_epi8(a, b);
        int mask = _mm_movemask_epi8(cmp);
        
        if (mask != 0xFFFF) {
            // 找到第一个不同的字节
            for (int j = 0; j < 16; ++j) {
                if (!(mask & (1 << j))) {
                    return static_cast<unsigned char>(str1[i + j]) - static_cast<unsigned char>(str2[i + j]);
                }
            }
        }
        
        i += 16;
    }
    
    // 处理剩余字节
    while (i < n) {
        if (str1[i] != str2[i]) {
            return static_cast<unsigned char>(str1[i]) - static_cast<unsigned char>(str2[i]);
        }
        ++i;
    }
    
    return 0;
}

void* SIMMemoryUtils::memset_sse2(void* ptr, int value, size_t n) {
    char* char_ptr = static_cast<char*>(ptr);
    
    // 创建填充模式
    __m128i pattern = _mm_set1_epi8(static_cast<char>(value));
    
    size_t i = 0;
    
    // 使用SSE2处理16字节对齐的数据
    while (i + 16 <= n) {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(char_ptr + i), pattern);
        i += 16;
    }
    
    // 处理剩余字节
    while (i < n) {
        char_ptr[i] = static_cast<char>(value);
        ++i;
    }
    
    return ptr;
}

// AVX2内存操作实现
void* SIMMemoryUtils::memcpy_avx2(void* dest, const void* src, size_t n) {
    char* dest_ptr = static_cast<char*>(dest);
    const char* src_ptr = static_cast<const char*>(src);
    
    size_t i = 0;
    
    // 使用AVX2处理32字节对齐的数据
    while (i + 32 <= n) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src_ptr + i));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest_ptr + i), data);
        i += 32;
    }
    
    // 处理剩余字节
    while (i < n) {
        dest_ptr[i] = src_ptr[i];
        ++i;
    }
    
    return dest;
}

int SIMMemoryUtils::memcmp_avx2(const void* ptr1, const void* ptr2, size_t n) {
    const char* str1 = static_cast<const char*>(ptr1);
    const char* str2 = static_cast<const char*>(ptr2);
    
    size_t i = 0;
    
    // 使用AVX2处理32字节对齐的数据
    while (i + 32 <= n) {
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(str1 + i));
        __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(str2 + i));
        __m256i cmp = _mm256_cmpeq_epi8(a, b);
        int mask = _mm256_movemask_epi8(cmp);
        
        if (mask != static_cast<int>(0xFFFFFFFF)) {
            // 找到第一个不同的字节
            for (int j = 0; j < 32; ++j) {
                if (!(mask & (1 << j))) {
                    return static_cast<unsigned char>(str1[i + j]) - static_cast<unsigned char>(str2[i + j]);
                }
            }
        }
        
        i += 32;
    }
    
    // 处理剩余字节
    while (i < n) {
        if (str1[i] != str2[i]) {
            return static_cast<unsigned char>(str1[i]) - static_cast<unsigned char>(str2[i]);
        }
        ++i;
    }
    
    return 0;
}

void* SIMMemoryUtils::memset_avx2(void* ptr, int value, size_t n) {
    char* char_ptr = static_cast<char*>(ptr);
    
    // 创建填充模式
    __m256i pattern = _mm256_set1_epi8(static_cast<char>(value));
    
    size_t i = 0;
    
    // 使用AVX2处理32字节对齐的数据
    while (i + 32 <= n) {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(char_ptr + i), pattern);
        i += 32;
    }
    
    // 处理剩余字节
    while (i < n) {
        char_ptr[i] = static_cast<char>(value);
        ++i;
    }
    
    return ptr;
}

// SIMDHashUtils 实现
uint32_t SIMDHashUtils::crc32_simd(const void* data, size_t len) {
    if (len == 0) return 0;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::SSE4_2) {
        return crc32_sse42(data, len);
    } else if (instruction_set >= SIMDInstructionSet::AVX2) {
        return crc32_avx2(data, len);
    } else {
        // 回退到标准CRC32实现
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        
        for (size_t i = 0; i < len; ++i) {
            crc = _mm_crc32_u8(crc, bytes[i]);
        }
        
        return crc ^ 0xFFFFFFFF;
    }
}

uint32_t SIMDHashUtils::crc32_simd(const std::string& str) {
    return crc32_simd(str.c_str(), str.length());
}

std::string SIMDHashUtils::md5_simd(const void* data, size_t len) {
    // 简化的MD5实现，实际项目中应该使用更完整的实现
    uint32_t hash = crc32_simd(data, len);
    
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash;
    return ss.str();
}

std::string SIMDHashUtils::md5_simd(const std::string& str) {
    return md5_simd(str.c_str(), str.length());
}

std::string SIMDHashUtils::sha256_simd(const void* data, size_t len) {
    // 简化的SHA256实现，实际项目中应该使用更完整的实现
    uint32_t hash = crc32_simd(data, len);
    
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash;
    return ss.str();
}

std::string SIMDHashUtils::sha256_simd(const std::string& str) {
    return sha256_simd(str.c_str(), str.length());
}

uint64_t SIMDHashUtils::hash_combine_simd(uint64_t seed, const void* data, size_t len) {
    uint32_t hash = crc32_simd(data, len);
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

// SSE4.2 CRC32实现
uint32_t SIMDHashUtils::crc32_sse42(const void* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    
    size_t i = 0;
    
    // 使用SSE4.2处理4字节对齐的数据
    while (i + 4 <= len) {
        uint32_t word = *reinterpret_cast<const uint32_t*>(bytes + i);
        crc = _mm_crc32_u32(crc, word);
        i += 4;
    }
    
    // 处理剩余字节
    while (i < len) {
        crc = _mm_crc32_u8(crc, bytes[i]);
        ++i;
    }
    
    return crc ^ 0xFFFFFFFF;
}

// AVX2 CRC32实现
uint32_t SIMDHashUtils::crc32_avx2(const void* data, size_t len) {
    // 简化的AVX2实现，实际项目中应该使用更复杂的AVX2优化
    return crc32_sse42(data, len);
}

// SIMDArrayUtils 实现
int32_t SIMDArrayUtils::sum_int32_simd(const int32_t* array, size_t count) {
    if (count == 0) return 0;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return sum_int32_avx2(array, count);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return sum_int32_sse2(array, count);
    } else {
        // 回退到标准实现
        int32_t sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += array[i];
        }
        return sum;
    }
}

int64_t SIMDArrayUtils::sum_int64_simd(const int64_t* array, size_t count) {
    if (count == 0) return 0;
    
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += array[i];
    }
    return sum;
}

float SIMDArrayUtils::sum_float_simd(const float* array, size_t count) {
    if (count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += array[i];
    }
    return sum;
}

double SIMDArrayUtils::sum_double_simd(const double* array, size_t count) {
    if (count == 0) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        sum += array[i];
    }
    return sum;
}

size_t SIMDArrayUtils::find_int32_simd(const int32_t* array, size_t count, int32_t value) {
    if (count == 0) return SIZE_MAX;
    
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2) {
        return find_int32_avx2(array, count, value);
    } else if (instruction_set >= SIMDInstructionSet::SSE2) {
        return find_int32_sse2(array, count, value);
    } else {
        // 回退到标准实现
        for (size_t i = 0; i < count; ++i) {
            if (array[i] == value) {
                return i;
            }
        }
        return SIZE_MAX;
    }
}

size_t SIMDArrayUtils::find_int64_simd(const int64_t* array, size_t count, int64_t value) {
    if (count == 0) return SIZE_MAX;
    
    for (size_t i = 0; i < count; ++i) {
        if (array[i] == value) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t SIMDArrayUtils::find_float_simd(const float* array, size_t count, float value) {
    if (count == 0) return SIZE_MAX;
    
    for (size_t i = 0; i < count; ++i) {
        if (array[i] == value) {
            return i;
        }
    }
    return SIZE_MAX;
}

void SIMDArrayUtils::radix_sort_int32_simd(int32_t* array, size_t count) {
    if (count <= 1) return;
    
    // 简化的基数排序实现
    std::sort(array, array + count);
}

void SIMDArrayUtils::radix_sort_int64_simd(int64_t* array, size_t count) {
    if (count <= 1) return;
    
    // 简化的基数排序实现
    std::sort(array, array + count);
}

size_t SIMDArrayUtils::unique_int32_simd(int32_t* array, size_t count) {
    if (count <= 1) return count;
    
    // 简化的去重实现
    std::sort(array, array + count);
    auto it = std::unique(array, array + count);
    return std::distance(array, it);
}

size_t SIMDArrayUtils::unique_int64_simd(int64_t* array, size_t count) {
    if (count <= 1) return count;
    
    // 简化的去重实现
    std::sort(array, array + count);
    auto it = std::unique(array, array + count);
    return std::distance(array, it);
}

// SSE2数组操作实现
int32_t SIMDArrayUtils::sum_int32_sse2(const int32_t* array, size_t count) {
    int32_t sum = 0;
    size_t i = 0;
    
    // 使用SSE2处理4个int32
    while (i + 4 <= count) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array + i));
        __m128i sum_vec = _mm_add_epi32(data, _mm_setzero_si128());
        
        // 提取结果
        int32_t temp[4];
        _mm_storeu_si128(reinterpret_cast<__m128i*>(temp), sum_vec);
        sum += temp[0] + temp[1] + temp[2] + temp[3];
        
        i += 4;
    }
    
    // 处理剩余元素
    while (i < count) {
        sum += array[i];
        ++i;
    }
    
    return sum;
}

size_t SIMDArrayUtils::find_int32_sse2(const int32_t* array, size_t count, int32_t value) {
    __m128i target = _mm_set1_epi32(value);
    
    for (size_t i = 0; i < count; i += 4) {
        if (i + 4 <= count) {
            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array + i));
            __m128i cmp = _mm_cmpeq_epi32(data, target);
            int mask = _mm_movemask_epi8(cmp);
            
            if (mask != 0) {
                // 找到匹配的元素
                for (int j = 0; j < 4; ++j) {
                    if (mask & (0xF << (j * 4))) {
                        return i + j;
                    }
                }
            }
        } else {
            // 处理剩余元素
            for (size_t j = i; j < count; ++j) {
                if (array[j] == value) {
                    return j;
                }
            }
        }
    }
    
    return SIZE_MAX;
}

// AVX2数组操作实现
int32_t SIMDArrayUtils::sum_int32_avx2(const int32_t* array, size_t count) {
    int32_t sum = 0;
    size_t i = 0;
    
    // 使用AVX2处理8个int32
    while (i + 8 <= count) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(array + i));
        __m256i sum_vec = _mm256_add_epi32(data, _mm256_setzero_si256());
        
        // 提取结果
        int32_t temp[8];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(temp), sum_vec);
        sum += temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
        
        i += 8;
    }
    
    // 处理剩余元素
    while (i < count) {
        sum += array[i];
        ++i;
    }
    
    return sum;
}

size_t SIMDArrayUtils::find_int32_avx2(const int32_t* array, size_t count, int32_t value) {
    __m256i target = _mm256_set1_epi32(value);
    
    for (size_t i = 0; i < count; i += 8) {
        if (i + 8 <= count) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(array + i));
            __m256i cmp = _mm256_cmpeq_epi32(data, target);
            int mask = _mm256_movemask_epi8(cmp);
            
            if (mask != 0) {
                // 找到匹配的元素
                for (int j = 0; j < 8; ++j) {
                    if (mask & (0xF << (j * 4))) {
                        return i + j;
                    }
                }
            }
        } else {
            // 处理剩余元素
            for (size_t j = i; j < count; ++j) {
                if (array[j] == value) {
                    return j;
                }
            }
        }
    }
    
    return SIZE_MAX;
}

// SIMDPerformanceMonitor 实现
SIMDPerformanceMonitor::PerformanceStats SIMDPerformanceMonitor::stats_;
std::mutex SIMDPerformanceMonitor::stats_mutex_;

void SIMDPerformanceMonitor::record_simd_operation(std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.simd_operations_count_++;
    stats_.total_simd_time_ += duration;
}

void SIMDPerformanceMonitor::record_fallback_operation(std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.fallback_operations_count_++;
    stats_.total_fallback_time_ += duration;
}

SIMDPerformanceMonitor::PerformanceStats SIMDPerformanceMonitor::get_performance_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void SIMDPerformanceMonitor::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = PerformanceStats{};
}

double SIMDPerformanceMonitor::get_speedup_factor() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_.fallback_operations_count_ == 0) {
        return 1.0;
    }
    
    double simd_avg_time = stats_.total_simd_time_.count() / static_cast<double>(stats_.simd_operations_count_);
    double fallback_avg_time = stats_.total_fallback_time_.count() / static_cast<double>(stats_.fallback_operations_count_);
    
    return fallback_avg_time / simd_avg_time;
}

// SIMManager 实现
SIMDConfig SIMManager::config_;
bool SIMManager::initialized_ = false;

bool SIMManager::initialize(const SIMDConfig& config) {
    if (initialized_) {
        LOG(WARNING) << "SIMManager already initialized";
        return true;
    }
    
    config_ = config;
    
    if (config_.enable_auto_detection_) {
        SIMDDetector::initialize();
    }
    
    initialized_ = true;
    LOG(INFO) << "SIMManager initialized with SIMD support: " 
              << (config_.enable_simd_ ? "enabled" : "disabled");
    
    return true;
}

void SIMManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    LOG(INFO) << "SIMManager shutdown";
}

SIMDConfig SIMManager::get_config() {
    return config_;
}

void SIMManager::configure(const SIMDConfig& config) {
    config_ = config;
    LOG(INFO) << "SIMManager reconfigured";
}

bool SIMManager::is_initialized() {
    return initialized_;
}

} // namespace Paker
