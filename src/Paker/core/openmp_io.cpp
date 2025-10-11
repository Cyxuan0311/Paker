#include "Paker/core/openmp_io.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <glog/logging.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <zlib.h>

namespace Paker {

OpenMPIOManager::OpenMPIOManager(int max_threads) 
    : max_threads_(max_threads == 0 ? omp_get_max_threads() : max_threads) {
    omp_set_num_threads(max_threads_);
    LOG(INFO) << "OpenMPIOManager initialized with " << max_threads_ << " threads";
}

std::vector<std::string> OpenMPIOManager::read_text_files_parallel(const std::vector<std::string>& file_paths) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::string> results(file_paths.size());
    std::vector<bool> success_flags(file_paths.size(), false);
    
    // 使用OpenMP并行读取文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        auto [success, content] = read_single_text_file(file_paths[i]);
        if (success) {
            results[i] = std::move(content);
            success_flags[i] = true;
        } else {
            results[i] = "";
            success_flags[i] = false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(success_flags.begin(), success_flags.end(), true);
    update_stats(duration_ms, successful_count == file_paths.size(), 0);
    
    LOG(INFO) << "Parallel text file reading completed: " << successful_count 
              << "/" << file_paths.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<std::vector<char>> OpenMPIOManager::read_binary_files_parallel(const std::vector<std::string>& file_paths) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<char>> results(file_paths.size());
    std::vector<bool> success_flags(file_paths.size(), false);
    
    // 使用OpenMP并行读取文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        auto [success, data] = read_single_binary_file(file_paths[i]);
        if (success) {
            results[i] = std::move(data);
            success_flags[i] = true;
        } else {
            results[i] = std::vector<char>();
            success_flags[i] = false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(success_flags.begin(), success_flags.end(), true);
    update_stats(duration_ms, successful_count == file_paths.size(), 0);
    
    LOG(INFO) << "Parallel binary file reading completed: " << successful_count 
              << "/" << file_paths.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<bool> OpenMPIOManager::write_text_files_parallel(const std::vector<std::pair<std::string, std::string>>& file_contents) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<bool> results(file_contents.size());
    
    // 使用OpenMP并行写入文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_contents.size(); ++i) {
        results[i] = write_single_text_file(file_contents[i].first, file_contents[i].second);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(results.begin(), results.end(), true);
    update_stats(duration_ms, successful_count == file_contents.size(), 0);
    
    LOG(INFO) << "Parallel text file writing completed: " << successful_count 
              << "/" << file_contents.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<bool> OpenMPIOManager::write_binary_files_parallel(const std::vector<std::pair<std::string, std::vector<char>>>& file_data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<bool> results(file_data.size());
    
    // 使用OpenMP并行写入文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_data.size(); ++i) {
        results[i] = write_single_binary_file(file_data[i].first, file_data[i].second);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(results.begin(), results.end(), true);
    update_stats(duration_ms, successful_count == file_data.size(), 0);
    
    LOG(INFO) << "Parallel binary file writing completed: " << successful_count 
              << "/" << file_data.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<std::string> OpenMPIOManager::calculate_file_hashes_parallel(
    const std::vector<std::string>& file_paths, const std::string& hash_algorithm) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::string> results(file_paths.size());
    
    // 使用OpenMP并行计算哈希
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        results[i] = calculate_file_hash(file_paths[i], hash_algorithm);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count_if(results.begin(), results.end(), 
        [](const std::string& hash) { return !hash.empty(); });
    update_stats(duration_ms, successful_count == file_paths.size(), 0);
    
    LOG(INFO) << "Parallel hash calculation completed: " << successful_count 
              << "/" << file_paths.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<bool> OpenMPIOManager::copy_files_parallel(const std::vector<std::pair<std::string, std::string>>& source_dest_pairs) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<bool> results(source_dest_pairs.size());
    
    // 使用OpenMP并行复制文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < source_dest_pairs.size(); ++i) {
        try {
            std::filesystem::path source(source_dest_pairs[i].first);
            std::filesystem::path dest(source_dest_pairs[i].second);
            
            // 创建目标目录
            if (dest.has_parent_path()) {
                std::filesystem::create_directories(dest.parent_path());
            }
            
            // 复制文件
            std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing);
            results[i] = true;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to copy file " << source_dest_pairs[i].first 
                       << " to " << source_dest_pairs[i].second << ": " << e.what();
            results[i] = false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(results.begin(), results.end(), true);
    update_stats(duration_ms, successful_count == source_dest_pairs.size(), 0);
    
    LOG(INFO) << "Parallel file copying completed: " << successful_count 
              << "/" << source_dest_pairs.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<bool> OpenMPIOManager::create_directories_parallel(const std::vector<std::string>& directory_paths) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<bool> results(directory_paths.size());
    
    // 使用OpenMP并行创建目录
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < directory_paths.size(); ++i) {
        try {
            results[i] = std::filesystem::create_directories(directory_paths[i]);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to create directory " << directory_paths[i] << ": " << e.what();
            results[i] = false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(results.begin(), results.end(), true);
    update_stats(duration_ms, successful_count == directory_paths.size(), 0);
    
    LOG(INFO) << "Parallel directory creation completed: " << successful_count 
              << "/" << directory_paths.size() << " directories in " << duration_ms << "ms";
    
    return results;
}

std::vector<bool> OpenMPIOManager::delete_files_parallel(const std::vector<std::string>& file_paths) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<bool> results(file_paths.size());
    
    // 使用OpenMP并行删除文件
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        try {
            results[i] = std::filesystem::remove(file_paths[i]);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to delete file " << file_paths[i] << ": " << e.what();
            results[i] = false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count(results.begin(), results.end(), true);
    update_stats(duration_ms, successful_count == file_paths.size(), 0);
    
    LOG(INFO) << "Parallel file deletion completed: " << successful_count 
              << "/" << file_paths.size() << " files in " << duration_ms << "ms";
    
    return results;
}

std::vector<std::vector<std::string>> OpenMPIOManager::list_directories_parallel(const std::vector<std::string>& directory_paths) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<std::string>> results(directory_paths.size());
    
    // 使用OpenMP并行列出目录内容
    #pragma omp parallel for num_threads(max_threads_) schedule(dynamic)
    for (size_t i = 0; i < directory_paths.size(); ++i) {
        try {
            std::vector<std::string> files;
            for (const auto& entry : std::filesystem::directory_iterator(directory_paths[i])) {
                files.push_back(entry.path().string());
            }
            results[i] = std::move(files);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to list directory " << directory_paths[i] << ": " << e.what();
            results[i] = std::vector<std::string>();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 更新统计信息
    size_t successful_count = std::count_if(results.begin(), results.end(), 
        [](const std::vector<std::string>& files) { return !files.empty(); });
    update_stats(duration_ms, successful_count == directory_paths.size(), 0);
    
    LOG(INFO) << "Parallel directory listing completed: " << successful_count 
              << "/" << directory_paths.size() << " directories in " << duration_ms << "ms";
    
    return results;
}

int OpenMPIOManager::get_thread_count() const {
    return max_threads_;
}

void OpenMPIOManager::set_thread_count(int thread_count) {
    max_threads_ = thread_count;
    omp_set_num_threads(thread_count);
    LOG(INFO) << "OpenMP thread count set to " << thread_count;
}

OpenMPIOManager::PerformanceStats OpenMPIOManager::get_performance_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void OpenMPIOManager::reset_performance_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = PerformanceStats{};
}

// 私有方法实现
std::pair<bool, std::string> OpenMPIOManager::read_single_text_file(const std::string& file_path) {
    try {
        std::ifstream file(file_path, std::ios::in);
        if (!file.is_open()) {
            return {false, ""};
        }
        
        std::string content;
        content.reserve(std::filesystem::file_size(file_path));
        
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        
        return {true, std::move(content)};
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to read text file " << file_path << ": " << e.what();
        return {false, ""};
    }
}

std::pair<bool, std::vector<char>> OpenMPIOManager::read_single_binary_file(const std::string& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return {false, {}};
        }
        
        std::vector<char> data(std::filesystem::file_size(file_path));
        file.read(data.data(), data.size());
        
        return {true, std::move(data)};
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to read binary file " << file_path << ": " << e.what();
        return {false, {}};
    }
}

bool OpenMPIOManager::write_single_text_file(const std::string& file_path, const std::string& content) {
    try {
        // 创建目录
        std::filesystem::path path(file_path);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        
        std::ofstream file(file_path, std::ios::out);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to write text file " << file_path << ": " << e.what();
        return false;
    }
}

bool OpenMPIOManager::write_single_binary_file(const std::string& file_path, const std::vector<char>& data) {
    try {
        // 创建目录
        std::filesystem::path path(file_path);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(data.data(), data.size());
        return true;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to write binary file " << file_path << ": " << e.what();
        return false;
    }
}

void OpenMPIOManager::update_stats(double operation_time, bool success, size_t data_size) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_operations++;
    if (success) {
        stats_.successful_operations++;
    } else {
        stats_.failed_operations++;
    }
    
    stats_.total_time_ms += operation_time;
    stats_.average_time_ms = stats_.total_time_ms / stats_.total_operations;
    
    if (data_size > 0) {
        double throughput_mbps = (data_size / (1024.0 * 1024.0)) / (operation_time / 1000.0);
        stats_.throughput_mbps = throughput_mbps;
    }
}

std::string OpenMPIOManager::calculate_file_hash(const std::string& file_path, const std::string& algorithm) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        
        if (algorithm == "sha256") {
            SHA256_CTX sha256;
            SHA256_Init(&sha256);
            
            char buffer[8192];
            while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                SHA256_Update(&sha256, buffer, file.gcount());
            }
            
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256_Final(hash, &sha256);
            
            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
            }
            return ss.str();
        } else if (algorithm == "md5") {
            MD5_CTX md5;
            MD5_Init(&md5);
            
            char buffer[8192];
            while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                MD5_Update(&md5, buffer, file.gcount());
            }
            
            unsigned char hash[MD5_DIGEST_LENGTH];
            MD5_Final(hash, &md5);
            
            std::stringstream ss;
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
            }
            return ss.str();
        }
        
        return "";
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to calculate hash for file " << file_path << ": " << e.what();
        return "";
    }
}

// OpenMPBatchProcessor 实现
OpenMPBatchProcessor::OpenMPBatchProcessor(size_t batch_size, int max_threads)
    : batch_size_(batch_size), max_threads_(max_threads == 0 ? omp_get_max_threads() : max_threads) {
    LOG(INFO) << "OpenMPBatchProcessor initialized with batch size " << batch_size_ 
              << " and " << max_threads_ << " threads";
}

void OpenMPBatchProcessor::set_batch_size(size_t batch_size) {
    batch_size_ = batch_size;
}

size_t OpenMPBatchProcessor::get_batch_size() const {
    return batch_size_;
}

} // namespace Paker
