#include "Paker/commands/remove_project.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include <filesystem>
#include <iostream>
#include <glog/logging.h>

namespace fs = std::filesystem;

namespace Paker {

void pm_remove_project(bool force) {
    try {
        Output::info("Removing Paker project...");
        
        // 检查是否在Paker项目中
        if (!fs::exists("Paker.json")) {
            Output::error("Not in a Paker project directory");
            return;
        }
        
        if (!force) {
            Output::warning("This will permanently delete all Paker project files and dependencies.");
            Output::info("Use --force flag to confirm removal");
            return;
        }
        
        // 删除Paker相关文件和目录
        std::vector<std::string> files_to_remove = {
            "Paker.json",
            "Paker.lock",
            "Paker.sources.json",
            "paker.json",
            "paker.lock",
            "paker.sources.json"
        };
        
        std::vector<std::string> dirs_to_remove = {
            ".paker",
            "packages",
            "build",
            "cmake-build-debug",
            "cmake-build-release"
        };
        
        // 删除文件
        for (const auto& file : files_to_remove) {
            if (fs::exists(file)) {
                try {
                    fs::remove(file);
                    Output::info("Removed file: " + file);
                } catch (const std::exception& e) {
                    Output::warning("Failed to remove file " + file + ": " + e.what());
                }
            }
        }
        
        // 删除目录
        for (const auto& dir : dirs_to_remove) {
            if (fs::exists(dir)) {
                try {
                    fs::remove_all(dir);
                    Output::info("Removed directory: " + dir);
                } catch (const std::exception& e) {
                    Output::warning("Failed to remove directory " + dir + ": " + e.what());
                }
            }
        }
        
        // 查找并删除可能的安装记录文件
        std::vector<std::string> record_files = {
            "test_project_install_record.json",
            "install_record.json",
            "package_records.json",
            get_project_name() + "_install_record.json",
            ".paker_record.json",
            "paker_record.json"
        };
        
        for (const auto& record_file : record_files) {
            if (fs::exists(record_file)) {
                try {
                    fs::remove(record_file);
                    Output::info("Removed record file: " + record_file);
                } catch (const std::exception& e) {
                    Output::warning("Failed to remove record file " + record_file + ": " + e.what());
                }
            }
        }
        
        // 清理缓存和临时文件
        std::vector<std::string> cache_files = {
            ".paker/cache",
            ".paker/links",
            ".paker/temp",
            ".paker/logs",
            "paker_cache",
            "paker_temp"
        };
        
        for (const auto& cache_file : cache_files) {
            if (fs::exists(cache_file)) {
                try {
                    if (fs::is_directory(cache_file)) {
                        fs::remove_all(cache_file);
                        Output::info("Removed cache directory: " + cache_file);
                    } else {
                        fs::remove(cache_file);
                        Output::info("Removed cache file: " + cache_file);
                    }
                } catch (const std::exception& e) {
                    Output::warning("Failed to remove cache " + cache_file + ": " + e.what());
                }
            }
        }
        
        // 清理可能的CMake缓存文件
        std::vector<std::string> cmake_files = {
            "CMakeCache.txt",
            "CMakeFiles",
            "cmake_install.cmake",
            "Makefile",
            "build.ninja"
        };
        
        for (const auto& cmake_file : cmake_files) {
            if (fs::exists(cmake_file)) {
                try {
                    if (fs::is_directory(cmake_file)) {
                        fs::remove_all(cmake_file);
                        Output::info("Removed CMake directory: " + cmake_file);
                    } else {
                        fs::remove(cmake_file);
                        Output::info("Removed CMake file: " + cmake_file);
                    }
                } catch (const std::exception& e) {
                    Output::warning("Failed to remove CMake file " + cmake_file + ": " + e.what());
                }
            }
        }
        
        Output::success("Paker project removed successfully!");
        Output::info("All Paker files and dependencies have been deleted");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error removing Paker project: " << e.what();
        Output::error("Error removing Paker project: " + std::string(e.what()));
    }
}

void pm_remove_project_confirm() {
    try {
        Output::info("Removing Paker project (confirmed)...");
        
        // 检查是否在Paker项目中
        if (!fs::exists("Paker.json")) {
            Output::error("Not in a Paker project directory");
            return;
        }
        
        // 显示将要删除的内容
        Output::info("The following will be removed:");
        
        std::vector<std::string> files_to_remove = {
            "Paker.json",
            "Paker.lock",
            "Paker.sources.json"
        };
        
        std::vector<std::string> dirs_to_remove = {
            ".paker",
            "packages"
        };
        
        // 显示文件
        for (const auto& file : files_to_remove) {
            if (fs::exists(file)) {
                Output::info("  File: " + file);
            }
        }
        
        // 显示目录
        for (const auto& dir : dirs_to_remove) {
            if (fs::exists(dir)) {
                Output::info("  Directory: " + dir + "/");
            }
        }
        
        // 执行删除
        pm_remove_project(true);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in remove project confirm: " << e.what();
        Output::error("Error in remove project confirm: " + std::string(e.what()));
    }
}

} // namespace Paker
