#include "Paker/network/cdn_manager.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <algorithm>
#include <random>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <chrono>

namespace Paker {

CDNManager::CDNManager(const CDNManagerConfig& config) : config_(config) {
    LOG(INFO) << "CDNManager created with strategy: " << static_cast<int>(config_.strategy_);
}

CDNManager::~CDNManager() {
    shutdown();
}

bool CDNManager::initialize() {
    if (health_check_running_) {
        LOG(WARNING) << "CDNManager already initialized";
        return true;
    }
    
    // 启动健康检查线程
    health_check_running_ = true;
    health_check_thread_ = std::thread(&CDNManager::health_check_loop, this);
    
    LOG(INFO) << "CDNManager initialized with " << cdn_nodes_.size() << " nodes";
    return true;
}

void CDNManager::shutdown() {
    if (!health_check_running_) {
        return;
    }
    
    health_check_running_ = false;
    if (health_check_thread_.joinable()) {
        health_check_thread_.join();
    }
    
    LOG(INFO) << "CDNManager shutdown completed";
}

bool CDNManager::add_cdn_node(const std::string& name, const std::string& base_url, 
                             const std::string& region, double priority) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // 检查是否已存在
    for (const auto& node : cdn_nodes_) {
        if (node->name_ == name) {
            LOG(WARNING) << "CDN node already exists: " << name;
            return false;
        }
    }
    
    auto node = std::make_unique<CDNNode>(name, base_url, region);
    node->priority_ = priority;
    cdn_nodes_.push_back(std::move(node));
    
    LOG(INFO) << "Added CDN node: " << name << " (" << base_url << ")";
    return true;
}

bool CDNManager::remove_cdn_node(const std::string& name) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = std::find_if(cdn_nodes_.begin(), cdn_nodes_.end(),
                          [&name](const std::unique_ptr<CDNNode>& node) {
                              return node->name_ == name;
                          });
    
    if (it != cdn_nodes_.end()) {
        cdn_nodes_.erase(it);
        LOG(INFO) << "Removed CDN node: " << name;
        return true;
    }
    
    LOG(WARNING) << "CDN node not found: " << name;
    return false;
}

bool CDNManager::update_cdn_node(const std::string& name, const std::string& base_url, 
                                double priority) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = std::find_if(cdn_nodes_.begin(), cdn_nodes_.end(),
                          [&name](const std::unique_ptr<CDNNode>& node) {
                              return node->name_ == name;
                          });
    
    if (it != cdn_nodes_.end()) {
        (*it)->base_url_ = base_url;
        (*it)->priority_ = priority;
        LOG(INFO) << "Updated CDN node: " << name;
        return true;
    }
    
    LOG(WARNING) << "CDN node not found for update: " << name;
    return false;
}

std::future<bool> CDNManager::download_file(const std::string& file_path, 
                                          const std::string& local_path,
                                          std::function<void(size_t, size_t)> progress_callback) {
    return std::async(std::launch::async, [this, file_path, local_path, progress_callback]() -> bool {
        try {
            // 选择最佳CDN节点
            auto* best_node = select_best_cdn(file_path);
            if (!best_node) {
                LOG(ERROR) << "No available CDN nodes for download: " << file_path;
                return false;
            }
            
            // 构建完整URL
            std::string full_url = build_full_url(best_node, file_path);
            
            // 创建HTTP/2客户端
            HTTP2PoolConfig http_config;
            http_config.max_connections_ = 1;
            HTTP2Client client(http_config);
            if (!client.initialize()) {
                LOG(ERROR) << "Failed to initialize HTTP2 client";
                return false;
            }
            
            // 执行下载
            auto download_future = client.download_async(full_url, local_path, progress_callback);
            bool success = download_future.get();
            
            // 更新节点性能统计
            update_node_performance(best_node->name_, success, 0.0, 0);
            
            if (success) {
                LOG(INFO) << "Download completed: " << file_path << " from " << best_node->name_;
            } else {
                LOG(ERROR) << "Download failed: " << file_path << " from " << best_node->name_;
                
                // 尝试故障转移
                if (config_.enable_failover_) {
                    auto alternatives = select_cdn_alternatives(file_path, 3);
                    return try_failover_download(file_path, local_path, progress_callback, alternatives);
                }
            }
            
            return success;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during CDN download: " << e.what();
            return false;
        }
    });
}

std::future<std::vector<char>> CDNManager::download_data(const std::string& file_path,
                                                        std::function<void(size_t, size_t)> progress_callback) {
    return std::async(std::launch::async, [this, file_path, progress_callback]() -> std::vector<char> {
        try {
            // 选择最佳CDN节点
            auto* best_node = select_best_cdn(file_path);
            if (!best_node) {
                LOG(ERROR) << "No available CDN nodes for download: " << file_path;
                return {};
            }
            
            // 构建完整URL
            std::string full_url = build_full_url(best_node, file_path);
            
            // 创建HTTP/2客户端
            HTTP2PoolConfig http_config;
            http_config.max_connections_ = 1;
            HTTP2Client client(http_config);
            if (!client.initialize()) {
                LOG(ERROR) << "Failed to initialize HTTP2 client";
                return {};
            }
            
            // 执行下载
            auto download_future = client.download_data_async(full_url, progress_callback);
            auto data = download_future.get();
            
            // 更新节点性能统计
            update_node_performance(best_node->name_, !data.empty(), 0.0, data.size());
            
            if (!data.empty()) {
                LOG(INFO) << "Data download completed: " << file_path << " from " << best_node->name_;
            } else {
                LOG(ERROR) << "Data download failed: " << file_path << " from " << best_node->name_;
            }
            
            return data;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception during CDN data download: " << e.what();
            return {};
        }
    });
}

std::vector<std::future<bool>> CDNManager::download_multiple_files(
    const std::vector<std::string>& file_paths,
    const std::vector<std::string>& local_paths,
    std::function<void(size_t, size_t)> progress_callback) {
    
    std::vector<std::future<bool>> futures;
    futures.reserve(file_paths.size());
    
    for (size_t i = 0; i < file_paths.size(); ++i) {
        std::string local_path = (i < local_paths.size()) ? local_paths[i] : "";
        futures.push_back(download_file(file_paths[i], local_path, progress_callback));
    }
    
    return futures;
}

CDNNode* CDNManager::select_best_cdn(const std::string& file_path) {
    (void)file_path; // 暂时未使用，保留参数以备将来使用
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    if (cdn_nodes_.empty()) {
        return nullptr;
    }
    
    // 过滤活跃节点
    std::vector<CDNNode*> active_nodes;
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_) {
            active_nodes.push_back(node.get());
        }
    }
    
    if (active_nodes.empty()) {
        return nullptr;
    }
    
    // 根据策略选择节点
    switch (config_.strategy_) {
        case CDNSelectionStrategy::ROUND_ROBIN:
            return select_node_round_robin();
        case CDNSelectionStrategy::PRIORITY_BASED:
            return select_node_priority_based();
        case CDNSelectionStrategy::LATENCY_BASED:
            return select_node_latency_based();
        case CDNSelectionStrategy::BANDWIDTH_BASED:
            return select_node_bandwidth_based();
        case CDNSelectionStrategy::SUCCESS_RATE_BASED:
            return select_node_success_rate_based();
        case CDNSelectionStrategy::ADAPTIVE:
            return select_node_adaptive();
        default:
            return active_nodes[0];
    }
}

std::vector<CDNNode*> CDNManager::select_cdn_alternatives(const std::string& file_path, size_t count) {
    (void)file_path; // 暂时未使用，保留参数以备将来使用
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<CDNNode*> active_nodes;
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_) {
            active_nodes.push_back(node.get());
        }
    }
    
    if (active_nodes.empty()) {
        return {};
    }
    
    // 按性能评分排序
    std::sort(active_nodes.begin(), active_nodes.end(),
              [this](const CDNNode* a, const CDNNode* b) {
                  return calculate_node_score(a) > calculate_node_score(b);
              });
    
    // 返回前N个节点
    count = std::min(count, active_nodes.size());
    return std::vector<CDNNode*>(active_nodes.begin(), active_nodes.begin() + count);
}

void CDNManager::configure(const CDNManagerConfig& config) {
    config_ = config;
    LOG(INFO) << "CDNManager reconfigured";
}

CDNManagerConfig CDNManager::get_config() const {
    return config_;
}

std::vector<CDNNode*> CDNManager::get_active_nodes() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(nodes_mutex_));
    
    std::vector<CDNNode*> active_nodes;
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_) {
            active_nodes.push_back(node.get());
        }
    }
    
    return active_nodes;
}

std::vector<CDNNode*> CDNManager::get_nodes_by_region(const std::string& region) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(nodes_mutex_));
    
    std::vector<CDNNode*> region_nodes;
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_ && node->region_ == region) {
            region_nodes.push_back(node.get());
        }
    }
    
    return region_nodes;
}

CDNNode* CDNManager::get_node_by_name(const std::string& name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(nodes_mutex_));
    
    for (const auto& node : cdn_nodes_) {
        if (node->name_ == name) {
            return node.get();
        }
    }
    
    return nullptr;
}

void CDNManager::update_node_performance(const std::string& node_name, bool success, 
                                        double latency_ms, size_t bytes_transferred) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    for (const auto& node : cdn_nodes_) {
        if (node->name_ == node_name) {
            update_node_statistics(node.get(), success, latency_ms, bytes_transferred);
            break;
        }
    }
    
    // 更新全局统计
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_downloads_++;
        if (success) {
            stats_.successful_downloads_++;
        } else {
            stats_.failed_downloads_++;
        }
    }
}

void CDNManager::perform_health_check() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    for (const auto& node : cdn_nodes_) {
        bool is_healthy = check_node_health(node.get());
        update_node_health(node.get(), is_healthy);
    }
}

CDNManager::CDNStats CDNManager::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

std::vector<std::pair<std::string, double>> CDNManager::get_node_performance_ranking() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(nodes_mutex_));
    
    std::vector<std::pair<std::string, double>> ranking;
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_) {
            double score = calculate_node_score(node.get());
            ranking.emplace_back(node->name_, score);
        }
    }
    
    std::sort(ranking.begin(), ranking.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    return ranking;
}

void CDNManager::set_selection_strategy(CDNSelectionStrategy strategy) {
    config_.strategy_ = strategy;
    LOG(INFO) << "CDN selection strategy changed to: " << static_cast<int>(strategy);
}

CDNSelectionStrategy CDNManager::get_selection_strategy() const {
    return config_.strategy_;
}

// 私有方法实现
std::string CDNManager::build_full_url(const CDNNode* node, const std::string& file_path) const {
    std::string url = node->base_url_;
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }
    if (!file_path.empty() && file_path.front() == '/') {
        url += file_path.substr(1);
    } else {
        url += file_path;
    }
    return url;
}

CDNNode* CDNManager::select_node_round_robin() {
    if (cdn_nodes_.empty()) return nullptr;
    
    size_t index = round_robin_index_.fetch_add(1) % cdn_nodes_.size();
    return cdn_nodes_[index].get();
}

CDNNode* CDNManager::select_node_priority_based() {
    CDNNode* best_node = nullptr;
    double best_priority = -1.0;
    
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_ && node->priority_ > best_priority) {
            best_priority = node->priority_;
            best_node = node.get();
        }
    }
    
    return best_node;
}

CDNNode* CDNManager::select_node_latency_based() {
    CDNNode* best_node = nullptr;
    double best_latency = std::numeric_limits<double>::max();
    
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_ && node->latency_ms_ < best_latency) {
            best_latency = node->latency_ms_;
            best_node = node.get();
        }
    }
    
    return best_node;
}

CDNNode* CDNManager::select_node_bandwidth_based() {
    CDNNode* best_node = nullptr;
    double best_bandwidth = 0.0;
    
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_ && node->bandwidth_mbps_ > best_bandwidth) {
            best_bandwidth = node->bandwidth_mbps_;
            best_node = node.get();
        }
    }
    
    return best_node;
}

CDNNode* CDNManager::select_node_success_rate_based() {
    CDNNode* best_node = nullptr;
    double best_success_rate = 0.0;
    
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_ && node->success_rate_ > best_success_rate) {
            best_success_rate = node->success_rate_;
            best_node = node.get();
        }
    }
    
    return best_node;
}

CDNNode* CDNManager::select_node_adaptive() {
    CDNNode* best_node = nullptr;
    double best_score = -1.0;
    
    for (const auto& node : cdn_nodes_) {
        if (node->is_active_) {
            double score = calculate_node_score(node.get());
            if (score > best_score) {
                best_score = score;
                best_node = node.get();
            }
        }
    }
    
    return best_node;
}

void CDNManager::health_check_loop() {
    while (health_check_running_) {
        std::this_thread::sleep_for(config_.health_check_interval_);
        if (health_check_running_) {
            perform_health_check();
        }
    }
}

bool CDNManager::check_node_health(CDNNode* node) {
    if (!node) {
        LOG(ERROR) << "Invalid CDN node for health check";
        return false;
    }
    
    try {
        VLOG(1) << "Checking health of CDN node: " << node->url;
        
        // 1. 基本连接测试
        if (!test_basic_connectivity(node->url)) {
            LOG(WARNING) << "Basic connectivity test failed for node: " << node->url;
            return false;
        }
        
        // 2. HTTP健康检查端点测试
        if (!test_health_endpoint(node)) {
            LOG(WARNING) << "Health endpoint test failed for node: " << node->url;
            return false;
        }
        
        // 3. 响应时间测试
        double response_time = measure_response_time(node->url);
        if (response_time > 5000.0) { // 5秒超时
            LOG(WARNING) << "Response time too high for node " << node->url 
                        << ": " << response_time << "ms (max: 5000ms)";
            return false;
        }
        
        // 4. 可用性测试
        if (!test_availability(node)) {
            LOG(WARNING) << "Availability test failed for node: " << node->url;
            return false;
        }
        
        VLOG(1) << "Health check passed for node: " << node->url;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception during health check for node " << node->url << ": " << e.what();
        return false;
    } catch (...) {
        LOG(ERROR) << "Unknown exception during health check for node: " << node->url;
        return false;
    }
}

void CDNManager::update_node_health(CDNNode* node, bool is_healthy) {
    node->is_active_ = is_healthy;
    if (!is_healthy) {
        LOG(WARNING) << "CDN node marked as unhealthy: " << node->name_;
    }
}

double CDNManager::calculate_node_score(const CDNNode* node) const {
    if (!node->is_active_) {
        return 0.0;
    }
    
    // 综合评分：优先级 + 成功率 + 带宽 - 延迟
    double score = node->priority_ * 0.3 +
                   node->success_rate_ * 0.3 +
                   (node->bandwidth_mbps_ / 100.0) * 0.2 +
                   (1.0 / (1.0 + node->latency_ms_ / 1000.0)) * 0.2;
    
    return std::max(0.0, std::min(1.0, score));
}

void CDNManager::update_node_statistics(CDNNode* node, bool success, double latency_ms, size_t bytes_transferred) {
    if (!node) {
        LOG(ERROR) << "Invalid CDN node for statistics update";
        return;
    }
    
    std::lock_guard<std::mutex> lock(node->stats_mutex_);
    
    // 更新请求计数
    node->total_requests_++;
    if (success) {
        node->successful_requests_++;
    } else {
        node->failed_requests_++;
    }
    
    // 计算成功率
    node->success_rate_ = static_cast<double>(node->successful_requests_) / node->total_requests_;
    
    // 更新延迟统计
    if (latency_ms > 0) {
        // 使用指数移动平均更新延迟
        double alpha = 0.1;
        if (node->latency_ms_ == 0) {
            node->latency_ms_ = latency_ms; // 第一次测量
        } else {
            node->latency_ms_ = alpha * latency_ms + (1.0 - alpha) * node->latency_ms_;
        }
        
        // 更新延迟统计
        update_latency_statistics(node, latency_ms);
    }
    
    // 更新带宽统计
    if (bytes_transferred > 0) {
        update_bandwidth_statistics(node, bytes_transferred, latency_ms);
    }
    
    // 更新负载统计
    update_load_statistics(node);
    
    // 更新健康状态
    update_health_status(node);
    
    // 更新时间戳
    node->last_used_ = std::chrono::steady_clock::now();
    
    VLOG(1) << "Updated statistics for node " << node->name_ 
            << ": success_rate=" << node->success_rate_ 
            << ", latency=" << node->latency_ms_ << "ms"
            << ", total_requests=" << node->total_requests_;
}

// 健康检查辅助函数
bool CDNManager::test_basic_connectivity(const std::string& url) {
    try {
        // 使用curl进行基本连接测试
        std::string curl_command = "curl -s --connect-timeout 5 --max-time 10 -I " + url + " > /dev/null 2>&1";
        int result = std::system(curl_command.c_str());
        return (result == 0);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing basic connectivity: " << e.what();
        return false;
    }
}

bool CDNManager::test_health_endpoint(CDNNode* node) {
    try {
        // 构建健康检查URL
        std::string health_url = node->url;
        if (!health_url.empty() && health_url.back() != '/') {
            health_url += "/";
        }
        health_url += "health";
        
        // 发送HEAD请求到健康检查端点
        std::string curl_command = "curl -s --connect-timeout 5 --max-time 10 -I " + health_url + " > /dev/null 2>&1";
        int result = std::system(curl_command.c_str());
        
        if (result == 0) {
            VLOG(1) << "Health endpoint check passed for: " << node->url;
            return true;
        } else {
            LOG(WARNING) << "Health endpoint check failed for: " << node->url;
            return false;
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing health endpoint: " << e.what();
        return false;
    }
}

double CDNManager::measure_response_time(const std::string& url) {
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 使用curl测量响应时间
        std::string curl_command = "curl -s --connect-timeout 5 --max-time 10 -o /dev/null -w '%{time_total}' " + url;
        
        FILE* pipe = popen(curl_command.c_str(), "r");
        if (!pipe) return -1;
        
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // 解析响应时间（秒转换为毫秒）
        double time_seconds = std::stod(result);
        return time_seconds * 1000.0; // 转换为毫秒
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error measuring response time: " << e.what();
        return -1;
    }
}

bool CDNManager::test_availability(CDNNode* node) {
    try {
        // 测试节点可用性（检查最近的成功率）
        if (node->total_requests_ < 10) {
            return true; // 新节点，假设可用
        }
        
        // 检查最近的成功率
        double recent_success_rate = calculate_recent_success_rate(node);
        return recent_success_rate >= config_.min_success_rate_;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing availability: " << e.what();
        return false;
    }
}

// 统计更新辅助函数
void CDNManager::update_latency_statistics(CDNNode* node, double latency_ms) {
    // 更新延迟历史
    node->latency_history_.push_back(latency_ms);
    if (node->latency_history_.size() > 100) { // 保持最近100次测量
        node->latency_history_.erase(node->latency_history_.begin());
    }
    
    // 计算延迟统计
    if (!node->latency_history_.empty()) {
        double sum = 0;
        for (double lat : node->latency_history_) {
            sum += lat;
        }
        node->average_latency_ms_ = sum / node->latency_history_.size();
        
        // 计算延迟标准差
        double variance = 0;
        for (double lat : node->latency_history_) {
            variance += (lat - node->average_latency_ms_) * (lat - node->average_latency_ms_);
        }
        node->latency_std_dev_ = std::sqrt(variance / node->latency_history_.size());
    }
}

void CDNManager::update_bandwidth_statistics(CDNNode* node, size_t bytes_transferred, double latency_ms) {
    if (latency_ms > 0) {
        // 计算瞬时带宽 (bytes per second)
        double instantaneous_bandwidth = (bytes_transferred * 1000.0) / latency_ms;
        
        // 更新带宽历史
        node->bandwidth_history_.push_back(instantaneous_bandwidth);
        if (node->bandwidth_history_.size() > 50) { // 保持最近50次测量
            node->bandwidth_history_.erase(node->bandwidth_history_.begin());
        }
        
        // 计算平均带宽
        if (!node->bandwidth_history_.empty()) {
            double sum = 0;
            for (double bw : node->bandwidth_history_) {
                sum += bw;
            }
            node->average_bandwidth_bps_ = sum / node->bandwidth_history_.size();
        }
        
        // 更新总传输字节数
        node->total_bytes_transferred_ += bytes_transferred;
    }
}

void CDNManager::update_load_statistics(CDNNode* node) {
    // 计算当前负载（基于请求频率）
    auto now = std::chrono::steady_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(now - node->last_used_).count();
    
    if (time_since_last > 0) {
        node->current_load_ = static_cast<double>(node->total_requests_) / (time_since_last + 1);
    }
    
    // 更新负载历史
    node->load_history_.push_back(node->current_load_);
    if (node->load_history_.size() > 20) { // 保持最近20次测量
        node->load_history_.erase(node->load_history_.begin());
    }
}

void CDNManager::update_health_status(CDNNode* node) {
    // 基于多个指标更新健康状态
    bool is_healthy = true;
    
    // 检查成功率
    if (node->success_rate_ < config_.min_success_rate_) {
        is_healthy = false;
        LOG(WARNING) << "Node " << node->name_ << " has low success rate: " << node->success_rate_;
    }
    
    // 检查延迟
    if (node->latency_ms_ > 5000.0) { // 5秒超时
        is_healthy = false;
        LOG(WARNING) << "Node " << node->name_ << " has high latency: " << node->latency_ms_ << "ms";
    }
    
    // 检查可用性
    if (!test_availability(node)) {
        is_healthy = false;
        LOG(WARNING) << "Node " << node->name_ << " is not available";
    }
    
    // 更新健康状态
    if (node->is_active_ != is_healthy) {
        node->is_active_ = is_healthy;
        LOG(INFO) << "Node " << node->name_ << " health status changed to: " 
                  << (is_healthy ? "healthy" : "unhealthy");
    }
}

double CDNManager::calculate_recent_success_rate(CDNNode* node) {
    // 计算最近的成功率（基于最近的请求）
    if (node->total_requests_ < 5) {
        return 1.0; // 新节点，假设100%成功率
    }
    
    // 使用指数加权移动平均
    double recent_success_rate = node->success_rate_;
    
    // 这里可以添加更复杂的最近成功率计算逻辑
    return recent_success_rate;
}

void CDNManager::calculate_average_throughput() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_.total_download_time_.count() > 0) {
        double seconds = stats_.total_download_time_.count() / 1000.0;
        stats_.average_throughput_mbps_ = (stats_.total_downloads_ * 1024.0 / (1024.0 * 1024.0)) / seconds;
    }
}

bool CDNManager::try_failover_download(const std::string& file_path, const std::string& local_path,
                                      std::function<void(size_t, size_t)> progress_callback,
                                      const std::vector<CDNNode*>& alternative_nodes) {
    for (auto* node : alternative_nodes) {
        if (!node->is_active_) {
            continue;
        }
        
        try {
            std::string full_url = build_full_url(node, file_path);
            
            HTTP2PoolConfig http_config;
            http_config.max_connections_ = 1;
            HTTP2Client client(http_config);
            if (!client.initialize()) {
                continue;
            }
            
            auto download_future = client.download_async(full_url, local_path, progress_callback);
            bool success = download_future.get();
            
            if (success) {
                LOG(INFO) << "Failover download successful: " << file_path << " from " << node->name_;
                update_node_performance(node->name_, true, 0.0, 0);
                
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    stats_.failover_count_++;
                }
                
                return true;
            }
            
        } catch (const std::exception& e) {
            LOG(WARNING) << "Failover attempt failed for " << node->name_ << ": " << e.what();
        }
    }
    
    LOG(ERROR) << "All failover attempts failed for: " << file_path;
    return false;
}

} // namespace Paker
