#pragma once

#include <string>

namespace Paker {

/**
 * @brief 版本信息管理类
 */
class Version {
public:
    // 版本号
    static constexpr const char* VERSION = "0.1.0";
    static constexpr const char* VERSION_MAJOR = "0";
    static constexpr const char* VERSION_MINOR = "1";
    static constexpr const char* VERSION_PATCH = "0";
    
    // 构建信息
    static constexpr const char* BUILD_DATE = __DATE__;
    static constexpr const char* BUILD_TIME = __TIME__;
    
    // 项目信息
    static constexpr const char* PROJECT_NAME = "Paker";
    static constexpr const char* PROJECT_DESCRIPTION = "Modern C++ Package Manager";
    static constexpr const char* PROJECT_LICENSE = "MIT";
    static constexpr const char* PROJECT_URL = "https://github.com/Cyxuan0311/Paker.git";
    
    // C++ 标准
    static constexpr const char* CPP_STANDARD = "C++17";
    
    /**
     * @brief 获取完整版本信息
     * @return 版本信息字符串
     */
    static std::string get_version_string() {
        return std::string(VERSION);
    }
    
    /**
     * @brief 获取详细版本信息
     * @return 详细版本信息字符串
     */
    static std::string get_detailed_version() {
        std::string info;
        info += PROJECT_NAME + std::string(" v") + VERSION + "\n";
        info += PROJECT_DESCRIPTION + std::string("\n");
        info += "Build Time: " + std::string(BUILD_DATE) + " " + std::string(BUILD_TIME) + "\n";
        info += "Compiler: " + get_compiler_info() + "\n";
        info += "C++ Standard: " + std::string(CPP_STANDARD) + "\n";
        info += "Features:\n";
        info += "  - OpenMP Parallel Processing\n";
        info += "  - SIMD Instruction Set Optimization\n";
        info += "  - Asynchronous I/O Operations\n";
        info += "  - Intelligent Cache Management\n";
        info += "  - Incremental Dependency Parsing\n";
        info += "  - Network Performance Optimization\n";
        info += "  - Version Rollback System\n";
        info += "  - Performance Monitoring Diagnostics\n";
        info += "License: " + std::string(PROJECT_LICENSE) + "\n";
        info += "Project Address: " + std::string(PROJECT_URL) + "\n";
        return info;
    }
    
    /**
     * @brief 获取编译器信息
     * @return 编译器信息字符串
     */
    static std::string get_compiler_info() {
#ifdef __GNUC__
        return "GCC " + std::to_string(__GNUC__) + "." + 
               std::to_string(__GNUC_MINOR__) + "." + 
               std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(__clang__)
        return "Clang " + std::to_string(__clang_major__) + "." + 
               std::to_string(__clang_minor__) + "." + 
               std::to_string(__clang_patchlevel__);
#elif defined(_MSC_VER)
        return "MSVC " + std::to_string(_MSC_VER);
#else
        return "Unknown";
#endif
    }
    
    /**
     * @brief 获取构建配置信息
     * @return 构建配置信息字符串
     */
    static std::string get_build_info() {
        std::string info;
        info += "Build Configuration:\n";
        
#ifdef NDEBUG
        info += "  - Release Mode\n";
#else
        info += "  - Debug Mode\n";
#endif

#ifdef _OPENMP
        info += "  - OpenMP Support\n";
#endif

#ifdef __SSE2__
        info += "  - SSE2 Support\n";
#endif

#ifdef __SSE4_2__
        info += "  - SSE4.2 Support\n";
#endif

#ifdef __AVX2__
        info += "  - AVX2 Support\n";
#endif

#ifdef __AVX512F__
        info += "  - AVX512 Support\n";
#endif

        return info;
    }
    
    /**
     * @brief 检查版本兼容性
     * @param required_version 要求的版本号
     * @return 是否兼容
     */
    static bool is_compatible(const std::string& required_version) {
        // 简单的版本比较逻辑
        // 这里可以实现更复杂的版本比较算法
        return VERSION >= required_version;
    }
};

} // namespace Paker

