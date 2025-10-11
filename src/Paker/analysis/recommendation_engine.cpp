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
        {"boost-beast", "High-performance HTTP and WebSocket library", "Perfect for high-performance web applications", "web", 0.95, 0.90, 0.90, 0.85, "high", {"http", "websocket", "async"}, "Paker add boost-beast", "https://github.com/boostorg/beast"},
        {"crow", "Lightweight C++ web framework", "Simple and easy-to-use web framework", "web", 0.90, 0.85, 0.85, 0.80, "high", {"web", "framework", "rest"}, "Paker add crow", "https://github.com/CrowCpp/Crow"},
        {"cpp-httplib", "Single-header HTTP library", "Simple HTTP client/server", "web", 0.85, 0.95, 0.80, 0.75, "medium", {"http", "simple", "header-only"}, "Paker add cpp-httplib", "https://github.com/yhirose/cpp-httplib"},
        {"pistache", "Modern C++ HTTP framework", "RESTful API framework", "web", 0.88, 0.80, 0.75, 0.70, "medium", {"rest", "api", "modern"}, "Paker add pistache", "https://github.com/oktal/pistache"},
        {"spdlog", "Fast C++ logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog", "https://github.com/gabime/spdlog"},
        {"nlohmann-json", "Modern C++ JSON library", "Easy-to-use JSON processing", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json", "https://github.com/nlohmann/json"}
    };
    
    // 桌面应用相关包
    package_knowledge_base_["desktop_application"] = {
        {"qt", "Cross-platform GUI framework", "Powerful and feature-rich GUI framework", "gui", 0.95, 0.90, 0.95, 0.90, "high", {"gui", "cross-platform", "widgets"}, "Paker add qt", "https://github.com/qtproject/qt"},
        {"gtkmm", "GTK+ C++ bindings", "Native Linux GUI", "gui", 0.85, 0.80, 0.70, 0.75, "medium", {"gui", "linux", "gtk"}, "Paker add gtkmm", "https://github.com/GNOME/gtkmm"},
        {"wxwidgets", "Native GUI framework", "Cross-platform native look", "gui", 0.80, 0.85, 0.75, 0.80, "medium", {"gui", "native", "cross-platform"}, "Paker add wxwidgets", "https://github.com/wxWidgets/wxWidgets"},
        {"fltk", "Lightweight GUI library", "Fast and lightweight GUI", "gui", 0.75, 0.90, 0.65, 0.70, "low", {"gui", "lightweight", "fast"}, "Paker add fltk", "https://github.com/fltk/fltk"}
    };
    
    // 游戏引擎相关包
    package_knowledge_base_["game_engine"] = {
        {"sdl2", "Cross-platform multimedia library", "Essential for game development", "graphics", 0.95, 0.90, 0.95, 0.90, "high", {"graphics", "audio", "input"}, "Paker add sdl2", "https://github.com/libsdl-org/SDL"},
        {"sfml", "Simple and fast multimedia library", "Perfect for 2D game development", "graphics", 0.90, 0.85, 0.85, 0.80, "high", {"graphics", "2d", "simple"}, "Paker add sfml", "https://github.com/SFML/SFML"},
        {"opengl", "Graphics rendering API", "3D graphics rendering", "graphics", 0.95, 0.90, 0.90, 0.85, "high", {"3d", "graphics", "rendering"}, "Paker add opengl", "https://github.com/KhronosGroup/OpenGL-Registry"},
        {"vulkan", "Modern graphics API", "High-performance 3D rendering", "graphics", 0.85, 0.80, 0.75, 0.70, "high", {"3d", "high-performance", "modern"}, "Paker add vulkan", "https://github.com/KhronosGroup/Vulkan-Headers"}
    };
    
    // 科学计算相关包
    package_knowledge_base_["scientific_computing"] = {
        {"eigen", "Linear algebra library", "Matrix and vector operations", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "matrix", "vector"}, "Paker add eigen", "https://github.com/eigenteam/eigen-git-mirror"},
        {"armadillo", "C++ linear algebra library", "Advanced linear algebra", "math", 0.90, 0.85, 0.80, 0.75, "medium", {"linear-algebra", "matlab-like"}, "Paker add armadillo", "https://github.com/conradsnicta/armadillo-code"},
        {"gsl", "GNU Scientific Library", "Numerical computation functions", "math", 0.85, 0.80, 0.75, 0.70, "medium", {"numerical", "scientific", "gnu"}, "Paker add gsl", "https://github.com/ampl/gsl"}
    };
    
    // 机器学习相关包
    package_knowledge_base_["machine_learning"] = {
        {"opencv", "Computer vision library", "Image processing and computer vision", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "ml"}, "Paker add opencv", "https://github.com/opencv/opencv"},
        {"tensorflow", "Machine learning framework", "Deep learning framework", "ml", 0.90, 0.85, 0.90, 0.85, "high", {"deep-learning", "neural-networks", "ai"}, "Paker add tensorflow", "https://github.com/tensorflow/tensorflow"},
        {"pytorch", "Dynamic neural networks", "Research-friendly ML framework", "ml", 0.85, 0.80, 0.85, 0.80, "high", {"deep-learning", "research", "dynamic"}, "Paker add pytorch", "https://github.com/pytorch/pytorch"}
    };
    
    // 通用包
    package_knowledge_base_["general"] = {
        {"fmt", "Modern C++ formatting library", "Type-safe formatting", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt", "https://github.com/fmtlib/fmt"},
        {"spdlog", "Fast C++ logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog", "https://github.com/gabime/spdlog"},
        {"nlohmann-json", "Modern C++ JSON library", "Easy-to-use JSON processing", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json", "https://github.com/nlohmann/json"},
        {"gtest", "Google Test framework", "Unit testing framework", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest", "https://github.com/google/googletest"},
        {"catch2", "Modern C++ testing framework", "Simple and easy testing", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2", "https://github.com/catchorg/Catch2"},
        {"boost", "C++ extension libraries", "Comprehensive library collection", "utility", 0.90, 0.85, 0.90, 0.85, "high", {"utilities", "extensions", "comprehensive"}, "Paker add boost", "https://github.com/boostorg/boost"}
    };
    
    // 新增专业领域包
    package_knowledge_base_["blockchain"] = {
        {"libsecp256k1", "Bitcoin cryptographic library", "Elliptic curve cryptography", "crypto", 0.90, 0.85, 0.80, 0.75, "high", {"bitcoin", "crypto", "secp256k1"}, "Paker add libsecp256k1", "https://github.com/bitcoin-core/secp256k1"},
        {"openssl", "Cryptographic library", "SSL/TLS and general cryptography", "crypto", 0.95, 0.90, 0.95, 0.90, "high", {"ssl", "tls", "crypto", "security"}, "Paker add openssl", "https://github.com/openssl/openssl"},
        {"cryptopp", "Crypto++ library", "Comprehensive cryptographic library", "crypto", 0.90, 0.85, 0.85, 0.80, "high", {"crypto", "encryption", "hashing"}, "Paker add cryptopp", "https://github.com/weidai11/cryptopp"}
    };
    
    package_knowledge_base_["database"] = {
        {"sqlite3", "SQLite database", "Embedded SQL database", "database", 0.95, 0.90, 0.95, 0.90, "high", {"sql", "embedded", "lightweight"}, "Paker add sqlite3", "https://github.com/sqlite/sqlite"},
        {"mysql-connector-cpp", "MySQL C++ connector", "MySQL database connectivity", "database", 0.85, 0.80, 0.75, 0.70, "medium", {"mysql", "database", "sql"}, "Paker add mysql-connector-cpp", "https://github.com/mysql/mysql-connector-cpp"},
        {"mongocxx", "MongoDB C++ driver", "MongoDB database connectivity", "database", 0.80, 0.75, 0.70, 0.65, "medium", {"mongodb", "nosql", "document"}, "Paker add mongocxx", "https://github.com/mongodb/mongo-cxx-driver"}
    };
    
    package_knowledge_base_["networking"] = {
        {"libuv", "Cross-platform asynchronous I/O", "Event-driven programming", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "io", "event-driven"}, "Paker add libuv", "https://github.com/libuv/libuv"},
        {"asio", "Boost.Asio networking", "Asynchronous I/O and networking", "async", 0.95, 0.90, 0.90, 0.85, "high", {"async", "networking", "boost"}, "Paker add asio", "https://github.com/boostorg/asio"},
        {"libevent", "Event notification library", "High-performance event loop", "async", 0.85, 0.80, 0.75, 0.70, "medium", {"event", "async", "network"}, "Paker add libevent", "https://github.com/libevent/libevent"}
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
    
    // 1. 基于GitHub热门包的推荐（权重最高 - 主要推荐来源）
    auto github_recommendations = get_github_based_recommendations(analysis);
    for (auto& rec : github_recommendations) {
        rec.confidence *= 3.0; // GitHub热门包推荐权重最高
    }
    all_recommendations.insert(all_recommendations.end(), github_recommendations.begin(), github_recommendations.end());
    
    // 2. 基于相似项目的推荐（权重很高 - 主要推荐来源）
    auto similar_recommendations = get_similar_project_recommendations(analysis);
    for (auto& rec : similar_recommendations) {
        rec.confidence *= 2.8; // 相似项目推荐权重很高
    }
    all_recommendations.insert(all_recommendations.end(), similar_recommendations.begin(), similar_recommendations.end());
    
    // 3-16. 硬编码推荐（几乎完全禁用，权重极低）
    // 只有在GitHub推荐不足时才使用硬编码推荐作为补充
    
    // 检查GitHub推荐数量，如果不足则添加少量硬编码推荐
    if (github_recommendations.size() + similar_recommendations.size() < 5) {
        // 基于项目类型的推荐（权重极低）
        auto type_recommendations = get_type_based_recommendations(analysis.project_type);
        for (auto& rec : type_recommendations) {
            rec.confidence *= 0.05; // 项目类型推荐权重极低
        }
        all_recommendations.insert(all_recommendations.end(), type_recommendations.begin(), type_recommendations.end());
        
        // 基于现有依赖的推荐（权重极低）
        auto dep_recommendations = get_dependency_based_recommendations(analysis.existing_dependencies);
        for (auto& rec : dep_recommendations) {
            rec.confidence *= 0.05; // 依赖相关推荐权重极低
        }
        all_recommendations.insert(all_recommendations.end(), dep_recommendations.begin(), dep_recommendations.end());
    }
    
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
        recommendations.push_back({"boost", "C++扩展库集合", "高性能库集合", "utility", 0.9, 0.85, 0.90, 0.85, "high", {"performance", "optimized"}, "Paker add boost", "https://github.com/boostorg/boost"});
        recommendations.push_back({"eigen", "线性代数库", "高性能矩阵运算", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "performance"}, "Paker add eigen", "https://github.com/eigenteam/eigen-git-mirror"});
        recommendations.push_back({"openmp", "并行计算", "多线程并行", "parallel", 0.85, 0.80, 0.75, 0.70, "high", {"parallel", "performance"}, "Paker add openmp", "https://github.com/OpenMP/OpenMP"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_security_based_recommendations(
    const std::string& security_level
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (security_level == "high") {
        recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"crypto", "ssl", "tls"}, "Paker add openssl", "https://github.com/openssl/openssl"});
        recommendations.push_back({"libsodium", "现代加密库", "易用的加密API", "security", 0.90, 0.85, 0.80, 0.75, "high", {"crypto", "modern", "easy"}, "Paker add libsodium", "https://github.com/jedisct1/libsodium"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_testing_based_recommendations(
    const std::string& testing_level
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (testing_level == "high") {
        recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test"}, "Paker add gtest", "https://github.com/google/googletest"});
        recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp"}, "Paker add catch2", "https://github.com/catchorg/Catch2"});
        recommendations.push_back({"benchmark", "Google基准测试", "性能测试框架", "testing", 0.85, 0.80, 0.80, 0.75, "medium", {"benchmark", "performance"}, "Paker add benchmark", "https://github.com/google/benchmark"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_pattern_based_recommendations(
    const std::vector<std::string>& code_patterns
) {
    std::vector<PackageRecommendation> recommendations;
    
    for (const auto& pattern : code_patterns) {
        if (pattern == "async_io") {
            recommendations.push_back({"boost-asio", "异步I/O库", "适合异步编程", "async", 0.9, 0.95, 0.90, 0.85, "high", {"async", "io", "boost"}, "Paker add boost-asio", "https://github.com/boostorg/asio"});
            recommendations.push_back({"libuv", "跨平台异步I/O", "高性能异步库", "async", 0.85, 0.90, 0.80, 0.75, "high", {"async", "cross-platform"}, "Paker add libuv", "https://github.com/libuv/libuv"});
        } else if (pattern == "network_programming") {
            recommendations.push_back({"cpp-httplib", "HTTP库", "简单HTTP客户端/服务器", "network", 0.9, 0.95, 0.85, 0.80, "high", {"http", "network"}, "Paker add cpp-httplib", "https://github.com/yhirose/cpp-httplib"});
            recommendations.push_back({"curl", "网络传输库", "强大的网络库", "network", 0.95, 0.90, 0.95, 0.90, "high", {"network", "http", "ftp"}, "Paker add curl", "https://github.com/curl/curl"});
        } else if (pattern == "concurrent_programming") {
            recommendations.push_back({"tbb", "Intel线程构建块", "并行计算库", "parallel", 0.9, 0.85, 0.80, 0.75, "high", {"parallel", "threading"}, "Paker add tbb", "https://github.com/oneapi-src/oneTBB"});
            recommendations.push_back({"openmp", "OpenMP", "并行计算", "parallel", 0.85, 0.80, 0.75, 0.70, "medium", {"parallel", "openmp"}, "Paker add openmp", "https://github.com/OpenMP/OpenMP"});
        } else if (pattern == "template_programming") {
            recommendations.push_back({"boost-hana", "元编程库", "现代C++元编程", "meta", 0.8, 0.75, 0.70, 0.65, "medium", {"metaprogramming", "template"}, "Paker add boost-hana", "https://github.com/boostorg/hana"});
            recommendations.push_back({"magic_enum", "枚举反射", "枚举到字符串转换", "utility", 0.85, 0.90, 0.80, 0.75, "medium", {"enum", "reflection"}, "Paker add magic_enum", "https://github.com/Neargye/magic_enum"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_standard_based_recommendations(
    const std::string& cpp_standard
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (cpp_standard == "c++20") {
        recommendations.push_back({"ranges-v3", "范围库", "C++20范围的前身", "utility", 0.9, 0.85, 0.80, 0.75, "high", {"ranges", "c++20"}, "Paker add ranges-v3", "https://github.com/ericniebler/range-v3"});
        recommendations.push_back({"concepts", "概念库", "C++20概念支持", "utility", 0.85, 0.80, 0.75, 0.70, "medium", {"concepts", "c++20"}, "Paker add concepts", "https://github.com/ConceptualCpp/ConceptualCpp"});
    } else if (cpp_standard == "c++17") {
        recommendations.push_back({"std17", "C++17特性", "C++17标准库扩展", "utility", 0.8, 0.90, 0.85, 0.80, "medium", {"c++17", "standard"}, "Paker add std17", "https://github.com/microsoft/STL"});
        recommendations.push_back({"optional", "可选值", "C++17 std::optional", "utility", 0.85, 0.95, 0.90, 0.85, "high", {"optional", "c++17"}, "Paker add optional", "https://github.com/akrzemi1/Optional"});
    } else if (cpp_standard == "c++14") {
        recommendations.push_back({"std14", "C++14特性", "C++14标准库", "utility", 0.75, 0.85, 0.80, 0.75, "medium", {"c++14", "standard"}, "Paker add std14", "https://github.com/microsoft/STL"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_build_system_recommendations(
    const std::string& build_system
) {
    std::vector<PackageRecommendation> recommendations;
    
    if (build_system == "cmake") {
        recommendations.push_back({"cmake", "CMake构建系统", "跨平台构建工具", "build", 0.95, 0.95, 0.95, 0.90, "high", {"cmake", "build", "cross-platform"}, "Paker add cmake", "https://github.com/Kitware/CMake"});
        recommendations.push_back({"cmake-modules", "CMake模块", "常用CMake模块", "build", 0.85, 0.90, 0.80, 0.75, "medium", {"cmake", "modules", "utilities"}, "Paker add cmake-modules", "https://github.com/Kitware/CMake"});
    } else if (build_system == "make") {
        recommendations.push_back({"make", "Make构建系统", "传统构建工具", "build", 0.90, 0.95, 0.90, 0.85, "high", {"make", "build", "traditional"}, "Paker add make", "https://github.com/GNU-Make"});
    } else if (build_system == "meson") {
        recommendations.push_back({"meson", "Meson构建系统", "现代构建工具", "build", 0.85, 0.80, 0.75, 0.70, "medium", {"meson", "build", "modern"}, "Paker add meson", "https://github.com/mesonbuild/meson"});
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
            recommendations.push_back({"boost", "C++扩展库集合", "功能丰富的库集合", "utility", 0.90, 0.85, 0.90, 0.85, "high", {"utilities", "extensions", "comprehensive"}, "Paker add boost", "https://github.com/boostorg/boost"});
            recommendations.push_back({"fmt", "现代C++格式化库", "类型安全的格式化", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt", "https://github.com/fmtlib/fmt"});
        } else if (complexity > 0.4) {
            // 中等复杂度项目推荐基础工具
            recommendations.push_back({"spdlog", "快速C++日志库", "高性能日志记录", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog", "https://github.com/gabime/spdlog"});
            recommendations.push_back({"nlohmann-json", "现代C++ JSON库", "易用的JSON处理", "json", 0.95, 0.95, 0.95, 0.90, "high", {"json", "modern-cpp", "easy-to-use"}, "Paker add nlohmann-json", "https://github.com/nlohmann/json"});
        } else {
            // 低复杂度项目推荐简单工具
            recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2", "https://github.com/catchorg/Catch2"});
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
                // 优先保留GitHub推荐
                bool is_github_rec = false;
                bool is_existing_github = false;
                
                // 检查当前推荐是否为GitHub推荐
                for (const auto& tag : rec.tags) {
                    if (tag == "github" || tag == "github-trending" || tag == "similar-project" || 
                        tag == "trending" || tag == "real-time") {
                        is_github_rec = true;
                        break;
                    }
                }
                
                // 检查已存在的推荐是否为GitHub推荐
                for (const auto& tag : it->second.tags) {
                    if (tag == "github" || tag == "github-trending" || tag == "similar-project" || 
                        tag == "trending" || tag == "real-time") {
                        is_existing_github = true;
                        break;
                    }
                }
                
                // GitHub推荐优先级最高
                if (is_github_rec && !is_existing_github) {
                    it->second = rec; // 用GitHub推荐替换非GitHub推荐
                } else if (is_github_rec && is_existing_github) {
                    // 都是GitHub推荐，选择置信度更高的
                    if (rec.confidence > it->second.confidence) {
                        it->second = rec;
                    }
                } else if (!is_github_rec && !is_existing_github) {
                    // 都不是GitHub推荐，选择置信度更高的
                    if (rec.confidence > it->second.confidence) {
                        it->second = rec;
                    }
                }
                // 如果已存在的是GitHub推荐，新的不是，则保留GitHub推荐
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
    
    // GitHub数据权重加成（大幅提高）
    for (const auto& tag : recommendation.tags) {
        if (tag == "github-trending") {
            score += 0.60; // GitHub热门包额外加分（大幅提高）
        } else if (tag == "similar-project") {
            score += 0.55; // 相似项目包额外加分（大幅提高）
        } else if (tag == "real-time") {
            score += 0.50; // 实时数据额外加分（大幅提高）
        } else if (tag == "github") {
            score += 0.45; // GitHub推荐额外加分
        } else if (tag == "trending") {
            score += 0.48; // 趋势推荐额外加分
        }
    }
    
    // 基于GitHub数据的额外权重（大幅提高）
    if (std::find(analysis.trending_packages.begin(), analysis.trending_packages.end(), recommendation.name) != analysis.trending_packages.end()) {
        score += 0.80; // 在GitHub热门包中额外加分（大幅提高）
    }
    
    if (std::find(analysis.similar_projects.begin(), analysis.similar_projects.end(), recommendation.name) != analysis.similar_projects.end()) {
        score += 0.75; // 在相似项目中额外加分（大幅提高）
    }
    
    // GitHub URL加分
    if (!recommendation.github_url.empty() && recommendation.github_url.find("github.com") != std::string::npos) {
        score += 0.30; // 有GitHub地址额外加分
    }
    
    // GitHub搜索URL额外加分
    if (!recommendation.github_url.empty() && recommendation.github_url.find("search?q=") != std::string::npos) {
        score += 0.25; // GitHub搜索URL额外加分
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
            recommendations.push_back({"opencv", "计算机视觉库", "图像处理和计算机视觉", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "ml"}, "Paker add opencv", "https://github.com/opencv/opencv"});
        }
        
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"tensorflow", "机器学习框架", "深度学习框架", "ml", 0.90, 0.85, 0.90, 0.85, "high", {"deep-learning", "neural-networks", "ai"}, "Paker add tensorflow", "https://github.com/tensorflow/tensorflow"});
        }
        
        if (analysis.testing_requirements == "high") {
            recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest", "https://github.com/google/googletest"});
        }
    }
    
    if (analysis.project_type == "web_application") {
        // Web应用特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"boost-beast", "高性能HTTP和WebSocket库", "适合高性能Web应用", "web", 0.95, 0.90, 0.90, 0.85, "high", {"http", "websocket", "async"}, "Paker add boost-beast", "https://github.com/boostorg/beast"});
        }
        
        if (analysis.security_requirements == "high") {
            recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"security", "crypto", "ssl"}, "Paker add openssl", "https://github.com/openssl/openssl"});
        }
        
        if (std::find(analysis.code_patterns.begin(), analysis.code_patterns.end(), "async_io") != analysis.code_patterns.end()) {
            recommendations.push_back({"libuv", "跨平台异步I/O", "高性能异步I/O", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "io", "performance", "nodejs"}, "Paker add libuv", "https://github.com/libuv/libuv"});
        }
    }
    
    if (analysis.project_type == "desktop_application") {
        // 桌面应用特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"qt", "跨平台GUI框架", "功能强大的GUI框架", "gui", 0.95, 0.90, 0.95, 0.90, "high", {"gui", "cross-platform", "widgets"}, "Paker add qt", "https://github.com/qt/qtbase"});
        }
        
        if (analysis.testing_requirements == "high") {
            recommendations.push_back({"catch2", "现代C++测试框架", "简单易用的测试", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2", "https://github.com/catchorg/Catch2"});
        }
    }
    
    if (analysis.project_type == "game_engine") {
        // 游戏引擎特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"vulkan", "现代图形API", "高性能3D渲染", "graphics", 0.85, 0.80, 0.75, 0.70, "high", {"3d", "high-performance", "modern"}, "Paker add vulkan", "https://github.com/KhronosGroup/Vulkan-Headers"});
        } else {
            recommendations.push_back({"sdl2", "跨平台多媒体库", "游戏开发基础库", "graphics", 0.95, 0.90, 0.95, 0.90, "high", {"graphics", "audio", "input"}, "Paker add sdl2", "https://github.com/libsdl-org/SDL"});
        }
        
        if (std::find(analysis.code_patterns.begin(), analysis.code_patterns.end(), "3d_rendering") != analysis.code_patterns.end()) {
            recommendations.push_back({"glm", "OpenGL数学库", "3D数学运算", "math", 0.90, 0.95, 0.90, 0.85, "high", {"math", "graphics", "vectors"}, "Paker add glm", "https://github.com/g-truc/glm"});
        }
    }
    
    if (analysis.project_type == "scientific_computing") {
        // 科学计算特征推荐
        if (analysis.performance_requirements == "high") {
            recommendations.push_back({"eigen", "线性代数库", "矩阵和向量运算", "math", 0.95, 0.90, 0.90, 0.85, "high", {"linear-algebra", "matrix", "vector"}, "Paker add eigen", "https://github.com/eigenteam/eigen-git-mirror"});
        }
        
        if (analysis.cpp_standard == "c++17" || analysis.cpp_standard == "c++20") {
            recommendations.push_back({"fmt", "现代C++格式化库", "类型安全的格式化", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"formatting", "modern-cpp", "type-safe"}, "Paker add fmt", "https://github.com/fmtlib/fmt"});
        }
    }
    
    // 通用推荐
    if (analysis.testing_requirements == "high") {
        recommendations.push_back({"gtest", "Google测试框架", "单元测试框架", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "google"}, "Paker add gtest", "https://github.com/google/googletest"});
    }
    
    if (analysis.performance_requirements == "high") {
        recommendations.push_back({"spdlog", "快速C++日志库", "高性能日志记录", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "fast", "header-only"}, "Paker add spdlog", "https://github.com/gabime/spdlog"});
    }
    
    if (analysis.security_requirements == "high") {
        recommendations.push_back({"openssl", "加密库", "SSL/TLS加密", "security", 0.95, 0.90, 0.95, 0.90, "high", {"security", "crypto", "ssl"}, "Paker add openssl", "https://github.com/openssl/openssl"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_github_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于GitHub热门包推荐（增加更多包）
    std::vector<std::string> github_packages;
    
    // 根据项目类型添加更多GitHub包（大幅增加数量）
    if (analysis.project_type == "game_engine") {
        github_packages = {"sdl2", "sfml", "opengl", "vulkan", "glm", "assimp", "bullet", "box2d", "chipmunk", "raylib", "bgfx", "magnum", "ogre3d", "irrlicht", "cocos2d", "godot", "unity", "unreal", "cryengine", "lumberyard"};
    } else if (analysis.project_type == "web_application") {
        github_packages = {"cpp-httplib", "crow", "pistache", "boost-beast", "cpprestsdk", "httplib", "drogon", "oatpp", "seastar", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib"};
    } else if (analysis.project_type == "desktop_application") {
        github_packages = {"qt", "gtkmm", "wxwidgets", "fltk", "imgui", "nuklear", "dear-imgui", "nanogui", "cef", "electron", "tauri", "flutter", "gtk", "kde", "gnome", "xfce", "lxde", "mate", "cinnamon", "budgie"};
    } else if (analysis.project_type == "scientific_computing") {
        github_packages = {"eigen", "armadillo", "gsl", "fftw", "blas", "lapack", "mkl", "openblas", "intel-mkl", "cuda", "opencl", "sycl", "openmp", "mpi", "petsc", "slepc", "trilinos", "dealii", "fenics", "dolfin"};
    } else if (analysis.project_type == "machine_learning") {
        github_packages = {"opencv", "tensorflow", "pytorch", "eigen", "onnx", "tflite", "sklearn", "xgboost", "lightgbm", "catboost", "mlpack", "shark", "dlib", "torch", "caffe", "mxnet", "paddle", "mindspore", "jax", "flax"};
    } else {
        github_packages = {"fmt", "spdlog", "nlohmann-json", "gtest", "catch2", "boost", "asio", "beast", "filesystem", "range-v3", "abseil", "folly", "glog", "gflags", "protobuf", "grpc", "thrift", "zeromq", "nanomsg", "libevent"};
    }
    
    // 添加分析结果中的热门包
    for (const auto& trending_package : analysis.trending_packages) {
        github_packages.push_back(trending_package);
    }
    
    // 去重
    std::set<std::string> unique_packages(github_packages.begin(), github_packages.end());
    
    for (const auto& package_name : unique_packages) {
        // 获取GitHub包详细信息
        ProjectAnalyzer analyzer;
        auto github_info = analyzer.get_github_package_info(package_name);
        
        PackageRecommendation rec;
        rec.name = package_name;
        rec.description = github_info.found ? github_info.description : "Popular C++ library from GitHub";
        rec.reason = github_info.found ? github_info.description : "Trending package with high GitHub stars for " + analysis.project_type;
        rec.category = "github";
        rec.confidence = 0.99; // GitHub数据极可靠
        rec.compatibility = 0.95;
        rec.popularity = github_info.found && github_info.stars > 0 ? std::min(0.99, github_info.stars / 10000.0) : 0.99; // 基于星标数计算流行度
        rec.maintenance = 0.95;
        rec.priority = "high";
        rec.tags = {"trending", "github", "popular", "real-time", "github-trending"};
        rec.install_command = "Paker add " + package_name;
        rec.github_url = github_info.found ? github_info.github_url : "https://github.com/search?q=" + package_name + "+language%3AC%2B%2B&s=stars&o=desc";
        
        recommendations.push_back(rec);
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_similar_project_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于相似项目推荐（增加更多包）
    std::vector<std::string> similar_packages;
    
    // 根据项目类型添加相似项目包（大幅增加数量）
    if (analysis.project_type == "game_engine") {
        similar_packages = {"sdl2", "sfml", "opengl", "vulkan", "glm", "assimp", "bullet", "box2d", "chipmunk", "raylib", "bgfx", "magnum", "ogre3d", "irrlicht", "cocos2d", "godot", "unity", "unreal", "cryengine", "lumberyard", "phaser", "threejs", "babylon", "pixi", "konva"};
    } else if (analysis.project_type == "web_application") {
        similar_packages = {"cpp-httplib", "crow", "pistache", "boost-beast", "cpprestsdk", "httplib", "drogon", "oatpp", "seastar", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib", "cpp-httplib"};
    } else if (analysis.project_type == "desktop_application") {
        similar_packages = {"qt", "gtkmm", "wxwidgets", "fltk", "imgui", "nuklear", "dear-imgui", "nanogui", "cef", "electron", "tauri", "flutter", "gtk", "kde", "gnome", "xfce", "lxde", "mate", "cinnamon", "budgie", "xfce4", "lxqt", "enlightenment", "openbox", "fluxbox"};
    } else if (analysis.project_type == "scientific_computing") {
        similar_packages = {"eigen", "armadillo", "gsl", "fftw", "blas", "lapack", "mkl", "openblas", "intel-mkl", "cuda", "opencl", "sycl", "openmp", "mpi", "petsc", "slepc", "trilinos", "dealii", "fenics", "dolfin", "scipy", "numpy", "matlab", "octave", "sage"};
    } else if (analysis.project_type == "machine_learning") {
        similar_packages = {"opencv", "tensorflow", "pytorch", "eigen", "onnx", "tflite", "sklearn", "xgboost", "lightgbm", "catboost", "mlpack", "shark", "dlib", "torch", "caffe", "mxnet", "paddle", "mindspore", "jax", "flax", "keras", "theano", "lasagne", "blocks", "fuel"};
    } else {
        similar_packages = {"fmt", "spdlog", "nlohmann-json", "gtest", "catch2", "boost", "asio", "beast", "filesystem", "range-v3", "abseil", "folly", "glog", "gflags", "protobuf", "grpc", "thrift", "zeromq", "nanomsg", "libevent", "libuv", "libev", "libevent2", "libasync", "libdispatch"};
    }
    
    // 添加分析结果中的相似项目
    for (const auto& similar_project : analysis.similar_projects) {
        std::string package_name = extract_package_from_project(similar_project);
        if (!package_name.empty() && package_name.length() > 2) {
            similar_packages.push_back(package_name);
        }
    }
    
    // 去重
    std::set<std::string> unique_packages(similar_packages.begin(), similar_packages.end());
    
    for (const auto& package_name : unique_packages) {
        // 获取GitHub包详细信息
        ProjectAnalyzer analyzer;
        auto github_info = analyzer.get_github_package_info(package_name);
        
        PackageRecommendation rec;
        rec.name = package_name;
        rec.description = github_info.found ? github_info.description : "Recommended by similar projects";
        rec.reason = github_info.found ? github_info.description : "Found in similar GitHub projects for " + analysis.project_type;
        rec.category = "similar-project";
        rec.confidence = 0.98; // 相似项目推荐极可靠
        rec.compatibility = 0.95;
        rec.popularity = github_info.found && github_info.stars > 0 ? std::min(0.95, github_info.stars / 10000.0) : 0.95; // 基于星标数计算流行度
        rec.maintenance = 0.90;
        rec.priority = "high";
        rec.tags = {"similar-project", "github", "real-time", "similar-project"};
        rec.install_command = "Paker add " + package_name;
        rec.github_url = github_info.found ? github_info.github_url : "https://github.com/" + package_name;
        
        recommendations.push_back(rec);
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
            recommendations.push_back({"tensorflow", "Deep learning framework", "Neural network implementation", "ml", 0.95, 0.90, 0.90, 0.85, "high", {"neural-network", "deep-learning", "ai"}, "Paker add tensorflow", "https://github.com/tensorflow/tensorflow"});
            recommendations.push_back({"pytorch", "Dynamic neural networks", "Research-friendly ML", "ml", 0.90, 0.85, 0.85, 0.80, "high", {"neural-network", "research", "dynamic"}, "Paker add pytorch", "https://github.com/pytorch/pytorch"});
        }
        
        if (feature == "computer_vision" || feature == "image_processing") {
            recommendations.push_back({"opencv", "Computer vision library", "Image processing and CV", "ml", 0.95, 0.90, 0.95, 0.90, "high", {"computer-vision", "image-processing", "opencv"}, "Paker add opencv", "https://github.com/opencv/opencv"});
        }
        
        if (feature == "optimization" || feature == "gradient_descent") {
            recommendations.push_back({"eigen", "Linear algebra library", "Mathematical optimization", "math", 0.90, 0.85, 0.85, 0.80, "high", {"linear-algebra", "optimization", "matrix"}, "Paker add eigen", "https://github.com/eigenteam/eigen-git-mirror"});
        }
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_quality_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于代码质量评分推荐
    if (analysis.code_quality_score < 0.3) {
        // 代码质量较低，推荐现代化工具
        recommendations.push_back({"fmt", "Modern C++ formatting", "Type-safe string formatting", "utility", 0.95, 0.95, 0.95, 0.90, "high", {"modern-cpp", "formatting", "type-safe"}, "Paker add fmt", "https://github.com/fmtlib/fmt"});
        recommendations.push_back({"spdlog", "Fast logging library", "High-performance logging", "logging", 0.95, 0.90, 0.90, 0.85, "high", {"logging", "performance", "modern"}, "Paker add spdlog", "https://github.com/gabime/spdlog"});
    }
    
    if (analysis.code_quality_score < 0.5) {
        // 推荐测试框架
        recommendations.push_back({"gtest", "Google Test framework", "Unit testing framework", "testing", 0.95, 0.95, 0.95, 0.90, "high", {"testing", "unit-test", "quality"}, "Paker add gtest", "https://github.com/google/googletest"});
        recommendations.push_back({"catch2", "Modern C++ testing", "Simple testing framework", "testing", 0.90, 0.90, 0.85, 0.80, "high", {"testing", "modern-cpp", "simple"}, "Paker add catch2", "https://github.com/catchorg/Catch2"});
    }
    
    return recommendations;
}

std::vector<PackageRecommendation> RecommendationEngine::get_architecture_based_recommendations(const ProjectAnalysis& analysis) {
    std::vector<PackageRecommendation> recommendations;
    
    // 基于架构模式推荐
    for (const auto& pattern : analysis.architecture_patterns) {
        if (pattern == "microservice" || pattern == "soa") {
            recommendations.push_back({"grpc", "gRPC framework", "High-performance RPC", "rpc", 0.90, 0.85, 0.80, 0.75, "high", {"rpc", "microservice", "grpc"}, "Paker add grpc", "https://github.com/grpc/grpc"});
            recommendations.push_back({"protobuf", "Protocol Buffers", "Efficient serialization", "serialization", 0.95, 0.90, 0.90, 0.85, "high", {"serialization", "protobuf", "efficient"}, "Paker add protobuf", "https://github.com/protocolbuffers/protobuf"});
        }
        
        if (pattern == "event_driven" || pattern == "reactive") {
            recommendations.push_back({"libuv", "Event-driven I/O", "Asynchronous programming", "async", 0.90, 0.85, 0.80, 0.85, "high", {"async", "event-driven", "io"}, "Paker add libuv", "https://github.com/libuv/libuv"});
            recommendations.push_back({"asio", "Boost.Asio", "Asynchronous I/O", "async", 0.95, 0.90, 0.90, 0.85, "high", {"async", "networking", "boost"}, "Paker add asio", "https://github.com/boostorg/asio"});
        }
        
        if (pattern == "plugin" || pattern == "component") {
            recommendations.push_back({"dlfcn", "Dynamic loading", "Plugin system support", "plugin", 0.85, 0.80, 0.75, 0.70, "medium", {"plugin", "dynamic", "loading"}, "Paker add dlfcn", "https://github.com/boostorg/dlfcn"});
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
            recommendations.push_back({"gdb", "GNU Debugger", "Advanced debugging", "debug", 0.90, 0.85, 0.80, 0.75, "high", {"debug", "gdb", "development"}, "Paker add gdb", "https://github.com/bminor/binutils-gdb"});
            recommendations.push_back({"valgrind", "Memory debugging", "Memory leak detection", "debug", 0.85, 0.80, 0.75, 0.70, "high", {"memory", "debug", "valgrind"}, "Paker add valgrind", "https://github.com/valgrind/valgrind"});
            recommendations.push_back({"perf", "Performance analysis", "CPU profiling", "profiling", 0.80, 0.75, 0.70, 0.65, "medium", {"profiling", "performance", "analysis"}, "Paker add perf", "https://github.com/torvalds/linux"});
        }
        
        if (score > 0.05) {
            // 中等复杂度，推荐代码分析工具
            recommendations.push_back({"clang-tidy", "Static analysis", "Code quality analysis", "analysis", 0.85, 0.80, 0.75, 0.70, "high", {"static-analysis", "quality", "clang"}, "Paker add clang-tidy", "https://github.com/llvm/llvm-project"});
            recommendations.push_back({"cppcheck", "Static analysis", "Bug detection", "analysis", 0.80, 0.75, 0.70, 0.65, "medium", {"static-analysis", "bugs", "cppcheck"}, "Paker add cppcheck", "https://github.com/danmar/cppcheck"});
        }
    }
    
    return recommendations;
}

} // namespace Analysis
} // namespace Paker
