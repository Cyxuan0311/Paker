#include "Paker/dependency/dependency_resolver.h"
#include "Paker/dependency/incremental_parser.h"
#include "Paker/core/utils.h"
#include "Paker/dependency/sources.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/version_manager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <glog/logging.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

DependencyResolver::DependencyResolver() : recursive_mode_(false), incremental_parser_(nullptr) {
    try {
        // 初始化仓库映射
        repositories_ = get_builtin_repos();
        
        // 注意：不在构造函数中初始化 incremental_parser_，避免循环依赖
        // incremental_parser_ 将在首次使用时延迟初始化
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error initializing DependencyResolver: " << e.what();
        // 使用空的仓库映射作为后备
        repositories_.clear();
    }
}

DependencyResolver::~DependencyResolver() {
    if (incremental_parser_) {
        incremental_parser_->shutdown();
        delete incremental_parser_;
        incremental_parser_ = nullptr;
    }
}

bool DependencyResolver::resolve_package(const std::string& package, const std::string& version) {
    LOG(INFO) << "Resolving package: " << package << (version.empty() ? "" : "@" + version);
    
    // 如果启用了增量解析，使用增量解析器
    if (incremental_parser_ && incremental_parser_->get_config().enable_incremental) {
        return incremental_parser_->parse_package(package, version);
    }
    
    // 检查包是否已经解析
    if (is_package_resolved(package)) {
        LOG(INFO) << "Package " << package << " already resolved";
        return true;
    }
    
    // 创建依赖节点
    DependencyNode node(package, version);
    
    // 获取仓库URL
    std::string repo_url = get_repository_url(package);
    if (repo_url.empty()) {
        LOG(WARNING) << "No repository found for package: " << package;
        // 继续执行，可能包已经安装
    } else {
        node.repository = repo_url;
    }
    
    // 检查包是否已安装
    std::string install_path = get_package_install_path(package);
    if (fs::exists(install_path)) {
        node.is_installed = true;
        node.install_path = install_path;
        
        // 读取已安装包的依赖信息
        if (!read_package_dependencies(install_path, node)) {
            LOG(WARNING) << "Failed to read dependencies for installed package: " << package;
        }
    }
    
    // 添加节点到图
    graph_.add_node(node);
    
    // 如果是递归模式，解析依赖
    if (recursive_mode_) {
        return resolve_recursive_dependencies(package, version);
    }
    
    return true;
}

bool DependencyResolver::resolve_project_dependencies() {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Project JSON file not found: " << json_file;
        return false;
    }
    
    return load_dependencies_from_json(json_file);
}

bool DependencyResolver::resolve_recursive_dependencies(const std::string& package, const std::string& version) {
    LOG(INFO) << "Resolving recursive dependencies for: " << package;
    
    // 获取包的依赖信息
    std::string install_path = get_package_install_path(package);
    if (!fs::exists(install_path)) {
        LOG(WARNING) << "Package not installed, cannot resolve recursive dependencies: " << package;
        return false;
    }
    
    DependencyNode temp_node(package, version);
    if (!read_package_dependencies(install_path, temp_node)) {
        LOG(WARNING) << "Failed to read dependencies for package: " << package;
        return false;
    }
    
    // 递归解析每个依赖
    for (const auto& dep : temp_node.dependencies) {
        if (!is_package_resolved(dep)) {
            if (!resolve_package(dep)) {
                LOG(WARNING) << "Failed to resolve dependency: " << dep;
                continue;
            }
        }
        
        // 添加依赖关系到图
        graph_.add_dependency(package, dep);
    }
    
    return true;
}

bool DependencyResolver::validate_dependencies() {
    LOG(INFO) << "Validating dependency graph";
    
    // 检查是否有循环依赖
    auto cycles = graph_.detect_cycles();
    if (!cycles.empty()) {
        LOG(ERROR) << "Circular dependencies detected:";
        for (const auto& cycle : cycles) {
            std::string cycle_str;
            for (size_t i = 0; i < cycle.size(); ++i) {
                cycle_str += cycle[i];
                if (i < cycle.size() - 1) cycle_str += " -> ";
            }
            LOG(ERROR) << "  " << cycle_str;
        }
        return false;
    }
    
    // 检查是否有缺失的依赖
    for (const auto& [package, node] : graph_.get_nodes()) {
        for (const auto& dep : node.dependencies) {
            if (!graph_.has_node(dep)) {
                LOG(ERROR) << "Missing dependency: " << dep << " required by " << package;
                return false;
            }
        }
    }
    
    LOG(INFO) << "Dependency validation passed";
    return true;
}

void DependencyResolver::set_repositories(const std::map<std::string, std::string>& repos) {
    repositories_ = repos;
}

void DependencyResolver::add_repository(const std::string& name, const std::string& url) {
    repositories_[name] = url;
}

std::string DependencyResolver::get_repository_url(const std::string& package) const {
    auto it = repositories_.find(package);
    return it != repositories_.end() ? it->second : "";
}

void DependencyResolver::clear() {
    graph_.clear();
    repositories_.clear();
}

bool DependencyResolver::load_dependencies_from_json(const std::string& json_file) {
    try {
        std::ifstream ifs(json_file);
        if (!ifs.is_open()) {
            LOG(ERROR) << "Failed to open JSON file: " << json_file;
            return false;
        }
        
        json j;
        ifs >> j;
        
        // 解析依赖
        if (j.contains("dependencies")) {
            for (const auto& [package, version] : j["dependencies"].items()) {
                std::string version_str = version.is_string() ? version.get<std::string>() : "*";
                if (!resolve_package(package, version_str)) {
                    LOG(WARNING) << "Failed to resolve package: " << package;
                }
            }
        }
        
        // 解析URL依赖
        if (j.contains("url_dependencies")) {
            for (const auto& [package, url] : j["url_dependencies"].items()) {
                if (!resolve_package(package, "url")) {
                    LOG(WARNING) << "Failed to resolve URL package: " << package;
                }
            }
        }
        
        // 扫描已安装的包
        scan_installed_packages();
        
        // 解析自定义仓库
        if (j.contains("remotes")) {
            for (const auto& remote : j["remotes"]) {
                if (remote.contains("name") && remote.contains("url")) {
                    std::string name = remote["name"];
                    std::string url = remote["url"];
                    add_repository(name, url);
                }
            }
        }
        
        LOG(INFO) << "Loaded dependencies from JSON file: " << json_file;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to parse JSON file: " << e.what();
        return false;
    }
}

void DependencyResolver::scan_installed_packages() {
    try {
        fs::path packages_dir = "packages";
        if (!fs::exists(packages_dir) || !fs::is_directory(packages_dir)) {
            LOG(INFO) << "No packages directory found for dependency scanning";
            return;
        }
        
        LOG(INFO) << "Scanning installed packages for dependency analysis in " << packages_dir.string();
        
        for (const auto& entry : fs::directory_iterator(packages_dir)) {
            if (entry.is_directory()) {
                std::string package_name = entry.path().filename().string();
                std::string version = "unknown";
                
                // 获取Git版本信息
                fs::path head_file = entry.path() / ".git" / "HEAD";
                if (fs::exists(head_file)) {
                    std::ifstream hfs(head_file);
                    std::string head_line;
                    if (std::getline(hfs, head_line)) {
                        if (head_line.find("ref:") == 0) {
                            version = head_line.substr(head_line.find_last_of('/') + 1);
                        } else {
                            version = head_line.substr(0, 8);
                        }
                    }
                }
                
                // 添加到依赖图
                if (!is_package_resolved(package_name)) {
                    if (resolve_package(package_name, version)) {
                        LOG(INFO) << "Scanned installed package for analysis: " << package_name << "@" << version;
                    } else {
                        LOG(WARNING) << "Failed to resolve scanned package: " << package_name;
                    }
                }
            }
        }
        
        LOG(INFO) << "Completed scanning installed packages for dependency analysis";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error scanning installed packages for dependency analysis: " << e.what();
    }
}

bool DependencyResolver::save_dependencies_to_json(const std::string& json_file) const {
    try {
        json j;
        j["dependencies"] = json::object();
        
        // 保存依赖信息
        for (const auto& [package, node] : graph_.get_nodes()) {
            j["dependencies"][package] = node.version.empty() ? "*" : node.version;
        }
        
        // 保存仓库信息
        j["remotes"] = json::array();
        for (const auto& [name, url] : repositories_) {
            json remote;
            remote["name"] = name;
            remote["url"] = url;
            j["remotes"].push_back(remote);
        }
        
        std::ofstream ofs(json_file);
        ofs << j.dump(4);
        
        LOG(INFO) << "Saved dependencies to JSON file: " << json_file;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save JSON file: " << e.what();
        return false;
    }
}

bool DependencyResolver::parse_package_metadata(const std::string& package, const std::string& version) {
    // 这里应该解析包的元数据文件（如 package.json, CMakeLists.txt 等）
    // 简化实现，返回 true
    return true;
}

bool DependencyResolver::read_package_dependencies(const std::string& package_path, DependencyNode& node) {
    // 尝试读取各种可能的依赖配置文件
    std::vector<std::string> config_files = {
        "package.json",
        "CMakeLists.txt",
        "paker.json",
        "dependencies.json"
    };
    
    for (const auto& config_file : config_files) {
        fs::path config_path = fs::path(package_path) / config_file;
        if (fs::exists(config_path)) {
            if (read_dependencies_from_file(config_path.string(), node)) {
                return true;
            }
        }
    }
    
    // 如果没有找到配置文件，尝试从目录结构推断依赖
    return infer_dependencies_from_structure(package_path, node);
}

bool DependencyResolver::read_dependencies_from_file(const std::string& file_path, DependencyNode& node) {
    try {
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            return false;
        }
        
        std::string extension = fs::path(file_path).extension().string();
        
        if (extension == ".json") {
            return read_dependencies_from_json(ifs, node);
        } else if (extension == ".txt" || file_path.find("CMakeLists") != std::string::npos) {
            return read_dependencies_from_cmake(ifs, node);
        }
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to read dependencies from file: " << file_path << " - " << e.what();
    }
    
    return false;
}

bool DependencyResolver::read_dependencies_from_json(std::ifstream& ifs, DependencyNode& node) {
    try {
        json j;
        ifs >> j;
        
        if (j.contains("dependencies")) {
            for (const auto& [dep, version] : j["dependencies"].items()) {
                node.dependencies.insert(dep);
                
                std::string version_str = version.is_string() ? version.get<std::string>() : "*";
                VersionConstraint constraint = VersionConstraint::parse(version_str);
                node.version_constraints[dep] = constraint;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to parse JSON dependencies: " << e.what();
        return false;
    }
}

bool DependencyResolver::read_dependencies_from_cmake(std::ifstream& ifs, DependencyNode& node) {
    // 改进的 CMake 依赖解析
    std::string line;
    while (std::getline(ifs, line)) {
        // 移除注释和多余空格
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // 移除前后空格
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // 只处理 find_package 命令，忽略 add_subdirectory
        if (line.find("find_package") == 0) {
            // 解析 find_package(包名 REQUIRED) 格式
            size_t open_paren = line.find('(');
            size_t close_paren = line.find(')');
            
            if (open_paren != std::string::npos && close_paren != std::string::npos) {
                std::string package_part = line.substr(open_paren + 1, close_paren - open_paren - 1);
                
                // 提取包名（第一个单词）
                std::istringstream iss(package_part);
                std::string package_name;
                if (iss >> package_name) {
                    // 验证包名是否有效（不包含特殊字符）
                    if (is_valid_package_name(package_name)) {
                        node.dependencies.insert(package_name);
                    }
                }
            }
        }
        // 忽略 add_subdirectory 命令，因为它们不是包依赖
    }
    
    return !node.dependencies.empty();
}

bool DependencyResolver::is_valid_package_name(const std::string& name) {
    if (name.empty()) return false;
    
    // 检查是否包含无效字符
    for (char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }
    
    // 检查是否以数字开头（通常不是有效的包名）
    if (std::isdigit(name[0])) {
        return false;
    }
    
    // 检查长度
    if (name.length() < 2 || name.length() > 50) {
        return false;
    }
    
    return true;
}

bool DependencyResolver::infer_dependencies_from_structure(const std::string& package_path, DependencyNode& node) {
    try {
        // 从目录结构推断依赖（简化实现）
        // 检查是否有第三方库目录
        std::vector<std::string> third_party_dirs = {
            "third_party",
            "external",
            "deps",
            "dependencies",
            "vendor"
        };
        
        for (const auto& dir : third_party_dirs) {
            fs::path third_party_path = fs::path(package_path) / dir;
            if (fs::exists(third_party_path) && fs::is_directory(third_party_path)) {
                try {
                    for (const auto& entry : fs::directory_iterator(third_party_path)) {
                        if (entry.is_directory()) {
                            std::string dep_name = entry.path().filename().string();
                            node.dependencies.insert(dep_name);
                        }
                    }
                } catch (const std::exception& e) {
                    LOG(WARNING) << "Error reading directory " << third_party_path << ": " << e.what();
                    continue;
                }
            }
        }
        
        return !node.dependencies.empty();
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in infer_dependencies_from_structure: " << e.what();
        return false;
    }
}

bool DependencyResolver::parse_version_constraints(const std::string& constraints_str,
                                                 std::map<std::string, VersionConstraint>& constraints) {
    try {
        std::istringstream iss(constraints_str);
        std::string constraint;
        
        while (std::getline(iss, constraint, ',')) {
            // 去除空白字符
            constraint.erase(0, constraint.find_first_not_of(" \t"));
            constraint.erase(constraint.find_last_not_of(" \t") + 1);
            
            if (!constraint.empty()) {
                size_t space_pos = constraint.find(' ');
                if (space_pos != std::string::npos) {
                    std::string package = constraint.substr(0, space_pos);
                    std::string version = constraint.substr(space_pos + 1);
                    constraints[package] = VersionConstraint::parse(version);
                }
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(WARNING) << "Failed to parse version constraints: " << e.what();
        return false;
    }
}

bool DependencyResolver::is_package_resolved(const std::string& package) const {
    return graph_.has_node(package);
}

std::string DependencyResolver::get_package_install_path(const std::string& package) const {
    return "packages/" + package;
}

bool DependencyResolver::validate_package(const std::string& package, const std::string& version) {
    // 检查包是否存在于仓库中
    if (get_repository_url(package).empty()) {
        LOG(WARNING) << "Package not found in any repository: " << package;
        return false;
    }
    
    // 检查版本是否有效
    if (!version.empty() && version != "*") {
        SemanticVersion semver(version);
        if (!semver.parse(version)) {
            LOG(WARNING) << "Invalid version format: " << version;
            return false;
        }
    }
    
    return true;
}

bool DependencyResolver::enable_incremental_parsing(bool enable) {
    if (incremental_parser_) {
        auto config = incremental_parser_->get_config();
        config.enable_incremental = enable;
        incremental_parser_->set_config(config);
        LOG(INFO) << "Incremental parsing " << (enable ? "enabled" : "disabled");
        return true;
    }
    return false;
}

bool DependencyResolver::is_incremental_parsing_enabled() const {
    if (incremental_parser_) {
        return incremental_parser_->get_config().enable_incremental;
    }
    return false;
}

IncrementalParser* DependencyResolver::get_incremental_parser() const {
    return incremental_parser_;
}

} // namespace Paker 