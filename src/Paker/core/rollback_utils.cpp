#include "Paker/core/version_history.h"
#include "Paker/core/output.h"
#include "Paker/dependency/version_manager.h"
#include "Paker/dependency/dependency_resolver.h"
#include "Paker/cache/cache_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <glog/logging.h>

namespace fs = std::filesystem;

namespace Paker {

bool RollbackUtils::check_rollback_safety(const std::string& package_name, const std::string& target_version) {
    try {
        LOG(INFO) << "Checking rollback safety for " << package_name << " to " << target_version;
        
        // 1. 检查目标版本是否存在
        auto* history_manager = get_history_manager();
        auto rollbackable_versions = history_manager->get_rollbackable_versions(package_name);
        
        if (std::find(rollbackable_versions.begin(), rollbackable_versions.end(), target_version) 
            == rollbackable_versions.end()) {
            LOG(WARNING) << "Target version " << target_version << " not found in rollbackable versions";
            return false;
        }
        
        // 2. 检查版本兼容性
        if (!VersionManager::is_version_compatible(target_version, "current")) {
            LOG(WARNING) << "Version compatibility check failed for " << target_version;
            return false;
        }
        
        // 3. 检查依赖关系
        DependencyResolver resolver;
        if (resolver.resolve_project_dependencies()) {
            auto& graph = resolver.get_dependency_graph();
            auto* node = graph.get_node(package_name);
            if (node) {
                // 检查依赖该包的包
                for (const auto& [other_name, other_node] : graph.get_nodes()) {
                    if (other_node.dependencies.find(package_name) != other_node.dependencies.end()) {
                        // 检查依赖包是否兼容目标版本
                        auto constraint_it = other_node.version_constraints.find(package_name);
                        if (constraint_it != other_node.version_constraints.end()) {
                            if (!constraint_it->second.satisfies(target_version)) {
                                LOG(WARNING) << "Dependency constraint violation: " << other_name 
                                           << " requires " << package_name << " " 
                                           << constraint_it->second.to_string();
                                return false;
                            }
                        }
                    }
                }
            }
        }
        
        // 4. 检查文件系统状态
        std::string current_path;
        if (Paker::g_cache_manager) {
            std::string project_path = fs::current_path().string();
            current_path = Paker::g_cache_manager->get_project_package_path(package_name, project_path);
        } else {
            current_path = "packages/" + package_name;
        }
        
        if (!fs::exists(current_path)) {
            LOG(WARNING) << "Current package path does not exist: " << current_path;
            return false;
        }
        
        // 5. 检查备份可用性
        auto history = history_manager->get_package_history(package_name);
        for (const auto& entry : history) {
            if (entry.new_version == target_version && !entry.backup_path.empty()) {
                if (!fs::exists(entry.backup_path)) {
                    LOG(WARNING) << "Backup file not found: " << entry.backup_path;
                    return false;
                }
            }
        }
        
        LOG(INFO) << "Rollback safety check passed for " << package_name << " to " << target_version;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error during rollback safety check: " << e.what();
        return false;
    }
}

std::string RollbackUtils::generate_rollback_report(const RollbackResult& result) {
    std::ostringstream report;
    
    report << "🔄 Rollback Report\n";
    report << "==================\n\n";
    
    // 基本信息
    report << "Status: " << (result.success ? "✅ Success" : "❌ Failed") << "\n";
    report << "Duration: " << result.duration.count() << "ms\n";
    report << "Message: " << result.message << "\n\n";
    
    // 成功回滚的包
    if (!result.rolled_back_packages.empty()) {
        report << "✅ Successfully Rolled Back:\n";
        for (const auto& pkg : result.rolled_back_packages) {
            report << "  - " << pkg << "\n";
        }
        report << "\n";
    }
    
    // 失败的包
    if (!result.failed_packages.empty()) {
        report << "❌ Failed to Rollback:\n";
        for (const auto& pkg : result.failed_packages) {
            report << "  - " << pkg << "\n";
        }
        report << "\n";
    }
    
    // 备份信息
    if (!result.backup_location.empty()) {
        report << "💾 Backup Location: " << result.backup_location << "\n";
    }
    
    // 文件统计
    if (result.total_files_affected > 0) {
        report << "📁 Files Affected: " << result.total_files_affected << "\n";
    }
    
    // 建议
    if (result.success) {
        report << "\n💡 Recommendations:\n";
        report << "  - Verify the rolled back packages work correctly\n";
        report << "  - Test your application thoroughly\n";
        report << "  - Consider updating your dependency specifications\n";
    } else {
        report << "\n⚠️  Troubleshooting:\n";
        report << "  - Check if the target version exists in history\n";
        report << "  - Verify backup files are accessible\n";
        report << "  - Consider using --force flag if safe\n";
        report << "  - Check dependency constraints\n";
    }
    
    return report.str();
}

bool RollbackUtils::validate_backup_integrity(const std::string& backup_path) {
    try {
        if (!fs::exists(backup_path)) {
            LOG(ERROR) << "Backup file does not exist: " << backup_path;
            return false;
        }
        
        // 检查文件大小
        auto file_size = fs::file_size(backup_path);
        if (file_size == 0) {
            LOG(ERROR) << "Backup file is empty: " << backup_path;
            return false;
        }
        
        // 检查文件格式（tar.gz）
        if (backup_path.find(".tar.gz") == std::string::npos) {
            LOG(WARNING) << "Backup file may not be in tar.gz format: " << backup_path;
        }
        
        // 尝试列出tar文件内容（验证完整性）
        std::ostringstream cmd;
        cmd << "tar -tzf " << backup_path << " > /dev/null 2>&1";
        int ret = std::system(cmd.str().c_str());
        
        if (ret != 0) {
            LOG(ERROR) << "Backup file integrity check failed: " << backup_path;
            return false;
        }
        
        LOG(INFO) << "Backup integrity check passed: " << backup_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error validating backup integrity: " << e.what();
        return false;
    }
}

std::vector<std::string> RollbackUtils::calculate_file_differences(const std::string& path1, 
                                                                 const std::string& path2) {
    std::vector<std::string> differences;
    
    try {
        if (!fs::exists(path1) || !fs::exists(path2)) {
            LOG(WARNING) << "One or both paths do not exist for diff calculation";
            return differences;
        }
        
        // 使用diff命令计算差异
        std::ostringstream cmd;
        cmd << "diff -r " << path1 << " " << path2 << " 2>/dev/null";
        
        FILE* pipe = popen(cmd.str().c_str(), "r");
        if (!pipe) {
            LOG(ERROR) << "Failed to execute diff command";
            return differences;
        }
        
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        pclose(pipe);
        
        // 解析差异输出
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                differences.push_back(line);
            }
        }
        
        LOG(INFO) << "Calculated " << differences.size() << " file differences";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error calculating file differences: " << e.what();
    }
    
    return differences;
}

bool RollbackUtils::create_differential_backup(const std::string& source_path, 
                                             const std::string& backup_path) {
    try {
        if (!fs::exists(source_path)) {
            LOG(ERROR) << "Source path does not exist: " << source_path;
            return false;
        }
        
        // 创建差异备份目录
        fs::create_directories(fs::path(backup_path).parent_path());
        
        // 使用rsync创建差异备份
        std::ostringstream cmd;
        cmd << "rsync -av --delete " << source_path << "/ " << backup_path << "/";
        
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to create differential backup: " << backup_path;
            return false;
        }
        
        LOG(INFO) << "Created differential backup: " << backup_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error creating differential backup: " << e.what();
        return false;
    }
}

bool RollbackUtils::apply_differential_backup(const std::string& backup_path, 
                                            const std::string& target_path) {
    try {
        if (!fs::exists(backup_path)) {
            LOG(ERROR) << "Backup path does not exist: " << backup_path;
            return false;
        }
        
        // 创建目标目录
        fs::create_directories(target_path);
        
        // 使用rsync应用差异备份
        std::ostringstream cmd;
        cmd << "rsync -av --delete " << backup_path << "/ " << target_path << "/";
        
        int ret = std::system(cmd.str().c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to apply differential backup: " << backup_path;
            return false;
        }
        
        LOG(INFO) << "Applied differential backup: " << backup_path << " to " << target_path;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error applying differential backup: " << e.what();
        return false;
    }
}

} // namespace Paker