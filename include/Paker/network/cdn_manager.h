#pragma once

#include "Paker/common.h"
#include "Paker/network/http2_client.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace Paker {

// CDN节点信息
struct CDNNode {
    std::string name_;           // CDN名称
    std::string base_url_;       // 基础URL
    std::string region_;         // 地区
    double priority_;            // 优先级 (0.0-1.0)
    bool is_active_;             // 是否活跃
    std::chrono::steady_clock::time_point last_used_;
    
    // 性能指标
    double latency_ms_;         // 延迟
    double bandwidth_mbps_;     // 带宽
    double success_rate_;       // 成功率
    size_t total_requests_;     // 总请求数
    size_t successful_requests_; // 成功请求数
    
    CDNNode(const std::string& name, const std::string& base_url, const std::string& region = "")
        : name_(name), base_url_(base_url), region_(region), priority_(1.0), is_active_(true)
        , latency_ms_(100.0), bandwidth_mbps_(10.0), success_rate_(0.95)
        , total_requests_(0), successful_requests_(0) {
    }
};

// CDN选择策略
enum class CDNSelectionStrategy {
    ROUND_ROBIN,        // 轮询
    PRIORITY_BASED,     // 基于优先级
    LATENCY_BASED,      // 基于延迟
    BANDWIDTH_BASED,    // 基于带宽
    SUCCESS_RATE_BASED, // 基于成功率
    ADAPTIVE           // 自适应选择
};

// CDN管理器配置
struct CDNManagerConfig {
    CDNSelectionStrategy strategy_ = CDNSelectionStrategy::ADAPTIVE;
    size_t max_concurrent_downloads_ = 4;
    std::chrono::seconds health_check_interval_{60};
    std::chrono::seconds node_timeout_{30};
    bool enable_failover_ = true;
    bool enable_load_balancing_ = true;
    double min_success_rate_ = 0.8;
    size_t max_retries_per_node_ = 3;
};

// CDN管理器
class CDNManager {
private:
    CDNManagerConfig config_;
    std::vector<std::unique_ptr<CDNNode>> cdn_nodes_;
    std::mutex nodes_mutex_;
    
    // 选择策略状态
    std::atomic<size_t> round_robin_index_{0};
    std::unordered_map<std::string, size_t> node_usage_count_;
    std::mutex usage_mutex_;
    
    // 性能监控
    std::chrono::steady_clock::time_point last_health_check_;
    std::thread health_check_thread_;
    std::atomic<bool> health_check_running_{false};
    
    // 统计信息
    struct CDNStats {
        size_t total_downloads_{0};
        size_t successful_downloads_{0};
        size_t failed_downloads_{0};
        size_t failover_count_{0};
        std::chrono::milliseconds total_download_time_{0};
        double average_throughput_mbps_{0.0};
    } stats_;
    mutable std::mutex stats_mutex_;

public:
    CDNManager(const CDNManagerConfig& config = CDNManagerConfig{});
    ~CDNManager();
    
    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // CDN节点管理
    bool add_cdn_node(const std::string& name, const std::string& base_url, 
                      const std::string& region = "", double priority = 1.0);
    bool remove_cdn_node(const std::string& name);
    bool update_cdn_node(const std::string& name, const std::string& base_url, 
                        double priority = 1.0);
    
    // 下载操作
    std::future<bool> download_file(const std::string& file_path, 
                                  const std::string& local_path,
                                  std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    std::future<std::vector<char>> download_data(const std::string& file_path,
                                                std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    // 并行下载
    std::vector<std::future<bool>> download_multiple_files(
        const std::vector<std::string>& file_paths,
        const std::vector<std::string>& local_paths,
        std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    // CDN选择
    CDNNode* select_best_cdn(const std::string& file_path);
    std::vector<CDNNode*> select_cdn_alternatives(const std::string& file_path, size_t count = 3);
    
    // 配置管理
    void configure(const CDNManagerConfig& config);
    CDNManagerConfig get_config() const;
    
    // 节点管理
    std::vector<CDNNode*> get_active_nodes() const;
    std::vector<CDNNode*> get_nodes_by_region(const std::string& region) const;
    CDNNode* get_node_by_name(const std::string& name) const;
    
    // 性能监控
    void update_node_performance(const std::string& node_name, bool success, 
                                double latency_ms, size_t bytes_transferred);
    void perform_health_check();
    
    // 统计信息
    CDNStats get_stats() const;
    std::vector<std::pair<std::string, double>> get_node_performance_ranking() const;
    
    // 策略管理
    void set_selection_strategy(CDNSelectionStrategy strategy);
    CDNSelectionStrategy get_selection_strategy() const;
    
private:
    // 内部方法
    std::string build_full_url(const CDNNode* node, const std::string& file_path) const;
    CDNNode* select_node_round_robin();
    CDNNode* select_node_priority_based();
    CDNNode* select_node_latency_based();
    CDNNode* select_node_bandwidth_based();
    CDNNode* select_node_success_rate_based();
    CDNNode* select_node_adaptive();
    
    // 健康检查
    void health_check_loop();
    bool check_node_health(CDNNode* node);
    void update_node_health(CDNNode* node, bool is_healthy);
    
    // 性能计算
    double calculate_node_score(const CDNNode* node) const;
    void update_node_statistics(CDNNode* node, bool success, double latency_ms, size_t bytes_transferred);
    void calculate_average_throughput();
    
    // 故障转移
    bool try_failover_download(const std::string& file_path, const std::string& local_path,
                              std::function<void(size_t, size_t)> progress_callback,
                              const std::vector<CDNNode*>& alternative_nodes);
};

// CDN下载任务
class CDNDownloadTask {
private:
    std::string file_path_;
    std::string local_path_;
    std::function<void(size_t, size_t)> progress_callback_;
    std::vector<CDNNode*> available_nodes_;
    size_t current_node_index_;
    size_t retry_count_;
    
public:
    CDNDownloadTask(const std::string& file_path, const std::string& local_path,
                   std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    bool execute(CDNManager* cdn_manager);
    void set_available_nodes(const std::vector<CDNNode*>& nodes);
    
private:
    bool try_download_from_node(CDNNode* node, const std::string& full_url);
    void handle_download_failure(CDNNode* node, const std::string& error_message);
    void handle_download_success(CDNNode* node, size_t bytes_transferred);
};

} // namespace Paker
