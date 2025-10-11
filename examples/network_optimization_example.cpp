#include "Paker/network/http2_client.h"
#include "Paker/network/cdn_manager.h"
#include "Paker/core/output.h"
#include <iostream>
#include <vector>
#include <chrono>

using namespace Paker;

void http2_example() {
    std::cout << "=== HTTP/2 客户端示例 ===" << std::endl;
    
    // 配置HTTP/2连接池
    HTTP2PoolConfig config;
    config.max_connections_ = 5;
    config.max_connections_per_host_ = 3;
    config.enable_http2_ = true;
    config.enable_compression_ = true;
    config.enable_pipelining_ = true;
    
    // 创建HTTP/2客户端
    HTTP2Client client(config);
    if (!client.initialize()) {
        std::cerr << "Failed to initialize HTTP2 client" << std::endl;
        return;
    }
    
    // 下载单个文件
    std::string url = "https://httpbin.org/json";
    std::string local_path = "/tmp/http2_test.json";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto download_future = client.download_async(url, local_path, 
        [](size_t current, size_t total) {
            if (total > 0) {
                double progress = (static_cast<double>(current) / total) * 100.0;
                std::cout << "下载进度: " << progress << "% (" << current << "/" << total << ")" << std::endl;
            }
        });
    
    bool success = download_future.get();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (success) {
        std::cout << "下载成功! 耗时: " << duration.count() << "ms" << std::endl;
    } else {
        std::cout << "下载失败!" << std::endl;
    }
    
    // 获取统计信息
    auto stats = client.get_stats();
    std::cout << "总请求数: " << stats.total_requests_ << std::endl;
    std::cout << "成功请求数: " << stats.successful_requests_ << std::endl;
    std::cout << "平均吞吐量: " << stats.average_throughput_mbps_ << " Mbps" << std::endl;
    
    client.shutdown();
}

void cdn_example() {
    std::cout << "\n=== CDN 管理器示例 ===" << std::endl;
    
    // 配置CDN管理器
    CDNManagerConfig config;
    config.strategy_ = CDNSelectionStrategy::ADAPTIVE;
    config.max_concurrent_downloads_ = 4;
    config.enable_failover_ = true;
    config.enable_load_balancing_ = true;
    
    // 创建CDN管理器
    CDNManager cdn_manager(config);
    if (!cdn_manager.initialize()) {
        std::cerr << "Failed to initialize CDN manager" << std::endl;
        return;
    }
    
    // 添加CDN节点
    cdn_manager.add_cdn_node("cdn1", "https://cdn1.example.com", "us-east", 1.0);
    cdn_manager.add_cdn_node("cdn2", "https://cdn2.example.com", "us-west", 0.9);
    cdn_manager.add_cdn_node("cdn3", "https://cdn3.example.com", "eu-west", 0.8);
    
    // 下载文件
    std::string file_path = "packages/example-package.tar.gz";
    std::string local_path = "/tmp/example-package.tar.gz";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto download_future = cdn_manager.download_file(file_path, local_path,
        [](size_t current, size_t total) {
            if (total > 0) {
                double progress = (static_cast<double>(current) / total) * 100.0;
                std::cout << "CDN下载进度: " << progress << "% (" << current << "/" << total << ")" << std::endl;
            }
        });
    
    bool success = download_future.get();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (success) {
        std::cout << "CDN下载成功! 耗时: " << duration.count() << "ms" << std::endl;
    } else {
        std::cout << "CDN下载失败!" << std::endl;
    }
    
    // 获取CDN统计信息
    auto stats = cdn_manager.get_stats();
    std::cout << "总下载数: " << stats.total_downloads_ << std::endl;
    std::cout << "成功下载数: " << stats.successful_downloads_ << std::endl;
    std::cout << "故障转移次数: " << stats.failover_count_ << std::endl;
    
    // 获取节点性能排名
    auto ranking = cdn_manager.get_node_performance_ranking();
    std::cout << "节点性能排名:" << std::endl;
    for (const auto& [name, score] : ranking) {
        std::cout << "  " << name << ": " << score << std::endl;
    }
    
    cdn_manager.shutdown();
}

void parallel_download_example() {
    std::cout << "\n=== 并行下载示例 ===" << std::endl;
    
    // 配置HTTP/2客户端
    HTTP2PoolConfig config;
    config.max_connections_ = 8;
    config.max_connections_per_host_ = 4;
    config.enable_http2_ = true;
    
    HTTP2Client client(config);
    if (!client.initialize()) {
        std::cerr << "Failed to initialize HTTP2 client" << std::endl;
        return;
    }
    
    // 准备下载URL列表
    std::vector<std::string> urls = {
        "https://httpbin.org/json",
        "https://httpbin.org/xml",
        "https://httpbin.org/html",
        "https://httpbin.org/robots.txt"
    };
    
    std::vector<std::string> local_paths = {
        "/tmp/parallel1.json",
        "/tmp/parallel2.xml", 
        "/tmp/parallel3.html",
        "/tmp/parallel4.txt"
    };
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 并行下载
    auto futures = client.download_multiple_async(urls, local_paths,
        [](size_t current, size_t total) {
            if (total > 0) {
                double progress = (static_cast<double>(current) / total) * 100.0;
                std::cout << "并行下载进度: " << progress << "%" << std::endl;
            }
        });
    
    // 等待所有下载完成
    int success_count = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        bool success = futures[i].get();
        if (success) {
            success_count++;
            std::cout << "文件 " << (i+1) << " 下载成功" << std::endl;
        } else {
            std::cout << "文件 " << (i+1) << " 下载失败" << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "并行下载完成! 成功: " << success_count << "/" << urls.size() 
              << ", 耗时: " << duration.count() << "ms" << std::endl;
    
    client.shutdown();
}

int main() {
    try {
        // HTTP/2示例
        http2_example();
        
        // CDN示例
        cdn_example();
        
        // 并行下载示例
        parallel_download_example();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
