#include "Paker/analysis/project_analyzer.h"
#include "Paker/analysis/project_type_config.h"
#include "Paker/core/output.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <curl/curl.h>
#include <json/json.h>

namespace Paker {
namespace Analysis {

ProjectAnalyzer::ProjectAnalyzer() : config_() {
    // 初始化GitHub API相关
    github_api_base_ = "https://api.github.com";
    github_token_ = getenv("GITHUB_TOKEN"); // 从环境变量获取token
}

ProjectAnalysis ProjectAnalyzer::analyze_project(const std::string& project_path) {
    ProjectAnalysis analysis;
    
    try {
        analysis.project_type = detect_project_type(project_path);
        analysis.build_system = detect_build_system(project_path);
        analysis.cpp_standard = detect_cpp_standard(project_path);
        analysis.existing_dependencies = scan_dependencies(project_path);
        analysis.code_patterns = analyze_code_patterns(project_path);
        analysis.performance_requirements = assess_performance_needs(project_path);
        analysis.security_requirements = assess_security_needs(project_path);
        analysis.testing_requirements = assess_testing_needs(project_path);
        analysis.feature_scores = calculate_feature_scores(project_path);
        
        // 添加GitHub项目分析
        analysis.github_analysis = analyze_github_project(project_path);
        analysis.trending_packages = get_trending_packages(analysis.project_type);
        analysis.similar_projects = find_similar_projects(analysis.project_type);
        
        // 添加高级分析功能
        analysis.ml_features = detect_ml_features(project_path);
        analysis.code_quality_score = calculate_code_quality_score(project_path);
        analysis.architecture_patterns = detect_architecture_patterns(project_path);
        analysis.complexity_metrics = calculate_complexity_metrics(project_path);
        analysis.performance_indicators = detect_performance_indicators(project_path);
        
        analysis.is_initialized = true;
    } catch (const std::exception& e) {
        Paker::Output::error("项目分析失败: " + std::string(e.what()));
        analysis.is_initialized = false;
    }
    
    return analysis;
}

std::string ProjectAnalyzer::detect_project_type(const std::string& project_path) {
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    std::map<std::string, double> type_scores;
    
    // 1. 基于文件结构的深度分析
    std::filesystem::path path(project_path);
    
    // 检查常见的项目结构模式
    if (file_exists(path / "src" / "main.cpp") || file_exists(path / "main.cpp")) {
        type_scores["desktop_application"] += 3.0;
    }
    if (file_exists(path / "src" / "server.cpp") || file_exists(path / "server.cpp") || 
        file_exists(path / "src" / "http_server.cpp") || file_exists(path / "http_server.cpp")) {
        type_scores["web_application"] += 4.0;
    }
    if (file_exists(path / "src" / "game.cpp") || file_exists(path / "game.cpp") ||
        file_exists(path / "src" / "engine.cpp") || file_exists(path / "engine.cpp")) {
        type_scores["game_engine"] += 4.0;
    }
    if (file_exists(path / "src" / "train.cpp") || file_exists(path / "train.cpp") ||
        file_exists(path / "src" / "model.cpp") || file_exists(path / "model.cpp")) {
        type_scores["machine_learning"] += 4.0;
    }
    if (file_exists(path / "src" / "compute.cpp") || file_exists(path / "compute.cpp") ||
        file_exists(path / "src" / "math.cpp") || file_exists(path / "math.cpp")) {
        type_scores["scientific_computing"] += 4.0;
    }
    
    // 2. 基于CMakeLists.txt的深度分析
    std::filesystem::path cmake_file(project_path);
    cmake_file /= "CMakeLists.txt";
    if (file_exists(cmake_file)) {
        std::string cmake_content = read_file_content(cmake_file);
        
        // 检查项目类型提示（更精确的匹配）
        if (cmake_content.find("find_package(Qt") != std::string::npos ||
            cmake_content.find("Qt5") != std::string::npos ||
            cmake_content.find("Qt6") != std::string::npos) {
            type_scores["desktop_application"] += 10.0;
        }
        if (cmake_content.find("find_package(OpenGL") != std::string::npos ||
            cmake_content.find("find_package(Vulkan") != std::string::npos ||
            cmake_content.find("find_package(SDL2") != std::string::npos) {
            type_scores["game_engine"] += 10.0;
        }
        if (cmake_content.find("find_package(Boost") != std::string::npos ||
            cmake_content.find("find_package(Beast") != std::string::npos ||
            cmake_content.find("find_package(Crow") != std::string::npos) {
            type_scores["web_application"] += 8.0;
        }
        if (cmake_content.find("find_package(OpenCV") != std::string::npos ||
            cmake_content.find("find_package(TensorFlow") != std::string::npos ||
            cmake_content.find("find_package(PyTorch") != std::string::npos) {
            type_scores["machine_learning"] += 10.0;
        }
        if (cmake_content.find("find_package(Eigen") != std::string::npos ||
            cmake_content.find("find_package(Armadillo") != std::string::npos ||
            cmake_content.find("find_package(GSL") != std::string::npos) {
            type_scores["scientific_computing"] += 8.0;
        }
        if (cmake_content.find("find_package(FreeRTOS") != std::string::npos ||
            cmake_content.find("find_package(Zephyr") != std::string::npos ||
            cmake_content.find("find_package(mbed") != std::string::npos) {
            type_scores["embedded_system"] += 10.0;
        }
    }
    
    // 3. 基于源代码内容的深度分析（使用配置系统）
    auto all_types = config_.get_all_project_types();
    for (const auto& type : all_types) {
        const auto& indicators = config_.get_project_indicators(type);
        double score = 0.0;
        for (const auto& indicator : indicators) {
            // 计算关键词出现频率和权重
            size_t pos = 0;
            int count = 0;
            while ((pos = content.find(indicator, pos)) != std::string::npos) {
                count++;
                pos += indicator.length();
            }
            
            // 根据关键词重要性加权
            double weight = 1.0;
            if (indicator == "qt" || indicator == "gtk" || indicator == "wxwidgets") weight = 4.0;
            else if (indicator == "opengl" || indicator == "vulkan" || indicator == "sdl") weight = 4.0;
            else if (indicator == "boost-beast" || indicator == "crow" || indicator == "http") weight = 4.0;
            else if (indicator == "eigen" || indicator == "armadillo" || indicator == "gsl") weight = 4.0;
            else if (indicator == "opencv" || indicator == "tensorflow" || indicator == "pytorch") weight = 5.0;
            else if (indicator == "freertos" || indicator == "zephyr" || indicator == "mbed") weight = 4.0;
            
            score += count * weight;
        }
        type_scores[type] += score;
    }
    
    // 4. 基于包含文件的深度分析
    if (content.find("#include <QApplication>") != std::string::npos ||
        content.find("#include <QWidget>") != std::string::npos ||
        content.find("#include <QMainWindow>") != std::string::npos) {
        type_scores["desktop_application"] += 6.0;
    }
    if (content.find("#include <GL/gl.h>") != std::string::npos ||
        content.find("#include <vulkan/vulkan.h>") != std::string::npos ||
        content.find("#include <SDL2/SDL.h>") != std::string::npos) {
        type_scores["game_engine"] += 6.0;
    }
    if (content.find("#include <opencv2/opencv.hpp>") != std::string::npos ||
        content.find("#include <tensorflow/") != std::string::npos ||
        content.find("#include <torch/") != std::string::npos) {
        type_scores["machine_learning"] += 6.0;
    }
    if (content.find("#include <eigen3/Eigen/") != std::string::npos ||
        content.find("#include <armadillo>") != std::string::npos ||
        content.find("#include <gsl/gsl_") != std::string::npos) {
        type_scores["scientific_computing"] += 6.0;
    }
    if (content.find("#include <freertos/") != std::string::npos ||
        content.find("#include <zephyr/") != std::string::npos ||
        content.find("#include <mbed.h>") != std::string::npos) {
        type_scores["embedded_system"] += 6.0;
    }
    
    // 5. 基于函数调用的深度分析
    if (content.find("QApplication") != std::string::npos ||
        content.find("QWidget") != std::string::npos ||
        content.find("QMainWindow") != std::string::npos) {
        type_scores["desktop_application"] += 4.0;
    }
    if (content.find("glClear") != std::string::npos ||
        content.find("vkCreateInstance") != std::string::npos ||
        content.find("SDL_Init") != std::string::npos) {
        type_scores["game_engine"] += 4.0;
    }
    if (content.find("cv::Mat") != std::string::npos ||
        content.find("tensorflow::") != std::string::npos ||
        content.find("torch::") != std::string::npos) {
        type_scores["machine_learning"] += 4.0;
    }
    if (content.find("Eigen::") != std::string::npos ||
        content.find("arma::") != std::string::npos ||
        content.find("gsl_") != std::string::npos) {
        type_scores["scientific_computing"] += 4.0;
    }
    if (content.find("xTaskCreate") != std::string::npos ||
        content.find("k_thread") != std::string::npos ||
        content.find("mbed::") != std::string::npos) {
        type_scores["embedded_system"] += 4.0;
    }
    
    // 6. 基于类名和命名空间的深度分析
    if (content.find("class Q") != std::string::npos ||
        content.find("namespace Qt") != std::string::npos) {
        type_scores["desktop_application"] += 3.0;
    }
    if (content.find("class Game") != std::string::npos ||
        content.find("class Engine") != std::string::npos ||
        content.find("class Renderer") != std::string::npos) {
        type_scores["game_engine"] += 3.0;
    }
    if (content.find("class Model") != std::string::npos ||
        content.find("class Neural") != std::string::npos ||
        content.find("class AI") != std::string::npos) {
        type_scores["machine_learning"] += 3.0;
    }
    if (content.find("class Matrix") != std::string::npos ||
        content.find("class Vector") != std::string::npos ||
        content.find("class Algorithm") != std::string::npos) {
        type_scores["scientific_computing"] += 3.0;
    }
    
    // 返回得分最高的类型，提高检测精度
    auto max_type = std::max_element(type_scores.begin(), type_scores.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // 提高检测阈值，确保更准确的类型识别
    if (max_type->second > 5.0) {
        return max_type->first;
    } else if (max_type->second > 3.0) {
        // 如果得分不够高，尝试基于文件结构进一步分析
        std::filesystem::path path(project_path);
        if (file_exists(path / "CMakeLists.txt")) {
            return "cmake_project";
        } else if (file_exists(path / "Makefile")) {
            return "make_project";
        } else if (file_exists(path / "meson.build")) {
            return "meson_project";
        }
        return "general";
    } else {
        return "general";
    }
}

std::string ProjectAnalyzer::detect_build_system(const std::string& project_path) {
    std::filesystem::path path(project_path);
    
    if (file_exists(path / "CMakeLists.txt")) {
        return "cmake";
    } else if (file_exists(path / "Makefile") || file_exists(path / "makefile")) {
        return "make";
    } else if (file_exists(path / "meson.build")) {
        return "meson";
    } else if (file_exists(path / "conanfile.txt") || file_exists(path / "conanfile.py")) {
        return "conan";
    } else if (file_exists(path / "vcpkg.json")) {
        return "vcpkg";
    }
    
    return "unknown";
}

std::string ProjectAnalyzer::detect_cpp_standard(const std::string& project_path) {
    std::filesystem::path cmake_file(project_path);
    cmake_file /= "CMakeLists.txt";
    
    if (file_exists(cmake_file)) {
        auto cmake_analysis = analyze_cmake_file(cmake_file);
        auto it = cmake_analysis.find("CXX_STANDARD");
        if (it != cmake_analysis.end()) {
            return it->second;
        }
    }
    
    // 分析源代码文件中的C++标准特征
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    // 检测C++20特征
    if (content.find("concepts") != std::string::npos ||
        content.find("requires") != std::string::npos ||
        content.find("std::ranges") != std::string::npos) {
        return "c++20";
    }
    
    // 检测C++17特征
    if (content.find("std::optional") != std::string::npos ||
        content.find("std::variant") != std::string::npos ||
        content.find("std::any") != std::string::npos ||
        content.find("if constexpr") != std::string::npos) {
        return "c++17";
    }
    
    // 检测C++14特征
    if (content.find("auto") != std::string::npos ||
        content.find("decltype") != std::string::npos ||
        content.find("std::make_unique") != std::string::npos) {
        return "c++14";
    }
    
    return "c++11";
}

std::vector<std::string> ProjectAnalyzer::scan_dependencies(const std::string& project_path) {
    std::vector<std::string> dependencies;
    
    // 检查Paker.json文件
    std::filesystem::path paker_json(project_path);
    paker_json /= "Paker.json";
    
    if (file_exists(paker_json)) {
        // 简化的依赖提取（实际应该用JSON解析库）
        std::string content = read_file_content(paker_json);
        
        // 简单的字符串搜索来提取依赖
        size_t pos = 0;
        while ((pos = content.find("\"dependencies\"", pos)) != std::string::npos) {
            size_t start = content.find("{", pos);
            size_t end = content.find("}", start);
            if (start != std::string::npos && end != std::string::npos) {
                std::string deps_section = content.substr(start + 1, end - start - 1);
                
                // 提取包名
                size_t dep_pos = 0;
                while ((dep_pos = deps_section.find("\"", dep_pos)) != std::string::npos) {
                    size_t name_start = dep_pos + 1;
                    size_t name_end = deps_section.find("\"", name_start);
                    if (name_end != std::string::npos) {
                        std::string dep_name = deps_section.substr(name_start, name_end - name_start);
                        if (dep_name != "name" && dep_name != "version" && dep_name != "description") {
                            dependencies.push_back(dep_name);
                        }
                        dep_pos = name_end + 1;
                    } else {
                        break;
                    }
                }
            }
            pos = end;
        }
    }
    
    return dependencies;
}

std::vector<std::string> ProjectAnalyzer::analyze_code_patterns(const std::string& project_path) {
    std::vector<std::string> patterns;
    
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    // 检测异步I/O模式
    if (content.find("async") != std::string::npos ||
        content.find("await") != std::string::npos ||
        content.find("coroutine") != std::string::npos) {
        patterns.push_back("async_io");
    }
    
    // 检测网络编程模式
    if (content.find("socket") != std::string::npos ||
        content.find("tcp") != std::string::npos ||
        content.find("udp") != std::string::npos ||
        content.find("http") != std::string::npos) {
        patterns.push_back("network_programming");
    }
    
    // 检测并发编程模式
    if (content.find("thread") != std::string::npos ||
        content.find("mutex") != std::string::npos ||
        content.find("condition_variable") != std::string::npos) {
        patterns.push_back("concurrent_programming");
    }
    
    // 检测模板编程模式
    if (content.find("template") != std::string::npos ||
        content.find("typename") != std::string::npos ||
        content.find("concept") != std::string::npos) {
        patterns.push_back("template_programming");
    }
    
    return patterns;
}

std::string ProjectAnalyzer::assess_performance_needs(const std::string& project_path) {
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    int performance_score = 0;
    const auto& indicators = config_.get_performance_indicators();
    for (const auto& indicator : indicators) {
        if (content.find(indicator) != std::string::npos) {
            performance_score++;
        }
    }
    
    if (performance_score >= 5) {
        return "high";
    } else if (performance_score >= 2) {
        return "medium";
    } else {
        return "low";
    }
}

std::string ProjectAnalyzer::assess_security_needs(const std::string& project_path) {
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    int security_score = 0;
    const auto& indicators = config_.get_security_indicators();
    for (const auto& indicator : indicators) {
        if (content.find(indicator) != std::string::npos) {
            security_score++;
        }
    }
    
    if (security_score >= 3) {
        return "high";
    } else if (security_score >= 1) {
        return "medium";
    } else {
        return "low";
    }
}

std::string ProjectAnalyzer::assess_testing_needs(const std::string& project_path) {
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    int testing_score = 0;
    const auto& indicators = config_.get_testing_indicators();
    for (const auto& indicator : indicators) {
        if (content.find(indicator) != std::string::npos) {
            testing_score++;
        }
    }
    
    if (testing_score >= 3) {
        return "high";
    } else if (testing_score >= 1) {
        return "medium";
    } else {
        return "low";
    }
}

std::string ProjectAnalyzer::read_file_content(const std::filesystem::path& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool ProjectAnalyzer::file_exists(const std::filesystem::path& file_path) {
    return std::filesystem::exists(file_path);
}

std::vector<std::filesystem::path> ProjectAnalyzer::scan_directory(
    const std::filesystem::path& dir_path, 
    const std::vector<std::string>& extensions
) {
    std::vector<std::filesystem::path> files;
    
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return files;
    }
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                    files.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        Paker::Output::warning("扫描目录时出错: " + std::string(e.what()));
    }
    
    return files;
}

std::map<std::string, std::string> ProjectAnalyzer::analyze_cmake_file(const std::filesystem::path& file_path) {
    std::map<std::string, std::string> analysis;
    std::string content = read_file_content(file_path);
    
    // 提取CXX_STANDARD
    std::regex cxx_standard_regex(R"(set\s*\(\s*CMAKE_CXX_STANDARD\s+(\d+)\s*\))");
    std::smatch match;
    if (std::regex_search(content, match, cxx_standard_regex)) {
        analysis["CXX_STANDARD"] = "c++" + match[1].str();
    }
    
    return analysis;
}

std::vector<std::string> ProjectAnalyzer::analyze_source_file(const std::filesystem::path& file_path) {
    std::vector<std::string> features;
    std::string content = read_file_content(file_path);
    
    // 这里可以添加更复杂的源代码分析逻辑
    // 例如：检测设计模式、算法复杂度等
    
    return features;
}

std::map<std::string, double> ProjectAnalyzer::calculate_feature_scores(const std::string& /* project_path */) {
    std::map<std::string, double> scores;
    
    // 计算各种特征的评分
    scores["complexity"] = 0.5;  // 简化实现
    scores["performance"] = 0.5;
    scores["security"] = 0.5;
    scores["maintainability"] = 0.5;
    
    return scores;
}

// GitHub API相关方法
std::string ProjectAnalyzer::make_github_request(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Paker-Package-Manager/1.0");
        
        // 添加认证头
        if (github_token_) {
            std::string auth_header = "Authorization: token " + std::string(github_token_);
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, auth_header.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    return response;
}

size_t ProjectAnalyzer::write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (std::bad_alloc& e) {
        return 0;
    }
}

GitHubAnalysis ProjectAnalyzer::analyze_github_project(const std::string& project_path) {
    GitHubAnalysis analysis;
    
    // 检查是否是GitHub项目
    std::filesystem::path git_path(project_path);
    git_path /= ".git";
    
    if (!std::filesystem::exists(git_path)) {
        analysis.is_github_project = false;
        return analysis;
    }
    
    // 尝试从.git/config获取远程URL
    std::filesystem::path git_config(project_path);
    git_config /= ".git/config";
    
    if (std::filesystem::exists(git_config)) {
        std::string config_content = read_file_content(git_config);
        
        // 提取GitHub URL
        std::regex url_regex(R"(url\s*=\s*https://github\.com/([^/]+)/([^/]+)\.git)");
        std::smatch match;
        if (std::regex_search(config_content, match, url_regex)) {
            analysis.owner = match[1].str();
            analysis.repo = match[2].str();
            analysis.is_github_project = true;
            
            // 获取项目信息
            std::string api_url = github_api_base_ + "/repos/" + analysis.owner + "/" + analysis.repo;
            std::string response = make_github_request(api_url);
            
            if (!response.empty()) {
                parse_github_response(response, analysis);
            }
        }
    }
    
    return analysis;
}

void ProjectAnalyzer::parse_github_response(const std::string& response, GitHubAnalysis& analysis) {
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(response, root)) {
        analysis.stars = root.get("stargazers_count", 0).asInt();
        analysis.forks = root.get("forks_count", 0).asInt();
        analysis.watchers = root.get("watchers_count", 0).asInt();
        analysis.language = root.get("language", "").asString();
        analysis.description = root.get("description", "").asString();
        analysis.updated_at = root.get("updated_at", "").asString();
        analysis.license = root.get("license", Json::Value()).get("name", "").asString();
        
        // 获取topics
        if (root.isMember("topics")) {
            for (const auto& topic : root["topics"]) {
                analysis.topics.push_back(topic.asString());
            }
        }
    }
}

std::vector<std::string> ProjectAnalyzer::get_trending_packages(const std::string& project_type) {
    std::vector<std::string> trending;
    
    // 根据项目类型搜索相关项目
    std::string search_query = build_search_query(project_type);
    std::string api_url = github_api_base_ + "/search/repositories?q=" + search_query + "&sort=stars&order=desc&per_page=30";
    
    std::string response = make_github_request(api_url);
    
    if (!response.empty()) {
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root) && root.isMember("items")) {
            for (const auto& item : root["items"]) {
                std::string repo_name = item.get("name", "").asString();
                std::string full_name = item.get("full_name", "").asString();
                int stars = item.get("stargazers_count", 0).asInt();
                int forks = item.get("forks_count", 0).asInt();
                std::string language = item.get("language", "").asString();
                std::string description = item.get("description", "").asString();
                
                // 只选择C++项目且有一定星数的项目
                if (language == "C++" && stars > 100) {
                    // 提取包名（去掉前缀，如lib-, boost-等）
                    std::string package_name = extract_package_name(repo_name);
                    if (!package_name.empty() && package_name.length() > 2) {
                        trending.push_back(package_name);
                    }
                }
            }
        }
    }
    
    // 如果GitHub API失败，使用备用方案
    if (trending.empty()) {
        trending = get_fallback_trending_packages(project_type);
    }
    
    return trending;
}

std::vector<std::string> ProjectAnalyzer::get_fallback_trending_packages(const std::string& project_type) {
    std::vector<std::string> fallback_packages;
    
    // 基于项目类型的备用包推荐
    if (project_type == "web_application") {
        fallback_packages = {"boost-beast", "crow", "cpp-httplib", "pistache", "spdlog", "nlohmann-json"};
    } else if (project_type == "desktop_application") {
        fallback_packages = {"qt", "gtkmm", "wxwidgets", "fltk", "imgui", "nuklear"};
    } else if (project_type == "game_engine") {
        fallback_packages = {"sdl2", "sfml", "opengl", "vulkan", "glm", "assimp", "bullet"};
    } else if (project_type == "machine_learning") {
        fallback_packages = {"opencv", "tensorflow", "pytorch", "eigen", "gtest", "catch2"};
    } else if (project_type == "scientific_computing") {
        fallback_packages = {"eigen", "armadillo", "gsl", "fftw", "hdf5", "blas"};
    } else if (project_type == "embedded_system") {
        fallback_packages = {"freertos", "zephyr", "mbed", "stm32", "arduino"};
    } else if (project_type == "blockchain") {
        fallback_packages = {"libsecp256k1", "openssl", "cryptopp", "libsodium"};
    } else if (project_type == "database") {
        fallback_packages = {"sqlite3", "mysql-connector-cpp", "mongocxx", "redis"};
    } else if (project_type == "networking") {
        fallback_packages = {"libuv", "asio", "libevent", "curl", "cpprest"};
    } else {
        // 通用包
        fallback_packages = {"fmt", "spdlog", "nlohmann-json", "gtest", "catch2", "boost"};
    }
    
    return fallback_packages;
}

std::string ProjectAnalyzer::build_search_query(const std::string& project_type) {
    std::string query = "language:c++";
    
    if (project_type == "web_application") {
        query += " http server rest api";
    } else if (project_type == "desktop_application") {
        query += " gui qt gtk desktop";
    } else if (project_type == "game_engine") {
        query += " game graphics opengl sdl";
    } else if (project_type == "machine_learning") {
        query += " ml ai computer-vision opencv";
    } else if (project_type == "scientific_computing") {
        query += " math linear-algebra numerical";
    }
    
    return query;
}

std::string ProjectAnalyzer::extract_package_name(const std::string& repo_name) {
    // 移除常见前缀
    std::string name = repo_name;
    
    // 移除lib-前缀
    if (name.substr(0, 4) == "lib-") {
        name = name.substr(4);
    }
    
    // 移除boost-前缀
    if (name.substr(0, 6) == "boost-") {
        name = name.substr(6);
    }
    
    // 移除其他常见前缀
    std::vector<std::string> prefixes = {"cpp-", "cxx-", "c++-", "modern-", "fast-"};
    for (const auto& prefix : prefixes) {
        if (name.substr(0, prefix.length()) == prefix) {
            name = name.substr(prefix.length());
            break;
        }
    }
    
    return name;
}

std::vector<std::string> ProjectAnalyzer::find_similar_projects(const std::string& project_type) {
    std::vector<std::string> similar;
    
    // 基于项目类型查找相似项目
    std::string search_query = build_search_query(project_type);
    std::string api_url = github_api_base_ + "/search/repositories?q=" + search_query + "&sort=updated&order=desc&per_page=15";
    
    std::string response = make_github_request(api_url);
    
    if (!response.empty()) {
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root) && root.isMember("items")) {
            for (const auto& item : root["items"]) {
                std::string full_name = item.get("full_name", "").asString();
                int stars = item.get("stargazers_count", 0).asInt();
                int forks = item.get("forks_count", 0).asInt();
                std::string language = item.get("language", "").asString();
                std::string description = item.get("description", "").asString();
                
                // 只选择C++项目且有一定星数的项目
                if (language == "C++" && stars > 20 && !full_name.empty()) {
                    similar.push_back(full_name);
                }
            }
        }
    }
    
    // 如果GitHub API失败，使用备用方案
    if (similar.empty()) {
        similar = get_fallback_similar_projects(project_type);
    }
    
    return similar;
}

std::vector<std::string> ProjectAnalyzer::get_fallback_similar_projects(const std::string& project_type) {
    std::vector<std::string> fallback_projects;
    
    // 基于项目类型的备用相似项目
    if (project_type == "web_application") {
        fallback_projects = {"microsoft/cpprestsdk", "boostorg/beast", "crowcpp/crow", "p-ranav/httplib"};
    } else if (project_type == "desktop_application") {
        fallback_projects = {"qtproject/qt", "gtkmm/gtkmm", "wxWidgets/wxWidgets", "ocornut/imgui"};
    } else if (project_type == "game_engine") {
        fallback_projects = {"libsdl-org/SDL", "SFML/SFML", "g-truc/glm", "assimp/assimp"};
    } else if (project_type == "machine_learning") {
        fallback_projects = {"opencv/opencv", "tensorflow/tensorflow", "pytorch/pytorch", "eigenteam/eigen-git-mirror"};
    } else if (project_type == "scientific_computing") {
        fallback_projects = {"eigenteam/eigen-git-mirror", "conradsnicta/armadillo-code", "GSL/GSL", "FFTW/fftw3"};
    } else {
        // 通用项目
        fallback_projects = {"fmtlib/fmt", "gabime/spdlog", "nlohmann/json", "google/googletest"};
    }
    
    return fallback_projects;
}

// 新增的高级分析方法
std::vector<std::string> ProjectAnalyzer::detect_ml_features(const std::string& project_path) {
    std::vector<std::string> features;
    
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    const auto& ml_features = config_.get_ml_features();
    for (const auto& feature : ml_features) {
        if (content.find(feature) != std::string::npos) {
            features.push_back(feature);
        }
    }
    
    return features;
}

double ProjectAnalyzer::calculate_code_quality_score(const std::string& project_path) {
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    double score = 0.0;
    const auto& indicators = config_.get_code_quality_indicators();
    int total_indicators = indicators.size();
    int found_indicators = 0;
    
    for (const auto& indicator : indicators) {
        if (content.find(indicator) != std::string::npos) {
            found_indicators++;
        }
    }
    
    score = static_cast<double>(found_indicators) / total_indicators;
    return score;
}

std::vector<std::string> ProjectAnalyzer::detect_architecture_patterns(const std::string& project_path) {
    std::vector<std::string> patterns;
    
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    const auto& arch_patterns = config_.get_architecture_patterns();
    for (const auto& pattern : arch_patterns) {
        if (content.find(pattern) != std::string::npos) {
            patterns.push_back(pattern);
        }
    }
    
    return patterns;
}

std::map<std::string, double> ProjectAnalyzer::calculate_complexity_metrics(const std::string& project_path) {
    std::map<std::string, double> metrics;
    
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    int total_lines = 0;
    int total_functions = 0;
    int total_classes = 0;
    int total_templates = 0;
    
    for (const auto& file : source_files) {
        std::string content = read_file_content(file);
        
        // 计算行数
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.find_first_not_of(" \t") != std::string::npos) {
                total_lines++;
            }
        }
        
        // 计算函数数量
        size_t pos = 0;
        while ((pos = content.find("(", pos)) != std::string::npos) {
            if (content.substr(pos - 10, 10).find("class") == std::string::npos &&
                content.substr(pos - 10, 10).find("struct") == std::string::npos) {
                total_functions++;
            }
            pos++;
        }
        
        // 计算类数量
        pos = 0;
        while ((pos = content.find("class ", pos)) != std::string::npos) {
            total_classes++;
            pos += 6;
        }
        
        // 计算模板数量
        pos = 0;
        while ((pos = content.find("template", pos)) != std::string::npos) {
            total_templates++;
            pos += 8;
        }
    }
    
    metrics["total_lines"] = total_lines;
    metrics["total_functions"] = total_functions;
    metrics["total_classes"] = total_classes;
    metrics["total_templates"] = total_templates;
    metrics["complexity_score"] = (total_functions + total_classes + total_templates) / static_cast<double>(total_lines + 1);
    
    return metrics;
}

std::vector<std::string> ProjectAnalyzer::detect_performance_indicators(const std::string& project_path) {
    std::vector<std::string> indicators;
    
    std::vector<std::filesystem::path> source_files = scan_directory(
        std::filesystem::path(project_path), 
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".h", ".hxx"}
    );
    
    std::string content;
    for (const auto& file : source_files) {
        content += read_file_content(file);
    }
    
    const auto& perf_indicators = config_.get_performance_indicators();
    for (const auto& indicator : perf_indicators) {
        if (content.find(indicator) != std::string::npos) {
            indicators.push_back(indicator);
        }
    }
    
    return indicators;
}

GitHubPackageInfo ProjectAnalyzer::get_github_package_info(const std::string& package_name) {
    GitHubPackageInfo info;
    info.name = package_name;
    info.found = false;
    
    // 已知的GitHub项目映射
    std::map<std::string, std::pair<std::string, std::string>> known_projects = {
        {"sdl2", {"libsdl-org/SDL", "Simple DirectMedia Layer - A cross-platform development library"}},
        {"sfml", {"SFML/SFML", "Simple and Fast Multimedia Library"}},
        {"opengl", {"KhronosGroup/OpenGL-Registry", "The OpenGL Registry"}},
        {"vulkan", {"KhronosGroup/Vulkan-Headers", "Vulkan header files and API registry"}},
        {"glm", {"g-truc/glm", "OpenGL Mathematics (GLM)"}},
        {"assimp", {"assimp/assimp", "Official Open Asset Import Library Repository"}},
        {"bullet", {"bulletphysics/bullet3", "Bullet Physics SDK: real-time collision detection and multi-physics simulation for VR, games, visual effects, robotics, machine learning etc."}},
        {"box2d", {"erincatto/box2d", "Box2D is a 2D physics engine for games"}},
        {"raylib", {"raysan5/raylib", "A simple and easy-to-use library to enjoy videogames programming"}},
        {"bgfx", {"bkaradzic/bgfx", "Cross-platform, graphics API agnostic, \"Bring Your Own Engine/Framework\" style rendering library"}},
        {"magnum", {"mosra/magnum", "Lightweight and modular C++11/C++14 graphics middleware for games and data visualization"}},
        {"ogre3d", {"OGRECave/ogre", "Scene-oriented, flexible 3D engine (C++, Python, C#, Java)"}},
        {"irrlicht", {"zaki/irrlicht", "The Irrlicht Engine is an open source realtime 3D engine written in C++"}},
        {"cocos2d", {"cocos2d/cocos2d-x", "Cocos2d-x is a suite of open-source, cross-platform, game-development tools used by thousands of developers all over the world"}},
        {"godot", {"godotengine/godot", "Godot Engine – Multi-platform 2D and 3D game engine"}},
        {"unity", {"Unity-Technologies/UnityCsReference", "Unity C# reference source code"}},
        {"unreal", {"EpicGames/UnrealEngine", "Unreal Engine 5"}},
        {"cryengine", {"CRYTEK/CRYENGINE", "CRYENGINE is a powerful real-time game development platform created by Crytek"}},
        {"lumberyard", {"aws/lumberyard", "Amazon Lumberyard is a free AAA game engine deeply integrated with AWS and Twitch"}},
        {"phaser", {"photonstorm/phaser", "Phaser is a fun, free and fast 2D game framework for making HTML5 games for desktop and mobile web browsers"}},
        {"threejs", {"mrdoob/three.js", "JavaScript 3D library"}},
        {"babylon", {"BabylonJS/Babylon.js", "Babylon.js is a powerful, beautiful, simple, and open game and rendering engine packed into a friendly JavaScript framework"}},
        {"pixi", {"pixijs/pixi.js", "The HTML5 Creation Engine: Create beautiful digital content with the fastest, most flexible 2D WebGL renderer"}},
        {"konva", {"konvajs/konva", "Konva.js 2D canvas library for desktop and mobile applications"}},
        {"fmt", {"fmtlib/fmt", "A modern formatting library"}},
        {"spdlog", {"gabime/spdlog", "Fast C++ logging library"}},
        {"nlohmann-json", {"nlohmann/json", "JSON for Modern C++"}},
        {"gtest", {"google/googletest", "GoogleTest - Google Testing and Mocking Framework"}},
        {"catch2", {"catchorg/Catch2", "A modern, C++-native, header-only, test framework for unit-tests, TDD and BDD"}},
        {"boost", {"boostorg/boost", "Super-project for modularized Boost"}},
        {"asio", {"boostorg/asio", "Asio C++ Library"}},
        {"beast", {"boostorg/beast", "HTTP and WebSocket built on Boost.Asio in C++11"}},
        {"filesystem", {"boostorg/filesystem", "Boost.Filesystem"}},
        {"range-v3", {"ericniebler/range-v3", "Range library for C++14/17/20, basis for C++20's std::ranges"}},
        {"abseil", {"abseil/abseil-cpp", "Abseil Common Libraries (C++)"}},
        {"folly", {"facebook/folly", "An open-source C++ library developed and used at Facebook"}},
        {"glog", {"google/glog", "C++ implementation of the Google logging library"}},
        {"gflags", {"gflags/gflags", "The gflags package contains a C++ library that implements commandline flags processing"}},
        {"protobuf", {"protocolbuffers/protobuf", "Protocol Buffers - Google's data interchange format"}},
        {"grpc", {"grpc/grpc", "The C based gRPC (C++, Python, Ruby, Objective-C, PHP, C#)"}},
        {"thrift", {"apache/thrift", "Apache Thrift"}},
        {"zeromq", {"zeromq/libzmq", "ZeroMQ core engine in C++, implements ZMTP/3.1"}},
        {"nanomsg", {"nanomsg/nanomsg", "Event notification library"}},
        {"libevent", {"libevent/libevent", "Event notification library"}},
        {"libuv", {"libuv/libuv", "Cross-platform asynchronous I/O"}},
        {"libev", {"enki/libev", "Full-featured and high-performance event loop library"}},
        {"libevent2", {"libevent/libevent", "Event notification library"}},
        {"libasync", {"facebook/folly", "Folly: Facebook's C++ library"}},
        {"libdispatch", {"apple/swift-corelibs-libdispatch", "The libdispatch project, (a.k.a. Grand Central Dispatch), for concurrency on multicore hardware"}},
        {"eigen", {"eigenteam/eigen-git-mirror", "Eigen is a C++ template library for linear algebra: matrices, vectors, numerical solvers, and related algorithms"}},
        {"armadillo", {"conradsnicta/armadillo-code", "Armadillo: fast C++ library for linear algebra & scientific computing"}},
        {"gsl", {"ampl/gsl", "GNU Scientific Library"}},
        {"fftw", {"FFTW/fftw3", "The Fastest Fourier Transform in the West"}},
        {"blas", {"Reference-LAPACK/lapack", "LAPACK development repository"}},
        {"lapack", {"Reference-LAPACK/lapack", "LAPACK development repository"}},
        {"mkl", {"intel/mkl-dnn", "Deep Neural Network Library (DNNL)"}},
        {"openblas", {"xianyi/OpenBLAS", "OpenBLAS is an optimized BLAS library based on GotoBLAS2 1.13 BSD version"}},
        {"intel-mkl", {"intel/mkl-dnn", "Deep Neural Network Library (DNNL)"}},
        {"cuda", {"NVIDIA/cuda-samples", "Samples for CUDA Developers which demonstrates features in CUDA Toolkit"}},
        {"opencl", {"KhronosGroup/OpenCL-Headers", "OpenCL header files"}},
        {"sycl", {"KhronosGroup/SYCL-Headers", "SYCL header files"}},
        {"openmp", {"OpenMP/OpenMP", "OpenMP: The Open API for Multi-Platform Parallel Programming"}},
        {"mpi", {"open-mpi/ompi", "Open MPI main development repository"}},
        {"petsc", {"petsc/petsc", "Portable, Extensible Toolkit for Scientific Computation"}},
        {"slepc", {"slepc/slepc", "Scalable Library for Eigenvalue Problem Computations"}},
        {"trilinos", {"trilinos/Trilinos", "Primary repository for the Trilinos Project"}},
        {"dealii", {"dealii/dealii", "The deal.II finite element library"}},
        {"fenics", {"FEniCS/dolfin", "DOLFIN is the C++/Python interface of FEniCS"}},
        {"dolfin", {"FEniCS/dolfin", "DOLFIN is the C++/Python interface of FEniCS"}},
        {"opencv", {"opencv/opencv", "Open Source Computer Vision Library"}},
        {"tensorflow", {"tensorflow/tensorflow", "An Open Source Machine Learning Framework"}},
        {"pytorch", {"pytorch/pytorch", "Tensors and Dynamic neural networks in Python with strong GPU acceleration"}},
        {"onnx", {"onnx/onnx", "Open standard for machine learning interoperability"}},
        {"tflite", {"tensorflow/tensorflow", "An Open Source Machine Learning Framework"}},
        {"sklearn", {"scikit-learn/scikit-learn", "scikit-learn: machine learning in Python"}},
        {"xgboost", {"dmlc/xgboost", "Scalable, Portable and Distributed Gradient Boosting (GBDT, GBRT or GBM) Library"}},
        {"lightgbm", {"microsoft/LightGBM", "A fast, distributed, high performance gradient boosting (GBDT, GBRT, GBM or MART) framework based on decision tree algorithms"}},
        {"catboost", {"catboost/catboost", "A fast, scalable, high performance Gradient Boosting on Decision Trees library"}},
        {"mlpack", {"mlpack/mlpack", "mlpack: a scalable C++ machine learning library"}},
        {"shark", {"Shark-ML/Shark", "A fast, modular, general open-source machine learning library"}},
        {"dlib", {"davisking/dlib", "A toolkit for making real world machine learning and data analysis applications in C++"}},
        {"torch", {"pytorch/pytorch", "Tensors and Dynamic neural networks in Python with strong GPU acceleration"}},
        {"caffe", {"BVLC/caffe", "Caffe: a fast open framework for deep learning"}},
        {"mxnet", {"apache/incubator-mxnet", "Lightweight, Portable, Flexible Distributed/Mobile Deep Learning with Dynamic, Mutation-aware Dataflow Dep Scheduler"}},
        {"paddle", {"PaddlePaddle/Paddle", "PArallel Distributed Deep LEarning: Machine Learning Framework"}},
        {"mindspore", {"mindspore-ai/mindspore", "MindSpore is a new open source deep learning training/inference framework"}},
        {"jax", {"google/jax", "Composable transformations of Python+NumPy programs: differentiate, vectorize, JIT to GPU/TPU, and more"}},
        {"flax", {"google/flax", "Flax is a neural network library for JAX that is designed for flexibility"}},
        {"keras", {"keras-team/keras", "Deep Learning for humans"}},
        {"theano", {"Theano/Theano", "Theano was a Python library that allowed you to define, optimize, and evaluate mathematical expressions involving multi-dimensional arrays efficiently"}},
        {"lasagne", {"Lasagne/Lasagne", "Lightweight library to build and train neural networks in Theano"}},
        {"blocks", {"mila-udem/blocks", "A Theano framework for building and training neural networks"}},
        {"fuel", {"mila-udem/fuel", "A data pipeline framework for machine learning"}},
        {"qt", {"qtproject/qt", "Qt Project"}},
        {"gtkmm", {"GNOME/gtkmm", "gtkmm is the official C++ interface for the GTK+ GUI library"}},
        {"wxwidgets", {"wxWidgets/wxWidgets", "Cross-Platform C++ GUI Library"}},
        {"fltk", {"fltk/fltk", "Fast Light Tool Kit (FLTK)"}},
        {"imgui", {"ocornut/imgui", "Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies"}},
        {"nuklear", {"vurtun/nuklear", "A single-header ANSI C gui library"}},
        {"dear-imgui", {"ocornut/imgui", "Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies"}},
        {"nanogui", {"wjakob/nanogui", "Minimalistic GUI library for OpenGL"}},
        {"cef", {"chromiumembedded/cef", "Chromium Embedded Framework (CEF)"}},
        {"electron", {"electron/electron", "Build cross-platform desktop apps with JavaScript, HTML, and CSS"}},
        {"tauri", {"tauri-apps/tauri", "Build smaller, faster, and more secure desktop applications with a web frontend"}},
        {"flutter", {"flutter/flutter", "Flutter makes it easy and fast to build beautiful apps for mobile and beyond"}},
        {"gtk", {"GNOME/gtk", "GTK is a multi-platform toolkit for creating graphical user interfaces"}},
        {"kde", {"KDE", "KDE is an international technology team that creates free and open source software for desktop and portable computing"}},
        {"gnome", {"GNOME/gnome-shell", "GNOME Shell"}},
        {"xfce", {"xfce-mirror", "Xfce desktop environment"}},
        {"lxde", {"lxde", "Lightweight X11 Desktop Environment"}},
        {"mate", {"mate-desktop", "MATE Desktop Environment"}},
        {"cinnamon", {"linuxmint/cinnamon", "Cinnamon Desktop Environment"}},
        {"budgie", {"solus-project/budgie-desktop", "I Took a Pill in Ibiza"}},
        {"xfce4", {"xfce-mirror", "Xfce desktop environment"}},
        {"lxqt", {"lxqt", "The LXQt desktop environment"}},
        {"enlightenment", {"Enlightenment", "Enlightenment window manager"}},
        {"openbox", {"danakj/openbox", "Openbox window manager"}},
        {"fluxbox", {"fluxbox", "Fluxbox window manager"}},
        {"scipy", {"scipy/scipy", "SciPy library main repository"}},
        {"numpy", {"numpy/numpy", "The fundamental package for scientific computing with Python"}},
        {"matlab", {"mathworks", "MathWorks"}},
        {"octave", {"gnu-octave/octave", "GNU Octave"}},
        {"sage", {"sagemath/sage", "SageMath"}}
    };
    
    // 查找已知项目
    auto it = known_projects.find(package_name);
    if (it != known_projects.end()) {
        info.full_name = it->second.first;
        info.description = it->second.second;
        info.github_url = "https://github.com/" + it->second.first;
        info.found = true;
        
        // 尝试获取更多信息
        try {
            std::string api_url = github_api_base_ + "/repos/" + it->second.first;
            std::string response = make_github_request(api_url);
            
            if (!response.empty()) {
                // 解析JSON响应获取更多信息
                // 这里简化处理，实际应该使用JSON解析库
                if (response.find("\"stargazers_count\"") != std::string::npos) {
                    // 提取星标数
                    size_t start = response.find("\"stargazers_count\":") + 19;
                    size_t end = response.find(",", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string stars_str = response.substr(start, end - start);
                        info.stars = std::stoi(stars_str);
                    }
                }
                
                if (response.find("\"forks_count\"") != std::string::npos) {
                    // 提取分支数
                    size_t start = response.find("\"forks_count\":") + 14;
                    size_t end = response.find(",", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string forks_str = response.substr(start, end - start);
                        info.forks = std::stoi(forks_str);
                    }
                }
                
                if (response.find("\"language\"") != std::string::npos) {
                    // 提取语言
                    size_t start = response.find("\"language\":\"") + 12;
                    size_t end = response.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        info.language = response.substr(start, end - start);
                    }
                }
                
                if (response.find("\"license\"") != std::string::npos) {
                    // 提取许可证
                    size_t start = response.find("\"name\":\"") + 8;
                    size_t end = response.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        info.license = response.substr(start, end - start);
                    }
                }
            }
        } catch (...) {
            // 如果API调用失败，使用默认信息
        }
    } else {
        // 如果不在已知列表中，使用搜索
        info.github_url = "https://github.com/search?q=" + package_name + "+language%3AC%2B%2B&s=stars&o=desc";
        info.description = "C++ library found on GitHub";
    }
    
    return info;
}

} // namespace Analysis
} // namespace Paker
