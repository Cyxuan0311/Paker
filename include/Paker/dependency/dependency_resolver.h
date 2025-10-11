#pragma once

#include <string>
#include <map>
#include <memory>
#include "dependency/dependency_graph.h"

namespace Paker {

// 前向声明
class IncrementalParser;

// 依赖解析器
class DependencyResolver {
private:
    DependencyGraph graph_;
    std::map<std::string, std::string> repositories_;
    bool recursive_mode_;
    IncrementalParser* incremental_parser_;
    
    // 内部辅助方法
    void scan_installed_packages();
    
public:
    DependencyResolver();
    ~DependencyResolver();
    
    // 解析单个包的依赖
    bool resolve_package(const std::string& package, const std::string& version = "");
    
    // 解析整个项目的依赖树
    bool resolve_project_dependencies();
    
    // 递归解析依赖
    bool resolve_recursive_dependencies(const std::string& package, const std::string& version = "");
    
    // 获取解析后的依赖图
    const DependencyGraph& get_dependency_graph() const { return graph_; }
    DependencyGraph& get_dependency_graph() { return graph_; }
    
    // 检查依赖完整性
    bool validate_dependencies();
    
    // 设置仓库映射
    void set_repositories(const std::map<std::string, std::string>& repos);
    
    // 添加仓库
    void add_repository(const std::string& name, const std::string& url);
    
    // 获取仓库URL
    std::string get_repository_url(const std::string& package) const;
    
    // 设置递归模式
    void set_recursive_mode(bool recursive) { recursive_mode_ = recursive; }
    
    // 获取递归模式
    bool get_recursive_mode() const { return recursive_mode_; }
    
    // 清空解析器状态
    void clear();
    
    // 从JSON文件加载依赖
    bool load_dependencies_from_json(const std::string& json_file);
    
    // 保存依赖到JSON文件
    bool save_dependencies_to_json(const std::string& json_file) const;
    
    // 增量解析功能
    bool enable_incremental_parsing(bool enable = true);
    bool is_incremental_parsing_enabled() const;
    
    // 获取增量解析器
    IncrementalParser* get_incremental_parser() const;
    
private:
    // 解析包的元数据
    bool parse_package_metadata(const std::string& package, const std::string& version);
    
    // 读取包的依赖信息
    bool read_package_dependencies(const std::string& package_path, DependencyNode& node);
    
    // 解析版本约束
    bool parse_version_constraints(const std::string& constraints_str, 
                                 std::map<std::string, VersionConstraint>& constraints);
    
    // 检查包是否已解析
    bool is_package_resolved(const std::string& package) const;
    
    // 获取包的安装路径
    std::string get_package_install_path(const std::string& package) const;
    
    // 验证包的有效性
    bool validate_package(const std::string& package, const std::string& version);
    
private:
    // 从文件读取依赖
    bool read_dependencies_from_file(const std::string& file_path, DependencyNode& node);
    
    // 从JSON文件读取依赖
    bool read_dependencies_from_json(std::ifstream& ifs, DependencyNode& node);
    
    // 从CMake文件读取依赖
    bool read_dependencies_from_cmake(std::ifstream& ifs, DependencyNode& node);
    
    // 从目录结构推断依赖
    bool infer_dependencies_from_structure(const std::string& package_path, DependencyNode& node);
    
    // 验证包名是否有效
    bool is_valid_package_name(const std::string& name);
};

} // namespace Paker 