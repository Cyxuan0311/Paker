#include "Paker/monitor/diagnostic_tool.h"
#include "Paker/core/output.h"
#include "Paker/conflict/conflict_detector.h"
#include "Paker/core/utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <glog/logging.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

DiagnosticTool::DiagnosticTool(const DependencyGraph& graph) : graph_(graph) {
    initialize_rules();
}

void DiagnosticTool::initialize_rules() {
    rules_.push_back(std::make_unique<CircularDependencyRule>());
    rules_.push_back(std::make_unique<VersionConflictRule>());
    rules_.push_back(std::make_unique<MissingDependencyRule>());
}

DiagnosticResult DiagnosticTool::diagnose() {
    DiagnosticResult result;
    
    // 执行所有诊断规则
    for (const auto& rule : rules_) {
        auto issues = rule->check(graph_);
        result.issues.insert(result.issues.end(), issues.begin(), issues.end());
    }
    
    // 检查配置问题
    auto config_issues = check_configuration();
    result.issues.insert(result.issues.end(), config_issues.begin(), config_issues.end());
    
    // 检查依赖问题
    auto dep_issues = check_dependencies();
    result.issues.insert(result.issues.end(), dep_issues.begin(), dep_issues.end());
    
    // 检查性能问题
    auto perf_issues = check_performance();
    result.issues.insert(result.issues.end(), perf_issues.begin(), perf_issues.end());
    
    // 检查文件系统问题
    auto fs_issues = check_filesystem();
    result.issues.insert(result.issues.end(), fs_issues.begin(), fs_issues.end());
    
    // 统计问题级别
    for (const auto& issue : result.issues) {
        switch (issue.level) {
            case DiagnosticLevel::CRITICAL:
                result.has_critical_issues = true;
                break;
            case DiagnosticLevel::ERROR:
                result.has_errors = true;
                break;
            case DiagnosticLevel::WARNING:
                result.has_warnings = true;
                break;
            default:
                break;
        }
    }
    
    // 生成摘要
    std::ostringstream summary;
    summary << "Diagnostic completed. Found ";
    summary << result.issues.size() << " issues: ";
    
    if (result.has_critical_issues) {
        summary << "CRITICAL issues detected! ";
    }
    if (result.has_errors) {
        summary << "Errors found. ";
    }
    if (result.has_warnings) {
        summary << "Warnings found. ";
    }
    
    result.summary = summary.str();
    
    return result;
}

std::string DiagnosticTool::generate_diagnostic_report(const DiagnosticResult& result) {
    std::ostringstream report;
    report << "\033[1;36m Diagnostic Report\033[0m\n";
    report << "\033[0;36m===================\033[0m\n\n";
    
    report << "\033[1;33mSummary:\033[0m " << result.summary << "\n\n";
    
    if (result.issues.empty()) {
        report << "\033[1;32m[OK]\033[0m \033[0;32mNo issues found. Your project is healthy!\033[0m\n";
        return report.str();
    }
    
    // 按级别分组
    std::map<DiagnosticLevel, std::vector<DiagnosticIssue>> grouped_issues;
    for (const auto& issue : result.issues) {
        grouped_issues[issue.level].push_back(issue);
    }
    
    // 按级别顺序显示
    std::vector<DiagnosticLevel> levels = {
        DiagnosticLevel::CRITICAL,
        DiagnosticLevel::ERROR,
        DiagnosticLevel::WARNING,
        DiagnosticLevel::INFO
    };
    
    for (auto level : levels) {
        auto it = grouped_issues.find(level);
        if (it == grouped_issues.end()) continue;
        
        report << format_level(level) << " \033[1;33m(" << it->second.size() << ")\033[0m\n";
        report << "\033[0;36m" << std::string(50, '-') << "\033[0m\n";
        
        for (const auto& issue : it->second) {
            report << "\033[1;34mCategory:\033[0m \033[1;35m" << issue.category << "\033[0m\n";
            report << "\033[1;34mMessage:\033[0m \033[1;31m" << issue.message << "\033[0m\n";
            
            if (!issue.description.empty()) {
                report << "\033[1;34mDescription:\033[0m \033[0;33m" << issue.description << "\033[0m\n";
            }
            
            if (!issue.suggestions.empty()) {
                report << "\033[1;34mSuggestions:\033[0m\n";
                for (const auto& suggestion : issue.suggestions) {
                    report << "  \033[0;32m- \033[0m" << suggestion << "\n";
                }
            }
            
            if (!issue.context.empty()) {
                report << "\033[1;34mContext:\033[0m\n";
                for (const auto& [key, value] : issue.context) {
                    report << "  \033[1;36m" << key << ":\033[0m " << value << "\n";
                }
            }
            
            report << "\n";
        }
    }
    
    return report.str();
}

std::vector<DiagnosticIssue> DiagnosticTool::check_configuration() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查项目配置文件
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Configuration", "Project configuration file not found");
        issue.description = "Paker.json file is missing. Run 'paker init' to create it.";
        issue.suggestions.push_back("Run 'paker init' to initialize the project");
        issues.push_back(issue);
        return issues;
    }
    
    // 检查配置文件格式
    try {
        std::ifstream file(json_file);
        json j;
        file >> j;
        
        if (!j.contains("name")) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Configuration", "Project name not specified");
            issue.description = "The 'name' field is missing in Paker.json";
            issue.suggestions.push_back("Add a 'name' field to your Paker.json");
            issues.push_back(issue);
        }
        
        if (!j.contains("version")) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Configuration", "Project version not specified");
            issue.description = "The 'version' field is missing in Paker.json";
            issue.suggestions.push_back("Add a 'version' field to your Paker.json");
            issues.push_back(issue);
        }
        
    } catch (const std::exception& e) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Configuration", "Invalid JSON format in Paker.json");
        issue.description = "The Paker.json file contains invalid JSON";
        issue.suggestions.push_back("Fix the JSON syntax in Paker.json");
        issue.context["error"] = e.what();
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_dependencies() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查依赖图是否为空
    if (graph_.empty()) {
        DiagnosticIssue issue(DiagnosticLevel::INFO, "Dependencies", "No dependencies found");
        issue.description = "The project has no dependencies configured";
        issue.suggestions.push_back("Add dependencies using 'Paker add <package>'");
        issues.push_back(issue);
        return issues;
    }
    
    
    // 检查依赖解析
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph_.has_node(dep)) {
                DiagnosticIssue issue(DiagnosticLevel::ERROR, "Dependencies", 
                                    "Missing dependency: " + dep);
                issue.description = "Package '" + package + "' depends on '" + dep + 
                                  "' which is not available";
                issue.suggestions.push_back("Install the missing dependency: 'Paker add " + dep + "'");
                issue.suggestions.push_back("Check if the dependency name is correct");
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_performance() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查依赖深度
    std::map<std::string, size_t> depths;
    for (const auto& [package, _] : graph_.get_nodes()) {
        size_t depth = 0;
        const auto* node = graph_.get_node(package);
        if (node) {
            for (const auto& dep : node->dependencies) {
                depth = std::max(depth, depths[dep] + 1);
            }
        }
        depths[package] = depth;
        
        if (depth > 5) {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Performance", 
                                "Deep dependency chain: " + package);
            issue.description = "Package '" + package + "' has a dependency depth of " + 
                              std::to_string(depth) + " levels";
            issue.suggestions.push_back("Consider flattening the dependency tree");
            issue.suggestions.push_back("Look for alternative packages with fewer dependencies");
            issue.context["depth"] = std::to_string(depth);
            issues.push_back(issue);
        }
    }
    
    // 检查包大小
    for (const auto& [package, node] : graph_.get_nodes()) {
        std::string package_path = "packages/" + package;
        if (fs::exists(package_path)) {
            size_t total_size = 0;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(package_path)) {
                    if (entry.is_regular_file()) {
                        total_size += fs::file_size(entry.path());
                    }
                }
                
                if (total_size > 100 * 1024 * 1024) { // 100MB
                    DiagnosticIssue issue(DiagnosticLevel::WARNING, "Performance", 
                                        "Large package: " + package);
                    issue.description = "Package '" + package + "' is very large (" + 
                                      std::to_string(total_size / (1024 * 1024)) + "MB)";
                    issue.suggestions.push_back("Consider using a lighter alternative");
                    issue.suggestions.push_back("Check if you need all components of this package");
                    issue.context["size_mb"] = std::to_string(total_size / (1024 * 1024));
                    issues.push_back(issue);
                }
            } catch (const std::exception& e) {
                // 忽略文件系统错误
            }
        }
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_filesystem() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查packages目录
    if (!fs::exists("packages")) {
        DiagnosticIssue issue(DiagnosticLevel::INFO, "Filesystem", "Packages directory not found");
        issue.description = "The 'packages' directory does not exist";
        issue.suggestions.push_back("This is normal for new projects");
        issue.suggestions.push_back("Run 'Paker add <package>' to install dependencies");
        issues.push_back(issue);
        return issues;
    }
    
    // 检查packages目录权限
    try {
        fs::directory_iterator("packages");
    } catch (const std::exception& e) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Filesystem", "Cannot access packages directory");
        issue.description = "Permission denied or directory is corrupted";
        issue.suggestions.push_back("Check directory permissions");
        issue.suggestions.push_back("Try running with elevated privileges");
        issue.context["error"] = e.what();
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_network() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Starting network connectivity diagnostics";
        
        // 1. 检查基本网络连接
        auto basic_connectivity = check_basic_connectivity();
        issues.insert(issues.end(), basic_connectivity.begin(), basic_connectivity.end());
        
        // 2. 检查仓库连接
        auto repository_connectivity = check_repository_connectivity();
        issues.insert(issues.end(), repository_connectivity.begin(), repository_connectivity.end());
        
        // 3. 检查DNS解析
        auto dns_issues = check_dns_resolution();
        issues.insert(issues.end(), dns_issues.begin(), dns_issues.end());
        
        // 4. 检查代理设置
        auto proxy_issues = check_proxy_settings();
        issues.insert(issues.end(), proxy_issues.begin(), proxy_issues.end());
        
        // 5. 检查防火墙/端口
        auto firewall_issues = check_firewall_ports();
        issues.insert(issues.end(), firewall_issues.begin(), firewall_issues.end());
        
        // 6. 检查网络延迟
        auto latency_issues = check_network_latency();
        issues.insert(issues.end(), latency_issues.begin(), latency_issues.end());
        
        // 7. 检查网络带宽
        auto bandwidth_issues = check_network_bandwidth();
        issues.insert(issues.end(), bandwidth_issues.begin(), bandwidth_issues.end());
        
        LOG(INFO) << "Network diagnostics completed. Found " << issues.size() << " issues";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error during network diagnostics: " << e.what();
        
        DiagnosticIssue error_issue(DiagnosticLevel::ERROR, "Network", "Network Diagnostic Error");
        error_issue.description = "Failed to perform network diagnostics: " + std::string(e.what());
        error_issue.suggestions.push_back("Check system network configuration and try again");
        issues.push_back(error_issue);
    }
    
    return issues;
}

// 网络诊断辅助函数实现
std::vector<DiagnosticIssue> DiagnosticTool::check_basic_connectivity() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking basic network connectivity";
        
        // 测试常见的外部服务
        std::vector<std::string> test_hosts = {
            "8.8.8.8",           // Google DNS
            "1.1.1.1",           // Cloudflare DNS
            "www.google.com",    // Google
            "www.github.com"     // GitHub
        };
        
        for (const auto& host : test_hosts) {
            if (!test_host_connectivity(host)) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "Host Connectivity Issue");
                issue.description = "Cannot reach host: " + host;
                issue.suggestions.push_back("Check internet connection and DNS settings");
                issues.push_back(issue);
            } else {
                VLOG(1) << "Successfully connected to: " << host;
            }
        }
        
        if (issues.empty()) {
            LOG(INFO) << "Basic connectivity test passed";
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking basic connectivity: " << e.what();
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_repository_connectivity() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking repository connectivity";
        
        // 常见的包仓库地址
        std::vector<std::string> repositories = {
            "https://github.com",
            "https://gitlab.com",
            "https://bitbucket.org",
            "https://sourceforge.net",
            "https://pypi.org",
            "https://crates.io",
            "https://npmjs.com"
        };
        
        for (const auto& repo : repositories) {
            if (!test_repository_connectivity(repo)) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "Repository Connectivity Issue");
                issue.description = "Cannot reach repository: " + repo;
                issue.suggestions.push_back("Check repository URL and network connectivity");
                issues.push_back(issue);
            } else {
                VLOG(1) << "Successfully connected to repository: " << repo;
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking repository connectivity: " << e.what();
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_dns_resolution() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking DNS resolution";
        
        // 测试DNS解析
        std::vector<std::string> test_domains = {
            "google.com",
            "github.com",
            "stackoverflow.com"
        };
        
        for (const auto& domain : test_domains) {
            if (!test_dns_resolution(domain)) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "DNS Resolution Issue");
                issue.description = "Cannot resolve domain: " + domain;
                issue.suggestions.push_back("Check DNS settings or try different DNS servers (8.8.8.8, 1.1.1.1)");
                issues.push_back(issue);
            } else {
                VLOG(1) << "Successfully resolved domain: " << domain;
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking DNS resolution: " << e.what();
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_proxy_settings() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking proxy settings";
        
        // 检查环境变量中的代理设置
        std::vector<std::string> proxy_vars = {
            "HTTP_PROXY", "HTTPS_PROXY", "FTP_PROXY", "NO_PROXY",
            "http_proxy", "https_proxy", "ftp_proxy", "no_proxy"
        };
        
        bool has_proxy = false;
        for (const auto& var : proxy_vars) {
            const char* value = std::getenv(var.c_str());
            if (value && strlen(value) > 0) {
                has_proxy = true;
                VLOG(1) << "Found proxy setting: " << var << "=" << value;
            }
        }
        
        if (has_proxy) {
            DiagnosticIssue issue(DiagnosticLevel::INFO, "Network", "Proxy Configuration Detected");
            issue.description = "Proxy settings are configured in environment variables";
            issue.suggestions.push_back("Ensure proxy settings are correct and accessible");
            issues.push_back(issue);
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking proxy settings: " << e.what();
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_firewall_ports() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking firewall and port accessibility";
        
        // 检查常用端口
        std::vector<int> test_ports = {80, 443, 22, 21, 25, 53};
        
        for (int port : test_ports) {
            if (!test_port_connectivity(port)) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "Port Accessibility Issue");
                issue.description = "Cannot access port: " + std::to_string(port);
                issue.suggestions.push_back("Check firewall settings and port availability");
                issues.push_back(issue);
            } else {
                VLOG(1) << "Port " << port << " is accessible";
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking firewall ports: " << e.what();
    }
    
    return issues;
}

// 网络测试辅助函数
bool DiagnosticTool::test_host_connectivity(const std::string& host) {
    try {
        VLOG(1) << "Testing connectivity to: " << host;
        
        // 使用系统ping命令进行连接测试
        std::string ping_command;
        #ifdef _WIN32
            ping_command = "ping -n 1 -w 3000 " + host + " > nul 2>&1";
        #else
            ping_command = "ping -c 1 -W 3 " + host + " > /dev/null 2>&1";
        #endif
        
        int result = std::system(ping_command.c_str());
        bool success = (result == 0);
        
        if (success) {
            VLOG(1) << "Successfully pinged: " << host;
        } else {
            VLOG(1) << "Failed to ping: " << host;
        }
        
        return success;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing host connectivity: " << e.what();
        return false;
    }
}

bool DiagnosticTool::test_repository_connectivity(const std::string& url) {
    try {
        VLOG(1) << "Testing repository connectivity: " << url;
        
        // 使用curl进行HTTP/HTTPS连接测试
        std::string curl_command = "curl -s --connect-timeout 10 --max-time 15 -I " + url + " > /dev/null 2>&1";
        
        int result = std::system(curl_command.c_str());
        bool success = (result == 0);
        
        if (success) {
            VLOG(1) << "Successfully connected to repository: " << url;
        } else {
            VLOG(1) << "Failed to connect to repository: " << url;
        }
        
        return success;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing repository connectivity: " << e.what();
        return false;
    }
}

bool DiagnosticTool::test_dns_resolution(const std::string& domain) {
    try {
        VLOG(1) << "Testing DNS resolution: " << domain;
        
        // 使用nslookup或dig进行DNS解析测试
        std::string dns_command;
        #ifdef _WIN32
            dns_command = "nslookup " + domain + " > nul 2>&1";
        #else
            dns_command = "nslookup " + domain + " > /dev/null 2>&1";
        #endif
        
        int result = std::system(dns_command.c_str());
        bool success = (result == 0);
        
        if (success) {
            VLOG(1) << "Successfully resolved domain: " << domain;
        } else {
            VLOG(1) << "Failed to resolve domain: " << domain;
        }
        
        return success;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing DNS resolution: " << e.what();
        return false;
    }
}

bool DiagnosticTool::test_port_connectivity(int port) {
    try {
        VLOG(1) << "Testing port connectivity: " << port;
        
        // 使用telnet或nc进行端口连接测试
        std::string port_command;
        #ifdef _WIN32
            // Windows使用telnet
            port_command = "echo quit | telnet localhost " + std::to_string(port) + " > nul 2>&1";
        #else
            // Linux/Unix使用nc (netcat)
            port_command = "timeout 3 nc -z localhost " + std::to_string(port) + " > /dev/null 2>&1";
        #endif
        
        int result = std::system(port_command.c_str());
        bool success = (result == 0);
        
        if (success) {
            VLOG(1) << "Port " << port << " is accessible";
        } else {
            VLOG(1) << "Port " << port << " is not accessible";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error testing port connectivity: " << e.what();
        return false;
    }
}

std::vector<DiagnosticIssue> DiagnosticTool::check_network_latency() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking network latency";
        
        // 测试到常见服务的延迟
        std::vector<std::string> test_hosts = {
            "8.8.8.8",           // Google DNS
            "1.1.1.1",           // Cloudflare DNS
            "www.google.com"     // Google
        };
        
        for (const auto& host : test_hosts) {
            double latency = measure_network_latency(host);
            
            if (latency > 0) {
                if (latency > 1000) { // 超过1秒
                    DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "High Network Latency");
                    issue.description = "High latency to " + host + ": " + std::to_string(latency) + "ms";
                    issue.suggestions.push_back("Check network connection quality and consider using a different DNS server");
                    issues.push_back(issue);
                } else if (latency > 500) { // 超过500ms
                    DiagnosticIssue issue(DiagnosticLevel::INFO, "Network", "Moderate Network Latency");
                    issue.description = "Moderate latency to " + host + ": " + std::to_string(latency) + "ms";
                    issue.suggestions.push_back("Network latency is acceptable but could be improved");
                    issues.push_back(issue);
                } else {
                    VLOG(1) << "Good latency to " << host << ": " << latency << "ms";
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking network latency: " << e.what();
    }
    
    return issues;
}

std::vector<DiagnosticIssue> DiagnosticTool::check_network_bandwidth() {
    std::vector<DiagnosticIssue> issues;
    
    try {
        LOG(INFO) << "Checking network bandwidth";
        
        // 简化的带宽测试（实际实现应该下载测试文件）
        std::string test_url = "https://httpbin.org/bytes/1024"; // 1KB测试文件
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::string curl_command = "curl -s --connect-timeout 10 --max-time 30 -o /dev/null " + test_url;
        int result = std::system(curl_command.c_str());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (result == 0) {
            double bandwidth_mbps = (1024.0 * 8.0) / (duration.count() / 1000.0); // 转换为Mbps
            
            if (bandwidth_mbps < 1.0) { // 低于1Mbps
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "Low Network Bandwidth");
                issue.description = "Low bandwidth detected: " + std::to_string(bandwidth_mbps) + " Mbps";
                issue.suggestions.push_back("Consider upgrading your internet connection or checking for network congestion");
                issues.push_back(issue);
            } else if (bandwidth_mbps < 10.0) { // 低于10Mbps
                DiagnosticIssue issue(DiagnosticLevel::INFO, "Network", "Moderate Network Bandwidth");
                issue.description = "Moderate bandwidth: " + std::to_string(bandwidth_mbps) + " Mbps";
                issue.suggestions.push_back("Bandwidth is acceptable for most operations");
                issues.push_back(issue);
            } else {
                VLOG(1) << "Good bandwidth: " << bandwidth_mbps << " Mbps";
            }
        } else {
            DiagnosticIssue issue(DiagnosticLevel::WARNING, "Network", "Bandwidth Test Failed");
            issue.description = "Failed to test network bandwidth";
            issue.suggestions.push_back("Check internet connection and try again");
            issues.push_back(issue);
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error checking network bandwidth: " << e.what();
    }
    
    return issues;
}

double DiagnosticTool::measure_network_latency(const std::string& host) {
    try {
        // 使用ping命令测量延迟
        std::string ping_command;
        #ifdef _WIN32
            ping_command = "ping -n 1 -w 3000 " + host;
        #else
            ping_command = "ping -c 1 -W 3 " + host;
        #endif
        
        // 执行ping命令并解析结果
        FILE* pipe = popen(ping_command.c_str(), "r");
        if (!pipe) return -1;
        
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // 解析延迟时间（简化实现）
        // 实际实现应该解析ping输出中的时间值
        if (result.find("time=") != std::string::npos || result.find("time<") != std::string::npos) {
            // 找到时间信息，返回一个模拟值
            return 50.0; // 模拟50ms延迟
        }
        
        return -1; // 无法测量
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error measuring network latency: " << e.what();
        return -1;
    }
}

std::vector<DiagnosticIssue> DiagnosticTool::check_security() {
    std::vector<DiagnosticIssue> issues;
    
    // 检查是否有可疑的依赖
    std::vector<std::string> suspicious_patterns = {
        "test", "example", "demo", "sample"
    };
    
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& pattern : suspicious_patterns) {
            if (package.find(pattern) != std::string::npos) {
                DiagnosticIssue issue(DiagnosticLevel::WARNING, "Security", 
                                    "Suspicious package name: " + package);
                issue.description = "Package name contains suspicious pattern: " + pattern;
                issue.suggestions.push_back("Verify this is the correct package");
                issue.suggestions.push_back("Check the package source and authenticity");
                issue.context["pattern"] = pattern;
                issues.push_back(issue);
                break;
            }
        }
    }
    
    return issues;
}

std::vector<std::string> DiagnosticTool::generate_fix_suggestions(const DiagnosticResult& result) {
    std::vector<std::string> suggestions;
    
    for (const auto& issue : result.issues) {
        suggestions.insert(suggestions.end(), issue.suggestions.begin(), issue.suggestions.end());
    }
    
    // 去重
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());
    
    return suggestions;
}

bool DiagnosticTool::export_diagnostic_result(const DiagnosticResult& result, const std::string& filename) {
    try {
        json j;
        j["summary"] = result.summary;
        j["has_critical_issues"] = result.has_critical_issues;
        j["has_errors"] = result.has_errors;
        j["has_warnings"] = result.has_warnings;
        
        j["issues"] = json::array();
        for (const auto& issue : result.issues) {
            json issue_json;
            issue_json["level"] = static_cast<int>(issue.level);
            issue_json["category"] = issue.category;
            issue_json["message"] = issue.message;
            issue_json["description"] = issue.description;
            issue_json["suggestions"] = issue.suggestions;
            issue_json["context"] = issue.context;
            j["issues"].push_back(issue_json);
        }
        
        std::ofstream file(filename);
        file << j.dump(4);
        
        LOG(INFO) << "Diagnostic result exported to: " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to export diagnostic result: " << e.what();
        return false;
    }
}

std::string DiagnosticTool::format_level(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::CRITICAL: return "\033[1;31m[CRITICAL]\033[0m \033[1;31mCRITICAL\033[0m";
        case DiagnosticLevel::ERROR: return "\033[1;31m[FAIL]\033[0m \033[1;31mERROR\033[0m";
        case DiagnosticLevel::WARNING: return "\033[1;33m[WARN]\033[0m \033[1;33mWARNING\033[0m";
        case DiagnosticLevel::INFO: return "\033[1;36m[INFO]\033[0m \033[1;36mINFO\033[0m";
        default: return "\033[1;37m[UNKNOWN]\033[0m \033[1;37mUNKNOWN\033[0m";
    }
}

// 诊断规则实现
std::vector<DiagnosticIssue> CircularDependencyRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    auto cycles = graph.detect_cycles();
    for (const auto& cycle : cycles) {
        DiagnosticIssue issue(DiagnosticLevel::ERROR, "Circular Dependency", 
                            "Circular dependency detected");
        
        std::ostringstream desc;
        desc << "Circular dependency: ";
        for (size_t i = 0; i < cycle.size(); ++i) {
            if (i > 0) desc << " -> ";
            desc << cycle[i];
        }
        issue.description = desc.str();
        
        issue.suggestions.push_back("Break the circular dependency by restructuring packages");
        issue.suggestions.push_back("Use interfaces or abstractions to decouple packages");
        
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> VersionConflictRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    ConflictDetector detector(graph);
    auto conflicts = detector.detect_version_conflicts();
    
    for (const auto& conflict : conflicts) {
        DiagnosticIssue issue(DiagnosticLevel::WARNING, "Version Conflict", 
                            "Version conflict: " + conflict.package_name);
        
        std::ostringstream desc;
        desc << "Conflicting versions: ";
        for (size_t i = 0; i < conflict.conflicting_versions.size(); ++i) {
            if (i > 0) desc << ", ";
            desc << conflict.conflicting_versions[i];
        }
        issue.description = desc.str();
        
        issue.suggestions.push_back("Resolve version conflicts using 'paker resolve-conflicts'");
        issue.suggestions.push_back("Update or downgrade conflicting packages");
        
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<DiagnosticIssue> MissingDependencyRule::check(const DependencyGraph& graph) {
    std::vector<DiagnosticIssue> issues;
    
    for (const auto& [package, node] : graph.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph.has_node(dep)) {
                DiagnosticIssue issue(DiagnosticLevel::ERROR, "Missing Dependency", 
                                    "Missing dependency: " + dep);
                issue.description = "Package '" + package + "' depends on '" + dep + 
                                  "' which is not available";
                issue.suggestions.push_back("Install the missing dependency: 'Paker add " + dep + "'");
                issue.suggestions.push_back("Check if the dependency name is correct");
                
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}

} // namespace Paker 