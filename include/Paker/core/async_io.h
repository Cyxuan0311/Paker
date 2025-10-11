#pragma once

#include "Paker/common.h"
#include <future>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <unordered_map>

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

// 缓冲区类型
enum class BufferType {
    FILE_READ,
    FILE_WRITE,
    NETWORK_DOWNLOAD,
    NETWORK_UPLOAD
};

// 缓冲区配置
struct BufferConfig {
    size_t min_size;
    size_t max_size;
    size_t initial_size;
    double growth_factor;
    size_t max_memory_usage;
    
    BufferConfig() : min_size(4096), max_size(16 * 1024 * 1024), 
                    initial_size(64 * 1024), growth_factor(1.5), 
                    max_memory_usage(512 * 1024 * 1024) {}
};

// 缓冲区性能指标
struct BufferMetrics {
    size_t buffer_size;
    size_t bytes_processed;
    std::chrono::milliseconds processing_time;
    double throughput_mbps;
    
    BufferMetrics() : buffer_size(0), bytes_processed(0), processing_time(0), throughput_mbps(0.0) {}
};

// 文件访问模式
struct FileAccessPattern {
    std::string file_path;
    size_t access_count;
    std::chrono::steady_clock::time_point last_access;
    size_t average_read_size;
    double access_frequency;
    
    FileAccessPattern() : access_count(0), average_read_size(0), access_frequency(0.0) {}
};

// 网络重试配置
struct RetryConfig {
    size_t max_retries;
    std::chrono::milliseconds initial_delay;
    double backoff_factor;
    std::chrono::milliseconds max_delay;
    std::vector<int> retryable_http_codes;
    
    RetryConfig() : max_retries(3), initial_delay(std::chrono::milliseconds(1000)),
                   backoff_factor(2.0), max_delay(std::chrono::milliseconds(30000)) {
        retryable_http_codes = {408, 429, 500, 502, 503, 504};
    }
};

// 网络状况指标
struct NetworkMetrics {
    double latency_ms;           // 延迟（毫秒）
    double bandwidth_mbps;      // 带宽（Mbps）
    double packet_loss_rate;    // 丢包率
    double success_rate;        // 成功率
    std::chrono::steady_clock::time_point timestamp;
    
    NetworkMetrics() : latency_ms(0.0), bandwidth_mbps(0.0), 
                      packet_loss_rate(0.0), success_rate(1.0) {}
};

// 自适应重试策略
class AdaptiveRetryStrategy {
private:
    std::map<std::string, std::vector<NetworkMetrics>> network_history_;
    mutable std::mutex history_mutex_;
    std::chrono::milliseconds base_delay_;
    double adaptive_factor_;
    size_t max_history_size_;
    
public:
    AdaptiveRetryStrategy(std::chrono::milliseconds base_delay = std::chrono::milliseconds(1000),
                         double adaptive_factor = 1.5,
                         size_t max_history = 50);
    
    void record_network_metrics(const std::string& url, const NetworkMetrics& metrics);
    std::chrono::milliseconds calculate_retry_delay(const std::string& url, size_t attempt) const;
    bool should_retry(const std::string& url, int http_code, size_t attempt) const;
    void update_strategy_parameters();
    double get_network_quality(const std::string& url) const;
};

// 依赖关系预测
struct DependencyPrediction {
    std::string package_name;
    std::string version;
    double confidence;           // 预测置信度
    std::vector<std::string> dependencies;  // 依赖包列表
    std::chrono::steady_clock::time_point prediction_time;
    
    DependencyPrediction() : confidence(0.0) {}
};

// 预测性预加载策略
class PredictivePreloadStrategy {
private:
    std::map<std::string, std::vector<DependencyPrediction>> prediction_history_;
    mutable std::mutex prediction_mutex_;
    std::map<std::string, std::vector<std::string>> dependency_graph_;
    std::map<std::string, size_t> package_frequency_;
    std::chrono::milliseconds preload_interval_;
    std::chrono::steady_clock::time_point last_preload_;
    
    // 预测参数
    double confidence_threshold_;
    size_t max_predictions_;
    double frequency_weight_;
    double recency_weight_;
    double dependency_weight_;
    
public:
    PredictivePreloadStrategy(double confidence_threshold = 0.7,
                             size_t max_predictions = 10,
                             double freq_weight = 0.4,
                             double rec_weight = 0.3,
                             double dep_weight = 0.3);
    
    void record_package_usage(const std::string& package_name);
    void update_dependency_graph(const std::string& package, const std::vector<std::string>& dependencies);
    std::vector<DependencyPrediction> predict_dependencies(const std::string& package_name) const;
    void preload_predicted_packages();
    void update_prediction_parameters();
    double calculate_prediction_confidence(const std::string& package_name, 
                                         const std::string& dependency) const;
};

// 零拷贝缓冲区
class ZeroCopyBuffer {
private:
    void* data_;
    size_t size_;
    size_t capacity_;
    std::function<void(void*)> deleter_;
    std::atomic<size_t> ref_count_;
    bool is_mmap_;
    
public:
    ZeroCopyBuffer(size_t size, bool use_mmap = false);
    ZeroCopyBuffer(void* data, size_t size, std::function<void(void*)> deleter = nullptr);
    ~ZeroCopyBuffer();
    
    // 禁止拷贝，只允许移动
    ZeroCopyBuffer(const ZeroCopyBuffer&) = delete;
    ZeroCopyBuffer& operator=(const ZeroCopyBuffer&) = delete;
    ZeroCopyBuffer(ZeroCopyBuffer&& other) noexcept;
    ZeroCopyBuffer& operator=(ZeroCopyBuffer&& other) noexcept;
    
    // 引用计数管理
    ZeroCopyBuffer* add_ref();
    void release();
    
    // 数据访问
    void* data() const { return data_; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool is_mmap() const { return is_mmap_; }
    
    // 零拷贝操作
    ZeroCopyBuffer* slice(size_t offset, size_t length) const;
    bool resize(size_t new_size);
    void* get_write_ptr();
    
    // 内存映射
    bool map_file(const std::string& file_path, size_t offset = 0, size_t length = 0);
    void unmap();
    
private:
    void cleanup();
};

// 零拷贝I/O操作
class ZeroCopyIOOperation {
private:
    std::string file_path_;
    ZeroCopyBuffer* buffer_;
    IOOperationType type_;
    IOOperationStatus status_;
    std::string error_message_;
    std::chrono::milliseconds duration_;
    std::atomic<bool> cancelled_;
    
    // 进度回调
    std::function<void(size_t, size_t)> progress_callback_;
    
public:
    ZeroCopyIOOperation(const std::string& file_path, IOOperationType type);
    ~ZeroCopyIOOperation();
    
    // 操作执行
    void execute();
    void cancel();
    
    // 状态查询
    IOOperationStatus get_status() const { return status_; }
    std::string get_error() const { return error_message_; }
    std::chrono::milliseconds get_duration() const { return duration_; }
    bool is_cancelled() const { return cancelled_.load(); }
    
    // 进度管理
    void set_progress_callback(std::function<void(size_t, size_t)> callback);
    void update_progress(size_t current, size_t total);
    
    // 零拷贝数据访问
    ZeroCopyBuffer* get_buffer() const { return buffer_; }
    void* get_data() const;
    size_t get_data_size() const;
    
    // 描述信息
    std::string get_description() const;
    
private:
    void set_status(IOOperationStatus status);
    void set_error(const std::string& error);
    bool execute_file_read();
    bool execute_file_write();
    bool execute_network_download();
    bool execute_network_upload();
};

// 零拷贝I/O管理器
class ZeroCopyIOManager {
private:
    std::vector<std::thread> worker_threads_;
    std::queue<std::shared_ptr<ZeroCopyIOOperation>> operation_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_flag_;
    
    // 零拷贝配置
    size_t max_buffer_size_;
    size_t max_mmap_size_;
    bool enable_mmap_;
    bool enable_compression_;
    
    // 统计信息
    std::atomic<size_t> total_operations_;
    std::atomic<size_t> completed_operations_;
    std::atomic<size_t> failed_operations_;
    std::atomic<size_t> active_operations_;
    std::atomic<size_t> total_bytes_processed_;
    std::chrono::milliseconds total_io_time_;
    
public:
    ZeroCopyIOManager(size_t thread_count = std::thread::hardware_concurrency(),
                     size_t max_buffer_size = 1024 * 1024 * 1024,  // 1GB
                     size_t max_mmap_size = 256 * 1024 * 1024,      // 256MB
                     bool enable_mmap = true,
                     bool enable_compression = true);
    
    ~ZeroCopyIOManager();
    
    // 初始化和关闭
    bool initialize();
    void shutdown();
    bool is_running() const;
    
    // 零拷贝I/O操作
    std::future<ZeroCopyBuffer*> read_file_zero_copy(const std::string& file_path);
    std::future<bool> write_file_zero_copy(const std::string& file_path, ZeroCopyBuffer* buffer);
    std::future<ZeroCopyBuffer*> download_zero_copy(const std::string& url);
    std::future<bool> upload_zero_copy(const std::string& url, ZeroCopyBuffer* buffer);
    
    // 内存映射文件操作
    std::future<ZeroCopyBuffer*> mmap_file(const std::string& file_path, size_t offset = 0, size_t length = 0);
    std::future<bool> munmap_file(ZeroCopyBuffer* buffer);
    
    // 配置管理
    void set_max_buffer_size(size_t size);
    void set_max_mmap_size(size_t size);
    void enable_mmap(bool enable);
    void enable_compression(bool enable);
    
    // 统计信息
    size_t get_total_operations() const;
    size_t get_completed_operations() const;
    size_t get_failed_operations() const;
    size_t get_active_operations() const;
    size_t get_total_bytes_processed() const;
    double get_average_throughput() const;
    std::string get_detailed_report() const;
    
private:
    void worker_thread_function();
    void process_operation(std::shared_ptr<ZeroCopyIOOperation> operation);
    ZeroCopyBuffer* create_buffer(size_t size, bool use_mmap = false);
    void cleanup_buffer(ZeroCopyBuffer* buffer);
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
    
    // 动态缓冲区管理
    std::unordered_map<BufferType, BufferConfig> buffer_configs_;
    std::unordered_map<BufferType, std::vector<BufferMetrics>> performance_history_;
    std::atomic<size_t> total_memory_usage_{0};
    std::atomic<bool> adaptive_buffering_enabled_{true};
    
    // 智能预读策略
    std::unordered_map<std::string, FileAccessPattern> access_patterns_;
    mutable std::mutex patterns_mutex_;
    size_t max_patterns_;
    std::atomic<bool> smart_pre_read_enabled_{true};
    std::unique_ptr<PredictivePreloadStrategy> predictive_preload_strategy_;
    
    // 网络重试策略
    RetryConfig retry_config_;
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> retry_history_;
    mutable std::mutex retry_mutex_;
    std::atomic<bool> network_retry_enabled_{true};
    std::unique_ptr<AdaptiveRetryStrategy> adaptive_retry_strategy_;
    
    // 批量处理优化
    struct BatchOperation {
        std::string operation_id;
        IOOperationType type;
        std::vector<std::string> file_paths;
        std::chrono::steady_clock::time_point submit_time;
        size_t priority;
        
        BatchOperation() : priority(0) {}
    };
    
    std::vector<BatchOperation> pending_batch_operations_;
    mutable std::mutex batch_mutex_;
    std::atomic<bool> batch_optimization_enabled_{true};
    size_t max_batch_size_;
    std::chrono::milliseconds max_batch_wait_time_;
    
    // 内部方法
    void worker_thread_function();
    void process_operation(std::shared_ptr<AsyncIOOperation> operation);
    
    // 缓冲区管理
    std::vector<char> get_optimal_buffer(BufferType type, size_t preferred_size = 0);
    void record_buffer_operation(BufferType type, const BufferMetrics& metrics);
    void optimize_buffer_configs();
    
    // 智能预读
    void update_file_access_pattern(const std::string& file_path, size_t read_size);
    void perform_smart_pre_read();
    bool should_pre_read(const std::string& file_path) const;
    size_t get_suggested_pre_read_size(const std::string& file_path) const;
    
    // 网络重试
    bool should_retry_network_operation(const std::string& url, int http_code, const std::string& error);
    std::chrono::milliseconds get_retry_delay(const std::string& url, size_t attempt_number);
    void record_retry_attempt(const std::string& url);
    
    // 批量处理
    void process_batch_operations();
    void optimize_batch_scheduling();
    
public:
    AsyncIOManager(size_t thread_count = std::thread::hardware_concurrency(), 
                   size_t max_concurrent = 10,
                   size_t max_patterns = 1000,
                   size_t max_batch_size = 50,
                   std::chrono::milliseconds max_batch_wait_time = std::chrono::milliseconds(100));
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
    
    // 动态缓冲区管理
    void set_buffer_config(BufferType type, const BufferConfig& config);
    BufferConfig get_buffer_config(BufferType type) const;
    void enable_adaptive_buffering(bool enabled) { adaptive_buffering_enabled_ = enabled; }
    bool is_adaptive_buffering_enabled() const { return adaptive_buffering_enabled_; }
    size_t get_memory_usage() const { return total_memory_usage_; }
    
    // 智能预读管理
    void enable_smart_pre_read(bool enabled) { smart_pre_read_enabled_ = enabled; }
    bool is_smart_pre_read_enabled() const { return smart_pre_read_enabled_; }
    void trigger_pre_read_analysis();
    std::vector<std::string> get_pre_read_candidates() const;
    
    // 网络重试管理
    void set_retry_config(const RetryConfig& config);
    RetryConfig get_retry_config() const;
    void enable_network_retry(bool enabled) { network_retry_enabled_ = enabled; }
    bool is_network_retry_enabled() const { return network_retry_enabled_; }
    
    // 批量处理管理
    void enable_batch_optimization(bool enabled) { batch_optimization_enabled_ = enabled; }
    bool is_batch_optimization_enabled() const { return batch_optimization_enabled_; }
    void set_batch_config(size_t max_batch_size, std::chrono::milliseconds max_wait_time);
    void process_pending_batches();
    
    // 性能优化
    void optimize_performance();
    std::vector<std::string> get_optimization_suggestions() const;
    void apply_optimization_suggestions();
    
    // 增强的统计信息
    double get_average_throughput() const;
    double get_cache_hit_rate() const;
    size_t get_total_bytes_processed() const;
    std::string get_detailed_performance_report() const;
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
