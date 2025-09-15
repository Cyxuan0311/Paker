#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>

namespace Paker {

// I/O操作类型
enum class IOOperationType {
    READ_FILE,
    WRITE_FILE,
    READ_DIRECTORY,
    DELETE_FILE,
    COPY_FILE,
    MOVE_FILE,
    CREATE_DIRECTORY,
    NETWORK_DOWNLOAD,
    NETWORK_UPLOAD
};

// I/O操作状态
enum class IOOperationStatus {
    PENDING,        // 等待中
    IN_PROGRESS,    // 进行中
    COMPLETED,      // 已完成
    FAILED,         // 失败
    CANCELLED       // 已取消
};

// I/O操作结果
struct IOResult {
    IOOperationStatus status;
    std::string error_message;
    size_t bytes_processed;
    std::chrono::milliseconds duration;
    
    IOResult() : status(IOOperationStatus::PENDING), bytes_processed(0), duration(0) {}
};

// 文件读取结果
struct FileReadResult : public IOResult {
    std::vector<char> data;
    std::string content;  // 文本内容
    size_t file_size;
    
    FileReadResult() : file_size(0) {}
};

// 文件写入结果
struct FileWriteResult : public IOResult {
    std::string file_path;
    size_t bytes_written;
    
    FileWriteResult() : bytes_written(0) {}
};

// 网络下载结果
struct NetworkDownloadResult : public IOResult {
    std::string url;
    std::string local_path;
    std::vector<char> data;
    size_t content_length;
    int http_status_code;
    
    NetworkDownloadResult() : content_length(0), http_status_code(0) {}
};

// 异步I/O操作基类
class AsyncIOOperation {
public:
    virtual ~AsyncIOOperation() = default;
    
    virtual IOOperationType get_type() const = 0;
    virtual std::string get_description() const = 0;
    virtual void execute() = 0;
    virtual void cancel() = 0;
    
    // 状态管理
    IOOperationStatus get_status() const { return status_; }
    std::string get_error_message() const { return error_message_; }
    std::chrono::milliseconds get_duration() const { return duration_; }
    
    // 进度回调
    using ProgressCallback = std::function<void(size_t current, size_t total)>;
    void set_progress_callback(ProgressCallback callback) { progress_callback_ = callback; }
    
protected:
    std::atomic<IOOperationStatus> status_{IOOperationStatus::PENDING};
    std::string error_message_;
    std::chrono::milliseconds duration_{0};
    ProgressCallback progress_callback_;
    std::atomic<bool> cancelled_{false};
    
    void set_status(IOOperationStatus status) { status_ = status; }
    void set_error(const std::string& error) { error_message_ = error; status_ = IOOperationStatus::FAILED; }
public:
    void update_progress(size_t current, size_t total) {
        if (progress_callback_) {
            progress_callback_(current, total);
        }
    }
};

// 异步文件读取操作
class AsyncFileReadOperation : public AsyncIOOperation {
private:
    std::string file_path_;
    std::shared_ptr<FileReadResult> result_;
    bool read_as_text_;
    
public:
    AsyncFileReadOperation(const std::string& file_path, bool read_as_text = true);
    
    IOOperationType get_type() const override { return IOOperationType::READ_FILE; }
    std::string get_description() const override;
    void execute() override;
    void cancel() override;
    
    std::shared_ptr<FileReadResult> get_result() const { return result_; }
};

// 异步文件写入操作
class AsyncFileWriteOperation : public AsyncIOOperation {
private:
    std::string file_path_;
    std::vector<char> data_;
    std::string text_content_;
    std::shared_ptr<FileWriteResult> result_;
    bool write_as_text_;
    
public:
    AsyncFileWriteOperation(const std::string& file_path, const std::vector<char>& data);
    AsyncFileWriteOperation(const std::string& file_path, const std::string& content);
    
    IOOperationType get_type() const override { return IOOperationType::WRITE_FILE; }
    std::string get_description() const override;
    void execute() override;
    void cancel() override;
    
    std::shared_ptr<FileWriteResult> get_result() const { return result_; }
};

// 异步网络下载操作
class AsyncNetworkDownloadOperation : public AsyncIOOperation {
private:
    std::string url_;
    std::string local_path_;
    std::shared_ptr<NetworkDownloadResult> result_;
    
public:
    AsyncNetworkDownloadOperation(const std::string& url, const std::string& local_path = "");
    
    IOOperationType get_type() const override { return IOOperationType::NETWORK_DOWNLOAD; }
    std::string get_description() const override;
    void execute() override;
    void cancel() override;
    
    std::shared_ptr<NetworkDownloadResult> get_result() const { return result_; }
};

// 异步I/O管理器
class AsyncIOManager {
private:
    // 工作线程池
    std::vector<std::thread> worker_threads_;
    std::queue<std::shared_ptr<AsyncIOOperation>> operation_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 控制变量
    std::atomic<bool> running_{false};
    std::atomic<size_t> active_operations_{0};
    size_t max_concurrent_operations_;
    
    // 统计信息
    std::atomic<size_t> total_operations_{0};
    std::atomic<size_t> completed_operations_{0};
    std::atomic<size_t> failed_operations_{0};
    std::chrono::milliseconds total_io_time_{0};
    
    // 内部方法
    void worker_thread_function();
    void process_operation(std::shared_ptr<AsyncIOOperation> operation);
    
public:
    AsyncIOManager(size_t thread_count = std::thread::hardware_concurrency(), 
                   size_t max_concurrent = 10);
    ~AsyncIOManager();
    
    // 生命周期管理
    bool initialize();
    void shutdown();
    bool is_running() const { return running_; }
    
    // 操作提交
    std::future<std::shared_ptr<FileReadResult>> read_file_async(
        const std::string& file_path, bool read_as_text = true);
    
    std::future<std::shared_ptr<FileWriteResult>> write_file_async(
        const std::string& file_path, const std::vector<char>& data);
    
    std::future<std::shared_ptr<FileWriteResult>> write_file_async(
        const std::string& file_path, const std::string& content);
    
    std::future<std::shared_ptr<NetworkDownloadResult>> download_async(
        const std::string& url, const std::string& local_path = "");
    
    // 批量操作
    std::vector<std::future<std::shared_ptr<FileReadResult>>> read_files_async(
        const std::vector<std::string>& file_paths, bool read_as_text = true);
    
    std::vector<std::future<std::shared_ptr<FileWriteResult>>> write_files_async(
        const std::vector<std::pair<std::string, std::string>>& file_contents);
    
    // 配置管理
    void set_max_concurrent_operations(size_t max_concurrent);
    size_t get_max_concurrent_operations() const { return max_concurrent_operations_; }
    
    // 统计信息
    size_t get_total_operations() const { return total_operations_; }
    size_t get_completed_operations() const { return completed_operations_; }
    size_t get_failed_operations() const { return failed_operations_; }
    size_t get_active_operations() const { return active_operations_; }
    std::chrono::milliseconds get_total_io_time() const { return total_io_time_; }
    
    // 性能监控
    double get_success_rate() const;
    double get_average_operation_time() const;
    std::string get_performance_report() const;
    
    // 队列管理
    size_t get_queue_size() const;
    void clear_queue();
    void cancel_all_operations();
};

// 异步I/O工具类
class AsyncIOTools {
public:
    // 便捷方法
    static std::future<std::string> read_text_file_async(
        AsyncIOManager& manager, const std::string& file_path);
    
    static std::future<bool> write_text_file_async(
        AsyncIOManager& manager, const std::string& file_path, const std::string& content);
    
    static std::future<std::vector<char>> read_binary_file_async(
        AsyncIOManager& manager, const std::string& file_path);
    
    static std::future<bool> write_binary_file_async(
        AsyncIOManager& manager, const std::string& file_path, const std::vector<char>& data);
    
    // 批量操作
    static std::future<std::vector<std::string>> read_multiple_text_files_async(
        AsyncIOManager& manager, const std::vector<std::string>& file_paths);
    
    static std::future<bool> write_multiple_text_files_async(
        AsyncIOManager& manager, 
        const std::vector<std::pair<std::string, std::string>>& file_contents);
    
    // 目录操作
    static std::future<std::vector<std::string>> list_directory_async(
        AsyncIOManager& manager, const std::string& directory_path);
    
    static std::future<bool> create_directory_async(
        AsyncIOManager& manager, const std::string& directory_path);
    
    // 文件系统操作
    static std::future<bool> file_exists_async(
        AsyncIOManager& manager, const std::string& file_path);
    
    static std::future<size_t> get_file_size_async(
        AsyncIOManager& manager, const std::string& file_path);
    
    static std::future<std::chrono::system_clock::time_point> get_file_modification_time_async(
        AsyncIOManager& manager, const std::string& file_path);
};

// 全局异步I/O管理器实例
extern std::unique_ptr<AsyncIOManager> g_async_io_manager;

// 初始化函数
bool initialize_async_io_manager(size_t thread_count = std::thread::hardware_concurrency(),
                                 size_t max_concurrent = 10);
void cleanup_async_io_manager();

// 便捷访问函数
AsyncIOManager* get_async_io_manager();

} // namespace Paker
