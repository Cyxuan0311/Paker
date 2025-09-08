#include "Recorder/record.h"
#include "third_party/json.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace Recorder {

Record::Record(const std::string& record_file) 
    : record_file_path_(record_file) {
    loadFromFile();
}

Record::~Record() {
    saveToFile();
}

void Record::addPackageRecord(const std::string& package_name, 
                             const std::string& install_path,
                             const std::vector<std::string>& files) {
    PackageInfo& info = packages_[package_name];
    info.install_path = install_path;
    
    // 添加文件列表
    for (const auto& file : files) {
        if (std::find(info.files.begin(), info.files.end(), file) == info.files.end()) {
            info.files.push_back(file);
        }
    }
    
    saveToFile();
}

void Record::addFileRecord(const std::string& package_name, const std::string& file_path) {
    if (packages_.find(package_name) == packages_.end()) {
        // 如果包不存在，创建一个新的记录
        packages_[package_name] = PackageInfo{};
    }
    
    PackageInfo& info = packages_[package_name];
    if (std::find(info.files.begin(), info.files.end(), file_path) == info.files.end()) {
        info.files.push_back(file_path);
        saveToFile();
    }
}

std::vector<std::string> Record::getPackageFiles(const std::string& package_name) const {
    auto it = packages_.find(package_name);
    if (it != packages_.end()) {
        return it->second.files;
    }
    return {};
}

std::string Record::getPackageInstallPath(const std::string& package_name) const {
    auto it = packages_.find(package_name);
    if (it != packages_.end()) {
        return it->second.install_path;
    }
    return "";
}

std::vector<std::string> Record::getAllPackages() const {
    std::vector<std::string> package_names;
    for (const auto& pair : packages_) {
        package_names.push_back(pair.first);
    }
    return package_names;
}

bool Record::isPackageInstalled(const std::string& package_name) const {
    return packages_.find(package_name) != packages_.end();
}

bool Record::removePackageRecord(const std::string& package_name) {
    auto it = packages_.find(package_name);
    if (it != packages_.end()) {
        packages_.erase(it);
        saveToFile();
        return true;
    }
    return false;
}

void Record::showPackageFiles(const std::string& package_name) const {
    auto it = packages_.find(package_name);
    if (it == packages_.end()) {
        std::cout << "Package '" << package_name << "' not found in records." << std::endl;
        return;
    }
    
    const PackageInfo& info = it->second;
    std::cout << "Package: " << package_name << std::endl;
    std::cout << "Install Path: " << info.install_path << std::endl;
    std::cout << "Files (" << info.files.size() << "):" << std::endl;
    
    for (const auto& file : info.files) {
        std::cout << "  " << file << std::endl;
    }
}

void Record::showAllPackages() const {
    if (packages_.empty()) {
        std::cout << "No packages recorded." << std::endl;
        return;
    }
    
    std::cout << "Installed packages (" << packages_.size() << "):" << std::endl;
    for (const auto& pair : packages_) {
        const std::string& package_name = pair.first;
        const PackageInfo& info = pair.second;
        std::cout << "  " << package_name << " (" << info.files.size() << " files)" << std::endl;
        std::cout << "    Install Path: " << info.install_path << std::endl;
    }
}

bool Record::saveToFile() const {
    try {
        nlohmann::json j;
        
        for (const auto& pair : packages_) {
            const std::string& package_name = pair.first;
            const PackageInfo& info = pair.second;
            
            j[package_name] = {
                {"install_path", info.install_path},
                {"files", info.files}
            };
        }
        
        ensureRecordFileExists();
        
        std::ofstream file(record_file_path_);
        if (!file.is_open()) {
            std::cerr << "Failed to open record file for writing: " << record_file_path_ << std::endl;
            return false;
        }
        
        file << j.dump(2);
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving record file: " << e.what() << std::endl;
        return false;
    }
}

bool Record::loadFromFile() {
    try {
        std::ifstream file(record_file_path_);
        if (!file.is_open()) {
            // 文件不存在，这是正常的（首次运行）
            return true;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        packages_.clear();
        
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& package_name = it.key();
            const auto& package_data = it.value();
            
            PackageInfo info;
            info.install_path = package_data["install_path"];
            info.files = package_data["files"].get<std::vector<std::string>>();
            
            packages_[package_name] = info;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading record file: " << e.what() << std::endl;
        return false;
    }
}

void Record::ensureRecordFileExists() const {
    std::filesystem::path file_path(record_file_path_);
    std::filesystem::path dir = file_path.parent_path();
    
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}

} // namespace Recorder
