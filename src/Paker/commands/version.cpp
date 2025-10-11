#include "Paker/commands/version.h"
#include "Paker/version.h"
#include "Paker/core/output.h"
#include <iostream>

namespace Paker {

/**
 * @brief 显示版本信息
 */
void pm_version() {
    Paker::Output::info("Paker Version Information:");
    std::cout << Paker::Version::get_detailed_version() << std::endl;
}

/**
 * @brief 显示简短版本信息
 */
void pm_version_short() {
    std::cout << Paker::Version::get_version_string() << std::endl;
}

/**
 * @brief 显示构建信息
 */
void pm_version_build() {
    Paker::Output::info("Build Information:");
    std::cout << Paker::Version::get_build_info() << std::endl;
}

/**
 * @brief 检查版本兼容性
 * @param required_version 要求的版本号
 */
void pm_version_check(const std::string& required_version) {
    if (Paker::Version::is_compatible(required_version)) {
        Paker::Output::success("Version compatible: " + std::string(Paker::Version::VERSION) + 
                              " >= " + required_version);
    } else {
        Paker::Output::error("Version incompatible: " + std::string(Paker::Version::VERSION) + 
                            " < " + required_version);
    }
}

/**
 * @brief 显示帮助信息
 */
void pm_version_help() {
    Paker::Output::info("Version Command Help:");
    std::cout << "  Paker version          - Show detailed version information" << std::endl;
    std::cout << "  Paker --version        - Show detailed version information and exit" << std::endl;
    std::cout << "  Paker version --short  - Show short version information" << std::endl;
    std::cout << "  Paker version --build - Show build information" << std::endl;
    std::cout << "  Paker version --check <version> - Check version compatibility" << std::endl;
    std::cout << "  Paker version --help  - Show this help information" << std::endl;
}

} // namespace Paker

