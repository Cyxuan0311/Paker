#pragma once

#include "Paker/common.h"
#include "Paker/simd/simd_utils.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

namespace Paker {

// SIMD优化的哈希计算器
class SIMDHashCalculator {
public:
    // 快速SHA256计算
    static std::string sha256_simd(const void* data, size_t len);
    static std::string sha256_simd(const std::string& str);
    static std::string sha256_simd_file(const std::string& file_path);
    
    // 快速MD5计算
    static std::string md5_simd(const void* data, size_t len);
    static std::string md5_simd(const std::string& str);
    static std::string md5_simd_file(const std::string& file_path);
    
    // 快速CRC32计算
    static uint32_t crc32_simd(const void* data, size_t len);
    static uint32_t crc32_simd(const std::string& str);
    static uint32_t crc32_simd_file(const std::string& file_path);
    
    // 批量哈希计算
    static std::vector<std::string> batch_sha256_simd(const std::vector<std::string>& data_list);
    static std::vector<std::string> batch_md5_simd(const std::vector<std::string>& data_list);
    static std::vector<uint32_t> batch_crc32_simd(const std::vector<std::string>& data_list);
    
    // 增量哈希计算
    class IncrementalHash {
    public:
        virtual ~IncrementalHash() = default;
        virtual void update(const void* data, size_t len) = 0;
        virtual void update(const std::string& str) = 0;
        virtual std::string finalize() = 0;
        virtual void reset() = 0;
    };
    
    // SHA256增量计算器
    class IncrementalSHA256 : public IncrementalHash {
    private:
        SHA256_CTX ctx_;
        bool initialized_;
        
    public:
        IncrementalSHA256();
        ~IncrementalSHA256() override = default;
        
        void update(const void* data, size_t len) override;
        void update(const std::string& str) override;
        std::string finalize() override;
        void reset() override;
    };
    
    // MD5增量计算器
    class IncrementalMD5 : public IncrementalHash {
    private:
        MD5_CTX ctx_;
        bool initialized_;
        
    public:
        IncrementalMD5();
        ~IncrementalMD5() override = default;
        
        void update(const void* data, size_t len) override;
        void update(const std::string& str) override;
        std::string finalize() override;
        void reset() override;
    };
    
    // CRC32增量计算器
    class IncrementalCRC32 : public IncrementalHash {
    private:
        uint32_t crc_;
        bool initialized_;
        
    public:
        IncrementalCRC32();
        ~IncrementalCRC32() override = default;
        
        void update(const void* data, size_t len) override;
        void update(const std::string& str) override;
        std::string finalize() override;
        void reset() override;
    };
    
    // 哈希比较器
    class HashComparator {
    public:
        static bool compare_hashes(const std::string& hash1, const std::string& hash2);
        static bool compare_hashes_case_insensitive(const std::string& hash1, const std::string& hash2);
        static int hash_compare(const std::string& hash1, const std::string& hash2);
    };
    
    // 哈希验证器
    class HashValidator {
    public:
        static bool is_valid_sha256(const std::string& hash);
        static bool is_valid_md5(const std::string& hash);
        static bool is_valid_crc32(const std::string& hash);
        static bool is_valid_hex_string(const std::string& str);
    };
    
private:
    // 内部辅助方法
    static std::string bytes_to_hex(const unsigned char* bytes, size_t len);
    static std::string uint32_to_hex(uint32_t value);
    static bool is_hex_char(char c);
    static uint8_t hex_char_to_byte(char c);
    
    // 私有实现方法
    static std::string sha256_standard(const void* data, size_t len);
    static std::string md5_standard(const void* data, size_t len);
    static uint32_t crc32_standard(const void* data, size_t len);
    static std::string sha256_sse2_optimized(const void* data, size_t len);
    static std::string md5_sse2_optimized(const void* data, size_t len);
    static uint32_t crc32_sse42_optimized(const void* data, size_t len);
    static std::string sha256_avx2_optimized(const void* data, size_t len);
    static std::string md5_avx2_optimized(const void* data, size_t len);
    static uint32_t crc32_avx2_optimized(const void* data, size_t len);
};

// SIMD优化的文件哈希计算器
class SIMDFileHasher {
public:
    // 单文件哈希计算
    static std::string calculate_file_sha256(const std::string& file_path);
    static std::string calculate_file_md5(const std::string& file_path);
    static uint32_t calculate_file_crc32(const std::string& file_path);
    
    // 批量文件哈希计算
    static std::map<std::string, std::string> batch_calculate_sha256(const std::vector<std::string>& file_paths);
    static std::map<std::string, std::string> batch_calculate_md5(const std::vector<std::string>& file_paths);
    static std::map<std::string, uint32_t> batch_calculate_crc32(const std::vector<std::string>& file_paths);
    
    // 目录哈希计算
    static std::string calculate_directory_sha256(const std::string& dir_path);
    static std::string calculate_directory_md5(const std::string& dir_path);
    static uint32_t calculate_directory_crc32(const std::string& dir_path);
    
    // 哈希缓存管理
    class HashCache {
    private:
        std::unordered_map<std::string, std::string> sha256_cache_;
        std::unordered_map<std::string, std::string> md5_cache_;
        std::unordered_map<std::string, uint32_t> crc32_cache_;
        mutable std::mutex cache_mutex_;
        size_t max_cache_size_;
        
    public:
        HashCache(size_t max_size = 10000);
        ~HashCache() = default;
        
        bool get_sha256(const std::string& file_path, std::string& hash);
        bool get_md5(const std::string& file_path, std::string& hash);
        bool get_crc32(const std::string& file_path, uint32_t& hash);
        
        void set_sha256(const std::string& file_path, const std::string& hash);
        void set_md5(const std::string& file_path, const std::string& hash);
        void set_crc32(const std::string& file_path, uint32_t hash);
        
        void clear();
        void clear_sha256();
        void clear_md5();
        void clear_crc32();
        
        size_t size() const;
        size_t sha256_size() const;
        size_t md5_size() const;
        size_t crc32_size() const;
    };
    
    // 性能统计
    struct HashPerformanceStats {
        size_t total_files_processed_;
        size_t cache_hits_;
        size_t cache_misses_;
        std::chrono::milliseconds total_processing_time_;
        std::chrono::milliseconds avg_processing_time_;
        double cache_hit_rate_;
        
        HashPerformanceStats() 
            : total_files_processed_(0)
            , cache_hits_(0)
            , cache_misses_(0)
            , total_processing_time_(0)
            , avg_processing_time_(0)
            , cache_hit_rate_(0.0) {}
    };
    
    static HashPerformanceStats get_performance_stats();
    static void reset_performance_stats();
    
private:
    static HashCache global_cache_;
    static HashPerformanceStats performance_stats_;
    static std::mutex stats_mutex_;
    
    // 内部辅助方法
    static bool file_exists(const std::string& file_path);
    static size_t get_file_size(const std::string& file_path);
    static std::string get_file_extension(const std::string& file_path);
    static bool is_text_file(const std::string& file_path);
    static bool is_binary_file(const std::string& file_path);
};

// SIMD优化的哈希管理器
class SIMDHashManager {
private:
    static std::unique_ptr<SIMDHashCalculator> calculator_;
    static std::unique_ptr<SIMDFileHasher> file_hasher_;
    static bool initialized_;
    static std::mutex manager_mutex_;
    
public:
    static bool initialize();
    static void shutdown();
    static bool is_initialized();
    
    // 获取计算器实例
    static SIMDHashCalculator& get_calculator();
    static SIMDFileHasher& get_file_hasher();
    
    // 配置管理
    static void configure_simd(bool enable_simd);
    static void configure_cache_size(size_t max_cache_size);
    static void configure_performance_monitoring(bool enable_monitoring);
    
    // 性能监控
    static void enable_performance_monitoring();
    static void disable_performance_monitoring();
    static bool is_performance_monitoring_enabled();
    
    // 统计信息
    static SIMDFileHasher::HashPerformanceStats get_file_hasher_stats();
    static SIMDPerformanceMonitor::PerformanceStats get_simd_stats();
    static void reset_all_stats();
};

} // namespace Paker
