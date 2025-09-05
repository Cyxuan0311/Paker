#pragma once

#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <memory>

namespace Paker {

// 任务类型
enum class TaskType {
    DOWNLOAD,
    INSTALL,
    EXTRACT,
    VERIFY
};

// 任务状态
enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// 任务信息
struct Task {
    std::string id;
    TaskType type;
    std::string package_name;
    std::string version;
    std::string repository_url;
    std::string target_path;
    std::function<bool()> task_function;
    TaskStatus status;
    std::string error_message;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    Task(const std::string& id, TaskType type, const std::string& package_name)
        : id(id), type(type), package_name(package_name), status(TaskStatus::PENDING) {}
};

// 并行执行器
class ParallelExecutor {
private:
    std::vector<std::thread> workers_;
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> stop_flag_;
    std::atomic<size_t> active_tasks_;
    size_t max_workers_;
    size_t max_concurrent_tasks_;
    
    // 任务结果存储
    std::map<std::string, std::shared_ptr<Task>> completed_tasks_;
    std::mutex results_mutex_;
    
public:
    ParallelExecutor(size_t max_workers = std::thread::hardware_concurrency(),
                    size_t max_concurrent_tasks = 4);
    ~ParallelExecutor();
    
    // 启动和停止
    bool start();
    void stop();
    bool is_running() const;
    
    // 任务管理
    std::string submit_task(std::shared_ptr<Task> task);
    bool wait_for_task(const std::string& task_id, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    bool wait_for_all_tasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    
    // 任务状态查询
    TaskStatus get_task_status(const std::string& task_id) const;
    std::string get_task_error(const std::string& task_id) const;
    std::vector<std::shared_ptr<Task>> get_completed_tasks() const;
    std::vector<std::shared_ptr<Task>> get_failed_tasks() const;
    
    // 统计信息
    size_t get_pending_tasks_count() const;
    size_t get_active_tasks_count() const;
    size_t get_completed_tasks_count() const;
    size_t get_failed_tasks_count() const;
    
    // 配置
    void set_max_workers(size_t max_workers);
    void set_max_concurrent_tasks(size_t max_concurrent_tasks);
    
private:
    void worker_loop();
    void process_task(std::shared_ptr<Task> task);
};

// 下载任务创建器
class DownloadTaskFactory {
public:
    static std::shared_ptr<Task> create_download_task(
        const std::string& package_name,
        const std::string& version,
        const std::string& repository_url,
        const std::string& target_path);
    
    static std::shared_ptr<Task> create_install_task(
        const std::string& package_name,
        const std::string& version,
        const std::string& source_path,
        const std::string& target_path);
    
    static std::shared_ptr<Task> create_verify_task(
        const std::string& package_name,
        const std::string& version,
        const std::string& package_path);
};

// 全局并行执行器实例
extern std::unique_ptr<ParallelExecutor> g_parallel_executor;

// 初始化并行执行器
bool initialize_parallel_executor(size_t max_workers = 0, size_t max_concurrent_tasks = 4);

// 清理并行执行器
void cleanup_parallel_executor();

} // namespace Paker
