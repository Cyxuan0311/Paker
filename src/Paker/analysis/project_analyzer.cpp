#include "Paker/analysis/project_analyzer.h"
#include "Paker/core/output.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <curl/curl.h>
#include <json/json.h>

namespace Paker {
namespace Analysis {

ProjectAnalyzer::ProjectAnalyzer() {
    // 初始化项目类型检测关键词（增强版）
    project_type_indicators_ = {
        {"web_application", {
            "http", "server", "client", "rest", "api", "websocket", "tcp", "udp",
            "boost-beast", "crow", "cpp-httplib", "pistache", "cpprest", "drogon",
            "nginx", "apache", "microservice", "gateway", "load_balancer"
        }},
        {"desktop_application", {
            "qt", "gtk", "wxwidgets", "fltk", "gui", "window", "dialog", "widget",
            "qwidget", "qapplication", "gtkmm", "wxframe", "imwidget", "nuklear",
            "imgui", "dear_imgui", "native", "cross_platform", "desktop"
        }},
        {"embedded_system", {
            "embedded", "microcontroller", "rtos", "freertos", "zephyr", "threadx",
            "stm32", "arduino", "esp32", "bare_metal", "hal", "driver", "bsp",
            "mcu", "iot", "sensor", "actuator", "real_time", "low_power"
        }},
        {"game_engine", {
            "opengl", "vulkan", "sdl", "sfml", "game", "graphics", "rendering",
            "shader", "texture", "mesh", "sprite", "animation", "physics",
            "bullet", "box2d", "chipmunk", "unity", "unreal", "cocos2d"
        }},
        {"scientific_computing", {
            "eigen", "armadillo", "blas", "lapack", "fftw", "gsl", "mkl",
            "numerical", "matrix", "vector", "linear_algebra", "statistics",
            "optimization", "simulation", "finite_element", "computational"
        }},
        {"machine_learning", {
            "tensorflow", "pytorch", "opencv", "ml", "neural", "deep_learning",
            "ai", "computer_vision", "nlp", "sklearn", "xgboost", "lightgbm",
            "onnx", "tflite", "inference", "training", "model", "dataset"
        }},
        {"blockchain", {
            "blockchain", "crypto", "bitcoin", "ethereum", "smart_contract",
            "consensus", "mining", "hash", "merkle", "distributed", "p2p"
        }},
        {"database", {
            "database", "sql", "nosql", "mongodb", "redis", "postgresql",
            "mysql", "sqlite", "cassandra", "elasticsearch", "influxdb"
        }},
        {"networking", {
            "network", "socket", "tcp", "udp", "ip", "routing", "protocol",
            "packet", "bandwidth", "latency", "throughput", "distributed"
        }}
    };

    // 初始化性能要求检测关键词（增强版）
    performance_indicators_ = {
        "high_performance", "real_time", "low_latency", "boost", "eigen",
        "openmp", "simd", "parallel", "concurrent", "async", "thread",
        "lock_free", "atomic", "memory_pool", "cache_friendly", "vectorized",
        "gpu", "cuda", "opencl", "vulkan", "compute_shader", "optimization",
        "profiling", "benchmark", "performance", "throughput", "scalability"
    };

    // 初始化安全要求检测关键词（增强版）
    security_indicators_ = {
        "crypto", "encryption", "ssl", "tls", "secure", "authentication",
        "authorization", "oauth", "jwt", "hash", "signature", "rsa", "aes",
        "symmetric", "asymmetric", "key_exchange", "certificate", "pki",
        "vulnerability", "security_audit", "penetration_test", "firewall",
        "intrusion_detection", "access_control", "role_based", "permission"
    };

    // 初始化测试要求检测关键词（增强版）
    testing_indicators_ = {
        "gtest", "catch2", "boost-test", "doctest", "test", "unit_test",
        "integration_test", "benchmark", "mock", "fixture", "coverage",
        "regression_test", "stress_test", "load_test", "performance_test",
        "acceptance_test", "smoke_test", "sanity_test", "exploratory_test",
        "test_automation", "ci_cd", "continuous_integration", "tdd", "bdd"
    };
    
    // 初始化机器学习特征检测关键词
    ml_features_ = {
        "neural_network", "deep_learning", "cnn", "rnn", "lstm", "transformer",
        "attention", "backpropagation", "gradient_descent", "adam", "sgd",
        "dropout", "batch_normalization", "regularization", "overfitting",
        "cross_validation", "feature_engineering", "data_preprocessing"
    };
    
    // 初始化代码质量指标
    code_quality_indicators_ = {
        "const", "constexpr", "noexcept", "override", "final", "explicit",
        "smart_pointer", "raii", "move_semantics", "perfect_forwarding",
        "template_metaprogramming", "concepts", "ranges", "coroutines"
    };
    
    // 初始化架构模式检测
    architecture_patterns_ = {
        "singleton", "factory", "observer", "strategy", "command", "mvc",
        "mvp", "mvvm", "microservice", "soa", "event_driven", "reactive",
        "actor_model", "pipeline", "middleware", "plugin", "component"
    };
    
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
    
    // 3. 基于源代码内容的深度分析
    for (const auto& [type, indicators] : project_type_indicators_) {
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
    for (const auto& indicator : performance_indicators_) {
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
    for (const auto& indicator : security_indicators_) {
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
    for (const auto& indicator : testing_indicators_) {
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
    std::string api_url = github_api_base_ + "/search/repositories?q=" + search_query + "&sort=stars&order=desc&per_page=10";
    
    std::string response = make_github_request(api_url);
    
    if (!response.empty()) {
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root) && root.isMember("items")) {
            for (const auto& item : root["items"]) {
                std::string repo_name = item.get("name", "").asString();
                std::string full_name = item.get("full_name", "").asString();
                
                // 提取包名（去掉前缀，如lib-, boost-等）
                std::string package_name = extract_package_name(repo_name);
                if (!package_name.empty()) {
                    trending.push_back(package_name);
                }
            }
        }
    }
    
    return trending;
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
    std::string api_url = github_api_base_ + "/search/repositories?q=" + search_query + "&sort=updated&order=desc&per_page=5";
    
    std::string response = make_github_request(api_url);
    
    if (!response.empty()) {
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(response, root) && root.isMember("items")) {
            for (const auto& item : root["items"]) {
                std::string full_name = item.get("full_name", "").asString();
                if (!full_name.empty()) {
                    similar.push_back(full_name);
                }
            }
        }
    }
    
    return similar;
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
    
    for (const auto& feature : ml_features_) {
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
    int total_indicators = code_quality_indicators_.size();
    int found_indicators = 0;
    
    for (const auto& indicator : code_quality_indicators_) {
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
    
    for (const auto& pattern : architecture_patterns_) {
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
    
    for (const auto& indicator : performance_indicators_) {
        if (content.find(indicator) != std::string::npos) {
            indicators.push_back(indicator);
        }
    }
    
    return indicators;
}

} // namespace Analysis
} // namespace Paker
