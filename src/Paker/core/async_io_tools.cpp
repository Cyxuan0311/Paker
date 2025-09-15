#include "Paker/core/async_io.h"
#include <filesystem>
#include <glog/logging.h>

namespace fs = std::filesystem;

namespace Paker {

// AsyncIOTools 实现
std::future<std::string> AsyncIOTools::read_text_file_async(
    AsyncIOManager& manager, const std::string& file_path) {
    
    return std::async(std::launch::async, [&manager, file_path]() -> std::string {
        auto future = manager.read_file_async(file_path, true);
        auto result = future.get();
        
        if (result && result->status == IOOperationStatus::COMPLETED) {
            return result->content;
        } else {
            LOG(ERROR) << "Failed to read text file: " << file_path;
            return "";
        }
    });
}

std::future<bool> AsyncIOTools::write_text_file_async(
    AsyncIOManager& manager, const std::string& file_path, const std::string& content) {
    
    return std::async(std::launch::async, [&manager, file_path, content]() -> bool {
        auto future = manager.write_file_async(file_path, content);
        auto result = future.get();
        
        if (result && result->status == IOOperationStatus::COMPLETED) {
            return true;
        } else {
            LOG(ERROR) << "Failed to write text file: " << file_path;
            return false;
        }
    });
}

std::future<std::vector<char>> AsyncIOTools::read_binary_file_async(
    AsyncIOManager& manager, const std::string& file_path) {
    
    return std::async(std::launch::async, [&manager, file_path]() -> std::vector<char> {
        auto future = manager.read_file_async(file_path, false);
        auto result = future.get();
        
        if (result && result->status == IOOperationStatus::COMPLETED) {
            return result->data;
        } else {
            LOG(ERROR) << "Failed to read binary file: " << file_path;
            return {};
        }
    });
}

std::future<bool> AsyncIOTools::write_binary_file_async(
    AsyncIOManager& manager, const std::string& file_path, const std::vector<char>& data) {
    
    return std::async(std::launch::async, [&manager, file_path, data]() -> bool {
        auto future = manager.write_file_async(file_path, data);
        auto result = future.get();
        
        if (result && result->status == IOOperationStatus::COMPLETED) {
            return true;
        } else {
            LOG(ERROR) << "Failed to write binary file: " << file_path;
            return false;
        }
    });
}

std::future<std::vector<std::string>> AsyncIOTools::read_multiple_text_files_async(
    AsyncIOManager& manager, const std::vector<std::string>& file_paths) {
    
    return std::async(std::launch::async, [&manager, file_paths]() -> std::vector<std::string> {
        std::vector<std::string> contents;
        contents.reserve(file_paths.size());
        
        auto futures = manager.read_files_async(file_paths, true);
        
        for (auto& future : futures) {
            auto result = future.get();
            if (result && result->status == IOOperationStatus::COMPLETED) {
                contents.push_back(result->content);
            } else {
                contents.push_back(""); // 失败时添加空字符串
            }
        }
        
        return contents;
    });
}

std::future<bool> AsyncIOTools::write_multiple_text_files_async(
    AsyncIOManager& manager, 
    const std::vector<std::pair<std::string, std::string>>& file_contents) {
    
    return std::async(std::launch::async, [&manager, file_contents]() -> bool {
        auto futures = manager.write_files_async(file_contents);
        bool all_success = true;
        
        for (auto& future : futures) {
            auto result = future.get();
            if (!result || result->status != IOOperationStatus::COMPLETED) {
                all_success = false;
            }
        }
        
        return all_success;
    });
}

std::future<std::vector<std::string>> AsyncIOTools::list_directory_async(
    AsyncIOManager& manager, const std::string& directory_path) {
    
    return std::async(std::launch::async, [directory_path]() -> std::vector<std::string> {
        std::vector<std::string> files;
        
        try {
            if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
                LOG(ERROR) << "Directory does not exist or is not a directory: " << directory_path;
                return files;
            }
            
            for (const auto& entry : fs::directory_iterator(directory_path)) {
                files.push_back(entry.path().string());
            }
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception while listing directory: " << e.what();
        }
        
        return files;
    });
}

std::future<bool> AsyncIOTools::create_directory_async(
    AsyncIOManager& manager, const std::string& directory_path) {
    
    return std::async(std::launch::async, [directory_path]() -> bool {
        try {
            if (fs::exists(directory_path)) {
                return fs::is_directory(directory_path);
            }
            
            return fs::create_directories(directory_path);
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception while creating directory: " << e.what();
            return false;
        }
    });
}

std::future<bool> AsyncIOTools::file_exists_async(
    AsyncIOManager& manager, const std::string& file_path) {
    
    return std::async(std::launch::async, [file_path]() -> bool {
        try {
            return fs::exists(file_path);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception while checking file existence: " << e.what();
            return false;
        }
    });
}

std::future<size_t> AsyncIOTools::get_file_size_async(
    AsyncIOManager& manager, const std::string& file_path) {
    
    return std::async(std::launch::async, [file_path]() -> size_t {
        try {
            if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
                return fs::file_size(file_path);
            }
            return 0;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception while getting file size: " << e.what();
            return 0;
        }
    });
}

std::future<std::chrono::system_clock::time_point> AsyncIOTools::get_file_modification_time_async(
    AsyncIOManager& manager, const std::string& file_path) {
    
    return std::async(std::launch::async, [file_path]() -> std::chrono::system_clock::time_point {
        try {
            if (fs::exists(file_path)) {
                auto ftime = fs::last_write_time(file_path);
                return std::chrono::system_clock::from_time_t(
                    std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count());
            }
            return std::chrono::system_clock::time_point{};
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception while getting file modification time: " << e.what();
            return std::chrono::system_clock::time_point{};
        }
    });
}

} // namespace Paker
