#include "Paker/analysis/recommendation_engine.h"
#include "Paker/core/output.h"
#include <algorithm>
#include <numeric>

namespace Paker {
namespace Analysis {

RecommendationEngine::RecommendationEngine() {
    // 初始化包知识库
    initialize_package_knowledge_base();
    
    // 初始化项目类型到包的映射
    initialize_type_package_mapping();
    
    // 初始化依赖关系映射
    initialize_dependency_relationships();
}

void RecommendationEngine::initialize_package_knowledge_base() {
    // Web应用相关包
    package_knowledge_base_["web_application"] = {
        {"boost-beast", "High-performance HTTP and WebSocket library", "Perfect for high-performance web applications", "web", 0.95, 0.90, 0.90, 0.85, "high", {"http", "websocket", "async"}, "Paker add boost-beast"},
        {"crow", "Lightweight C++ web framework", "Simple and easy-to-use web framework", "web", 0.90, 0.85, 0.85, 0.80, "high", {"web", "framework", "rest"}, "Paker add crow"},
        {"cpp-httplib", "Single-header HTTP library", "Simple HTTP client/server", "web", 0.85, 0.95, 0.80, 0.75, "medium", {"http", "simple", "header-only"}, "Paker add cpp-httplib"},
        {"pistache", "Modern C++ HTTP framework", "RESTful API framework", "web", 0.88, 0.80, 0.75, 0.70, "medium", {"rest", "api", "modern"}, "Paker add pistache"},
        {"spdlog", "Fast C++ logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog"},
        {"nlohmann-json", "Modern C++ JSON library", "Easy-to-use JSON processing", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json"}
    };
    
    // 桌面应用相关包
    package_knowledge_base_["desktop_application"] = {
        {"qt", "Cross-platform GUI framework", "Powerful and feature-rich GUI framework", "gui", 0.95, 0.90, 0.95, 0.90, "high", {"gui", "cross-platform", "widgets"}, "Paker add qt"},
        {"gtkmm", "GTK+ C++ bindings", "Native Linux GUI", "gui", 0.85, 0.80, 0.70, 0.75, "medium", {"gui", "linux", "gtk"}, "Paker add gtkmm"},
        {"wxwidgets", "Native GUI framework", "Cross-platform native look", "gui", 0.80, 0.85, 0.75, 0.80, "medium", {"gui", "native", "cross-platform"}, "Paker add wxwidgets"},
        {"fltk", "Lightweight GUI library", "Fast and lightweight GUI", "gui", 0.75, 0.90, 0.65, 0.70, "low", {"gui", "lightweight", "fast"}, "Paker add fltk"}
    };
    
    // 游戏引擎相关包
    package_knowledge_base_["game_engine"] = {
        {"sdl2", "Cross-platform multimedia library", "Essential for game development", "graphics", 0.95, 0.90, 0.95, 0.90, "high", {"graphics", "audio", "input"}, "Paker add sdl2"},
        {"sfml", "Simple and fast multimedia library", "Perfect for 2D game development", "graphics", 0.90, 0.85, 0.85, 0.80, "high", {"graphics", "2d", "simple"}, "Paker add sfml"},
        {"opengl", "Graphics rendering API", "3D graphics rendering", "graphics", 0.95, 0.90, 0.90, 0.85, "high", {"3d", "graphics", "rendering"}, "Paker add opengl"},
        {"vulkan", "Modern graphics API", "High-performance 3D rendering", "graphics", 0.85, 0.80, 0.75, 0.70, "high", {"3d", "high-performance", "modern"}, "Paker add vulkan"}
    };
    
    // 科学计算相关包
    package_knowledge_base_["scientific_computing"] = {
        {"eigen", "Linear algebra library", "Matrix and vector operations", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "matrix", "vector"}, "Paker add eigen"},
        {"armadillo", "C++ linear algebra library", "Advanced linear algebra", "math", 0.90, 0.85, 0.80, 0.75, "medium", {"linear-algebra", "matlab-like"}, "Paker add armadillo"},
        {"gsl", "GNU Scientific Library", "Numerical computation functions", "math", 0.85, 0.80, 0.75, 0.70, "medium", {"numerical", "scientific", "gnu"}, "Paker add gsl"}
    };
    
    // 机器学习相关包
    package_knowledge_base_["machine_learning"] = {
        {"opencv", "Computer vision library", "Image processing and computer vision", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "ml"}, "Paker add opencv"},
        {"tensorflow", "Machine learning framework", "Deep learning framework", "ml", 0.90, 0.85, 0.90, 0.85, "high", {"deep-learning", "neural-networks", "ai"}, "Paker add tensorflow"},
        {"pytorch", "Dynamic neural networks", "Research-friendly ML framework", "ml", 0.85, 0.80, 0.85, 0.80, "high", {"deep-learning", "research", "dynamic"}, "Paker add pytorch"}
    };
    
    // 通用包
    package_knowledge_base_["general"] = {
        {"fmt", "Modern C++ formatting library", "Type-safe formatting", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt"},
        {"spdlog", "Fast C++ logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog"},
        {"nlohmann-json", "Modern C++ JSON library", "Easy-to-use JSON processing", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json"},
        {"gtest", "Google Test framework", "Unit testing framework", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest"},
        {"catch2", "Modern C++ testing framework", "Simple and easy testing", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2"},
        {"boost", "C++ extension libraries", "Comprehensive library collection", "utility", 0.90, 0.85, 0.90, 0.85, "high", {"utilities", "extensions", "comprehensive"}, "Paker add boost"}
    };
    
    // 新增专业领域包
    package_knowledge_base_["blockchain"] = {
        {"libsecp256k1", "Bitcoin cryptographic library", "Elliptic curve cryptography", "crypto", 0.90, 0.85, 0.80, 0.75, "high", {"bitcoin", "crypto", "secp256k1"}, "Paker add libsecp256k1"},
        {"openssl", "Cryptographic library", "SSL/TLS and general cryptography", "crypto", 0.95, 0.90, 0.95, 0.90, "high", {"ssl", "tls", "crypto", "security"}, "Paker add openssl"},
        {"cryptopp", "Crypto++ library", "Comprehensive cryptographic library", "crypto", 0.90, 0.85, 0.85, 0.80, "high", {"crypto", "encryption", "hashing"}, "Paker add cryptopp"}
    };
    
    package_knowledge_base_["database"] = {
        {"sqlite3", "SQLite database", "Embedded SQL database", "database", 0.95, 0.90, 0.95, 0.90, "high", {"sql", "embedded", "lightweight"}, "Paker add sqlite3"},
        {"mysql-connector-cpp", "MySQL C++ connector", "MySQL database connectivity", "database", 0.85, 0.80, 0.75, 0.70, "medium", {"mysql", "database", "sql"}, "Paker add mysql-connector-cpp"},
        {"mongocxx", "MongoDB C++ driver", "MongoDB database connectivity", "database", 0.80, 0.75, 0.70, 0.65, "medium", {"mongodb", "nosql", "document"}, "Paker add mongocxx"}
    };
    
    package_knowledge_base_["networking"] = {
        {"libuv", "Cross-platform asynchronous I/O", "Event-driven programming", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "io", "event-driven"}, "Paker add libuv"},
        {"asio", "Boost.Asio networking", "Asynchronous I/O and networking", "async", 0.95, 0.90, 0.90, 0.85, "high", {"async", "networking", "boost"}, "Paker add asio"},
        {"libevent", "Event notification library", "High-performance event loop", "async", 0.85, 0.80, 0.75, 0.70, "medium", {"event", "async", "network"}, "Paker add libevent"}
    };
}

void RecommendationEngine::initialize_type_package_mapping() {
    type_package_mapping_ = {
        {"web_application", {"boost-beast", "crow", "cpp-httplib", "pistache", "spdlog", "nlohmann-json"}},
        {"desktop_application", {"qt", "gtkmm", "wxwidgets", "fltk", "spdlog", "nlohmann-json"}},
        {"game_engine", {"sdl2", "sfml", "opengl", "vulkan", "glm", "assimp"}},
        {"scientific_computing", {"eigen", "armadillo", "gsl", "fftw", "hdf5"}},
        {"machine_learning", {"opencv", "tensorflow", "pytorch", "eigen", "gtest"}},
        {"embedded_system", {"freertos", "zephyr", "mbed", "stm32"}}
    };
}

void RecommendationEngine::initialize_dependency_relationships() {
    dependency_relationships_ = {
        {"fmt", {"spdlog", "glog", "easyloggingpp"}},
        {"spdlog", {"fmt", "boost-log"}},
        {"nlohmann-json", {"rapidjson", "jsoncpp"}},
        {"gtest", {"gmock", "catch2", "doctest"}},
        {"boost", {"boost-beast", "boost-asio", "boost-log"}},
        {"opencv", {"eigen", "gtest"}},
        {"eigen", {"gtest", "benchmark"}}
    };
}

std::vector<PackageRecommendation> RecommendationEngine::generate_recommendations(
    const ProjectAnalysis& analysis,
    const std::string& category_filter,
    const std::string& performance_filter,
    const std::string& security_filter
) {
    std::vector<PackageRecommendation> all_recommendations;
    
    // 1. 基于项目类型的推荐（权重最高）
    auto type_recommendations = get_type_based_recommendations(analysis.project_type);
    for (auto& rec : type_recommendations) {
        rec.confidence *= 1.4; // 项目类型推荐权重更高
    }
    all_recommendations.insert(all_recommendations.end(), type_recommendations.begin(), type_recommendations.end());
    
    // 2. 基于现有依赖的推荐
    auto dep_recommendations = get_dependency_based_recommendations(analysis.existing_dependencies);
    for (auto& rec : dep_recommendations) {
        rec.confidence *= 1.3; // 依赖相关推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), dep_recommendations.begin(), dep_recommendations.end());
    
    // 3. 基于性能要求的推荐
    auto perf_recommendations = get_performance_based_recommendations(analysis.performance_requirements);
    for (auto& rec : perf_recommendations) {
        rec.confidence *= 1.2; // 性能要求推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), perf_recommendations.begin(), perf_recommendations.end());
    
    // 4. 基于安全要求的推荐
    auto sec_recommendations = get_security_based_recommendations(analysis.security_requirements);
    for (auto& rec : sec_recommendations) {
        rec.confidence *= 1.2; // 安全要求推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), sec_recommendations.begin(), sec_recommendations.end());
    
    // 5. 基于测试要求的推荐
    auto test_recommendations = get_testing_based_recommendations(analysis.testing_requirements);
    for (auto& rec : test_recommendations) {
        rec.confidence *= 1.1; // 测试要求推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), test_recommendations.begin(), test_recommendations.end());
    
    // 6. 基于代码模式的推荐
    auto pattern_recommendations = get_pattern_based_recommendations(analysis.code_patterns);
    for (auto& rec : pattern_recommendations) {
        rec.confidence *= 1.2; // 代码模式推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), pattern_recommendations.begin(), pattern_recommendations.end());
    
    // 7. 基于GitHub热门包的推荐
    auto github_recommendations = get_github_based_recommendations(analysis);
    for (auto& rec : github_recommendations) {
        rec.confidence *= 1.3; // GitHub热门包推荐权重高
    }
    all_recommendations.insert(all_recommendations.end(), github_recommendations.begin(), github_recommendations.end());
    
    // 8. 基于相似项目的推荐
    auto similar_recommendations = get_similar_project_recommendations(analysis);
    for (auto& rec : similar_recommendations) {
        rec.confidence *= 1.2; // 相似项目推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), similar_recommendations.begin(), similar_recommendations.end());
    
    // 9. 基于机器学习特征推荐
    auto ml_recommendations = get_ml_based_recommendations(analysis);
    for (auto& rec : ml_recommendations) {
        rec.confidence *= 1.4; // 机器学习推荐权重高
    }
    all_recommendations.insert(all_recommendations.end(), ml_recommendations.begin(), ml_recommendations.end());
    
    // 10. 基于代码质量推荐
    auto quality_recommendations = get_quality_based_recommendations(analysis);
    for (auto& rec : quality_recommendations) {
        rec.confidence *= 1.3; // 代码质量推荐权重高
    }
    all_recommendations.insert(all_recommendations.end(), quality_recommendations.begin(), quality_recommendations.end());
    
    // 11. 基于架构模式推荐
    auto architecture_recommendations = get_architecture_based_recommendations(analysis);
    for (auto& rec : architecture_recommendations) {
        rec.confidence *= 1.2; // 架构模式推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), architecture_recommendations.begin(), architecture_recommendations.end());
    
    // 12. 基于复杂度推荐
    auto complexity_recommendations_new = get_complexity_based_recommendations_new(analysis.complexity_metrics);
    for (auto& rec : complexity_recommendations_new) {
        rec.confidence *= 1.1; // 复杂度推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), complexity_recommendations_new.begin(), complexity_recommendations_new.end());
    
    // 9. 基于C++标准的推荐
    auto standard_recommendations = get_standard_based_recommendations(analysis.cpp_standard);
    for (auto& rec : standard_recommendations) {
        rec.confidence *= 1.1; // C++标准推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), standard_recommendations.begin(), standard_recommendations.end());
    
    // 10. 基于构建系统的推荐
    auto build_recommendations = get_build_system_recommendations(analysis.build_system);
    for (auto& rec : build_recommendations) {
        rec.confidence *= 1.1; // 构建系统推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), build_recommendations.begin(), build_recommendations.end());
    
    // 11. 基于项目复杂度的推荐
    auto complexity_recommendations = get_complexity_based_recommendations(analysis.feature_scores);
    for (auto& rec : complexity_recommendations) {
        rec.confidence *= 1.1; // 项目复杂度推荐权重稍高
    }
    all_recommendations.insert(all_recommendations.end(), complexity_recommendations.begin(), complexity_recommendations.end());
    
    // 12. 基于项目特征的智能推荐
    auto feature_recommendations = get_feature_based_recommendations(analysis);
    for (auto& rec : feature_recommendations) {
        rec.confidence *= 1.3; // 项目特征推荐权重高
    }
    all_recommendations.insert(all_recommendations.end(), feature_recommendations.begin(), feature_recommendations.end());
    
    // 合并和去重
    auto merged_recommendations = merge_recommendations({all_recommendations});
    
    // 过滤
    auto filtered_recommendations = filter_recommendations(
        merged_recommendations, category_filter, performance_filter, security_filter
    );
    
    // 智能排序
    auto ranked_recommendations = rank_recommendations(filtered_recommendations, analysis);
    
    // 限制推荐数量，只返回前10个最相关的
    if (ranked_recommendations.size() > 10) {
        ranked_recommendations.resize(10);
    }
    
    return ranked_recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_type_based_recommendations(
    const std::string& project_type
) {
    auto it = package_knowledge_base_.find(project_type);
    if (it != package_knowledge_base_.end()) {
        return it->second;
    }
    
    // 如果找不到特定类型，返回通用推荐
    return package_knowledge_base_["general"];
}

std::vector<PackageRecommendation> RecommendationEngine::get_dependency_based_recommendations(
    const std::vector<std::string>& existing_deps
) {
    std::vector<PackageRecommendation> recommendations;
    
    for (const auto& dep : existing_deps) {
        auto it = dependency_relationships_.find(dep);
        if (it != dependency_relationships_.end()) {
            for (const auto& related_dep : it->second) {
                // 创建相关依赖的推荐
                PackageRecommendation rec;
                rec.name = related_dep;
                rec.description = "与 " + dep + " 相关的包";
                rec.reason = "经常与 " + dep + " 一起使用";
                rec.confidence = 0.8;
                rec.compatibility = 0.9;
                rec.popularity = 0.7;
                rec.maintenance = 0.8;
                rec.priority = "medium";
                rec.install_command = "Paker add " + related_dep;
                recommendations.push_back(rec);
            }
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_performance_based_recommendations(
    const std::string& performance_level
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (performance_level == "high") {
        recommendations.push_back({"boost", "C++扩展库集合", "高性能库集合", "utility", 0.9, 0.85, 0.90, 0.85, "high", {"performance", "optimized"}, "Paker add boost"});
        recommendations.push_back({"eigen", "线性代数库", "高性能矩阵运算", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "performance"}, "Paker add eigen"});
        recommendations.push_back({"openmp", "并行计算", "多线程并行", "parallel", 0.85, 0.80, 0.75, 0.70, "high", {"parallel", "performance"}, "Paker add openmp"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_security_based_recommendations(
    const std::string& security_level
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (security_level == "high") {
        recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"crypto", "ssl", "tls"}, "Paker add openssl"});
        recommendations.push_back({"libsodium", "现代加密库", "易用的加密API", "security", 0.90, 0.85, 0.80, 0.75, "high", {"crypto", "modern", "easy"}, "Paker add libsodium"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_testing_based_recommendations(
    const std::string& testing_level
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (testing_level == "high") {
        recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test"}, "Paker add gtest"});
        recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp"}, "Paker add catch2"});
        recommendations.push_back({"benchmark", "Google基准测试", "性能测试框架", "testing", 0.85, 0.80, 0.80, 0.75, "medium", {"benchmark", "performance"}, "Paker add benchmark"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_pattern_based_recommendations(
    const std::vector<std::string>& code_patterns
) {
    std::vector<PackageRecommendation> recommendations;
    
    for (const auto& pattern : code_patterns) {
        if (pattern == "async_io") {
            recommendations.push_back({"boost-asio", "异步I/O库", "适合异步编程", "async", 0.9, 0.95, 0.90, 0.85, "high", {"async", "io", "boost"}, "Paker add boost-asio"});
            recommendations.push_back({"libuv", "跨平台异步I/O", "高性能异步库", "async", 0.85, 0.90, 0.80, 0.75, "high", {"async", "cross-platform"}, "Paker add libuv"});
        } else if (pattern == "network_programming") {
            recommendations.push_back({"cpp-httplib", "HTTP库", "简单HTTP客户端/服务器", "network", 0.9, 0.95, 0.85, 0.80, "high", {"http", "network"}, "Paker add cpp-httplib"});
            recommendations.push_back({"curl", "网络传输库", "强大的网络库", "network", 0.95, 0.90, 0.95, 0.90, "high", {"network", "http", "ftp"}, "Paker add curl"});
        } else if (pattern == "concurrent_programming") {
            recommendations.push_back({"tbb", "Intel线程构建块", "并行计算库", "parallel", 0.9, 0.85, 0.80, 0.75, "high", {"parallel", "threading"}, "Paker add tbb"});
            recommendations.push_back({"openmp", "OpenMP", "并行计算", "parallel", 0.85, 0.80, 0.75, 0.70, "medium", {"parallel", "openmp"}, "Paker add openmp"});
        } else if (pattern == "template_programming") {
            recommendations.push_back({"boost-hana", "元编程库", "现代C++元编程", "meta", 0.8, 0.75, 0.70, 0.65, "medium", {"metaprogramming", "template"}, "Paker add boost-hana"});
            recommendations.push_back({"magic_enum", "枚举反射", "枚举到字符串转换", "utility", 0.85, 0.90, 0.80, 0.75, "medium", {"enum", "reflection"}, "Paker add magic_enum"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_standard_based_recommendations(
    const std::string& cpp_standard
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (cpp_standard == "c++20") {
        recommendations.push_back({"ranges-v3", "范围库", "C++20范围的前身", "utility", 0.9, 0.85, 0.80, 0.75, "high", {"ranges", "c++20"}, "Paker add ranges-v3"});
        recommendations.push_back({"concepts", "概念库", "C++20概念支持", "utility", 0.85, 0.80, 0.75, 0.70, "medium", {"concepts", "c++20"}, "Paker add concepts"});
    } else if (cpp_standard == "c++17") {
        recommendations.push_back({"std17", "C++17特性", "C++17标准库扩展", "utility", 0.8, 0.90, 0.85, 0.80, "medium", {"c++17", "standard"}, "Paker add std17"});
        recommendations.push_back({"optional", "可选值", "C++17 std::optional", "utility", 0.85, 0.95, 0.90, 0.85, "high", {"optional", "c++17"}, "Paker add optional"});
    } else if (cpp_standard == "c++14") {
        recommendations.push_back({"std14", "C++14特性", "C++14标准库", "utility", 0.75, 0.85, 0.80, 0.75, "medium", {"c++14", "standard"}, "Paker add std14"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_build_system_recommendations(
    const std::string& build_system
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (build_system == "cmake") {
        recommendations.push_back({"cmake", "CMake构建系统", "跨平台构建工具", "build", 0.95, 0.95, 0.95, 0.90, "high", {"cmake", "build", "cross-platform"}, "Paker add cmake"});
        recommendations.push_back({"cmake-modules", "CMake模块", "常用CMake模块", "build", 0.85, 0.90, 0.80, 0.75, "medium", {"cmake", "modules", "utilities"}, "Paker add cmake-modules"});
    } else if (build_system == "make") {
        recommendations.push_back({"make", "Make构建系统", "传统构建工具", "build", 0.90, 0.95, 0.90, 0.85, "high", {"make", "build", "traditional"}, "Paker add make"});
    } else if (build_system == "meson") {
        recommendations.push_back({"meson", "Meson构建系统", "现代构建工具", "build", 0.85, 0.80, 0.75, 0.70, "medium", {"meson", "build", "modern"}, "Paker add meson"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_complexity_based_recommendations(
    const std::map<std::string, double>& feature_scores
) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于复杂度推荐工具包
    auto complexity_it = feature_scores.find("complexity");
    if (complexity_it != feature_scores.end()) {
        double complexity = complexity_it->second;
        
        if (complexity > 0.7) {
            // 高复杂度项目推荐高级工具
            recommendations.push_back({"boost", "C++扩展库集合", "功能丰富的库集合", "utility", 0.90, 0.85, 0.90, 0.85, "high", {"utilities", "extensions", "comprehensive"}, "Paker add boost"});
            recommendations.push_back({"fmt", "现代C++格式化库", "类型安全的格式化", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt"});
        } else if (complexity > 0.4) {
            // 中等复杂度项目推荐基础工具
            recommendations.push_back({"spdlog", "快速C++日志库", "高性能日志记录", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog"});
            recommendations.push_back({"nlohmann-json", "现代C++ JSON库", "易用的JSON处理", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json"});
        } else {
            // 低复杂度项目推荐简单工具
            recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::merge_recommendations(
    const std::vector<std::vector<PackageRecommendation>>& recommendations
) {
    std::vector<PackageRecommendation> merged;
    std::map<std::string, PackageRecommendation> unique_recommendations;
    
    for (const auto& rec_list : recommendations) {
        for (const auto& rec : rec_list) {
            auto it = unique_recommendations.find(rec.name);
            if (it != unique_recommendations.end()) {
                // 如果已存在，选择置信度更高的
                if (rec.confidence > it->second.confidence) {
                    it->second = rec;
                }
            } else {
                unique_recommendations[rec.name] = rec;
            }
        }
    }
    
    for (const auto& [name, rec] : unique_recommendations) {
        merged.push_back(rec);
    }
    
    return merged;
}

std::vector<PackageRecommendation> RecommendationEngine::rank_recommendations(
    const std::vector<PackageRecommendation>& recommendations,
    const ProjectAnalysis& analysis
) {
    std::vector<PackageRecommendation> ranked = recommendations;
    
    std::sort(ranked.begin(), ranked.end(), [&](const auto& a, const auto& b) {
        double score_a = calculate_recommendation_score(a, analysis);
        double score_b = calculate_recommendation_score(b, analysis);
        return score_a > score_b;
    });
    
    return ranked;
}

std::vector<PackageRecommendation> RecommendationEngine::filter_recommendations(
    const std::vector<PackageRecommendation>& recommendations,
    const std::string& category_filter,
    const std::string& performance_filter,
    const std::string& security_filter
) {
    std::vector<PackageRecommendation> filtered;
    
    for (const auto& rec : recommendations) {
        bool include = true;
        
        if (!category_filter.empty() && rec.category != category_filter) {
            include = false;
        }
        
        if (!performance_filter.empty()) {
            if (performance_filter == "high" && rec.popularity < 0.8) {
                include = false;
            } else if (performance_filter == "medium" && (rec.popularity < 0.6 || rec.popularity > 0.8)) {
                include = false;
            } else if (performance_filter == "low" && rec.popularity > 0.6) {
                include = false;
            }
        }
        
        if (!security_filter.empty()) {
            if (security_filter == "high" && rec.maintenance < 0.8) {
                include = false;
            } else if (security_filter == "medium" && (rec.maintenance < 0.6 || rec.maintenance > 0.8)) {
                include = false;
            } else if (security_filter == "low" && rec.maintenance > 0.6) {
                include = false;
            }
        }
        
        if (include) {
            filtered.push_back(rec);
        }
    }
    
    return filtered;
}

double RecommendationEngine::calculate_recommendation_score(
    const PackageRecommendation& recommendation,
    const ProjectAnalysis& analysis
) {
    double score = 0.0;
    
    // 基础评分（调整权重）
    score += recommendation.confidence * 0.35;      // 置信度权重最高
    score += recommendation.compatibility * 0.25;   // 兼容性权重
    score += recommendation.popularity * 0.20;      // 流行度权重
    score += recommendation.maintenance * 0.20;     // 维护性权重
    
    // 优先级加权
    if (recommendation.priority == "high") {
        score += 0.15;
    } else if (recommendation.priority == "medium") {
        score += 0.08;
    } else {
        score += 0.03;
    }
    
    // 基于项目类型的匹配度加权（更精确的匹配）
    if (recommendation.category == "gui" && analysis.project_type == "desktop_application") {
        score += 0.12;
    } else if (recommendation.category == "web" && analysis.project_type == "web_application") {
        score += 0.12;
    } else if (recommendation.category == "graphics" && analysis.project_type == "game_engine") {
        score += 0.12;
    } else if (recommendation.category == "math" && analysis.project_type == "scientific_computing") {
        score += 0.12;
    } else if (recommendation.category == "ml" && analysis.project_type == "machine_learning") {
        score += 0.12;
    }
    
    // 基于性能要求的匹配
    if (analysis.performance_requirements == "high") {
        if (recommendation.category == "performance" || recommendation.category == "async") {
            score += 0.08;
        }
        if (recommendation.name.find("boost") != std::string::npos || 
            recommendation.name.find("eigen") != std::string::npos) {
            score += 0.05;
        }
    }
    
    // 基于安全要求的匹配
    if (analysis.security_requirements == "high" && recommendation.category == "security") {
        score += 0.08;
    }
    
    // 基于测试要求的匹配
    if (analysis.testing_requirements == "high" && recommendation.category == "testing") {
        score += 0.08;
    }
    
    // 基于C++标准的匹配
    if (analysis.cpp_standard == "c++20" && recommendation.name.find("ranges") != std::string::npos) {
        score += 0.06;
    } else if (analysis.cpp_standard == "c++17" && recommendation.name.find("optional") != std::string::npos) {
        score += 0.06;
    }
    
    // 基于构建系统的匹配
    if (analysis.build_system == "cmake" && recommendation.name.find("cmake") != std::string::npos) {
        score += 0.05;
    }
    
    // 基于代码模式的匹配
    for (const auto& pattern : analysis.code_patterns) {
        if (pattern == "async_io" && recommendation.name.find("asio") != std::string::npos) {
            score += 0.06;
        } else if (pattern == "network_programming" && recommendation.category == "network") {
            score += 0.06;
        } else if (pattern == "concurrent_programming" && recommendation.category == "parallel") {
            score += 0.06;
        }
    }
    
    // 基于项目复杂度的匹配
    auto complexity_it = analysis.feature_scores.find("complexity");
    if (complexity_it != analysis.feature_scores.end()) {
        double complexity = complexity_it->second;
        if (complexity > 0.7 && recommendation.name == "boost") {
            score += 0.05;
        } else if (complexity < 0.3 && recommendation.name == "catch2") {
            score += 0.05;
        }
    }
    
    return std::min(score, 1.0); // 确保分数不超过1.0
}

double RecommendationEngine::check_package_compatibility(
    const std::string& /* package_name */,
    const ProjectAnalysis& /* analysis */
) {
    // 简化的兼容性检查
    // 实际实现中应该检查版本兼容性、依赖冲突等
    return 0.9; // 默认兼容性
}

std::map<std::string, std::string> RecommendationEngine::get_package_metadata(
    const std::string& package_name
) {
    auto it = package_metadata_cache_.find(package_name);
    if (it != package_metadata_cache_.end()) {
        return it->second;
    }
    
    // 简化的元数据获取
    std::map<std::string, std::string> metadata;
    metadata["name"] = package_name;
    metadata["version"] = "latest";
    metadata["description"] = "Package description";
    
    package_metadata_cache_[package_name] = metadata;
    return metadata;
}

std::vector<PackageRecommendation> RecommendationEngine::get_feature_based_recommendations(
    const ProjectAnalysis& analysis
) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于项目特征的智能推荐
    if (analysis.project_type == "machine_learning") {
        // 机器学习项目特征推荐
        if (analysis.code_patterns.empty() || 
            std::find(analysis.code_patterns.begin(), analysis.code_patterns.end(), "computer_vision") != analysis.code_patterns.end()) {
            recommendations.push_back({"opencv", "计算机视觉库", "图像处理和计算机视觉", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "ml"}, "Paker add opencv"});
        }
        
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"tensorflow", "机器学习框架", "深度学习框架", "ml", 0.90, 0.85, 0.90, 0.85, "high", {"deep-learning", "neural-networks", "ai"}, "Paker add tensorflow"});
        }
        
        if (analysis.testing_requirements == "high") {
            recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest"});
        }
    }
    
    if (analysis.project_type == "web_application") {
        // Web应用特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"boost-beast", "高性能HTTP和WebSocket库", "适合高性能Web应用", "web", 0.95, 0.90, 0.90, 0.85, "high", {"http", "websocket", "async"}, "Paker add boost-beast"});
        }
        
        if (analysis.security_requirements == "high") {
            recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"security", "crypto", "ssl"}, "Paker add openssl"});
        }
        
        if (std::find(analysis.code_patterns.begin(), analysis.code_patterns.end(), "async_io") != analysis.code_patterns.end()) {
            recommendations.push_back({"libuv", "跨平台异步I/O", "高性能异步I/O", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "io", "performance", "nodejs"}, "Paker add libuv"});
        }
    }
    
    if (analysis.project_type == "desktop_application") {
        // 桌面应用特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"qt", "跨平台GUI框架", "功能强大的GUI框架", "gui", 0.95, 0.90, 0.95, 0.90, "high", {"gui", "cross-platform", "widgets"}, "Paker add qt"});
        }
        
        if (analysis.testing_requirements == "high") {
            recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2"});
        }
    }
    
    if (analysis.project_type == "game_engine") {
        // 游戏引擎特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"vulkan", "现代图形API", "高性能3D渲染", "graphics", 0.85, 0.80, 0.75, 0.70, "high", {"3d", "high-performance", "modern"}, "Paker add vulkan"});
        } else {
            recommendations.push_back({"sdl2", "跨平台多媒体库", "游戏开发基础库", "graphics", 0.95, 0.90, 0.95, 0.90, "high", {"graphics", "audio", "input"}, "Paker add sdl2"});
        }
        
        if (std::find(analysis.code_patterns.begin(), analysis.code_patterns.end(), "3d_rendering") != analysis.code_patterns.end()) {
            recommendations.push_back({"glm", "OpenGL数学库", "3D数学运算", "math", 0.90, 0.95, 0.90, 0.85, "high", {"math", "graphics", "vectors"}, "Paker add glm"});
        }
    }
    
    if (analysis.project_type == "scientific_computing") {
        // 科学计算特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"eigen", "线性代数库", "矩阵和向量运算", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "matrix", "vector"}, "Paker add eigen"});
        }
        
        if (analysis.cpp_standard == "c++17" || analysis.cpp_standard == "c++20") {
            recommendations.push_back({"fmt", "现代C++格式化库", "类型安全的格式化", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt"});
        }
    }
    
    // 通用推荐
    if (analysis.testing_requirements == "high") {
        recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest"});
    }
    
    if (analysis.performance_requirements == "high") {
        recommendations.push_back({"spdlog", "快速C++日志库", "高性能日志记录", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog"});
    }
    
    if (analysis.security_requirements == "high") {
        recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"security", "crypto", "ssl"}, "Paker add openssl"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_github_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于GitHub热门包推荐
    for (const auto& trending_package : analysis.trending_packages) {
        PackageRecommendation rec;
        rec.name = trending_package;
        rec.description = "Trending package from GitHub";
        rec.reason = "Popular in similar projects on GitHub";
        rec.category = "trending";
        rec.confidence = 0.85;
        rec.compatibility = 0.80;
        rec.popularity = 0.95; // 热门包流行度高
        rec.maintenance = 0.85;
        rec.priority = "high";
        rec.tags = {"trending", "github", "popular"};
        rec.install_command = "Paker add " + trending_package;
        
        recommendations.push_back(rec);
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_similar_project_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于相似项目推荐
    for (const auto& similar_project : analysis.similar_projects) {
        // 从相似项目提取包名
        std::string package_name = extract_package_from_project(similar_project);
        if (!package_name.empty()) {
            PackageRecommendation rec;
            rec.name = package_name;
            rec.description = "Used in similar projects";
            rec.reason = "Found in similar GitHub projects";
            rec.category = "similar";
            rec.confidence = 0.80;
            rec.compatibility = 0.85;
            rec.popularity = 0.80;
            rec.maintenance = 0.80;
            rec.priority = "medium";
            rec.tags = {"similar", "github", "community"};
            rec.install_command = "Paker add " + package_name;
            
            recommendations.push_back(rec);
        }
    }
    
    return recommendations;
}

std::string RecommendationEngine::extract_package_from_project(const std::string& project_name) {
    // 从项目名提取包名
    std::string package_name = project_name;
    
    // 移除用户名前缀 (user/repo -> repo)
    size_t slash_pos = package_name.find('/');
    if (slash_pos != std::string::npos) {
        package_name = package_name.substr(slash_pos + 1);
    }
    
    // 移除常见后缀
    std::vector<std::string> suffixes = {"-cpp", "-cxx", "-c++", "-lib", "-library"};
    for (const auto& suffix : suffixes) {
        if (package_name.length() > suffix.length() && 
            package_name.substr(package_name.length() - suffix.length()) == suffix) {
            package_name = package_name.substr(0, package_name.length() - suffix.length());
            break;
        }
    }
    
    return package_name;
}

// 新增智能推荐算法
std::vector<PackageRecommendation> RecommendationEngine::get_ml_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于机器学习特征推荐
    for (const auto& feature : analysis.ml_features) {
        if (feature == "neural_network" || feature == "deep_learning") {
            recommendations.push_back({"tensorflow", "Deep learning framework", "Neural network implementation", "ml", 0.95, 0.90, 0.90, 0.85, "high", {"neural-network", "deep-learning", "ai"}, "Paker add tensorflow"});
            recommendations.push_back({"pytorch", "Dynamic neural networks", "Research-friendly ML", "ml", 0.90, 0.85, 0.85, 0.80, "high", {"neural-network", "research", "dynamic"}, "Paker add pytorch"});
        }
        
        if (feature == "computer_vision" || feature == "image_processing") {
            recommendations.push_back({"opencv", "Computer vision library", "Image processing and CV", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "opencv"}, "Paker add opencv"});
        }
        
        if (feature == "optimization" || feature == "gradient_descent") {
            recommendations.push_back({"eigen", "Linear algebra library", "Mathematical optimization", "math", 0.90, 0.85, 0.85, 0.80, "high", {"linear-algebra", "optimization", "matrix"}, "Paker add eigen"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_quality_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于代码质量评分推荐
    if (analysis.code_quality_score < 0.3) {
        // 代码质量较低，推荐现代化工具
        recommendations.push_back({"fmt", "Modern C++ formatting", "Type-safe string formatting", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"modern-cpp", "formatting", "type-safe"}, "Paker add fmt"});
        recommendations.push_back({"spdlog", "Fast logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "performance", "modern"}, "Paker add spdlog"});
    }
    
    if (analysis.code_quality_score < 0.5) {
        // 推荐测试框架
        recommendations.push_back({"gtest", "Google Test framework", "Unit testing framework", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "quality"}, "Paker add gtest"});
        recommendations.push_back({"catch2", "Modern C++ testing", "Simple testing framework", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_architecture_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于架构模式推荐
    for (const auto& pattern : analysis.architecture_patterns) {
        if (pattern == "microservice" || pattern == "soa") {
            recommendations.push_back({"grpc", "gRPC framework", "High-performance RPC", "rpc", 0.90, 0.85, 0.80, 0.75, "high", {"rpc", "microservice", "grpc"}, "Paker add grpc"});
            recommendations.push_back({"protobuf", "Protocol Buffers", "Efficient serialization", "serialization", 0.95, 0.90, 0.90, 0.85, "high", {"serialization", "protobuf", "efficient"}, "Paker add protobuf"});
        }
        
        if (pattern == "event_driven" || pattern == "reactive") {
            recommendations.push_back({"libuv", "Event-driven I/O", "Asynchronous programming", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "event-driven", "io"}, "Paker add libuv"});
            recommendations.push_back({"asio", "Boost.Asio", "Asynchronous I/O", "async", 0.95, 0.90, 0.90, 0.85, "high", {"async", "networking", "boost"}, "Paker add asio"});
        }
        
        if (pattern == "plugin" || pattern == "component") {
            recommendations.push_back({"dlfcn", "Dynamic loading", "Plugin system support", "plugin", 0.85, 0.80, 0.75, 0.70, "medium", {"plugin", "dynamic", "loading"}, "Paker add dlfcn"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_complexity_based_recommendations_new(const std::map<std::string, double>& complexity_metrics) {
    std::vector<PackageRecommendation> recommendations;
    
    auto complexity_score = complexity_metrics.find("complexity_score");
    if (complexity_score != complexity_metrics.end()) {
        double score = complexity_score->second;
        
        if (score > 0.1) {
            // 高复杂度项目，推荐调试和性能工具
            recommendations.push_back({"gdb", "GNU Debugger", "Advanced debugging", "debug", 0.90, 0.85, 0.80, 0.75, "high", {"debug", "gdb", "development"}, "Paker add gdb"});
            recommendations.push_back({"valgrind", "Memory debugging", "Memory leak detection", "debug", 0.85, 0.80, 0.75, 0.70, "high", {"memory", "debug", "valgrind"}, "Paker add valgrind"});
            recommendations.push_back({"perf", "Performance analysis", "CPU profiling", "profiling", 0.80, 0.75, 0.70, 0.65, "medium", {"profiling", "performance", "analysis"}, "Paker add perf"});
        }
        
        if (score > 0.05) {
            // 中等复杂度，推荐代码分析工具
            recommendations.push_back({"clang-tidy", "Static analysis", "Code quality analysis", "analysis", 0.85, 0.80, 0.75, 0.70, "high", {"static-analysis", "quality", "clang"}, "Paker add clang-tidy"});
            recommendations.push_back({"cppcheck", "Static analysis", "Bug detection", "analysis", 0.80, 0.75, 0.70, 0.65, "medium", {"static-analysis", "bugs", "cppcheck"}, "Paker add cppcheck"});
        }
    }
    
    return recommendations;
}

} // namespace Analysis
} // namespace Paker
