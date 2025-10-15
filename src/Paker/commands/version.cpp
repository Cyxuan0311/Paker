#include "Paker/commands/version.h"
#include "Paker/version.h"
#include "Paker/core/output.h"
#include <iostream>
#include <iomanip>

namespace Paker {

/**
 * @brief 显示版本信息
 */
void pm_version() {
    // 显示Paker品牌Logo（参考现代AI工具风格）
    std::cout << "\033[1;38;5;208m" << R"(
    ██████╗  █████╗ ██╗  ██╗███████╗██████╗ 
    ██╔══██╗██╔══██╗██║ ██╔╝██╔════╝██╔══██╗
    ██████╔╝███████║█████╔╝ █████╗  ██████╔╝
    ██╔═══╝ ██╔══██║██╔═██╗ ██╔══╝  ██╔══██╗
    ██║     ██║  ██║██║  ██╗███████╗██║  ██║
    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝
    )" << "\033[0m\n";

    std::cout<<"\033[1;38;5;208m"
    <<"════════════════════════════════════════════════"<<"\033[0m\n"<<std::endl;
    
    // 副标题
    std::cout << "\033[38;5;245m" << std::setw(50) << std::left<< "A modern intelligent C++ package manager" << "\033[0m\n\n";
    
    // 版本信息
    Paker::Output::info("Version Information:");
    std::cout << "  Version: " << "\033[38;5;208m" << Paker::Version::VERSION << "\033[0m\n";
    std::cout << "  Build: " << "\033[38;5;245m" << Paker::Version::BUILD_DATE << " " << Paker::Version::BUILD_TIME << "\033[0m\n";
    std::cout << "  Compiler: " << "\033[38;5;245m" << Paker::Version::get_compiler_info() << "\033[0m\n";
    std::cout << "  Standard: " << "\033[38;5;245m" << Paker::Version::CPP_STANDARD << "\033[0m\n\n";
    
    // 特性列表
    Paker::Output::info("Features:");
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mOpenMP Parallel Processing\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mSIMD Instruction Set Optimization\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mAsynchronous I/O Operations\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mIntelligent Cache Management\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mIncremental Dependency Parsing\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mNetwork Performance Optimization\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mVersion Rollback System\033[0m\n";
    std::cout << "  \033[38;5;208m•\033[0m \033[38;5;245mPerformance Monitoring Diagnostics\033[0m\n\n";
    
    // 项目信息
    Paker::Output::info("Project Information:");
    std::cout << "  License: " << "\033[38;5;245m" << Paker::Version::PROJECT_LICENSE << "\033[0m\n";
    std::cout << "  Repository: " << "\033[38;5;245m" << Paker::Version::PROJECT_URL << "\033[0m\n";
}

/**
 * @brief 显示简短版本信息
 */
void pm_version_short() {
    std::cout << "\033[1;38;5;208m" << R"(
    ██████╗  █████╗ ██╗  ██╗███████╗██████╗ 
    ██╔══██╗██╔══██╗██║ ██╔╝██╔════╝██╔══██╗
    ██████╔╝███████║█████╔╝ █████╗  ██████╔╝
    ██╔═══╝ ██╔══██║██╔═██╗ ██╔══╝  ██╔══██╗
    ██║     ██║  ██║██║  ██╗███████╗██║  ██║
    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝
    )" << "\033[0m\n";
    std::cout << "\033[38;5;245mVersion: \033[38;5;208m" << Paker::Version::VERSION << "\033[0m\n";
}

/**
 * @brief 显示构建信息
 */
void pm_version_build() {
    Paker::Output::info("Build Information:");
    std::cout << "  Build Time: " << "\033[38;5;245m" << Paker::Version::BUILD_DATE << " " << Paker::Version::BUILD_TIME << "\033[0m\n";
    std::cout << "  Compiler: " << "\033[38;5;245m" << Paker::Version::get_compiler_info() << "\033[0m\n";
    std::cout << "  Standard: " << "\033[38;5;245m" << Paker::Version::CPP_STANDARD << "\033[0m\n\n";
    
    Paker::Output::info("Build Configuration:");
    std::cout << Paker::Version::get_build_info() << std::endl;
}

/**
 * @brief 检查版本兼容性
 * @param required_version 要求的版本号
 */
void pm_version_check(const std::string& required_version) {
    Paker::Output::info("Version Compatibility Check:");
    std::cout << "  Current: " << "\033[38;5;208m" << Paker::Version::VERSION << "\033[0m\n";
    std::cout << "  Required: " << "\033[38;5;245m" << required_version << "\033[0m\n";
    
    if (Paker::Version::is_compatible(required_version)) {
        Paker::Output::success("✓ Compatible");
    } else {
        Paker::Output::error("✗ Incompatible");
    }
}

/**
 * @brief 显示帮助信息
 */
void pm_version_help() {
    Paker::Output::info("Version Command Help:");
    std::cout << "  \033[38;5;208mPaker version\033[0m          - Show detailed version information\n";
    std::cout << "  \033[38;5;208mPaker --version\033[0m        - Show detailed version information and exit\n";
    std::cout << "  \033[38;5;208mPaker version --short\033[0m  - Show short version information\n";
    std::cout << "  \033[38;5;208mPaker version --build\033[0m - Show build information\n";
    std::cout << "  \033[38;5;208mPaker version --check <version>\033[0m - Check version compatibility\n";
    std::cout << "  \033[38;5;208mPaker version --help\033[0m  - Show this help information\n";
}

} // namespace Paker