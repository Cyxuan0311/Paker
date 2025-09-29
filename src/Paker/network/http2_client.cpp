#include "Paker/network/http2_client.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Paker {

HTTP2Client::HTTP2Client(const HTTP2PoolConfig& config) : config_(config), multi_handle_(nullptr) {
    LOG(INFO) << "HTTP2Client created with config: max_connections=" << config_.max_connections_
              << ", max_per_host=" << config_.max_connections_per_host_;
}

HTTP2Client::~HTTP2Client() {
    shutdown();
}

bool HTTP2Client::initialize() {
    if (multi_handle_) {
        LOG(WARNING) << "HTTP2Client already initialized";
        return true;
    }
    
    // 初始化CURL多句柄
    multi_handle_ = curl_multi_init();
    if (!multi_handle_) {
        LOG(ERROR) << "Failed to initialize CURL multi handle";
        return false;
    }
    
    // 设置CURL多句柄选项
    curl_multi_setopt(multi_handle_, CURLMOPT_PIPELINING, config_.enable_pipelining_ ? CURLPIPE_MULTIPLEX : 0L);
    curl_multi_setopt(multi_handle_, CURLMOPT_MAX_TOTAL_CONNECTIONS, static_cast<long>(config_.max_connections_));
    curl_multi_setopt(multi_handle_, CURLMOPT_MAX_HOST_CONNECTIONS, static_cast<long>(config_.max_connections_per_host_));
    
    LOG(INFO) << "HTTP2Client initialized successfully";
    return true;
}

void HTTP2Client::shutdown() {
    if (!multi_handle_) {
        return;
    }
    
    // 清理活跃连接
    {
        std::lock_guard<std::mutex> lock(active_mutex_);
        active_connections_.clear();
    }
    
    // 清理连接池
    {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        connection_pools_.clear();
    }
    
    // 清理CURL多句柄
    curl_multi_cleanup(multi_handle_);
    multi_handle_ = nullptr;
    
    LOG(INFO) << "HTTP2Client shutdown completed";
}

std::future<bool> HTTP2Client::download_async(const std::string& url, 
                                             const std::string& local_path,
                                             std::function<void(size_t, size_t)> progress_callback) {
    return std::async(std::launch::async, [this, url, local_path, progress_callback]() -> bool {
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // 获取连接
            auto connection = get_connection(url);
            if (!connection) {
                LOG(ERROR) << "Failed to get connection for URL: " << url;
                return false;
            }
            
            // 设置下载选项
            curl_easy_setopt(connection->curl_handle_, CURLOPT_URL, url.c_str());
            curl_easy_setopt(connection->curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_USERAGENT, "Paker/1.0");
            
            // 启用HTTP/2
            if (config_.enable_http2_) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            }
            
            // 启用压缩
            if (config_.enable_compression_) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_ACCEPT_ENCODING, "");
            }
            
            // 设置进度回调
            if (progress_callback) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(connection->curl_handle_, CURLOPT_PROGRESSFUNCTION, 
                    [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) -> int {
                        auto* callback = static_cast<std::function<void(size_t, size_t)>*>(clientp);
                        if (callback && dltotal > 0) {
                            (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
                        }
                        return 0;
                    });
                curl_easy_setopt(connection->curl_handle_, CURLOPT_PROGRESSDATA, &progress_callback);
            }
            
            // 设置写入回调
            std::ofstream file(local_path, std::ios::binary);
            if (!file.is_open()) {
                LOG(ERROR) << "Failed to open file for writing: " << local_path;
                return_connection(std::move(connection));
                return false;
            }
            
            curl_easy_setopt(connection->curl_handle_, CURLOPT_WRITEFUNCTION, 
                [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
                    std::ofstream* file = static_cast<std::ofstream*>(userp);
                    size_t total_size = size * nmemb;
                    file->write(static_cast<char*>(contents), total_size);
                    return total_size;
                });
            curl_easy_setopt(connection->curl_handle_, CURLOPT_WRITEDATA, &file);
            
            // 执行下载
            CURLcode res = curl_easy_perform(connection->curl_handle_);
            file.close();
            
            // 获取HTTP状态码
            long http_code = 0;
            curl_easy_getinfo(connection->curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
            
            // 获取传输统计
            double content_length = 0;
            curl_easy_getinfo(connection->curl_handle_, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // 更新统计
            bool success = (res == CURLE_OK && http_code >= 200 && http_code < 300);
            update_stats(success, static_cast<size_t>(content_length), duration);
            
            // 返回连接
            return_connection(std::move(connection));
            
            if (!success) {
                LOG(ERROR) << "Download failed: " << curl_easy_strerror(res) << ", HTTP: " << http_code;
                return false;
            }
            
            LOG(INFO) << "Download completed: " << url << " -> " << local_path 
                      << " (" << content_length << " bytes in " << duration.count() << "ms)";
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during download: " << e.what();
            return false;
        }
    });
}

std::future<std::vector<char>> HTTP2Client::download_data_async(const std::string& url,
                                                              std::function<void(size_t, size_t)> progress_callback) {
    return std::async(std::launch::async, [this, url, progress_callback]() -> std::vector<char> {
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // 获取连接
            auto connection = get_connection(url);
            if (!connection) {
                LOG(ERROR) << "Failed to get connection for URL: " << url;
                return {};
            }
            
            // 设置下载选项
            curl_easy_setopt(connection->curl_handle_, CURLOPT_URL, url.c_str());
            curl_easy_setopt(connection->curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(connection->curl_handle_, CURLOPT_USERAGENT, "Paker/1.0");
            
            // 启用HTTP/2
            if (config_.enable_http2_) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            }
            
            // 启用压缩
            if (config_.enable_compression_) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_ACCEPT_ENCODING, "");
            }
            
            // 设置进度回调
            if (progress_callback) {
                curl_easy_setopt(connection->curl_handle_, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(connection->curl_handle_, CURLOPT_PROGRESSFUNCTION, 
                    [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) -> int {
                        auto* callback = static_cast<std::function<void(size_t, size_t)>*>(clientp);
                        if (callback && dltotal > 0) {
                            (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
                        }
                        return 0;
                    });
                curl_easy_setopt(connection->curl_handle_, CURLOPT_PROGRESSDATA, &progress_callback);
            }
            
            // 设置写入回调
            std::vector<char> data;
            data.reserve(1024 * 1024); // 预分配1MB
            
            curl_easy_setopt(connection->curl_handle_, CURLOPT_WRITEFUNCTION, 
                [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
                    std::vector<char>* data = static_cast<std::vector<char>*>(userp);
                    size_t total_size = size * nmemb;
                    data->insert(data->end(), static_cast<char*>(contents), 
                               static_cast<char*>(contents) + total_size);
                    return total_size;
                });
            curl_easy_setopt(connection->curl_handle_, CURLOPT_WRITEDATA, &data);
            
            // 执行下载
            CURLcode res = curl_easy_perform(connection->curl_handle_);
            
            // 获取HTTP状态码
            long http_code = 0;
            curl_easy_getinfo(connection->curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // 更新统计
            bool success = (res == CURLE_OK && http_code >= 200 && http_code < 300);
            update_stats(success, data.size(), duration);
            
            // 返回连接
            return_connection(std::move(connection));
            
            if (!success) {
                LOG(ERROR) << "Download failed: " << curl_easy_strerror(res) << ", HTTP: " << http_code;
                return {};
            }
            
            LOG(INFO) << "Data download completed: " << url << " (" << data.size() 
                      << " bytes in " << duration.count() << "ms)";
            return data;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during data download: " << e.what();
            return {};
        }
    });
}

std::vector<std::future<bool>> HTTP2Client::download_multiple_async(
    const std::vector<std::string>& urls,
    const std::vector<std::string>& local_paths,
    std::function<void(size_t, size_t)> progress_callback) {
    
    std::vector<std::future<bool>> futures;
    futures.reserve(urls.size());
    
    for (size_t i = 0; i < urls.size(); ++i) {
        if (i < local_paths.size()) {
            futures.push_back(download_async(urls[i], local_paths[i], progress_callback));
        } else {
            // 如果没有对应的本地路径，下载到内存
            auto data_future = download_data_async(urls[i], progress_callback);
            futures.push_back(std::async(std::launch::async, [data_future = std::move(data_future)]() mutable -> bool {
                auto data = data_future.get();
                return !data.empty();
            }));
        }
    }
    
    return futures;
}

std::unique_ptr<HTTP2Connection> HTTP2Client::get_connection(const std::string& url) {
    std::string host = extract_host(url);
    
    // 尝试从连接池获取
    auto connection = get_connection_from_pool(host);
    if (connection) {
        return connection;
    }
    
    // 创建新连接
    return create_connection(url);
}

void HTTP2Client::return_connection(std::unique_ptr<HTTP2Connection> connection) {
    if (!connection) {
        return;
    }
    
    // 重置连接状态
    connection->last_used_ = std::chrono::steady_clock::now();
    connection->is_active_ = false;
    
    // 添加到连接池
    add_connection_to_pool(std::move(connection));
}

void HTTP2Client::cleanup_idle_connections() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    for (auto& [host, pool] : connection_pools_) {
        while (!pool.empty()) {
            auto& connection = pool.front();
            if (now - connection->last_used_ > config_.idle_timeout_) {
                pool.pop();
                total_connections_--;
            } else {
                break;
            }
        }
    }
}

void HTTP2Client::configure(const HTTP2PoolConfig& config) {
    config_ = config;
    LOG(INFO) << "HTTP2Client reconfigured";
}

HTTP2PoolConfig HTTP2Client::get_config() const {
    return config_;
}

HTTP2Client::NetworkStats HTTP2Client::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

size_t HTTP2Client::get_active_connections() const {
    return active_connections_count_.load();
}

size_t HTTP2Client::get_total_connections() const {
    return total_connections_.load();
}

size_t HTTP2Client::get_http2_connections() const {
    return http2_connections_.load();
}

void HTTP2Client::enable_http2(bool enable) {
    config_.enable_http2_ = enable;
    LOG(INFO) << "HTTP/2 " << (enable ? "enabled" : "disabled");
}

void HTTP2Client::enable_compression(bool enable) {
    config_.enable_compression_ = enable;
    LOG(INFO) << "Compression " << (enable ? "enabled" : "disabled");
}

void HTTP2Client::enable_pipelining(bool enable) {
    config_.enable_pipelining_ = enable;
    LOG(INFO) << "Pipelining " << (enable ? "enabled" : "disabled");
}

// 私有方法实现
std::string HTTP2Client::extract_host(const std::string& url) const {
    // 简单的URL解析，提取主机名
    size_t start = url.find("://");
    if (start == std::string::npos) return "";
    
    start += 3;
    size_t end = url.find('/', start);
    if (end == std::string::npos) end = url.length();
    
    return url.substr(start, end - start);
}

std::string HTTP2Client::extract_scheme(const std::string& url) const {
    size_t end = url.find("://");
    if (end == std::string::npos) return "http";
    return url.substr(0, end);
}

std::unique_ptr<HTTP2Connection> HTTP2Client::create_connection(const std::string& url) {
    auto connection = std::make_unique<HTTP2Connection>();
    
    connection->curl_handle_ = curl_easy_init();
    if (!connection->curl_handle_) {
        LOG(ERROR) << "Failed to create CURL handle";
        return nullptr;
    }
    
    connection->host_ = extract_host(url);
    connection->scheme_ = extract_scheme(url);
    connection->last_used_ = std::chrono::steady_clock::now();
    connection->is_active_ = true;
    
    // 设置HTTP/2选项
    if (config_.enable_http2_) {
        curl_easy_setopt(connection->curl_handle_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        connection->is_http2_ = true;
        http2_connections_++;
    }
    
    // 设置连接选项
    setup_connection_options(connection->curl_handle_);
    
    total_connections_++;
    active_connections_count_++;
    
    LOG(INFO) << "Created new connection for " << connection->host_ 
              << " (HTTP/2: " << (connection->is_http2_ ? "yes" : "no") << ")";
    
    return connection;
}

bool HTTP2Client::setup_http2_options(CURL* curl, const std::string& url) {
    if (!config_.enable_http2_) {
        return true;
    }
    
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
    
    return true;
}

bool HTTP2Client::setup_connection_options(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(config_.connection_timeout_.count()));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Paker/1.0");
    
    if (config_.enable_compression_) {
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    }
    
    return true;
}

void HTTP2Client::add_connection_to_pool(std::unique_ptr<HTTP2Connection> connection) {
    if (!connection) return;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    connection_pools_[connection->host_].push(std::move(connection));
}

std::unique_ptr<HTTP2Connection> HTTP2Client::get_connection_from_pool(const std::string& host) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto it = connection_pools_.find(host);
    if (it != connection_pools_.end() && !it->second.empty()) {
        auto connection = std::move(const_cast<std::unique_ptr<HTTP2Connection>&>(it->second.front()));
        it->second.pop();
        return connection;
    }
    
    return nullptr;
}

void HTTP2Client::update_stats(bool success, size_t bytes_transferred, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_requests_++;
    if (success) {
        stats_.successful_requests_++;
    } else {
        stats_.failed_requests_++;
    }
    
    stats_.total_bytes_transferred_ += bytes_transferred;
    stats_.total_duration_ += duration;
    
    // 计算平均吞吐量
    if (stats_.total_duration_.count() > 0) {
        double seconds = stats_.total_duration_.count() / 1000.0;
        stats_.average_throughput_mbps_ = (stats_.total_bytes_transferred_ / (1024.0 * 1024.0)) / seconds;
    }
}

void HTTP2Client::calculate_throughput() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_.total_duration_.count() > 0) {
        double seconds = stats_.total_duration_.count() / 1000.0;
        stats_.average_throughput_mbps_ = (stats_.total_bytes_transferred_ / (1024.0 * 1024.0)) / seconds;
    }
}

// HTTP2ConnectionPool 实现
HTTP2ConnectionPool::HTTP2ConnectionPool(const HTTP2PoolConfig& config) 
    : client_(std::make_unique<HTTP2Client>(config)) {
}

HTTP2ConnectionPool::~HTTP2ConnectionPool() {
    shutdown();
}

bool HTTP2ConnectionPool::initialize() {
    if (!client_->initialize()) {
        return false;
    }
    
    running_ = true;
    cleanup_thread_ = std::thread(&HTTP2ConnectionPool::cleanup_loop, this);
    
    LOG(INFO) << "HTTP2ConnectionPool initialized";
    return true;
}

void HTTP2ConnectionPool::shutdown() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    client_->shutdown();
    LOG(INFO) << "HTTP2ConnectionPool shutdown";
}

HTTP2Client* HTTP2ConnectionPool::get_client() {
    return client_.get();
}

void HTTP2ConnectionPool::set_cleanup_interval(std::chrono::seconds interval) {
    cleanup_interval_ = interval;
}

void HTTP2ConnectionPool::cleanup_loop() {
    while (running_) {
        std::this_thread::sleep_for(cleanup_interval_);
        if (running_) {
            client_->cleanup_idle_connections();
        }
    }
}

} // namespace Paker
