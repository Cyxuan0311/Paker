#include "Paker/dependency/dependency_graph.h"
#include "Paker/dependency/version_manager.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <unordered_set>
#include <glog/logging.h>

namespace Paker {

// VersionConstraint 实现
bool VersionConstraint::satisfies(const std::string& version) const {
    if (op == VersionOp::ANY) return true;
    
    SemanticVersion semver(version);
    SemanticVersion constraint_version(this->version);
    
    switch (op) {
        case VersionOp::EQ:
            return semver == constraint_version;
        case VersionOp::GT:
            return semver > constraint_version;
        case VersionOp::GTE:
            return semver >= constraint_version;
        case VersionOp::LT:
            return semver < constraint_version;
        case VersionOp::LTE:
            return semver <= constraint_version;
        case VersionOp::NE:
            return semver != constraint_version;
        default:
            return false;
    }
}

std::string VersionConstraint::to_string() const {
    switch (op) {
        case VersionOp::EQ: return "=" + version;
        case VersionOp::GT: return ">" + version;
        case VersionOp::GTE: return ">=" + version;
        case VersionOp::LT: return "<" + version;
        case VersionOp::LTE: return "<=" + version;
        case VersionOp::NE: return "!=" + version;
        case VersionOp::ANY: return "*";
        default: return "unknown";
    }
}

VersionConstraint VersionConstraint::parse(const std::string& constraint) {
    if (constraint.empty() || constraint == "*") {
        return VersionConstraint(VersionOp::ANY);
    }
    
    std::string op_str, version_str;
    
    if (constraint[0] == '=' || constraint[0] == '>' || constraint[0] == '<' || constraint[0] == '!') {
        if (constraint[1] == '=') {
            op_str = constraint.substr(0, 2);
            version_str = constraint.substr(2);
        } else {
            op_str = constraint.substr(0, 1);
            version_str = constraint.substr(1);
        }
    } else {
        // 默认为等于
        op_str = "=";
        version_str = constraint;
    }
    
    VersionOp op = VersionOp::EQ;
    if (op_str == ">") op = VersionOp::GT;
    else if (op_str == ">=") op = VersionOp::GTE;
    else if (op_str == "<") op = VersionOp::LT;
    else if (op_str == "<=") op = VersionOp::LTE;
    else if (op_str == "!=") op = VersionOp::NE;
    
    return VersionConstraint(op, version_str);
}

// DependencyGraph 实现
void DependencyGraph::add_node(const DependencyNode& node) {
    nodes_[node.name] = node;
    if (adjacency_list_.find(node.name) == adjacency_list_.end()) {
        adjacency_list_[node.name] = std::set<std::string>();
    }
}

void DependencyGraph::add_dependency(const std::string& from, const std::string& to) {
    if (nodes_.find(from) == nodes_.end() || nodes_.find(to) == nodes_.end()) {
        LOG(WARNING) << "Cannot add dependency: node not found";
        return;
    }
    
    adjacency_list_[from].insert(to);
    nodes_[from].dependencies.insert(to);
}

const DependencyNode* DependencyGraph::get_node(const std::string& name) const {
    auto it = nodes_.find(name);
    return it != nodes_.end() ? &it->second : nullptr;
}

DependencyNode* DependencyGraph::get_node(const std::string& name) {
    auto it = nodes_.find(name);
    return it != nodes_.end() ? &it->second : nullptr;
}

bool DependencyGraph::has_node(const std::string& name) const {
    return nodes_.find(name) != nodes_.end();
}

std::set<std::string> DependencyGraph::get_dependencies(const std::string& name) const {
    auto it = adjacency_list_.find(name);
    return it != adjacency_list_.end() ? it->second : std::set<std::string>();
}

std::vector<std::string> DependencyGraph::topological_sort() const {
    std::vector<std::string> result;
    std::map<std::string, int> in_degree;
    std::queue<std::string> q;
    
    // 计算入度
    for (const auto& [node, deps] : adjacency_list_) {
        if (in_degree.find(node) == in_degree.end()) {
            in_degree[node] = 0;
        }
        for (const auto& dep : deps) {
            in_degree[dep]++;
        }
    }
    
    // 将入度为0的节点加入队列
    for (const auto& [node, degree] : in_degree) {
        if (degree == 0) {
            q.push(node);
        }
    }
    
    // 拓扑排序
    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        result.push_back(current);
        
        for (const auto& dep : get_dependencies(current)) {
            in_degree[dep]--;
            if (in_degree[dep] == 0) {
                q.push(dep);
            }
        }
    }
    
    // 检查是否有循环依赖
    if (result.size() != nodes_.size()) {
        LOG(WARNING) << "Circular dependency detected in topological sort";
    }
    
    return result;
}

std::vector<std::vector<std::string>> DependencyGraph::detect_cycles() const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> rec_stack;
    std::vector<std::string> path;
    
    for (const auto& [node_name, _] : nodes_) {
        if (visited.find(node_name) == visited.end()) {
            dfs_cycle_detection(node_name, visited, rec_stack, path, cycles);
        }
    }
    
    return cycles;
}

void DependencyGraph::dfs_cycle_detection(const std::string& node,
                                        std::unordered_set<std::string>& visited,
                                        std::unordered_set<std::string>& rec_stack,
                                        std::vector<std::string>& path,
                                        std::vector<std::vector<std::string>>& cycles) const {
    visited.insert(node);
    rec_stack.insert(node);
    path.push_back(node);
    
    for (const auto& dep : get_dependencies(node)) {
        if (visited.find(dep) == visited.end()) {
            dfs_cycle_detection(dep, visited, rec_stack, path, cycles);
        } else if (rec_stack.find(dep) != rec_stack.end()) {
            // 找到循环
            auto cycle_start = std::find(path.begin(), path.end(), dep);
            if (cycle_start != path.end()) {
                std::vector<std::string> cycle(cycle_start, path.end());
                cycle.push_back(dep);
                cycles.push_back(cycle);
            }
        }
    }
    
    rec_stack.erase(node);
    path.pop_back();
}

std::vector<std::vector<std::string>> DependencyGraph::get_all_paths(const std::string& from, const std::string& to) const {
    std::vector<std::vector<std::string>> paths;
    std::vector<std::string> current_path;
    std::unordered_set<std::string> visited;
    
    dfs_find_paths(from, to, visited, current_path, paths);
    return paths;
}

void DependencyGraph::dfs_find_paths(const std::string& current,
                                   const std::string& target,
                                   std::unordered_set<std::string>& visited,
                                   std::vector<std::string>& current_path,
                                   std::vector<std::vector<std::string>>& paths) const {
    visited.insert(current);
    current_path.push_back(current);
    
    if (current == target) {
        paths.push_back(current_path);
    } else {
        for (const auto& dep : get_dependencies(current)) {
            if (visited.find(dep) == visited.end()) {
                dfs_find_paths(dep, target, visited, current_path, paths);
            }
        }
    }
    
    visited.erase(current);
    current_path.pop_back();
}

std::vector<std::vector<std::string>> DependencyGraph::get_all_paths_to_package(const std::string& package) const {
    std::vector<std::vector<std::string>> all_paths;
    
    for (const auto& [node_name, _] : nodes_) {
        if (node_name != package) {
            auto paths = get_all_paths(node_name, package);
            all_paths.insert(all_paths.end(), paths.begin(), paths.end());
        }
    }
    
    return all_paths;
}

void DependencyGraph::clear() {
    nodes_.clear();
    adjacency_list_.clear();
}

} // namespace Paker 