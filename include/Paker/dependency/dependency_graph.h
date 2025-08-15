#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <unordered_set>

namespace Paker {

// 版本约束操作符
enum class VersionOp {
    EQ,     // 等于
    GT,     // 大于
    GTE,    // 大于等于
    LT,     // 小于
    LTE,    // 小于等于
    NE,     // 不等于
    ANY     // 任意版本
};

// 版本约束
struct VersionConstraint {
    VersionOp op;
    std::string version;
    
    VersionConstraint(VersionOp op = VersionOp::ANY, const std::string& version = "")
        : op(op), version(version) {}
    
    bool satisfies(const std::string& version) const;
    std::string to_string() const;
    
    static VersionConstraint parse(const std::string& constraint);
};

// 依赖图节点
struct DependencyNode {
    std::string name;                    // 包名
    std::string version;                 // 版本
    std::string repository;              // 仓库地址
    std::set<std::string> dependencies;  // 直接依赖
    std::map<std::string, VersionConstraint> version_constraints; // 版本约束
    bool is_installed;                   // 是否已安装
    std::string install_path;            // 安装路径
    
    DependencyNode() : is_installed(false) {}
    DependencyNode(const std::string& name, const std::string& version = "")
        : name(name), version(version), is_installed(false) {}
};

// 冲突信息
struct ConflictInfo {
    enum class Type {
        VERSION_CONFLICT,      // 版本冲突
        CIRCULAR_DEPENDENCY,   // 循环依赖
        MISSING_DEPENDENCY     // 缺失依赖
    };
    
    Type type;
    std::string package_name;
    std::vector<std::string> conflicting_versions;
    std::vector<std::string> conflict_path;  // 冲突路径
    std::string suggested_solution;
    
    ConflictInfo(Type type, const std::string& package_name)
        : type(type), package_name(package_name) {}
};

// 依赖图类
class DependencyGraph {
private:
    std::map<std::string, DependencyNode> nodes_;
    std::map<std::string, std::set<std::string>> adjacency_list_;
    
public:
    // 添加节点
    void add_node(const DependencyNode& node);
    
    // 添加边（依赖关系）
    void add_dependency(const std::string& from, const std::string& to);
    
    // 获取节点
    const DependencyNode* get_node(const std::string& name) const;
    DependencyNode* get_node(const std::string& name);
    
    // 获取所有节点
    const std::map<std::string, DependencyNode>& get_nodes() const { return nodes_; }
    
    // 检查节点是否存在
    bool has_node(const std::string& name) const;
    
    // 获取节点的直接依赖
    std::set<std::string> get_dependencies(const std::string& name) const;
    
    // 拓扑排序
    std::vector<std::string> topological_sort() const;
    
    // 检测循环依赖
    std::vector<std::vector<std::string>> detect_cycles() const;
    
    // 获取所有路径
    std::vector<std::vector<std::string>> get_all_paths(const std::string& from, const std::string& to) const;
    
    // 获取到指定包的所有路径
    std::vector<std::vector<std::string>> get_all_paths_to_package(const std::string& package) const;
    
    // 清空图
    void clear();
    
    // 获取图的大小
    size_t size() const { return nodes_.size(); }
    
    // 检查图是否为空
    bool empty() const { return nodes_.empty(); }
    
private:
    // DFS 循环检测
    void dfs_cycle_detection(const std::string& node,
                           std::unordered_set<std::string>& visited,
                           std::unordered_set<std::string>& rec_stack,
                           std::vector<std::string>& path,
                           std::vector<std::vector<std::string>>& cycles) const;
    
    // DFS 路径查找
    void dfs_find_paths(const std::string& current,
                      const std::string& target,
                      std::unordered_set<std::string>& visited,
                      std::vector<std::string>& current_path,
                      std::vector<std::vector<std::string>>& paths) const;
};

} // namespace Paker 