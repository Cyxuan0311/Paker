#pragma once

#include "Paker/common.h"
#include <omp.h>
#include <filesystem>
#include <vector>
#include <string>
#include <future>
#include <functional>

namespace Paker {

/**
 * @brief OpenMP优化的I/O操作管理器
 * 
 * 使用OpenMP并行化密集I/O操作，特别适用于批量文件处理
 */
class OpenMPIOManager {
public:
    /**
     * @brief 构造函数
     * @param max_threads 最大线程数，0表示使用默认值
     */
    explicit OpenMPIOManager(int max_threads = 0);
    
    /**
     * @brief 析构函数
     */
    ~OpenMPIOManager() = default;
    
    /**
     * @brief 并行读取多个文本文件
     * @param file_paths 文件路径列表
     * @return 文件内容列表，与输入路径一一对应
     */
    std::vector<std::string> read_text_files_parallel(const std::vector<std::string>& file_paths);
    
    /**
     * @brief 并行读取多个二进制文件
     * @param file_paths 文件路径列表
     * @return 文件数据列表，与输入路径一一对应
     */
    std::vector<std::vector<char>> read_binary_files_parallel(const std::vector<std::string>& file_paths);
    
    /**
     * @brief 并行写入多个文本文件
     * @param file_contents 文件路径和内容的配对列表
     * @return 写入结果列表
     */
    std::vector<bool> write_text_files_parallel(const std::vector<std::pair<std::string, std::string>>& file_contents);
    
    /**
     * @brief 并行写入多个二进制文件
     * @param file_data 文件路径和数据的配对列表
     * @return 写入结果列表
     */
    std::vector<bool> write_binary_files_parallel(const std::vector<std::pair<std::string, std::vector<char>>>& file_data);
    
    /**
     * @brief 并行计算文件哈希值
     * @param file_paths 文件路径列表
     * @param hash_algorithm 哈希算法类型
     * @return 哈希值列表
     */
    std::vector<std::string> calculate_file_hashes_parallel(
        const std::vector<std::string>& file_paths,
        const std::string& hash_algorithm = "sha256");
    
    /**
     * @brief 并行复制文件
     * @param source_dest_pairs 源文件路径和目标文件路径的配对列表
     * @return 复制结果列表
     */
    std::vector<bool> copy_files_parallel(const std::vector<std::pair<std::string, std::string>>& source_dest_pairs);
    
    /**
     * @brief 并行创建目录
     * @param directory_paths 目录路径列表
     * @return 创建结果列表
     */
    std::vector<bool> create_directories_parallel(const std::vector<std::string>& directory_paths);
    
    /**
     * @brief 并行删除文件
     * @param file_paths 文件路径列表
     * @return 删除结果列表
     */
    std::vector<bool> delete_files_parallel(const std::vector<std::string>& file_paths);
    
    /**
     * @brief 并行列出目录内容
     * @param directory_paths 目录路径列表
     * @return 每个目录的文件列表
     */
    std::vector<std::vector<std::string>> list_directories_parallel(const std::vector<std::string>& directory_paths);
    
    /**
     * @brief 获取当前线程数
     * @return 当前使用的线程数
     */
    int get_thread_count() const;
    
    /**
     * @brief 设置线程数
     * @param thread_count 线程数
     */
    void set_thread_count(int thread_count);
    
    /**
     * @brief 获取性能统计信息
     * @return 性能统计信息
     */
    struct PerformanceStats {
        size_t total_operations = 0;
        size_t successful_operations = 0;
        size_t failed_operations = 0;
        double total_time_ms = 0.0;
        double average_time_ms = 0.0;
        double throughput_mbps = 0.0;
    };
    
    PerformanceStats get_performance_stats() const;
    
    /**
     * @brief 重置性能统计
     */
    void reset_performance_stats();

private:
    int max_threads_;
    mutable std::mutex stats_mutex_;
    PerformanceStats stats_;
    
    /**
     * @brief 执行单个文件读取操作
     * @param file_path 文件路径
     * @param is_binary 是否为二进制文件
     * @return 文件内容或数据
     */
    std::pair<bool, std::string> read_single_text_file(const std::string& file_path);
    std::pair<bool, std::vector<char>> read_single_binary_file(const std::string& file_path);
    
    /**
     * @brief 执行单个文件写入操作
     * @param file_path 文件路径
     * @param content 文件内容
     * @param is_binary 是否为二进制文件
     * @return 写入是否成功
     */
    bool write_single_text_file(const std::string& file_path, const std::string& content);
    bool write_single_binary_file(const std::string& file_path, const std::vector<char>& data);
    
    /**
     * @brief 更新性能统计
     * @param operation_time 操作时间（毫秒）
     * @param success 操作是否成功
     * @param data_size 数据大小（字节）
     */
    void update_stats(double operation_time, bool success, size_t data_size = 0);
    
    /**
     * @brief 计算文件哈希值
     * @param file_path 文件路径
     * @param algorithm 哈希算法
     * @return 哈希值
     */
    std::string calculate_file_hash(const std::string& file_path, const std::string& algorithm);
};

/**
 * @brief OpenMP优化的批量文件处理器
 * 
 * 专门用于处理大量文件的批量操作
 */
class OpenMPBatchProcessor {
public:
    /**
     * @brief 构造函数
     * @param batch_size 批处理大小
     * @param max_threads 最大线程数
     */
    explicit OpenMPBatchProcessor(size_t batch_size = 100, int max_threads = 0);
    
    /**
     * @brief 批量处理文件操作
     * @tparam OperationType 操作类型
     * @param file_paths 文件路径列表
     * @param operation 操作函数
     * @return 操作结果列表
     */
    template<typename OperationType>
    std::vector<typename std::result_of<OperationType(const std::string&)>::type>
    process_batch(const std::vector<std::string>& file_paths, OperationType operation);
    
    /**
     * @brief 设置批处理大小
     * @param batch_size 批处理大小
     */
    void set_batch_size(size_t batch_size);
    
    /**
     * @brief 获取批处理大小
     * @return 批处理大小
     */
    size_t get_batch_size() const;

private:
    size_t batch_size_;
    int max_threads_;
};

// 模板实现
template<typename OperationType>
std::vector<typename std::result_of<OperationType(const std::string&)>::type>
OpenMPBatchProcessor::process_batch(const std::vector<std::string>& file_paths, OperationType operation) {
    using ResultType = typename std::result_of<OperationType(const std::string&)>::type;
    std::vector<ResultType> results;
    results.resize(file_paths.size());
    
    // 使用OpenMP并行处理
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic, batch_size_)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        try {
            results[i] = operation(file_paths[i]);
        } catch (const std::exception& e) {
            // 处理异常，设置默认值或错误值
            results[i] = ResultType{};
        }
    }
    
    return results;
}

} // namespace Paker
