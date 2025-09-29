#pragma once

#include "Paker/common.h"
#include <curl/curl.h>
#include <curl/multi.h>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Paker {

// HTTP/2连接池配置
struct HTTP2PoolConfig {
    size_t max_connections_ = 10;           // 最大连接数
    size_t max_connections_per_host_ = 6;   // 每个主机最大连接数
    std::chrono::seconds connection_timeout_{30};  // 连接超时
    std::chrono::seconds idle_timeout_{300};       // 空闲超时
    bool enable_http2_ = true;              // 启用HTTP/2
    bool enable_compression_ = true;       // 启用压缩
    bool enable_pipelining_ = true;        // 启用管道化
};

// HTTP/2连接信息
struct HTTP2Connection {
    CURL* curl_handle_;
    std::string host_;
    std::string scheme_;
    std::chrono::steady_clock::time_point last_used_;
    bool is_http2_;
    bool is_active_;
    
    HTTP2Connection() : curl_handle_(nullptr), is_http2_(false), is_active_(false) {}
    
    ~HTTP2Connection() {
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
        }
    }
};

// HTTP/2客户端
class HTTP2Client {
private:
    HTTP2PoolConfig config_;
    CURLM* multi_handle_;
    
    // 连接池
    std::unordered_map<std::string, std::queue<std::unique_ptr<HTTP2Connection>>> connection_pools_;
    std::mutex pool_mutex_;
    
    // 活跃连接
    std::unordered_map<CURL*, std::unique_ptr<HTTP2Connection>> active_connections_;
    std::mutex active_mutex_;
    
    // 连接统计
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> active_connections_count_{0};
    std::atomic<size_t> http2_connections_{0};
    
    // 性能统计
    struct NetworkStats {
        size_t total_requests_{0};
        size_t successful_requests_{0};
        size_t failed_requests_{0};
        std::chrono::milliseconds total_duration_{0};
        size_t total_bytes_transferred_{0};
        double average_throughput_mbps_{0.0};
    } stats_;
    
    mutable std::mutex stats_mutex_;

public:
    HTTP2Client(const HTTP2PoolConfig& config = HTTP2PoolConfig{});
    ~HTTP2Client();
    
    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // HTTP/2下载操作
    std::future<bool> download_async(const std::string& url, 
                                   const std::string& local_path,
                                   std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    std::future<std::vector<char>> download_data_async(const std::string& url,
                                                      std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    // 批量下载
    std::vector<std::future<bool>> download_multiple_async(
        const std::vector<std::string>& urls,
        const std::vector<std::string>& local_paths,
        std::function<void(size_t, size_t)> progress_callback = nullptr);
    
    // 连接池管理
    std::unique_ptr<HTTP2Connection> get_connection(const std::string& url);
    void return_connection(std::unique_ptr<HTTP2Connection> connection);
    void cleanup_idle_connections();
    
    // 配置管理
    void configure(const HTTP2PoolConfig& config);
    HTTP2PoolConfig get_config() const;
    
    // 统计信息
    NetworkStats get_stats() const;
    size_t get_active_connections() const;
    size_t get_total_connections() const;
    size_t get_http2_connections() const;
    
    // 性能优化
    void enable_http2(bool enable);
    void enable_compression(bool enable);
    void enable_pipelining(bool enable);
    
private:
    // 内部方法
    std::string extract_host(const std::string& url) const;
    std::string extract_scheme(const std::string& url) const;
    std::unique_ptr<HTTP2Connection> create_connection(const std::string& url);
    bool setup_http2_options(CURL* curl, const std::string& url);
    bool setup_connection_options(CURL* curl);
    
    // 连接池管理
    void add_connection_to_pool(std::unique_ptr<HTTP2Connection> connection);
    std::unique_ptr<HTTP2Connection> get_connection_from_pool(const std::string& host);
    void cleanup_expired_connections();
    
    // 统计更新
    void update_stats(bool success, size_t bytes_transferred, std::chrono::milliseconds duration);
    void calculate_throughput();
};

// HTTP/2连接池管理器
class HTTP2ConnectionPool {
private:
    std::unique_ptr<HTTP2Client> client_;
    std::thread cleanup_thread_;
    std::atomic<bool> running_{false};
    std::chrono::seconds cleanup_interval_{60};
    
public:
    HTTP2ConnectionPool(const HTTP2PoolConfig& config = HTTP2PoolConfig{});
    ~HTTP2ConnectionPool();
    
    bool initialize();
    void shutdown();
    
    HTTP2Client* get_client();
    void set_cleanup_interval(std::chrono::seconds interval);
    
private:
    void cleanup_loop();
};

} // namespace Paker
