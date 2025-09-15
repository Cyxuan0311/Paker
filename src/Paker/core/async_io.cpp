#include "Paker/core/async_io.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <glog/logging.h>
#include <curl/curl.h>

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
        
        // 读取文件
        std::ifstream file(file_path_, read_as_text_ ? std::ios::in : std::ios::binary);
        if (!file.is_open()) {
            set_error("Failed to open file: " + file_path_);
            return;
        }
        
        if (read_as_text_) {
            // 读取文本内容
            std::stringstream buffer;
            buffer << file.rdbuf();
            result_->content = buffer.str();
            result_->bytes_processed = result_->content.size();
        } else {
            // 读取二进制内容
            result_->data.resize(result_->file_size);
            file.read(result_->data.data(), result_->file_size);
            result_->bytes_processed = file.gcount();
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
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        // 注意：CURLOPT_XFERINFOFUNCTION 需要 CURL 7.32.0+
        // 如果版本不支持，可以注释掉进度回调
        // curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        // curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        
        if (cancelled_) {
            curl_easy_cleanup(curl);
            set_status(IOOperationStatus::CANCELLED);
            return;
        }
        
        // 执行下载
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            set_error("CURL error: " + std::string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return;
        }
        
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
AsyncIOManager::AsyncIOManager(size_t thread_count, size_t max_concurrent)
    : max_concurrent_operations_(max_concurrent) {
    worker_threads_.reserve(thread_count);
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
    active_operations_++;
    total_operations_++;
    
    try {
        operation->execute();
        
        if (operation->get_status() == IOOperationStatus::COMPLETED) {
            completed_operations_++;
        } else if (operation->get_status() == IOOperationStatus::FAILED) {
            failed_operations_++;
        }
        
        total_io_time_ += operation->get_duration();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception in async operation: " << e.what();
        failed_operations_++;
    }
    
    active_operations_--;
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
    return static_cast<double>(completed_operations_.load()) / total * 100.0;
}

double AsyncIOManager::get_average_operation_time() const {
    size_t completed = completed_operations_.load();
    if (completed == 0) return 0.0;
    return static_cast<double>(total_io_time_.count()) / completed;
}

std::string AsyncIOManager::get_performance_report() const {
    std::stringstream ss;
    ss << "AsyncIO Performance Report:\n";
    ss << "  Total operations: " << total_operations_.load() << "\n";
    ss << "  Completed operations: " << completed_operations_.load() << "\n";
    ss << "  Failed operations: " << failed_operations_.load() << "\n";
    ss << "  Active operations: " << active_operations_.load() << "\n";
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
    return g_async_io_manager.get();
}

} // namespace Paker
