#include "Paker/core/parallel_executor.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <algorithm>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Paker {

// 全局并行执行器实例
std::unique_ptr<ParallelExecutor> g_parallel_executor;

ParallelExecutor::ParallelExecutor(size_t max_workers, size_t max_concurrent_tasks)
    : stop_flag_(false)
    , active_tasks_(0)
    , max_workers_(max_workers == 0 ? std::thread::hardware_concurrency() : max_workers)
    , max_concurrent_tasks_(max_concurrent_tasks) {
    
    if (max_workers_ == 0) {
        max_workers_ = 1; // 至少一个工作线程
    }
    
    LOG(INFO) << "ParallelExecutor initialized with " << max_workers_ 
              << " workers, max " << max_concurrent_tasks_ << " concurrent tasks";
}

ParallelExecutor::~ParallelExecutor() {
    stop();
}

bool ParallelExecutor::start() {
    if (is_running()) {
        LOG(WARNING) << "ParallelExecutor is already running";
        return true;
    }
    
    stop_flag_ = false;
    
    // 启动工作线程
    for (size_t i = 0; i < max_workers_; ++i) {
        workers_.emplace_back(&ParallelExecutor::worker_loop, this);
    }
    
    LOG(INFO) << "Started " << max_workers_ << " worker threads";
    return true;
}

void ParallelExecutor::stop() {
    if (!is_running()) {
        return;
    }
    
    stop_flag_ = true;
    queue_cv_.notify_all();
    
    // 等待所有工作线程结束
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    
    LOG(INFO) << "ParallelExecutor stopped";
}

bool ParallelExecutor::is_running() const {
    return !workers_.empty() && !stop_flag_;
}

std::string ParallelExecutor::submit_task(std::shared_ptr<Task> task) {
    if (!is_running()) {
        LOG(ERROR) << "Cannot submit task: ParallelExecutor is not running";
        return "";
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push(task);
    queue_cv_.notify_one();
    
    LOG(INFO) << "Submitted task: " << task->id << " (" << task->package_name << ")";
    return task->id;
}

bool ParallelExecutor::wait_for_task(const std::string& task_id, std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(results_mutex_);
            auto it = completed_tasks_.find(task_id);
            if (it != completed_tasks_.end()) {
                return it->second->status == TaskStatus::COMPLETED;
            }
        }
        
        if (timeout.count() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout) {
                LOG(WARNING) << "Timeout waiting for task: " << task_id;
                return false;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool ParallelExecutor::wait_for_all_tasks(std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (task_queue_.empty() && active_tasks_.load() == 0) {
                return true;
            }
        }
        
        if (timeout.count() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout) {
                LOG(WARNING) << "Timeout waiting for all tasks to complete";
                return false;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TaskStatus ParallelExecutor::get_task_status(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    auto it = completed_tasks_.find(task_id);
    if (it != completed_tasks_.end()) {
        return it->second->status;
    }
    return TaskStatus::PENDING;
}

std::string ParallelExecutor::get_task_error(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    auto it = completed_tasks_.find(task_id);
    if (it != completed_tasks_.end()) {
        return it->second->error_message;
    }
    return "";
}

std::vector<std::shared_ptr<Task>> ParallelExecutor::get_completed_tasks() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    std::vector<std::shared_ptr<Task>> completed;
    
    for (const auto& [id, task] : completed_tasks_) {
        if (task->status == TaskStatus::COMPLETED) {
            completed.push_back(task);
        }
    }
    
    return completed;
}

std::vector<std::shared_ptr<Task>> ParallelExecutor::get_failed_tasks() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    std::vector<std::shared_ptr<Task>> failed;
    
    for (const auto& [id, task] : completed_tasks_) {
        if (task->status == TaskStatus::FAILED) {
            failed.push_back(task);
        }
    }
    
    return failed;
}

size_t ParallelExecutor::get_pending_tasks_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

size_t ParallelExecutor::get_active_tasks_count() const {
    return active_tasks_.load();
}

size_t ParallelExecutor::get_completed_tasks_count() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    size_t count = 0;
    for (const auto& [id, task] : completed_tasks_) {
        if (task->status == TaskStatus::COMPLETED) {
            count++;
        }
    }
    return count;
}

size_t ParallelExecutor::get_failed_tasks_count() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    size_t count = 0;
    for (const auto& [id, task] : completed_tasks_) {
        if (task->status == TaskStatus::FAILED) {
            count++;
        }
    }
    return count;
}

void ParallelExecutor::set_max_workers(size_t max_workers) {
    if (is_running()) {
        LOG(WARNING) << "Cannot change max_workers while running";
        return;
    }
    max_workers_ = max_workers;
}

void ParallelExecutor::set_max_concurrent_tasks(size_t max_concurrent_tasks) {
    max_concurrent_tasks_ = max_concurrent_tasks;
}

void ParallelExecutor::worker_loop() {
    while (!stop_flag_) {
        std::shared_ptr<Task> task;
        
        // 获取任务
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || stop_flag_; });
            
            if (stop_flag_) {
                break;
            }
            
            if (active_tasks_.load() >= max_concurrent_tasks_) {
                continue; // 等待当前任务完成
            }
            
            task = task_queue_.front();
            task_queue_.pop();
            active_tasks_++;
        }
        
        // 处理任务
        process_task(task);
        active_tasks_--;
    }
}

void ParallelExecutor::process_task(std::shared_ptr<Task> task) {
    task->status = TaskStatus::RUNNING;
    task->start_time = std::chrono::steady_clock::now();
    
    LOG(INFO) << "Processing task: " << task->id << " (" << task->package_name << ")";
    
    try {
        bool success = task->task_function();
        task->status = success ? TaskStatus::COMPLETED : TaskStatus::FAILED;
        
        if (!success) {
            task->error_message = "Task execution failed";
        }
    } catch (const std::exception& e) {
        task->status = TaskStatus::FAILED;
        task->error_message = e.what();
        LOG(ERROR) << "Task " << task->id << " failed with exception: " << e.what();
    } catch (...) {
        task->status = TaskStatus::FAILED;
        task->error_message = "Unknown error occurred";
        LOG(ERROR) << "Task " << task->id << " failed with unknown error";
    }
    
    task->end_time = std::chrono::steady_clock::now();
    
    // 存储结果
    {
        std::lock_guard<std::mutex> lock(results_mutex_);
        completed_tasks_[task->id] = task;
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        task->end_time - task->start_time).count();
    
    LOG(INFO) << "Task " << task->id << " completed in " << duration << "ms with status: " 
              << (task->status == TaskStatus::COMPLETED ? "SUCCESS" : "FAILED");
}

// DownloadTaskFactory 实现
std::shared_ptr<Task> DownloadTaskFactory::create_download_task(
    const std::string& package_name,
    const std::string& version,
    const std::string& repository_url,
    const std::string& target_path) {
    
    auto task = std::make_shared<Task>(
        package_name + "_" + version + "_download",
        TaskType::DOWNLOAD,
        package_name
    );
    
    task->version = version;
    task->repository_url = repository_url;
    task->target_path = target_path;
    
    task->task_function = [package_name, version, repository_url, target_path]() -> bool {
        try {
            LOG(INFO) << "Downloading " << package_name << "@" << version << " from " << repository_url;
            
            // 创建目标目录
            fs::create_directories(fs::path(target_path).parent_path());
            
            // 执行git clone
            std::ostringstream cmd;
            cmd << "git clone --depth 1 " << repository_url << " " << target_path;
            int ret = std::system(cmd.str().c_str());
            
            if (ret != 0) {
                LOG(ERROR) << "Failed to clone repository: " << repository_url;
                return false;
            }
            
            // 检出版本
            if (!version.empty() && version != "*" && version != "latest") {
                std::ostringstream checkout_cmd;
                checkout_cmd << "cd " << target_path << " && git fetch --tags && git checkout " << version;
                ret = std::system(checkout_cmd.str().c_str());
                if (ret != 0) {
                    LOG(WARNING) << "Failed to checkout version " << version;
                }
            }
            
            LOG(INFO) << "Successfully downloaded " << package_name << "@" << version;
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during download: " << e.what();
            return false;
        }
    };
    
    return task;
}

std::shared_ptr<Task> DownloadTaskFactory::create_install_task(
    const std::string& package_name,
    const std::string& version,
    const std::string& source_path,
    const std::string& target_path) {
    
    auto task = std::make_shared<Task>(
        package_name + "_" + version + "_install",
        TaskType::INSTALL,
        package_name
    );
    
    task->version = version;
    task->target_path = target_path;
    
    task->task_function = [package_name, version, source_path, target_path]() -> bool {
        try {
            LOG(INFO) << "Installing " << package_name << "@" << version << " to " << target_path;
            
            // 创建符号链接
            if (fs::exists(target_path)) {
                fs::remove(target_path);
            }
            
            fs::create_symlink(source_path, target_path);
            
            LOG(INFO) << "Successfully installed " << package_name << "@" << version;
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during install: " << e.what();
            return false;
        }
    };
    
    return task;
}

std::shared_ptr<Task> DownloadTaskFactory::create_verify_task(
    const std::string& package_name,
    const std::string& version,
    const std::string& package_path) {
    
    auto task = std::make_shared<Task>(
        package_name + "_" + version + "_verify",
        TaskType::VERIFY,
        package_name
    );
    
    task->version = version;
    task->target_path = package_path;
    
    task->task_function = [package_name, version, package_path]() -> bool {
        try {
            LOG(INFO) << "Verifying " << package_name << "@" << version;
            
            // 检查包路径是否存在
            if (!std::filesystem::exists(package_path)) {
                LOG(ERROR) << "Package path does not exist: " << package_path;
                return false;
            }
            
            // 检查关键文件
            std::vector<std::string> required_files = {"README.md", "CMakeLists.txt", "include", "src"};
            for (const auto& file : required_files) {
                std::string file_path = package_path + "/" + file;
                if (!std::filesystem::exists(file_path)) {
                    LOG(WARNING) << "Optional file not found: " << file_path;
                }
            }
            
            LOG(INFO) << "Successfully verified " << package_name << "@" << version;
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during verification: " << e.what();
            return false;
        }
    };
    
    return task;
}

// 全局函数实现
bool initialize_parallel_executor(size_t max_workers, size_t max_concurrent_tasks) {
    if (g_parallel_executor) {
        LOG(WARNING) << "ParallelExecutor is already initialized";
        return true;
    }
    
    g_parallel_executor = std::make_unique<ParallelExecutor>(max_workers, max_concurrent_tasks);
    return g_parallel_executor->start();
}

void cleanup_parallel_executor() {
    if (g_parallel_executor) {
        g_parallel_executor->stop();
        g_parallel_executor.reset();
    }
}

} // namespace Paker
