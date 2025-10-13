#include "Paker/core/async_io.h"
#include "Paker/core/memory_pool.h"
#include "Paker/core/package_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <glog/logging.h>
#include <curl/curl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

namespace Paker {

// 全局实例
std::unique_ptr<AsyncIOManager> g_async_io_manager = nullptr;

// AsyncFileReadOperation 实现
AsyncFileReadOperation::AsyncFileReadOperation(const std::string& file_path, bool read_as_text)
    : file_path_(file_path), read_as_text_(read_as_text) {
    result_ = std::make_shared<FileReadResult>();
}

std::string AsyncFileReadOperation::get_description() const {
    return "Read file: " + file_path_;
}

void AsyncFileReadOperation::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        set_status(IOOperationStatus::IN_PROGRESS);
        
        // 检查文件是否存在
        if (!fs::exists(file_path_)) {
            set_error("File does not exist: " + file_path_);
            return;
        }
        
        // 获取文件大小
        result_->file_size = fs::file_size(file_path_);
        update_progress(0, result_->file_size);
        
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        // 使用优化的缓冲区大小
        size_t buffer_size = std::min(result_->file_size, static_cast<size_t>(1024 * 1024)); // 最大1MB缓冲区
        
        // 读取文件
        std::ifstream file(file_path_, read_as_text_ ? std::ios::in : std::ios::binary);
        if (!file.is_open()) {
            set_error("Failed to open file: " + file_path_);
            return;
        }
        
        if (read_as_text_) {
            // 使用优化的文本读取
            std::string content;
            content.reserve(result_->file_size);
            
            std::vector<char> buffer(buffer_size);
            while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
                if (cancelled_) {
                    set_status(IOOperationStatus::CANCELLED);
                    return;
                }
                
                content.append(buffer.data(), file.gcount());
                update_progress(content.size(), result_->file_size);
            }
            
            result_->content = std::move(content);
            result_->bytes_processed = result_->content.size();
        } else {
            // 使用优化的二进制读取
            result_->data.reserve(result_->file_size);
            std::vector<char> buffer(buffer_size);
            
            while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
                if (cancelled_) {
                    set_status(IOOperationStatus::CANCELLED);
                    return;
                }
                
                result_->data.insert(result_->data.end(), buffer.data(), buffer.data() + file.gcount());
                update_progress(result_->data.size(), result_->file_size);
            }
            
            result_->bytes_processed = result_->data.size();
        }
        
        file.close();
        
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        update_progress(result_->bytes_processed, result_->file_size);
        set_status(IOOperationStatus::COMPLETED);
        
    } catch (const std::exception& e) {
        set_error("Exception during file read: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result_->duration = duration_;
    result_->status = get_status();
}

void AsyncFileReadOperation::cancel() {
    cancelled_ = true;
    if (get_status() == IOOperationStatus::IN_PROGRESS) {
        set_status(IOOperationStatus::CANCELLED);
    }
}

// AsyncFileWriteOperation 实现
AsyncFileWriteOperation::AsyncFileWriteOperation(const std::string& file_path, const std::vector<char>& data)
    : file_path_(file_path), data_(data), write_as_text_(false) {
    result_ = std::make_shared<FileWriteResult>();
    result_->file_path = file_path_;
}

AsyncFileWriteOperation::AsyncFileWriteOperation(const std::string& file_path, const std::string& content)
    : file_path_(file_path), text_content_(content), write_as_text_(true) {
    result_ = std::make_shared<FileWriteResult>();
    result_->file_path = file_path_;
}

std::string AsyncFileWriteOperation::get_description() const {
    return "Write file: " + file_path_;
}

void AsyncFileWriteOperation::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        set_status(IOOperationStatus::IN_PROGRESS);
        
        // 创建目录（如果不存在）
        fs::path file_path(file_path_);
        if (file_path.has_parent_path()) {
            fs::create_directories(file_path.parent_path());
        }
        
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        // 写入文件
        std::ofstream file(file_path_, write_as_text_ ? std::ios::out : std::ios::binary);
        if (!file.is_open()) {
            set_error("Failed to open file for writing: " + file_path_);
            return;
        }
        
        if (write_as_text_) {
            file << text_content_;
            result_->bytes_written = text_content_.size();
        } else {
            file.write(data_.data(), data_.size());
            result_->bytes_written = data_.size();
        }
        
        file.close();
        
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        update_progress(result_->bytes_written, result_->bytes_written);
        set_status(IOOperationStatus::COMPLETED);
        
    } catch (const std::exception& e) {
        set_error("Exception during file write: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result_->duration = duration_;
    result_->status = get_status();
}

void AsyncFileWriteOperation::cancel() {
    cancelled_ = true;
    if (get_status() == IOOperationStatus::IN_PROGRESS) {
        set_status(IOOperationStatus::CANCELLED);
    }
}

// AsyncNetworkDownloadOperation 实现
AsyncNetworkDownloadOperation::AsyncNetworkDownloadOperation(const std::string& url, const std::string& local_path)
    : url_(url), local_path_(local_path) {
    result_ = std::make_shared<NetworkDownloadResult>();
    result_->url = url_;
    result_->local_path = local_path_;
}

std::string AsyncNetworkDownloadOperation::get_description() const {
    return "Download: " + url_;
}

// CURL回调函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::vector<char>* data = static_cast<std::vector<char>*>(userp);
    data->insert(data->end(), static_cast<char*>(contents), static_cast<char*>(contents) + total_size);
    return total_size;
}

// CURL进度回调函数
static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    AsyncNetworkDownloadOperation* operation = static_cast<AsyncNetworkDownloadOperation*>(clientp);
    if (operation && dltotal > 0) {
        operation->update_progress(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0; // 继续下载
}

void AsyncNetworkDownloadOperation::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        set_status(IOOperationStatus::IN_PROGRESS);
        
        // 使用优化的下载缓冲区
        size_t download_buffer_size = 256 * 1024; // 256KB缓冲区
        result_->data.reserve(download_buffer_size);
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            set_error("Failed to initialize CURL");
            return;
        }
        
        // 设置CURL选项
        curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_->data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        
        // 设置用户代理
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Paker/1.0");
        
        // 启用压缩
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        
        // 设置缓冲区大小
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, static_cast<long>(download_buffer_size));
        
        // 启用HTTP/2支持
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        
        // 启用TCP_NODELAY
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        
        // 启用连接复用
        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 0L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 0L);
        
        // 设置DNS缓存
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 300L);
        
        // 启用管道化
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        
        if (cancelled_) {
            curl_easy_cleanup(curl);
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        // 执行下载
        CURLcode res = curl_easy_perform(curl);
        
        // 获取HTTP状态码
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        result_->http_status_code = static_cast<int>(http_code);
        
        // 获取内容长度
        double content_length = 0;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
        result_->content_length = static_cast<size_t>(content_length);
        
        curl_easy_cleanup(curl);
        
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        // 检查下载结果
        if (res != CURLE_OK) {
            std::string error_msg = "CURL error: " + std::string(curl_easy_strerror(res));
            
            // 检查是否需要重试
            if (res == CURLE_OPERATION_TIMEDOUT || res == CURLE_COULDNT_CONNECT) {
                set_error(error_msg + " (retryable)");
            } else {
                set_error(error_msg);
            }
            return;
        }
        
        // 检查HTTP状态码
        if (http_code >= 400) {
            std::string error_msg = "HTTP error: " + std::to_string(http_code);
            set_error(error_msg);
            return;
        }
        
        result_->bytes_processed = result_->data.size();
        update_progress(result_->bytes_processed, result_->content_length);
        
        // 如果指定了本地路径，保存文件
        if (!local_path_.empty()) {
            std::ofstream file(local_path_, std::ios::binary);
            if (file.is_open()) {
                file.write(result_->data.data(), result_->data.size());
                file.close();
            }
        }
        
        set_status(IOOperationStatus::COMPLETED);
        
    } catch (const std::exception& e) {
        set_error("Exception during download: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result_->duration = duration_;
    result_->status = get_status();
}

void AsyncNetworkDownloadOperation::cancel() {
    cancelled_ = true;
    if (get_status() == IOOperationStatus::IN_PROGRESS) {
        set_status(IOOperationStatus::CANCELLED);
    }
}

// AsyncIOManager 实现
AsyncIOManager::AsyncIOManager(size_t thread_count, size_t max_concurrent, 
                               size_t max_patterns, size_t max_batch_size, 
                               std::chrono::milliseconds max_batch_wait_time)
    : max_concurrent_operations_(max_concurrent)
    , max_patterns_(max_patterns)
    , max_batch_size_(max_batch_size)
    , max_batch_wait_time_(max_batch_wait_time) {
    worker_threads_.reserve(thread_count);
    
    // 初始化缓冲区配置
    buffer_configs_[BufferType::FILE_READ] = BufferConfig();
    buffer_configs_[BufferType::FILE_WRITE] = BufferConfig();
    buffer_configs_[BufferType::NETWORK_DOWNLOAD] = BufferConfig();
    buffer_configs_[BufferType::NETWORK_UPLOAD] = BufferConfig();
    
    // 设置网络下载的缓冲区更大
    buffer_configs_[BufferType::NETWORK_DOWNLOAD].initial_size = 256 * 1024; // 256KB
    buffer_configs_[BufferType::NETWORK_DOWNLOAD].max_size = 32 * 1024 * 1024; // 32MB
    
    // 初始化自适应重试策略
    adaptive_retry_strategy_ = std::make_unique<AdaptiveRetryStrategy>();
    
    // 初始化预测性预加载策略
    predictive_preload_strategy_ = std::make_unique<PredictivePreloadStrategy>();
    
    LOG(INFO) << "AsyncIOManager initialized with enhanced features";
}

AsyncIOManager::~AsyncIOManager() {
    shutdown();
}

bool AsyncIOManager::initialize() {
    if (running_) {
        LOG(WARNING) << "AsyncIOManager already initialized";
        return true;
    }
    
    running_ = true;
    
    // 启动工作线程
    size_t thread_count = worker_threads_.capacity();
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&AsyncIOManager::worker_thread_function, this);
    }
    
    LOG(INFO) << "AsyncIOManager initialized with " << thread_count << " threads";
    return true;
}

void AsyncIOManager::shutdown() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    queue_cv_.notify_all();
    
    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    LOG(INFO) << "AsyncIOManager shutdown complete";
}

void AsyncIOManager::worker_thread_function() {
    while (running_) {
        std::shared_ptr<AsyncIOOperation> operation;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !running_ || !operation_queue_.empty(); });
            
            if (!running_) {
                break;
            }
            
            if (!operation_queue_.empty()) {
                operation = operation_queue_.front();
                operation_queue_.pop();
            }
        }
        
        if (operation) {
            process_operation(operation);
        }
    }
}

void AsyncIOManager::process_operation(std::shared_ptr<AsyncIOOperation> operation) {
    active_operations_count_++;
    total_operations_++;
    
    try {
        operation->execute();
        
        if (operation->get_status() == IOOperationStatus::COMPLETED) {
            completed_operations_count_++;
        } else if (operation->get_status() == IOOperationStatus::FAILED) {
            failed_operations_++;
        }
        
        total_io_time_ += operation->get_duration();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in async operation: " << e.what();
        failed_operations_++;
    }
    
    active_operations_count_--;
}

std::future<std::shared_ptr<FileReadResult>> AsyncIOManager::read_file_async(
    const std::string& file_path, bool read_as_text) {
    
    auto operation = std::make_shared<AsyncFileReadOperation>(file_path, read_as_text);
    auto promise = std::make_shared<std::promise<std::shared_ptr<FileReadResult>>>();
    auto future = promise->get_future();
    
    // 设置完成回调
    operation->set_progress_callback([operation, promise](size_t current, size_t total) {
        if (operation->get_status() == IOOperationStatus::COMPLETED ||
            operation->get_status() == IOOperationStatus::FAILED ||
            operation->get_status() == IOOperationStatus::CANCELLED) {
            promise->set_value(operation->get_result());
        }
    });
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        operation_queue_.push(operation);
    }
    queue_cv_.notify_one();
    
    return future;
}

std::future<std::shared_ptr<FileWriteResult>> AsyncIOManager::write_file_async(
    const std::string& file_path, const std::vector<char>& data) {
    
    auto operation = std::make_shared<AsyncFileWriteOperation>(file_path, data);
    auto promise = std::make_shared<std::promise<std::shared_ptr<FileWriteResult>>>();
    auto future = promise->get_future();
    
    // 设置完成回调
    operation->set_progress_callback([operation, promise](size_t current, size_t total) {
        if (operation->get_status() == IOOperationStatus::COMPLETED ||
            operation->get_status() == IOOperationStatus::FAILED ||
            operation->get_status() == IOOperationStatus::CANCELLED) {
            promise->set_value(operation->get_result());
        }
    });
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        operation_queue_.push(operation);
    }
    queue_cv_.notify_one();
    
    return future;
}

std::future<std::shared_ptr<FileWriteResult>> AsyncIOManager::write_file_async(
    const std::string& file_path, const std::string& content) {
    
    auto operation = std::make_shared<AsyncFileWriteOperation>(file_path, content);
    auto promise = std::make_shared<std::promise<std::shared_ptr<FileWriteResult>>>();
    auto future = promise->get_future();
    
    // 设置完成回调
    operation->set_progress_callback([operation, promise](size_t current, size_t total) {
        if (operation->get_status() == IOOperationStatus::COMPLETED ||
            operation->get_status() == IOOperationStatus::FAILED ||
            operation->get_status() == IOOperationStatus::CANCELLED) {
            promise->set_value(operation->get_result());
        }
    });
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        operation_queue_.push(operation);
    }
    queue_cv_.notify_one();
    
    return future;
}

std::future<std::shared_ptr<NetworkDownloadResult>> AsyncIOManager::download_async(
    const std::string& url, const std::string& local_path) {
    
    auto operation = std::make_shared<AsyncNetworkDownloadOperation>(url, local_path);
    auto promise = std::make_shared<std::promise<std::shared_ptr<NetworkDownloadResult>>>();
    auto future = promise->get_future();
    
    // 设置完成回调
    operation->set_progress_callback([operation, promise](size_t current, size_t total) {
        if (operation->get_status() == IOOperationStatus::COMPLETED ||
            operation->get_status() == IOOperationStatus::FAILED ||
            operation->get_status() == IOOperationStatus::CANCELLED) {
            promise->set_value(operation->get_result());
        }
    });
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        operation_queue_.push(operation);
    }
    queue_cv_.notify_one();
    
    return future;
}

std::vector<std::future<std::shared_ptr<FileReadResult>>> AsyncIOManager::read_files_async(
    const std::vector<std::string>& file_paths, bool read_as_text) {
    
    std::vector<std::future<std::shared_ptr<FileReadResult>>> futures;
    futures.reserve(file_paths.size());
    
    for (const auto& file_path : file_paths) {
        futures.push_back(read_file_async(file_path, read_as_text));
    }
    
    return futures;
}

std::vector<std::future<std::shared_ptr<FileWriteResult>>> AsyncIOManager::write_files_async(
    const std::vector<std::pair<std::string, std::string>>& file_contents) {
    
    std::vector<std::future<std::shared_ptr<FileWriteResult>>> futures;
    futures.reserve(file_contents.size());
    
    for (const auto& [file_path, content] : file_contents) {
        futures.push_back(write_file_async(file_path, content));
    }
    
    return futures;
}

void AsyncIOManager::set_max_concurrent_operations(size_t max_concurrent) {
    max_concurrent_operations_ = max_concurrent;
}

double AsyncIOManager::get_success_rate() const {
    size_t total = total_operations_.load();
    if (total == 0) return 0.0;
    return static_cast<double>(completed_operations_count_.load()) / total * 100.0;
}

double AsyncIOManager::get_average_operation_time() const {
    size_t completed = completed_operations_count_.load();
    if (completed == 0) return 0.0;
    return static_cast<double>(total_io_time_.count()) / completed;
}

std::string AsyncIOManager::get_performance_report() const {
    std::stringstream ss;
    ss << "AsyncIO Performance Report:\n";
    ss << "  Total operations: " << total_operations_.load() << "\n";
    ss << "  Completed operations: " << completed_operations_count_.load() << "\n";
    ss << "  Failed operations: " << failed_operations_.load() << "\n";
    ss << "  Active operations: " << active_operations_count_.load() << "\n";
    ss << "  Queue size: " << get_queue_size() << "\n";
    ss << "  Success rate: " << get_success_rate() << "%\n";
    ss << "  Average operation time: " << get_average_operation_time() << "ms\n";
    ss << "  Total I/O time: " << total_io_time_.count() << "ms\n";
    
    return ss.str();
}

size_t AsyncIOManager::get_queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return operation_queue_.size();
}

void AsyncIOManager::clear_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!operation_queue_.empty()) {
        operation_queue_.pop();
    }
}

void AsyncIOManager::cancel_all_operations() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::queue<std::shared_ptr<AsyncIOOperation>> empty_queue;
    operation_queue_.swap(empty_queue);
}

// 全局函数实现
bool initialize_async_io_manager(size_t thread_count, size_t max_concurrent) {
    if (g_async_io_manager) {
        LOG(WARNING) << "AsyncIO manager already initialized";
        return true;
    }
    
    g_async_io_manager = std::make_unique<AsyncIOManager>(thread_count, max_concurrent);
    return g_async_io_manager->initialize();
}

void cleanup_async_io_manager() {
    if (g_async_io_manager) {
        g_async_io_manager->shutdown();
        g_async_io_manager.reset();
    }
}

AsyncIOManager* get_async_io_manager() {
    if (!g_async_io_manager) {
        // 尝试初始化服务
        if (initialize_paker_services()) {
            // 服务初始化后，创建异步I/O管理器
            g_async_io_manager = std::make_unique<AsyncIOManager>();
        }
    }
    return g_async_io_manager.get();
}

// 动态缓冲区管理实现
std::vector<char> AsyncIOManager::get_optimal_buffer(BufferType type, size_t preferred_size) {
    if (!adaptive_buffering_enabled_) {
        return std::vector<char>(preferred_size > 0 ? preferred_size : 64 * 1024);
    }
    
    auto it = buffer_configs_.find(type);
    if (it == buffer_configs_.end()) {
        return std::vector<char>(preferred_size > 0 ? preferred_size : 64 * 1024);
    }
    
    const auto& config = it->second;
    size_t buffer_size = preferred_size > 0 ? preferred_size : config.initial_size;
    
    // 确保缓冲区大小在合理范围内
    buffer_size = std::max(config.min_size, std::min(buffer_size, config.max_size));
    
    // 更新内存使用统计
    total_memory_usage_ += buffer_size;
    
    return std::vector<char>(buffer_size);
}

void AsyncIOManager::record_buffer_operation(BufferType type, const BufferMetrics& metrics) {
    if (!adaptive_buffering_enabled_) return;
    
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    
    auto& history = performance_history_[type];
    history.push_back(metrics);
    
    // 限制历史记录大小
    if (history.size() > 100) {
        history.erase(history.begin());
    }
    
    // 优化缓冲区配置
    optimize_buffer_configs();
}

void AsyncIOManager::optimize_buffer_configs() {
    for (auto& [type, history] : performance_history_) {
        if (history.empty()) continue;
        
        auto& config = buffer_configs_[type];
        
        // 计算平均吞吐量
        double avg_throughput = 0.0;
        for (const auto& metric : history) {
            avg_throughput += metric.throughput_mbps;
        }
        avg_throughput /= history.size();
        
        // 基于性能调整缓冲区大小
        if (avg_throughput < 50.0 && config.initial_size < config.max_size) {
            // 吞吐量低，增加缓冲区大小
            config.initial_size = std::min(static_cast<size_t>(config.initial_size * config.growth_factor), config.max_size);
        } else if (avg_throughput > 200.0 && config.initial_size > config.min_size) {
            // 吞吐量高，可以尝试减少缓冲区大小
            config.initial_size = std::max(static_cast<size_t>(config.initial_size / config.growth_factor), config.min_size);
        }
    }
}

// 智能预读实现
void AsyncIOManager::update_file_access_pattern(const std::string& file_path, size_t read_size) {
    if (!smart_pre_read_enabled_) return;
    
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    
    auto& pattern = access_patterns_[file_path];
    pattern.file_path = file_path;
    pattern.access_count++;
    pattern.last_access = std::chrono::steady_clock::now();
    
    // 更新平均读取大小
    pattern.average_read_size = (pattern.average_read_size * (pattern.access_count - 1) + read_size) / pattern.access_count;
    
    // 清理旧模式
    if (access_patterns_.size() > max_patterns_) {
        auto oldest = std::min_element(access_patterns_.begin(), access_patterns_.end(),
            [](const auto& a, const auto& b) {
                return a.second.last_access < b.second.last_access;
            });
        access_patterns_.erase(oldest);
    }
}

void AsyncIOManager::perform_smart_pre_read() {
    if (!smart_pre_read_enabled_) return;
    
    std::vector<std::string> candidates = get_pre_read_candidates();
    for (const auto& file_path : candidates) {
        if (should_pre_read(file_path)) {
            size_t pre_read_size = get_suggested_pre_read_size(file_path);
            // 异步预读文件
            read_file_async(file_path, true);
        }
    }
}

bool AsyncIOManager::should_pre_read(const std::string& file_path) const {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    
    auto it = access_patterns_.find(file_path);
    if (it == access_patterns_.end()) {
        return false;
    }
    
    const auto& pattern = it->second;
    return pattern.access_count > 3 && pattern.access_frequency > 0.1;
}

size_t AsyncIOManager::get_suggested_pre_read_size(const std::string& file_path) const {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    
    auto it = access_patterns_.find(file_path);
    if (it == access_patterns_.end()) {
        return 64 * 1024; // 默认64KB
    }
    
    const auto& pattern = it->second;
    return std::max(pattern.average_read_size * 2, static_cast<size_t>(64 * 1024));
}

// 网络重试实现
bool AsyncIOManager::should_retry_network_operation(const std::string& url, int http_code, const std::string& error) {
    if (!network_retry_enabled_) return false;
    
    std::lock_guard<std::mutex> lock(retry_mutex_);
    
    // 检查HTTP状态码
    for (int code : retry_config_.retryable_http_codes) {
        if (http_code == code) {
            return retry_history_[url].size() < retry_config_.max_retries;
        }
    }
    
    // 检查错误消息
    if (error.find("timeout") != std::string::npos ||
        error.find("connection") != std::string::npos) {
        return retry_history_[url].size() < retry_config_.max_retries;
    }
    
    return false;
}

std::chrono::milliseconds AsyncIOManager::get_retry_delay(const std::string& url, size_t attempt_number) {
    std::lock_guard<std::mutex> lock(retry_mutex_);
    
    auto delay = static_cast<double>(retry_config_.initial_delay.count()) * 
                 std::pow(retry_config_.backoff_factor, static_cast<double>(attempt_number - 1));
    
    auto max_delay_ms = retry_config_.max_delay.count();
    delay = std::min(delay, static_cast<double>(max_delay_ms));
    
    return std::chrono::milliseconds(static_cast<long>(delay));
}

void AsyncIOManager::record_retry_attempt(const std::string& url) {
    std::lock_guard<std::mutex> lock(retry_mutex_);
    retry_history_[url].push_back(std::chrono::steady_clock::now());
}

// 批量处理实现
void AsyncIOManager::process_batch_operations() {
    if (!batch_optimization_enabled_) return;
    
    std::lock_guard<std::mutex> lock(batch_mutex_);
    
    // 按优先级排序
    std::sort(pending_batch_operations_.begin(), pending_batch_operations_.end(),
        [](const BatchOperation& a, const BatchOperation& b) {
            return a.priority > b.priority;
        });
    
    // 处理批次
    for (auto& batch : pending_batch_operations_) {
        if (batch.file_paths.size() <= max_batch_size_) {
            // 执行批量操作
            switch (batch.type) {
                case IOOperationType::READ_FILE:
                    read_files_async(batch.file_paths, true);
                    break;
                case IOOperationType::WRITE_FILE:
                    // 这里需要实现批量写入
                    break;
                case IOOperationType::NETWORK_DOWNLOAD:
                    // 这里需要实现批量下载
                    break;
                default:
                    break;
            }
        }
    }
    
    pending_batch_operations_.clear();
}

void AsyncIOManager::optimize_batch_scheduling() {
    std::lock_guard<std::mutex> lock(operations_mutex_);
    
    if (pending_batch_operations_.empty()) {
        return;
    }
    
    // 按操作类型分组
    std::map<IOOperationType, std::vector<BatchOperation>> grouped_ops;
    for (auto& op : pending_batch_operations_) {
        grouped_ops[op.type].push_back(op);
    }
    
    // 优化每个组的调度
    for (auto& [type, ops] : grouped_ops) {
        if (ops.size() < 2) continue;
        
        // 按优先级排序，优先处理高优先级
        std::sort(ops.begin(), ops.end(), [](const auto& a, const auto& b) {
            return a.priority > b.priority;
        });
        
        // 合并相似操作
        std::vector<BatchOperation> optimized_ops;
        for (size_t i = 0; i < ops.size(); ) {
            auto& current_op = ops[i];
            optimized_ops.push_back(current_op);
            
            // 查找可以合并的后续操作
            size_t j = i + 1;
            while (j < ops.size() && 
                   ops[j].priority == current_op.priority) { // 优先级相同
                // 合并操作（这里简化处理，实际可以更复杂）
                current_op.file_paths.insert(current_op.file_paths.end(), 
                                            ops[j].file_paths.begin(), 
                                            ops[j].file_paths.end());
                j++;
            }
            i = j;
        }
        
        // 更新操作列表
        ops = std::move(optimized_ops);
    }
    
    // 重新组织批量操作
    pending_batch_operations_.clear();
    for (auto& [type, ops] : grouped_ops) {
        for (auto& op : ops) {
            pending_batch_operations_.push_back(op);
        }
    }
    
    LOG(INFO) << "Batch scheduling optimized: " << pending_batch_operations_.size() << " operations";
}

// 新增的公共方法实现
void AsyncIOManager::set_buffer_config(BufferType type, const BufferConfig& config) {
    buffer_configs_[type] = config;
}

BufferConfig AsyncIOManager::get_buffer_config(BufferType type) const {
    auto it = buffer_configs_.find(type);
    if (it != buffer_configs_.end()) {
        return it->second;
    }
    return BufferConfig();
}

void AsyncIOManager::trigger_pre_read_analysis() {
    perform_smart_pre_read();
}

std::vector<std::string> AsyncIOManager::get_pre_read_candidates() const {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    
    std::vector<std::string> candidates;
    for (const auto& [file_path, pattern] : access_patterns_) {
        if (should_pre_read(file_path)) {
            candidates.push_back(file_path);
        }
    }
    
    // 按访问频率排序
    std::sort(candidates.begin(), candidates.end(), [this](const std::string& a, const std::string& b) {
        auto it_a = access_patterns_.find(a);
        auto it_b = access_patterns_.find(b);
        
        if (it_a == access_patterns_.end() || it_b == access_patterns_.end()) {
            return false;
        }
        
        return it_a->second.access_frequency > it_b->second.access_frequency;
    });
    
    return candidates;
}

void AsyncIOManager::set_retry_config(const RetryConfig& config) {
    std::lock_guard<std::mutex> lock(retry_mutex_);
    retry_config_ = config;
}

RetryConfig AsyncIOManager::get_retry_config() const {
    std::lock_guard<std::mutex> lock(retry_mutex_);
    return retry_config_;
}

void AsyncIOManager::set_batch_config(size_t max_batch_size, std::chrono::milliseconds max_wait_time) {
    std::lock_guard<std::mutex> lock(batch_mutex_);
    max_batch_size_ = max_batch_size;
    max_batch_wait_time_ = max_wait_time;
}

void AsyncIOManager::process_pending_batches() {
    process_batch_operations();
}

void AsyncIOManager::optimize_performance() {
    optimize_buffer_configs();
    optimize_batch_scheduling();
}

std::vector<std::string> AsyncIOManager::get_optimization_suggestions() const {
    std::vector<std::string> suggestions;
    
    // 基于性能数据提供建议
    if (get_success_rate() < 90.0) {
        suggestions.push_back("Success rate is low, consider checking error handling");
    }
    
    if (get_average_throughput() < 10.0) {
        suggestions.push_back("Throughput is low, consider optimizing buffer sizes");
    }
    
    if (get_memory_usage() > 1024 * 1024 * 1024) { // 1GB
        suggestions.push_back("Memory usage is high, consider reducing buffer sizes");
    }
    
    return suggestions;
}

void AsyncIOManager::apply_optimization_suggestions() {
    optimize_performance();
}

double AsyncIOManager::get_average_throughput() const {
    if (total_io_time_.count() == 0) {
        return 0.0;
    }
    
    double seconds = static_cast<double>(total_io_time_.count()) / 1000.0;
    return static_cast<double>(get_total_bytes_processed()) / (seconds * 1024 * 1024); // MB/s
}

double AsyncIOManager::get_cache_hit_rate() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (total_cache_requests_ == 0) {
        return 0.0;
    }
    
    double hit_rate = static_cast<double>(cache_hits_) / total_cache_requests_ * 100.0;
    return std::min(100.0, std::max(0.0, hit_rate));
}

size_t AsyncIOManager::get_total_bytes_processed() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return total_bytes_processed_;
}

std::string AsyncIOManager::get_detailed_performance_report() const {
    std::stringstream ss;
    ss << "Enhanced AsyncIO Performance Report:\n";
    ss << "  Total operations: " << get_total_operations() << "\n";
    ss << "  Success rate: " << get_success_rate() << "%\n";
    ss << "  Average throughput: " << get_average_throughput() << " MB/s\n";
    ss << "  Memory usage: " << get_memory_usage() << " bytes\n";
    ss << "  Adaptive buffering: " << (is_adaptive_buffering_enabled() ? "enabled" : "disabled") << "\n";
    ss << "  Smart pre-read: " << (is_smart_pre_read_enabled() ? "enabled" : "disabled") << "\n";
    ss << "  Network retry: " << (is_network_retry_enabled() ? "enabled" : "disabled") << "\n";
    ss << "  Batch optimization: " << (is_batch_optimization_enabled() ? "enabled" : "disabled") << "\n";
    
    return ss.str();
}

// AdaptiveRetryStrategy 实现
AdaptiveRetryStrategy::AdaptiveRetryStrategy(std::chrono::milliseconds base_delay,
                                          double adaptive_factor,
                                          size_t max_history)
    : base_delay_(base_delay)
    , adaptive_factor_(adaptive_factor)
    , max_history_size_(max_history) {
}

void AdaptiveRetryStrategy::record_network_metrics(const std::string& url, const NetworkMetrics& metrics) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    auto& history = network_history_[url];
    history.push_back(metrics);
    
    // 保持历史记录大小限制
    if (history.size() > max_history_size_) {
        history.erase(history.begin());
    }
}

std::chrono::milliseconds AdaptiveRetryStrategy::calculate_retry_delay(const std::string& url, size_t attempt) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    auto it = network_history_.find(url);
    if (it == network_history_.end() || it->second.empty()) {
        // 没有历史数据，使用基础延迟
        return std::chrono::milliseconds(static_cast<long>(base_delay_.count() * std::pow(adaptive_factor_, attempt)));
    }
    
    const auto& history = it->second;
    const auto& latest = history.back();
    
    // 根据网络质量调整延迟
    double network_quality = get_network_quality(url);
    double quality_factor = 1.0 + (1.0 - network_quality) * 2.0; // 网络质量差时增加延迟
    
    // 智能延迟计算
    long base_delay = base_delay_.count();
    
    // 根据网络质量动态调整基础延迟
    if (network_quality < 0.3) {
        base_delay *= 2.0; // 网络质量差时加倍延迟
    } else if (network_quality > 0.8) {
        base_delay *= 0.5; // 网络质量好时减半延迟
    }
    
    // 指数退避 + 抖动
    long delay_ms = static_cast<long>(base_delay * std::pow(adaptive_factor_, attempt) * quality_factor);
    
    // 添加随机抖动避免雷群效应
    double jitter = 1.0 + (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.2; // ±10%抖动
    delay_ms = static_cast<long>(delay_ms * jitter);
    
    // 限制最大延迟
    return std::chrono::milliseconds(std::min(delay_ms, 30000L));
}

bool AdaptiveRetryStrategy::should_retry(const std::string& url, int http_code, size_t attempt) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    // 检查HTTP状态码是否可重试
    std::vector<int> retryable_codes = {408, 429, 500, 502, 503, 504};
    bool is_retryable_code = std::find(retryable_codes.begin(), retryable_codes.end(), http_code) != retryable_codes.end();
    
    if (!is_retryable_code) {
        return false;
    }
    
    // 检查网络质量
    double network_quality = get_network_quality(url);
    
    // 智能重试策略
    size_t max_retries = 3; // 默认最大重试次数
    
    // 根据网络质量动态调整重试次数
    if (network_quality < 0.2) {
        max_retries = 1; // 网络质量极差，只重试1次
    } else if (network_quality < 0.5) {
        max_retries = 2; // 网络质量差，重试2次
    } else if (network_quality > 0.8) {
        max_retries = 5; // 网络质量好，可以重试更多次
    }
    
    // 根据HTTP状态码调整重试策略
    if (http_code == 429) { // 限流
        max_retries = std::min(max_retries, size_t(2)); // 限流时减少重试
    } else if (http_code >= 500) { // 服务器错误
        max_retries = std::min(max_retries, size_t(3)); // 服务器错误时正常重试
    }
    
    // 检查是否超过最大重试次数
    if (attempt >= max_retries) {
        return false;
    }
    
    // 检查重试间隔是否合理（避免过于频繁的重试）
    if (attempt > 0) {
        auto delay = calculate_retry_delay(url, attempt - 1);
        if (delay.count() < 100) { // 延迟太短，可能网络有问题
            return false;
        }
    }
    
    return true;
}

void AdaptiveRetryStrategy::update_strategy_parameters() {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    // 分析所有URL的网络质量
    double total_quality = 0.0;
    size_t url_count = 0;
    
    for (const auto& [url, history] : network_history_) {
        if (!history.empty()) {
            total_quality += get_network_quality(url);
            url_count++;
        }
    }
    
    if (url_count > 0) {
        double avg_quality = total_quality / url_count;
        
        // 根据平均网络质量调整策略参数
        if (avg_quality < 0.3) {
            // 网络质量差，增加基础延迟
            base_delay_ = std::chrono::milliseconds(static_cast<long>(base_delay_.count() * 1.2));
            adaptive_factor_ = std::min(3.0, adaptive_factor_ * 1.1);
        } else if (avg_quality > 0.8) {
            // 网络质量好，减少基础延迟
            base_delay_ = std::chrono::milliseconds(static_cast<long>(base_delay_.count() * 0.9));
            adaptive_factor_ = std::max(1.5, adaptive_factor_ * 0.95);
        }
    }
}

double AdaptiveRetryStrategy::get_network_quality(const std::string& url) const {
    auto it = network_history_.find(url);
    if (it == network_history_.end() || it->second.empty()) {
        return 0.5; // 默认中等质量
    }
    
    const auto& history = it->second;
    const auto& latest = history.back();
    
    // 基于延迟、带宽、丢包率和成功率计算网络质量
    double latency_score = std::max(0.0, 1.0 - latest.latency_ms / 1000.0); // 延迟越低分数越高
    double bandwidth_score = std::min(1.0, latest.bandwidth_mbps / 100.0); // 带宽越高分数越高
    double loss_score = 1.0 - latest.packet_loss_rate; // 丢包率越低分数越高
    double success_score = latest.success_rate; // 成功率
    
    // 加权平均
    return (latency_score * 0.3 + bandwidth_score * 0.2 + loss_score * 0.2 + success_score * 0.3);
}

// PredictivePreloadStrategy 实现
PredictivePreloadStrategy::PredictivePreloadStrategy(double confidence_threshold,
                                                   size_t max_predictions,
                                                   double freq_weight,
                                                   double rec_weight,
                                                   double dep_weight)
    : preload_interval_(std::chrono::milliseconds(60000))  // 1分钟预加载一次
    , confidence_threshold_(confidence_threshold)
    , max_predictions_(max_predictions)
    , frequency_weight_(freq_weight)
    , recency_weight_(rec_weight)
    , dependency_weight_(dep_weight) {
}

void PredictivePreloadStrategy::record_package_usage(const std::string& package_name) {
    std::lock_guard<std::mutex> lock(prediction_mutex_);
    package_frequency_[package_name].usage_count++;
}

void PredictivePreloadStrategy::update_dependency_graph(const std::string& package, 
                                                      const std::vector<std::string>& dependencies) {
    std::lock_guard<std::mutex> lock(prediction_mutex_);
    dependency_graph_[package] = dependencies;
}

std::vector<DependencyPrediction> PredictivePreloadStrategy::predict_dependencies(const std::string& package_name) const {
    std::lock_guard<std::mutex> lock(prediction_mutex_);
    
    std::vector<DependencyPrediction> predictions;
    
    // 基于依赖图进行预测
    auto it = dependency_graph_.find(package_name);
    if (it != dependency_graph_.end()) {
        for (const auto& dep : it->second) {
            DependencyPrediction prediction;
            prediction.package_name = dep;
            prediction.confidence = calculate_prediction_confidence(package_name, dep);
            prediction.prediction_time = std::chrono::steady_clock::now();
            
            if (prediction.confidence >= confidence_threshold_) {
                predictions.push_back(prediction);
            }
        }
    }
    
    // 基于使用频率进行预测
    for (const auto& [pkg, freq] : package_frequency_) {
        if (pkg != package_name) {
            DependencyPrediction prediction;
            prediction.package_name = pkg;
            prediction.confidence = calculate_prediction_confidence(package_name, pkg);
            prediction.prediction_time = std::chrono::steady_clock::now();
            
            if (prediction.confidence >= confidence_threshold_) {
                predictions.push_back(prediction);
            }
        }
    }
    
    // 按置信度排序，返回前N个预测
    std::sort(predictions.begin(), predictions.end(),
              [](const auto& a, const auto& b) { return a.confidence > b.confidence; });
    
    if (predictions.size() > max_predictions_) {
        predictions.resize(max_predictions_);
    }
    
    return predictions;
}

void PredictivePreloadStrategy::preload_predicted_packages() {
    auto now = std::chrono::steady_clock::now();
    if (now - last_preload_ < preload_interval_) {
        return;
    }
    
    last_preload_ = now;
    
    std::lock_guard<std::mutex> lock(prediction_mutex_);
    
    // 为最常用的包预测依赖
    std::vector<std::pair<std::string, size_t>> sorted_packages;
    for (const auto& [pkg, info] : package_frequency_) {
        sorted_packages.emplace_back(pkg, info.usage_count);
    }
    
    std::sort(sorted_packages.begin(), sorted_packages.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 为前几个最常用的包进行预加载预测
    for (size_t i = 0; i < std::min(size_t(5), sorted_packages.size()); ++i) {
        const auto& package_name = sorted_packages[i].first;
        auto predictions = predict_dependencies(package_name);
        
        // 记录预测历史
        prediction_history_[package_name] = predictions;
        
        LOG(INFO) << "Predicted " << predictions.size() << " dependencies for " << package_name;
    }
}

void PredictivePreloadStrategy::update_prediction_parameters() {
    std::lock_guard<std::mutex> lock(prediction_mutex_);
    
    // 基于预测历史调整参数
    size_t total_predictions = 0;
    size_t successful_predictions = 0;
    size_t recent_predictions = 0;
    size_t recent_successful = 0;
    
    auto now = std::chrono::steady_clock::now();
    auto recent_threshold = now - std::chrono::hours(24); // 最近24小时
    
    for (const auto& [pkg, history] : prediction_history_) {
        for (const auto& record : history) {
            total_predictions++;
            
            // 检查预测是否成功（基于实际使用情况）
            PredictionRecord pred_record;
            pred_record.predicted_dependency = record.package_name;
            pred_record.prediction_time = record.prediction_time;
            pred_record.confidence = record.confidence;
            bool is_successful = check_prediction_success(pkg, pred_record);
            if (is_successful) {
                successful_predictions++;
            }
            
            // 统计最近预测
            if (record.prediction_time >= recent_threshold) {
                recent_predictions++;
                if (is_successful) {
                    recent_successful++;
                }
            }
        }
    }
    
    if (total_predictions > 0) {
        double overall_success_rate = static_cast<double>(successful_predictions) / total_predictions;
        double recent_success_rate = recent_predictions > 0 ? 
            static_cast<double>(recent_successful) / recent_predictions : overall_success_rate;
        
        // 使用加权平均，更重视最近的预测结果
        double weighted_success_rate = 0.7 * recent_success_rate + 0.3 * overall_success_rate;
        
        // 根据成功率调整置信度阈值
        if (weighted_success_rate > 0.8) {
            confidence_threshold_ = std::max(0.3, confidence_threshold_ - 0.02);
        } else if (weighted_success_rate < 0.5) {
            confidence_threshold_ = std::min(0.95, confidence_threshold_ + 0.02);
        }
        
        // 调整预测窗口大小
        if (weighted_success_rate > 0.7) {
            prediction_window_size_ = std::min(static_cast<size_t>(100), prediction_window_size_ + 5);
        } else if (weighted_success_rate < 0.4) {
            prediction_window_size_ = std::max(static_cast<size_t>(10), prediction_window_size_ - 5);
        }
        
        LOG(INFO) << "Prediction parameters updated - Success rate: " << weighted_success_rate 
                  << ", Confidence threshold: " << confidence_threshold_
                  << ", Window size: " << prediction_window_size_;
    }
}

bool PredictivePreloadStrategy::check_prediction_success(const std::string& package_name, 
                                                        const PredictionRecord& record) const {
    // 检查预测的依赖是否在实际使用中被访问
    auto it = package_frequency_.find(record.predicted_dependency);
    if (it == package_frequency_.end()) {
        return false;
    }
    
    // 检查预测时间后是否有实际使用
    auto usage_time = it->second.last_used;
    if (usage_time > record.prediction_time) {
        // 计算时间差，如果预测后很快被使用，认为预测成功
        auto time_diff = std::chrono::duration_cast<std::chrono::minutes>(
            usage_time - record.prediction_time);
        return time_diff.count() <= 60; // 1小时内使用认为预测成功
    }
    
    return false;
}

double PredictivePreloadStrategy::calculate_prediction_confidence(const std::string& package_name, 
                                                                const std::string& dependency) const {
    double confidence = 0.0;
    
    // 基于使用频率的置信度
    auto freq_it = package_frequency_.find(dependency);
    if (freq_it != package_frequency_.end()) {
        double max_freq = 0.0;
        for (const auto& [pkg, info] : package_frequency_) {
            max_freq = std::max(max_freq, static_cast<double>(info.usage_count));
        }
        if (max_freq > 0) {
            confidence += frequency_weight_ * (static_cast<double>(freq_it->second.usage_count) / max_freq);
        }
    }
    
    // 基于依赖关系的置信度
    auto dep_it = dependency_graph_.find(package_name);
    if (dep_it != dependency_graph_.end()) {
        const auto& deps = dep_it->second;
        if (std::find(deps.begin(), deps.end(), dependency) != deps.end()) {
            confidence += dependency_weight_;
        }
    }
    
    // 基于最近性的置信度（简化实现）
    confidence += recency_weight_ * 0.5; // 占位符
    
    return std::min(1.0, confidence);
}

// ZeroCopyBuffer 实现
ZeroCopyBuffer::ZeroCopyBuffer(size_t size, bool use_mmap)
    : data_(nullptr)
    , size_(size)
    , capacity_(size)
    , ref_count_(1)
    , is_mmap_(use_mmap) {
    
    if (use_mmap) {
        // 使用内存映射
        data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (data_ == MAP_FAILED) {
            data_ = nullptr;
            LOG(ERROR) << "Failed to create memory mapped buffer";
        }
    } else {
        // 使用普通内存分配
        data_ = GlobalMemoryManager::global_allocate(size);
    }
    
    if (data_) {
        deleter_ = [this](void* ptr) {
            if (is_mmap_) {
                munmap(ptr, capacity_);
            } else {
                GlobalMemoryManager::global_deallocate(ptr);
            }
        };
    }
}

ZeroCopyBuffer::ZeroCopyBuffer(void* data, size_t size, std::function<void(void*)> deleter)
    : data_(data)
    , size_(size)
    , capacity_(size)
    , deleter_(deleter)
    , ref_count_(1)
    , is_mmap_(false) {
}

ZeroCopyBuffer::~ZeroCopyBuffer() {
    cleanup();
}

ZeroCopyBuffer::ZeroCopyBuffer(ZeroCopyBuffer&& other) noexcept
    : data_(other.data_)
    , size_(other.size_)
    , capacity_(other.capacity_)
    , deleter_(std::move(other.deleter_))
    , ref_count_(other.ref_count_.load())
    , is_mmap_(other.is_mmap_) {
    
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
    other.ref_count_ = 0;
}

ZeroCopyBuffer& ZeroCopyBuffer::operator=(ZeroCopyBuffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        deleter_ = std::move(other.deleter_);
        ref_count_ = other.ref_count_.load();
        is_mmap_ = other.is_mmap_;
        
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        other.ref_count_ = 0;
    }
    return *this;
}

ZeroCopyBuffer* ZeroCopyBuffer::add_ref() {
    ref_count_++;
    return this;
}

void ZeroCopyBuffer::release() {
    if (ref_count_.fetch_sub(1) == 1) {
        delete this;
    }
}

ZeroCopyBuffer* ZeroCopyBuffer::slice(size_t offset, size_t length) const {
    if (offset + length > size_) {
        return nullptr;
    }
    
    void* slice_data = static_cast<char*>(data_) + offset;
    return new ZeroCopyBuffer(slice_data, length, nullptr);
}

bool ZeroCopyBuffer::resize(size_t new_size) {
    if (new_size <= capacity_) {
        size_ = new_size;
        return true;
    }
    
    // 需要重新分配更大的缓冲区
    void* new_data = GlobalMemoryManager::global_allocate(new_size);
    if (!new_data) {
        return false;
    }
    
    // 复制现有数据
    if (data_) {
        memcpy(new_data, data_, size_);
        if (deleter_) {
            deleter_(data_);
        }
    }
    
    data_ = new_data;
    size_ = new_size;
    capacity_ = new_size;
    deleter_ = [](void* ptr) { GlobalMemoryManager::global_deallocate(ptr); };
    
    return true;
}

void* ZeroCopyBuffer::get_write_ptr() {
    return data_;
}

bool ZeroCopyBuffer::map_file(const std::string& file_path, size_t offset, size_t length) {
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        LOG(ERROR) << "Failed to open file for memory mapping: " << file_path;
        return false;
    }
    
    if (length == 0) {
        struct stat st;
        if (stat(file_path.c_str(), &st) == -1) {
            close(fd);
            return false;
        }
        length = st.st_size - offset;
    }
    
    void* mapped_data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, offset);
    close(fd);
    
    if (mapped_data == MAP_FAILED) {
        LOG(ERROR) << "Failed to memory map file: " << file_path;
        return false;
    }
    
    if (data_ && deleter_) {
        deleter_(data_);
    }
    
    data_ = mapped_data;
    size_ = length;
    capacity_ = length;
    is_mmap_ = true;
    deleter_ = [length](void* ptr) { munmap(ptr, length); };
    
    return true;
}

void ZeroCopyBuffer::unmap() {
    if (is_mmap_ && data_) {
        munmap(data_, capacity_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
}

void ZeroCopyBuffer::cleanup() {
    if (data_ && deleter_) {
        deleter_(data_);
        data_ = nullptr;
    }
}

// ZeroCopyIOOperation 实现
ZeroCopyIOOperation::ZeroCopyIOOperation(const std::string& file_path, IOOperationType type)
    : file_path_(file_path)
    , buffer_(nullptr)
    , type_(type)
    , status_(IOOperationStatus::PENDING)
    , duration_(0)
    , cancelled_(false) {
}

ZeroCopyIOOperation::~ZeroCopyIOOperation() {
    if (buffer_) {
        buffer_->release();
    }
}

void ZeroCopyIOOperation::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (cancelled_) {
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        set_status(IOOperationStatus::IN_PROGRESS);
        
        bool success = false;
        switch (type_) {
            case IOOperationType::READ_FILE:
                success = execute_file_read();
                break;
            case IOOperationType::WRITE_FILE:
                success = execute_file_write();
                break;
            case IOOperationType::NETWORK_DOWNLOAD:
                success = execute_network_download();
                break;
            case IOOperationType::NETWORK_UPLOAD:
                success = execute_network_upload();
                break;
        }
        
        if (success) {
            set_status(IOOperationStatus::COMPLETED);
        } else {
            set_status(IOOperationStatus::FAILED);
        }
        
    } catch (const std::exception& e) {
        set_error("Exception during zero-copy operation: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
}

bool ZeroCopyIOOperation::execute_file_read() {
    // 使用内存映射进行零拷贝文件读取
    buffer_ = new ZeroCopyBuffer(0, true); // 创建内存映射缓冲区
    
    if (!buffer_->map_file(file_path_)) {
        set_error("Failed to memory map file: " + file_path_);
        return false;
    }
    
    update_progress(buffer_->size(), buffer_->size());
    return true;
}

bool ZeroCopyIOOperation::execute_file_write() {
    if (!buffer_) {
        set_error("No buffer provided for file write");
        return false;
    }
    
    std::ofstream file(file_path_, std::ios::binary);
    if (!file.is_open()) {
        set_error("Failed to open file for writing: " + file_path_);
        return false;
    }
    
    // 零拷贝写入
    file.write(static_cast<const char*>(buffer_->data()), buffer_->size());
    file.close();
    
    update_progress(buffer_->size(), buffer_->size());
    return true;
}

bool ZeroCopyIOOperation::execute_network_download() {
    // 使用CURL进行零拷贝网络下载
    buffer_ = new ZeroCopyBuffer(1024 * 1024); // 1MB初始缓冲区
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        set_error("Failed to initialize CURL");
        return false;
    }
    
    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, file_path_.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        ZeroCopyBuffer* buffer = static_cast<ZeroCopyBuffer*>(userp);
        size_t total_size = size * nmemb;
        
        // 动态调整缓冲区大小
        if (buffer->size() + total_size > buffer->capacity()) {
            buffer->resize(buffer->size() + total_size + 1024 * 1024); // 额外1MB
        }
        
        memcpy(static_cast<char*>(buffer->get_write_ptr()) + buffer->size(), contents, total_size);
        buffer->resize(buffer->size() + total_size);
        
        return total_size;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer_);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        set_error("CURL error: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    update_progress(buffer_->size(), buffer_->size());
    return true;
}

bool ZeroCopyIOOperation::execute_network_upload() {
    if (!buffer_) {
        set_error("No buffer provided for network upload");
        return false;
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        set_error("Failed to initialize CURL");
        return false;
    }
    
    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, file_path_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer_->data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, buffer_->size());
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        set_error("CURL error: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    update_progress(buffer_->size(), buffer_->size());
    return true;
}

void ZeroCopyIOOperation::set_status(IOOperationStatus status) {
    status_ = status;
}

void ZeroCopyIOOperation::set_error(const std::string& error) {
    error_message_ = error;
    set_status(IOOperationStatus::FAILED);
}

void ZeroCopyIOOperation::update_progress(size_t current, size_t total) {
    if (progress_callback_) {
        progress_callback_(current, total);
    }
}

void* ZeroCopyIOOperation::get_data() const {
    return buffer_ ? buffer_->data() : nullptr;
}

size_t ZeroCopyIOOperation::get_data_size() const {
    return buffer_ ? buffer_->size() : 0;
}

std::string ZeroCopyIOOperation::get_description() const {
    std::string type_str;
    switch (type_) {
        case IOOperationType::READ_FILE:
            type_str = "Zero-copy file read";
            break;
        case IOOperationType::WRITE_FILE:
            type_str = "Zero-copy file write";
            break;
        case IOOperationType::NETWORK_DOWNLOAD:
            type_str = "Zero-copy network download";
            break;
        case IOOperationType::NETWORK_UPLOAD:
            type_str = "Zero-copy network upload";
            break;
    }
    return type_str + ": " + file_path_;
}

// ZeroCopyIOManager 实现
ZeroCopyIOManager::ZeroCopyIOManager(size_t thread_count, size_t max_buffer_size,
                                    size_t max_mmap_size, bool enable_mmap, bool enable_compression)
    : max_buffer_size_(max_buffer_size)
    , max_mmap_size_(max_mmap_size)
    , enable_mmap_(enable_mmap)
    , enable_compression_(enable_compression)
    , running_(false)
    , stop_flag_(false)
    , total_operations_(0)
    , completed_operations_(0)
    , failed_operations_(0)
    , active_operations_(0)
    , total_bytes_processed_(0)
    , total_io_time_(0) {
    
    worker_threads_.reserve(thread_count);
}

ZeroCopyIOManager::~ZeroCopyIOManager() {
    shutdown();
}

bool ZeroCopyIOManager::initialize() {
    if (running_) {
        LOG(WARNING) << "ZeroCopyIOManager already initialized";
        return true;
    }
    
    running_ = true;
    stop_flag_ = false;
    
    // 启动工作线程
    size_t thread_count = worker_threads_.capacity();
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&ZeroCopyIOManager::worker_thread_function, this);
    }
    
    LOG(INFO) << "ZeroCopyIOManager initialized with " << thread_count << " threads";
    return true;
}

void ZeroCopyIOManager::shutdown() {
    if (!running_) {
        return;
    }
    
    stop_flag_ = true;
    running_ = false;
    queue_cv_.notify_all();
    
    // 等待所有工作线程结束
    for (auto& worker : worker_threads_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    worker_threads_.clear();
    
    LOG(INFO) << "ZeroCopyIOManager shutdown completed";
}

void ZeroCopyIOManager::worker_thread_function() {
    while (!stop_flag_) {
        std::shared_ptr<ZeroCopyIOOperation> operation;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !operation_queue_.empty() || stop_flag_; });
            
            if (stop_flag_) {
                break;
            }
            
            if (!operation_queue_.empty()) {
                operation = operation_queue_.front();
                operation_queue_.pop();
                active_operations_++;
            }
        }
        
        if (operation) {
            process_operation(operation);
            active_operations_--;
        }
    }
}

void ZeroCopyIOManager::process_operation(std::shared_ptr<ZeroCopyIOOperation> operation) {
    total_operations_++;
    
    try {
        operation->execute();
        
        if (operation->get_status() == IOOperationStatus::COMPLETED) {
            completed_operations_++;
            total_bytes_processed_ += operation->get_data_size();
        } else if (operation->get_status() == IOOperationStatus::FAILED) {
            failed_operations_++;
        }
        
        total_io_time_ += operation->get_duration();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in zero-copy operation: " << e.what();
        failed_operations_++;
    }
}

} // namespace Paker
