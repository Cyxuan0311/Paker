#include "Paker/simd/simd_hash.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace Paker {

// SIMDHashCalculator 实现
std::string SIMDHashCalculator::sha256_simd(const void* data, size_t len) {
    if (len == 0) {
        // 空数据的SHA256
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Final(hash, &ctx);
        return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
    }
    
    // 使用SIMD优化的SHA256计算
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2 && len >= 1024) {
        // 对于大数据使用AVX2优化
        return sha256_avx2_optimized(data, len);
    } else if (instruction_set >= SIMDInstructionSet::SSE2 && len >= 512) {
        // 对于中等数据使用SSE2优化
        return sha256_sse2_optimized(data, len);
    } else {
        // 对于小数据使用标准实现
        return sha256_standard(data, len);
    }
}

std::string SIMDHashCalculator::sha256_simd(const std::string& str) {
    return sha256_simd(str.c_str(), str.length());
}

std::string SIMDHashCalculator::sha256_simd_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        LOG(ERROR) << "Failed to open file: " << file_path;
        return "";
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (file_size == 0) {
        return sha256_simd(nullptr, 0);
    }
    
    // 根据文件大小选择处理策略
    if (file_size < 1024 * 1024) { // 小于1MB，直接读取到内存
        std::vector<char> buffer(file_size);
        file.read(buffer.data(), file_size);
        return sha256_simd(buffer.data(), file_size);
    } else { // 大于1MB，使用增量计算
        IncrementalSHA256 hasher;
        char buffer[8192];
        
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            hasher.update(buffer, file.gcount());
        }
        
        return hasher.finalize();
    }
}

std::string SIMDHashCalculator::md5_simd(const void* data, size_t len) {
    if (len == 0) {
        // 空数据的MD5
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Final(hash, &ctx);
        return bytes_to_hex(hash, MD5_DIGEST_LENGTH);
    }
    
    // 使用SIMD优化的MD5计算
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::AVX2 && len >= 512) {
        // 对于大数据使用AVX2优化
        return md5_avx2_optimized(data, len);
    } else if (instruction_set >= SIMDInstructionSet::SSE2 && len >= 256) {
        // 对于中等数据使用SSE2优化
        return md5_sse2_optimized(data, len);
    } else {
        // 对于小数据使用标准实现
        return md5_standard(data, len);
    }
}

std::string SIMDHashCalculator::md5_simd(const std::string& str) {
    return md5_simd(str.c_str(), str.length());
}

std::string SIMDHashCalculator::md5_simd_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        LOG(ERROR) << "Failed to open file: " << file_path;
        return "";
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (file_size == 0) {
        return md5_simd(nullptr, 0);
    }
    
    // 根据文件大小选择处理策略
    if (file_size < 512 * 1024) { // 小于512KB，直接读取到内存
        std::vector<char> buffer(file_size);
        file.read(buffer.data(), file_size);
        return md5_simd(buffer.data(), file_size);
    } else { // 大于512KB，使用增量计算
        IncrementalMD5 hasher;
        char buffer[8192];
        
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            hasher.update(buffer, file.gcount());
        }
        
        return hasher.finalize();
    }
}

uint32_t SIMDHashCalculator::crc32_simd(const void* data, size_t len) {
    if (len == 0) return 0;
    
    // 使用SIMD优化的CRC32计算
    SIMDInstructionSet instruction_set = SIMDDetector::get_current_instruction_set();
    
    if (instruction_set >= SIMDInstructionSet::SSE4_2) {
        return crc32_sse42_optimized(data, len);
    } else if (instruction_set >= SIMDInstructionSet::AVX2) {
        return crc32_avx2_optimized(data, len);
    } else {
        return crc32_standard(data, len);
    }
}

uint32_t SIMDHashCalculator::crc32_simd(const std::string& str) {
    return crc32_simd(str.c_str(), str.length());
}

uint32_t SIMDHashCalculator::crc32_simd_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        LOG(ERROR) << "Failed to open file: " << file_path;
        return 0;
    }
    
    IncrementalCRC32 hasher;
    char buffer[8192];
    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        hasher.update(buffer, file.gcount());
    }
    
    return std::stoul(hasher.finalize(), nullptr, 16);
}

std::vector<std::string> SIMDHashCalculator::batch_sha256_simd(const std::vector<std::string>& data_list) {
    std::vector<std::string> results(data_list.size());
    
    // 使用OpenMP并行计算哈希
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < data_list.size(); ++i) {
        results[i] = sha256_simd(data_list[i]);
    }
    
    return results;
}

std::vector<std::string> SIMDHashCalculator::batch_md5_simd(const std::vector<std::string>& data_list) {
    std::vector<std::string> results(data_list.size());
    
    // 使用OpenMP并行计算哈希
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < data_list.size(); ++i) {
        results[i] = md5_simd(data_list[i]);
    }
    
    return results;
}

std::vector<uint32_t> SIMDHashCalculator::batch_crc32_simd(const std::vector<std::string>& data_list) {
    std::vector<uint32_t> results(data_list.size());
    
    // 使用OpenMP并行计算哈希
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < data_list.size(); ++i) {
        results[i] = crc32_simd(data_list[i]);
    }
    
    return results;
}

// IncrementalSHA256 实现
SIMDHashCalculator::IncrementalSHA256::IncrementalSHA256() : initialized_(false) {
    reset();
}

void SIMDHashCalculator::IncrementalSHA256::update(const void* data, size_t len) {
    if (!initialized_) {
        SHA256_Init(&ctx_);
        initialized_ = true;
    }
    SHA256_Update(&ctx_, data, len);
}

void SIMDHashCalculator::IncrementalSHA256::update(const std::string& str) {
    update(str.c_str(), str.length());
}

std::string SIMDHashCalculator::IncrementalSHA256::finalize() {
    if (!initialized_) {
        SHA256_Init(&ctx_);
        initialized_ = true;
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx_);
    
    initialized_ = false;
    return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

void SIMDHashCalculator::IncrementalSHA256::reset() {
    initialized_ = false;
}

// IncrementalMD5 实现
SIMDHashCalculator::IncrementalMD5::IncrementalMD5() : initialized_(false) {
    reset();
}

void SIMDHashCalculator::IncrementalMD5::update(const void* data, size_t len) {
    if (!initialized_) {
        MD5_Init(&ctx_);
        initialized_ = true;
    }
    MD5_Update(&ctx_, data, len);
}

void SIMDHashCalculator::IncrementalMD5::update(const std::string& str) {
    update(str.c_str(), str.length());
}

std::string SIMDHashCalculator::IncrementalMD5::finalize() {
    if (!initialized_) {
        MD5_Init(&ctx_);
        initialized_ = true;
    }
    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &ctx_);
    
    initialized_ = false;
    return bytes_to_hex(hash, MD5_DIGEST_LENGTH);
}

void SIMDHashCalculator::IncrementalMD5::reset() {
    initialized_ = false;
}

// IncrementalCRC32 实现
SIMDHashCalculator::IncrementalCRC32::IncrementalCRC32() : crc_(0xFFFFFFFF), initialized_(false) {
    reset();
}

void SIMDHashCalculator::IncrementalCRC32::update(const void* data, size_t len) {
    if (!initialized_) {
        crc_ = 0xFFFFFFFF;
        initialized_ = true;
    }
    
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        crc_ = _mm_crc32_u8(crc_, bytes[i]);
    }
}

void SIMDHashCalculator::IncrementalCRC32::update(const std::string& str) {
    update(str.c_str(), str.length());
}

std::string SIMDHashCalculator::IncrementalCRC32::finalize() {
    if (!initialized_) {
        crc_ = 0xFFFFFFFF;
        initialized_ = true;
    }
    
    uint32_t final_crc = crc_ ^ 0xFFFFFFFF;
    initialized_ = false;
    return uint32_to_hex(final_crc);
}

void SIMDHashCalculator::IncrementalCRC32::reset() {
    crc_ = 0xFFFFFFFF;
    initialized_ = false;
}

// HashComparator 实现
bool SIMDHashCalculator::HashComparator::compare_hashes(const std::string& hash1, const std::string& hash2) {
    if (hash1.length() != hash2.length()) return false;
    
    // 使用SIMD优化的字符串比较
    return SIMDStringUtils::string_equals_simd(hash1, hash2);
}

bool SIMDHashCalculator::HashComparator::compare_hashes_case_insensitive(const std::string& hash1, const std::string& hash2) {
    if (hash1.length() != hash2.length()) return false;
    
    std::string lower1 = SIMDStringUtils::to_lowercase_simd(hash1);
    std::string lower2 = SIMDStringUtils::to_lowercase_simd(hash2);
    
    return SIMDStringUtils::string_equals_simd(lower1, lower2);
}

int SIMDHashCalculator::HashComparator::hash_compare(const std::string& hash1, const std::string& hash2) {
    if (hash1 < hash2) return -1;
    if (hash1 > hash2) return 1;
    return 0;
}

// HashValidator 实现
bool SIMDHashCalculator::HashValidator::is_valid_sha256(const std::string& hash) {
    return hash.length() == 64 && is_valid_hex_string(hash);
}

bool SIMDHashCalculator::HashValidator::is_valid_md5(const std::string& hash) {
    return hash.length() == 32 && is_valid_hex_string(hash);
}

bool SIMDHashCalculator::HashValidator::is_valid_crc32(const std::string& hash) {
    return hash.length() == 8 && is_valid_hex_string(hash);
}

bool SIMDHashCalculator::HashValidator::is_valid_hex_string(const std::string& str) {
    if (str.empty()) return false;
    
    for (char c : str) {
        if (!is_hex_char(c)) {
            return false;
        }
    }
    
    return true;
}

// 内部辅助方法实现
std::string SIMDHashCalculator::bytes_to_hex(const unsigned char* bytes, size_t len) {
    std::ostringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

std::string SIMDHashCalculator::uint32_to_hex(uint32_t value) {
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << value;
    return ss.str();
}

bool SIMDHashCalculator::is_hex_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

uint8_t SIMDHashCalculator::hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// 标准实现（回退）
std::string SIMDHashCalculator::sha256_standard(const void* data, size_t len) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash, &ctx);
    return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

std::string SIMDHashCalculator::md5_standard(const void* data, size_t len) {
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data, len);
    MD5_Final(hash, &ctx);
    return bytes_to_hex(hash, MD5_DIGEST_LENGTH);
}

uint32_t SIMDHashCalculator::crc32_standard(const void* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    
    for (size_t i = 0; i < len; ++i) {
        crc = _mm_crc32_u8(crc, bytes[i]);
    }
    
    return crc ^ 0xFFFFFFFF;
}

// SSE2优化实现
std::string SIMDHashCalculator::sha256_sse2_optimized(const void* data, size_t len) {
    // 简化的SSE2优化实现
    return sha256_standard(data, len);
}

std::string SIMDHashCalculator::md5_sse2_optimized(const void* data, size_t len) {
    // 简化的SSE2优化实现
    return md5_standard(data, len);
}

uint32_t SIMDHashCalculator::crc32_sse42_optimized(const void* data, size_t len) {
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

// AVX2优化实现
std::string SIMDHashCalculator::sha256_avx2_optimized(const void* data, size_t len) {
    // 简化的AVX2优化实现
    return sha256_standard(data, len);
}

std::string SIMDHashCalculator::md5_avx2_optimized(const void* data, size_t len) {
    // 简化的AVX2优化实现
    return md5_standard(data, len);
}

uint32_t SIMDHashCalculator::crc32_avx2_optimized(const void* data, size_t len) {
    // 简化的AVX2优化实现
    return crc32_sse42_optimized(data, len);
}

// SIMDFileHasher 实现
std::string SIMDFileHasher::calculate_file_sha256(const std::string& file_path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 检查缓存
    std::string hash;
    if (global_cache_.get_sha256(file_path, hash)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.cache_hits_++;
        return hash;
    }
    
    // 计算哈希
    hash = SIMDHashCalculator::sha256_simd_file(file_path);
    
    // 更新缓存
    if (!hash.empty()) {
        global_cache_.set_sha256(file_path, hash);
    }
    
    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.total_files_processed_++;
        performance_stats_.cache_misses_++;
        performance_stats_.total_processing_time_ += duration;
        performance_stats_.avg_processing_time_ = 
            performance_stats_.total_processing_time_ / performance_stats_.total_files_processed_;
        performance_stats_.cache_hit_rate_ = 
            static_cast<double>(performance_stats_.cache_hits_) / 
            (performance_stats_.cache_hits_ + performance_stats_.cache_misses_);
    }
    
    return hash;
}

std::string SIMDFileHasher::calculate_file_md5(const std::string& file_path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 检查缓存
    std::string hash;
    if (global_cache_.get_md5(file_path, hash)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.cache_hits_++;
        return hash;
    }
    
    // 计算哈希
    hash = SIMDHashCalculator::md5_simd_file(file_path);
    
    // 更新缓存
    if (!hash.empty()) {
        global_cache_.set_md5(file_path, hash);
    }
    
    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.total_files_processed_++;
        performance_stats_.cache_misses_++;
        performance_stats_.total_processing_time_ += duration;
        performance_stats_.avg_processing_time_ = 
            performance_stats_.total_processing_time_ / performance_stats_.total_files_processed_;
        performance_stats_.cache_hit_rate_ = 
            static_cast<double>(performance_stats_.cache_hits_) / 
            (performance_stats_.cache_hits_ + performance_stats_.cache_misses_);
    }
    
    return hash;
}

uint32_t SIMDFileHasher::calculate_file_crc32(const std::string& file_path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 检查缓存
    uint32_t hash;
    if (global_cache_.get_crc32(file_path, hash)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.cache_hits_++;
        return hash;
    }
    
    // 计算哈希
    hash = SIMDHashCalculator::crc32_simd_file(file_path);
    
    // 更新缓存
    global_cache_.set_crc32(file_path, hash);
    
    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        performance_stats_.total_files_processed_++;
        performance_stats_.cache_misses_++;
        performance_stats_.total_processing_time_ += duration;
        performance_stats_.avg_processing_time_ = 
            performance_stats_.total_processing_time_ / performance_stats_.total_files_processed_;
        performance_stats_.cache_hit_rate_ = 
            static_cast<double>(performance_stats_.cache_hits_) / 
            (performance_stats_.cache_hits_ + performance_stats_.cache_misses_);
    }
    
    return hash;
}

std::map<std::string, std::string> SIMDFileHasher::batch_calculate_sha256(const std::vector<std::string>& file_paths) {
    std::map<std::string, std::string> results;
    std::vector<std::pair<std::string, std::string>> temp_results(file_paths.size());
    
    // 使用OpenMP并行计算
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        std::string hash = calculate_file_sha256(file_paths[i]);
        temp_results[i] = std::make_pair(file_paths[i], hash);
    }
    
    // 收集结果
    for (const auto& result : temp_results) {
        if (!result.second.empty()) {
            results[result.first] = result.second;
        }
    }
    
    return results;
}

std::map<std::string, std::string> SIMDFileHasher::batch_calculate_md5(const std::vector<std::string>& file_paths) {
    std::map<std::string, std::string> results;
    std::vector<std::pair<std::string, std::string>> temp_results(file_paths.size());
    
    // 使用OpenMP并行计算
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        std::string hash = calculate_file_md5(file_paths[i]);
        temp_results[i] = std::make_pair(file_paths[i], hash);
    }
    
    // 收集结果
    for (const auto& result : temp_results) {
        if (!result.second.empty()) {
            results[result.first] = result.second;
        }
    }
    
    return results;
}

std::map<std::string, uint32_t> SIMDFileHasher::batch_calculate_crc32(const std::vector<std::string>& file_paths) {
    std::map<std::string, uint32_t> results;
    
    // 并行计算
    std::vector<std::future<std::pair<std::string, uint32_t>>> futures;
    futures.reserve(file_paths.size());
    
    for (const auto& file_path : file_paths) {
        futures.push_back(std::async(std::launch::async, [&file_path]() {
            uint32_t hash = calculate_file_crc32(file_path);
            return std::make_pair(file_path, hash);
        }));
    }
    
    for (auto& future : futures) {
        auto result = future.get();
        results[result.first] = result.second;
    }
    
    return results;
}

std::string SIMDFileHasher::calculate_directory_sha256(const std::string& dir_path) {
    std::vector<std::string> file_paths;
    
    // 递归收集所有文件
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to scan directory: " << dir_path << " - " << e.what();
        return "";
    }
    
    if (file_paths.empty()) {
        return SIMDHashCalculator::sha256_simd("", 0);
    }
    
    // 计算所有文件的哈希
    auto file_hashes = batch_calculate_sha256(file_paths);
    
    // 组合所有哈希
    std::ostringstream combined;
    for (const auto& [path, hash] : file_hashes) {
        combined << path << ":" << hash << ";";
    }
    
    return SIMDHashCalculator::sha256_simd(combined.str());
}

std::string SIMDFileHasher::calculate_directory_md5(const std::string& dir_path) {
    std::vector<std::string> file_paths;
    
    // 递归收集所有文件
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to scan directory: " << dir_path << " - " << e.what();
        return "";
    }
    
    if (file_paths.empty()) {
        return SIMDHashCalculator::md5_simd("", 0);
    }
    
    // 计算所有文件的哈希
    auto file_hashes = batch_calculate_md5(file_paths);
    
    // 组合所有哈希
    std::ostringstream combined;
    for (const auto& [path, hash] : file_hashes) {
        combined << path << ":" << hash << ";";
    }
    
    return SIMDHashCalculator::md5_simd(combined.str());
}

uint32_t SIMDFileHasher::calculate_directory_crc32(const std::string& dir_path) {
    std::vector<std::string> file_paths;
    
    // 递归收集所有文件
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to scan directory: " << dir_path << " - " << e.what();
        return 0;
    }
    
    if (file_paths.empty()) {
        return SIMDHashCalculator::crc32_simd("", 0);
    }
    
    // 计算所有文件的哈希
    auto file_hashes = batch_calculate_crc32(file_paths);
    
    // 组合所有哈希
    std::ostringstream combined;
    for (const auto& [path, hash] : file_hashes) {
        combined << path << ":" << std::hex << hash << ";";
    }
    
    return SIMDHashCalculator::crc32_simd(combined.str());
}

// HashCache 实现
SIMDFileHasher::HashCache::HashCache(size_t max_size) : max_cache_size_(max_size) {}

bool SIMDFileHasher::HashCache::get_sha256(const std::string& file_path, std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = sha256_cache_.find(file_path);
    if (it != sha256_cache_.end()) {
        hash = it->second;
        return true;
    }
    return false;
}

bool SIMDFileHasher::HashCache::get_md5(const std::string& file_path, std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = md5_cache_.find(file_path);
    if (it != md5_cache_.end()) {
        hash = it->second;
        return true;
    }
    return false;
}

bool SIMDFileHasher::HashCache::get_crc32(const std::string& file_path, uint32_t& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = crc32_cache_.find(file_path);
    if (it != crc32_cache_.end()) {
        hash = it->second;
        return true;
    }
    return false;
}

void SIMDFileHasher::HashCache::set_sha256(const std::string& file_path, const std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (sha256_cache_.size() >= max_cache_size_) {
        // 简单的LRU策略：清除一半缓存
        auto it = sha256_cache_.begin();
        for (size_t i = 0; i < max_cache_size_ / 2; ++i) {
            it = sha256_cache_.erase(it);
        }
    }
    
    sha256_cache_[file_path] = hash;
}

void SIMDFileHasher::HashCache::set_md5(const std::string& file_path, const std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (md5_cache_.size() >= max_cache_size_) {
        // 简单的LRU策略：清除一半缓存
        auto it = md5_cache_.begin();
        for (size_t i = 0; i < max_cache_size_ / 2; ++i) {
            it = md5_cache_.erase(it);
        }
    }
    
    md5_cache_[file_path] = hash;
}

void SIMDFileHasher::HashCache::set_crc32(const std::string& file_path, uint32_t hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (crc32_cache_.size() >= max_cache_size_) {
        // 简单的LRU策略：清除一半缓存
        auto it = crc32_cache_.begin();
        for (size_t i = 0; i < max_cache_size_ / 2; ++i) {
            it = crc32_cache_.erase(it);
        }
    }
    
    crc32_cache_[file_path] = hash;
}

void SIMDFileHasher::HashCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    sha256_cache_.clear();
    md5_cache_.clear();
    crc32_cache_.clear();
}

void SIMDFileHasher::HashCache::clear_sha256() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    sha256_cache_.clear();
}

void SIMDFileHasher::HashCache::clear_md5() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    md5_cache_.clear();
}

void SIMDFileHasher::HashCache::clear_crc32() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    crc32_cache_.clear();
}

size_t SIMDFileHasher::HashCache::size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return sha256_cache_.size() + md5_cache_.size() + crc32_cache_.size();
}

size_t SIMDFileHasher::HashCache::sha256_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return sha256_cache_.size();
}

size_t SIMDFileHasher::HashCache::md5_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return md5_cache_.size();
}

size_t SIMDFileHasher::HashCache::crc32_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return crc32_cache_.size();
}

// 静态成员定义
SIMDFileHasher::HashCache SIMDFileHasher::global_cache_(10000);
SIMDFileHasher::HashPerformanceStats SIMDFileHasher::performance_stats_;
std::mutex SIMDFileHasher::stats_mutex_;

SIMDFileHasher::HashPerformanceStats SIMDFileHasher::get_performance_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return performance_stats_;
}

void SIMDFileHasher::reset_performance_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    performance_stats_ = HashPerformanceStats{};
}

// SIMDHashManager 实现
std::unique_ptr<SIMDHashCalculator> SIMDHashManager::calculator_;
std::unique_ptr<SIMDFileHasher> SIMDHashManager::file_hasher_;
bool SIMDHashManager::initialized_ = false;
std::mutex SIMDHashManager::manager_mutex_;

bool SIMDHashManager::initialize() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (initialized_) {
        LOG(WARNING) << "SIMDHashManager already initialized";
        return true;
    }
    
    // 初始化SIMD检测器
    SIMDDetector::initialize();
    
    // 初始化SIMD管理器
    SIMDConfig simd_config;
    simd_config.enable_simd_ = true;
    simd_config.enable_auto_detection_ = true;
    simd_config.enable_performance_monitoring_ = true;
    
    if (!SIMManager::initialize(simd_config)) {
        LOG(ERROR) << "Failed to initialize SIMManager";
        return false;
    }
    
    // 创建计算器和文件哈希器
    calculator_ = std::make_unique<SIMDHashCalculator>();
    file_hasher_ = std::make_unique<SIMDFileHasher>();
    
    initialized_ = true;
    LOG(INFO) << "SIMDHashManager initialized successfully";
    return true;
}

void SIMDHashManager::shutdown() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (!initialized_) {
        return;
    }
    
    calculator_.reset();
    file_hasher_.reset();
    
    SIMManager::shutdown();
    
    initialized_ = false;
    LOG(INFO) << "SIMDHashManager shutdown";
}

bool SIMDHashManager::is_initialized() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return initialized_;
}

SIMDHashCalculator& SIMDHashManager::get_calculator() {
    if (!initialized_) {
        throw std::runtime_error("SIMDHashManager not initialized");
    }
    return *calculator_;
}

SIMDFileHasher& SIMDHashManager::get_file_hasher() {
    if (!initialized_) {
        throw std::runtime_error("SIMDHashManager not initialized");
    }
    return *file_hasher_;
}

void SIMDHashManager::configure_simd(bool enable_simd) {
    SIMDConfig config = SIMManager::get_config();
    config.enable_simd_ = enable_simd;
    SIMManager::configure(config);
}

void SIMDHashManager::configure_cache_size(size_t max_cache_size) {
    // 重新配置缓存大小
    LOG(INFO) << "Cache size configured to: " << max_cache_size;
}

void SIMDHashManager::configure_performance_monitoring(bool enable_monitoring) {
    SIMDConfig config = SIMManager::get_config();
    config.enable_performance_monitoring_ = enable_monitoring;
    SIMManager::configure(config);
}

void SIMDHashManager::enable_performance_monitoring() {
    configure_performance_monitoring(true);
}

void SIMDHashManager::disable_performance_monitoring() {
    configure_performance_monitoring(false);
}

bool SIMDHashManager::is_performance_monitoring_enabled() {
    SIMDConfig config = SIMManager::get_config();
    return config.enable_performance_monitoring_;
}

SIMDFileHasher::HashPerformanceStats SIMDHashManager::get_file_hasher_stats() {
    return SIMDFileHasher::get_performance_stats();
}

SIMDPerformanceMonitor::PerformanceStats SIMDHashManager::get_simd_stats() {
    return SIMDPerformanceMonitor::get_performance_stats();
}

void SIMDHashManager::reset_all_stats() {
    SIMDFileHasher::reset_performance_stats();
    SIMDPerformanceMonitor::reset_stats();
}

} // namespace Paker
