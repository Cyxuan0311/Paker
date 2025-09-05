#include "Paker/dependency/optimized_dependency_graph.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <fstream>
#include <algorithm>
#include <queue>
#include <stack>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace Paker {

OptimizedDependencyGraph::OptimizedDependencyGraph(size_t max_cached_nodes, 
                                                 size_t cache_cleanup_threshold)
    : max_cached_nodes_(max_cached_nodes)
    , cache_cleanup_threshold_(cache_cleanup_threshold) {
    
    // 预分配空间以减少重新分配
    nodes_.reserve(max_cached_nodes);
    name_to_index_.reserve(max_cached_nodes);
    
    LOG(INFO) << "OptimizedDependencyGraph initialized with max " << max_cached_nodes_ 
              << " cached nodes";
}

OptimizedDependencyGraph::~OptimizedDependencyGraph() {
    optimize_memory();
}

size_t OptimizedDependencyGraph::add_node(const LightweightDependencyNode& node) {
    // 检查是否已存在
    auto it = name_to_index_.find(node.name);
    if (it != name_to_index_.end()) {
        // 更新现有节点
        size_t index = it->second;
        nodes_[index] = node;
        update_access_time(index);
        return index;
    }
    
    // 添加新节点
    size_t index = nodes_.size();
    nodes_.push_back(node);
    name_to_index_[node.name] = index;
    access_counts_[index] = 1;
    
    // 检查是否需要清理缓存
    if (nodes_.size() > cache_cleanup_threshold_) {
        cleanup_cache();
    }
    
    LOG(DEBUG) << "Added node: " << node.name << " at index " << index;
    return index;
}

bool OptimizedDependencyGraph::remove_node(const std::string& name) {
    auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
        return false;
    }
    
    size_t index = it->second;
    
    // 移除所有依赖关系
    for (size_t dep_index : nodes_[index].dependency_indices) {
        auto& dep_node = nodes_[dep_index];
        auto dep_it = std::find(dep_node.dependent_indices.begin(), 
                               dep_node.dependent_indices.end(), index);
        if (dep_it != dep_node.dependent_indices.end()) {
            dep_node.dependent_indices.erase(dep_it);
        }
    }
    
    for (size_t dep_index : nodes_[index].dependent_indices) {
        auto& dep_node = nodes_[dep_index];
        auto dep_it = std::find(dep_node.dependency_indices.begin(), 
                               dep_node.dependency_indices.end(), index);
        if (dep_it != dep_node.dependency_indices.end()) {
            dep_node.dependency_indices.erase(dep_it);
        }
    }
    
    // 移除节点
    nodes_.erase(nodes_.begin() + index);
    name_to_index_.erase(it);
    access_counts_.erase(index);
    
    // 更新索引映射
    for (auto& [name, idx] : name_to_index_) {
        if (idx > index) {
            idx--;
        }
    }
    
    // 更新所有节点的依赖索引
    for (auto& node : nodes_) {
        for (auto& dep_idx : node.dependency_indices) {
            if (dep_idx > index) {
                dep_idx--;
            }
        }
        for (auto& dep_idx : node.dependent_indices) {
            if (dep_idx > index) {
                dep_idx--;
            }
        }
    }
    
    LOG(DEBUG) << "Removed node: " << name;
    return true;
}

bool OptimizedDependencyGraph::has_node(const std::string& name) const {
    return name_to_index_.find(name) != name_to_index_.end();
}

const LightweightDependencyNode* OptimizedDependencyGraph::get_node(const std::string& name) const {
    auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
        return nullptr;
    }
    
    size_t index = it->second;
    update_access_time(index);
    access_counts_[index]++;
    
    return &nodes_[index];
}

LightweightDependencyNode* OptimizedDependencyGraph::get_node(const std::string& name) {
    auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
        return nullptr;
    }
    
    size_t index = it->second;
    update_access_time(index);
    access_counts_[index]++;
    
    return &nodes_[index];
}

const LightweightDependencyNode* OptimizedDependencyGraph::get_node_by_index(size_t index) const {
    if (index >= nodes_.size()) {
        return nullptr;
    }
    
    update_access_time(index);
    access_counts_[index]++;
    
    return &nodes_[index];
}

LightweightDependencyNode* OptimizedDependencyGraph::get_node_by_index(size_t index) {
    if (index >= nodes_.size()) {
        return nullptr;
    }
    
    update_access_time(index);
    access_counts_[index]++;
    
    return &nodes_[index];
}

bool OptimizedDependencyGraph::add_dependency(const std::string& from, const std::string& to) {
    auto from_it = name_to_index_.find(from);
    auto to_it = name_to_index_.find(to);
    
    if (from_it == name_to_index_.end() || to_it == name_to_index_.end()) {
        return false;
    }
    
    size_t from_index = from_it->second;
    size_t to_index = to_it->second;
    
    // 检查是否已存在
    auto& from_node = nodes_[from_index];
    if (std::find(from_node.dependency_indices.begin(), 
                  from_node.dependency_indices.end(), to_index) != from_node.dependency_indices.end()) {
        return true; // 已存在
    }
    
    // 添加依赖关系
    from_node.dependency_indices.push_back(to_index);
    nodes_[to_index].dependent_indices.push_back(from_index);
    
    LOG(DEBUG) << "Added dependency: " << from << " -> " << to;
    return true;
}

bool OptimizedDependencyGraph::remove_dependency(const std::string& from, const std::string& to) {
    auto from_it = name_to_index_.find(from);
    auto to_it = name_to_index_.find(to);
    
    if (from_it == name_to_index_.end() || to_it == name_to_index_.end()) {
        return false;
    }
    
    size_t from_index = from_it->second;
    size_t to_index = to_it->second;
    
    // 移除依赖关系
    auto& from_node = nodes_[from_index];
    auto dep_it = std::find(from_node.dependency_indices.begin(), 
                           from_node.dependency_indices.end(), to_index);
    if (dep_it != from_node.dependency_indices.end()) {
        from_node.dependency_indices.erase(dep_it);
    }
    
    auto& to_node = nodes_[to_index];
    auto dep_it2 = std::find(to_node.dependent_indices.begin(), 
                            to_node.dependent_indices.end(), from_index);
    if (dep_it2 != to_node.dependent_indices.end()) {
        to_node.dependent_indices.erase(dep_it2);
    }
    
    LOG(DEBUG) << "Removed dependency: " << from << " -> " << to;
    return true;
}

std::vector<std::string> OptimizedDependencyGraph::get_dependencies(const std::string& name) const {
    const auto* node = get_node(name);
    if (!node) {
        return {};
    }
    
    std::vector<std::string> dependencies;
    for (size_t dep_index : node->dependency_indices) {
        if (dep_index < nodes_.size()) {
            dependencies.push_back(nodes_[dep_index].name);
        }
    }
    
    return dependencies;
}

std::vector<std::string> OptimizedDependencyGraph::get_dependents(const std::string& name) const {
    const auto* node = get_node(name);
    if (!node) {
        return {};
    }
    
    std::vector<std::string> dependents;
    for (size_t dep_index : node->dependent_indices) {
        if (dep_index < nodes_.size()) {
            dependents.push_back(nodes_[dep_index].name);
        }
    }
    
    return dependents;
}

std::vector<std::string> OptimizedDependencyGraph::topological_sort() const {
    std::vector<std::string> result;
    std::vector<int> in_degree(nodes_.size(), 0);
    std::queue<size_t> q;
    
    // 计算入度
    for (size_t i = 0; i < nodes_.size(); ++i) {
        in_degree[i] = nodes_[i].dependent_indices.size();
        if (in_degree[i] == 0) {
            q.push(i);
        }
    }
    
    // 拓扑排序
    while (!q.empty()) {
        size_t current = q.front();
        q.pop();
        result.push_back(nodes_[current].name);
        
        for (size_t dep_index : nodes_[current].dependency_indices) {
            in_degree[dep_index]--;
            if (in_degree[dep_index] == 0) {
                q.push(dep_index);
            }
        }
    }
    
    // 检查是否有循环依赖
    if (result.size() != nodes_.size()) {
        LOG(WARNING) << "Circular dependency detected in topological sort";
    }
    
    return result;
}

std::vector<std::vector<std::string>> OptimizedDependencyGraph::detect_cycles() const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<size_t> visited;
    std::unordered_set<size_t> rec_stack;
    std::vector<size_t> path;
    
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (visited.find(i) == visited.end()) {
            dfs_cycle_detection(i, visited, rec_stack, path, cycles);
        }
    }
    
    return cycles;
}

std::vector<std::vector<std::string>> OptimizedDependencyGraph::get_all_paths(const std::string& from, 
                                                                             const std::string& to) const {
    auto from_it = name_to_index_.find(from);
    auto to_it = name_to_index_.find(to);
    
    if (from_it == name_to_index_.end() || to_it == name_to_index_.end()) {
        return {};
    }
    
    size_t from_index = from_it->second;
    size_t to_index = to_it->second;
    
    std::vector<std::vector<std::string>> all_paths;
    std::unordered_set<size_t> visited;
    std::vector<size_t> current_path;
    
    dfs_path_finding(from_index, to_index, visited, current_path, all_paths);
    
    return all_paths;
}

void OptimizedDependencyGraph::optimize_memory() {
    LOG(INFO) << "Optimizing memory usage...";
    
    // 清理未使用的节点
    cleanup_cache();
    
    // 压缩向量
    nodes_.shrink_to_fit();
    
    LOG(INFO) << "Memory optimization completed. Nodes: " << nodes_.size() 
              << ", Memory usage: " << get_memory_usage() << " bytes";
}

void OptimizedDependencyGraph::clear_cache() {
    for (auto& node : nodes_) {
        node.is_cached = false;
    }
    access_counts_.clear();
    
    LOG(INFO) << "Cache cleared";
}

size_t OptimizedDependencyGraph::get_memory_usage() const {
    size_t usage = 0;
    
    // 节点向量
    usage += nodes_.capacity() * sizeof(LightweightDependencyNode);
    
    // 依赖索引
    for (const auto& node : nodes_) {
        usage += node.dependency_indices.capacity() * sizeof(size_t);
        usage += node.dependent_indices.capacity() * sizeof(size_t);
    }
    
    // 映射表
    usage += name_to_index_.bucket_count() * sizeof(std::pair<std::string, size_t>);
    usage += access_counts_.bucket_count() * sizeof(std::pair<size_t, size_t>);
    
    return usage;
}

size_t OptimizedDependencyGraph::get_cached_nodes_count() const {
    size_t count = 0;
    for (const auto& node : nodes_) {
        if (node.is_cached) {
            count++;
        }
    }
    return count;
}

size_t OptimizedDependencyGraph::get_edge_count() const {
    size_t count = 0;
    for (const auto& node : nodes_) {
        count += node.dependency_indices.size();
    }
    return count;
}

std::map<std::string, size_t> OptimizedDependencyGraph::get_access_statistics() const {
    std::map<std::string, size_t> stats;
    for (const auto& [index, count] : access_counts_) {
        if (index < nodes_.size()) {
            stats[nodes_[index].name] = count;
        }
    }
    return stats;
}

bool OptimizedDependencyGraph::save_to_file(const std::string& filename) const {
    try {
        json j;
        
        for (size_t i = 0; i < nodes_.size(); ++i) {
            const auto& node = nodes_[i];
            json node_json;
            node_json["name"] = node.name;
            node_json["version"] = node.version;
            node_json["repository"] = node.repository;
            node_json["is_installed"] = node.is_installed;
            node_json["install_path"] = node.install_path;
            
            // 保存依赖关系
            json deps = json::array();
            for (size_t dep_index : node.dependency_indices) {
                if (dep_index < nodes_.size()) {
                    deps.push_back(nodes_[dep_index].name);
                }
            }
            node_json["dependencies"] = deps;
            
            j["nodes"].push_back(node_json);
        }
        
        std::ofstream file(filename);
        file << j.dump(2);
        
        LOG(INFO) << "Saved dependency graph to " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to save dependency graph: " << e.what();
        return false;
    }
}

bool OptimizedDependencyGraph::load_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file) {
            LOG(ERROR) << "Cannot open file: " << filename;
            return false;
        }
        
        json j;
        file >> j;
        
        // 清空现有数据
        nodes_.clear();
        name_to_index_.clear();
        access_counts_.clear();
        
        // 加载节点
        for (const auto& node_json : j["nodes"]) {
            LightweightDependencyNode node;
            node.name = node_json["name"];
            node.version = node_json["version"];
            node.repository = node_json["repository"];
            node.is_installed = node_json["is_installed"];
            node.install_path = node_json["install_path"];
            
            add_node(node);
        }
        
        // 重建依赖关系
        size_t node_index = 0;
        for (const auto& node_json : j["nodes"]) {
            for (const auto& dep_name : node_json["dependencies"]) {
                add_dependency(nodes_[node_index].name, dep_name);
            }
            node_index++;
        }
        
        LOG(INFO) << "Loaded dependency graph from " << filename;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to load dependency graph: " << e.what();
        return false;
    }
}

void OptimizedDependencyGraph::add_nodes_batch(const std::vector<LightweightDependencyNode>& nodes) {
    nodes_.reserve(nodes_.size() + nodes.size());
    
    for (const auto& node : nodes) {
        add_node(node);
    }
    
    LOG(INFO) << "Added " << nodes.size() << " nodes in batch";
}

void OptimizedDependencyGraph::remove_nodes_batch(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        remove_node(name);
    }
    
    LOG(INFO) << "Removed " << names.size() << " nodes in batch";
}

void OptimizedDependencyGraph::cleanup_cache() {
    if (nodes_.size() <= max_cached_nodes_) {
        return;
    }
    
    LOG(INFO) << "Cleaning up cache, current size: " << nodes_.size();
    
    // 按访问次数排序，移除最少使用的节点
    std::vector<std::pair<size_t, size_t>> access_pairs;
    for (const auto& [index, count] : access_counts_) {
        access_pairs.emplace_back(index, count);
    }
    
    std::sort(access_pairs.begin(), access_pairs.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // 移除最少使用的节点
    size_t to_remove = nodes_.size() - max_cached_nodes_;
    for (size_t i = 0; i < to_remove && i < access_pairs.size(); ++i) {
        size_t index = access_pairs[i].first;
        if (index < nodes_.size()) {
            nodes_[index].is_cached = false;
        }
    }
    
    LOG(INFO) << "Cache cleanup completed, removed " << to_remove << " nodes";
}

void OptimizedDependencyGraph::update_access_time(size_t index) const {
    if (index < nodes_.size()) {
        nodes_[index].last_access = std::chrono::system_clock::now();
        nodes_[index].is_cached = true;
    }
}

void OptimizedDependencyGraph::evict_least_used_nodes() {
    cleanup_cache();
}

size_t OptimizedDependencyGraph::get_node_index(const std::string& name) const {
    auto it = name_to_index_.find(name);
    return it != name_to_index_.end() ? it->second : SIZE_MAX;
}

void OptimizedDependencyGraph::dfs_cycle_detection(size_t node_index,
                                                 std::unordered_set<size_t>& visited,
                                                 std::unordered_set<size_t>& rec_stack,
                                                 std::vector<size_t>& path,
                                                 std::vector<std::vector<std::string>>& cycles) const {
    if (node_index >= nodes_.size()) return;
    
    visited.insert(node_index);
    rec_stack.insert(node_index);
    path.push_back(node_index);
    
    for (size_t dep_index : nodes_[node_index].dependency_indices) {
        if (dep_index >= nodes_.size()) continue;
        
        if (visited.find(dep_index) == visited.end()) {
            dfs_cycle_detection(dep_index, visited, rec_stack, path, cycles);
        } else if (rec_stack.find(dep_index) != rec_stack.end()) {
            // 找到循环
            std::vector<std::string> cycle;
            auto cycle_start = std::find(path.begin(), path.end(), dep_index);
            for (auto it = cycle_start; it != path.end(); ++it) {
                cycle.push_back(nodes_[*it].name);
            }
            cycle.push_back(nodes_[dep_index].name);
            cycles.push_back(cycle);
        }
    }
    
    rec_stack.erase(node_index);
    path.pop_back();
}

void OptimizedDependencyGraph::dfs_path_finding(size_t from_index, size_t to_index,
                                              std::unordered_set<size_t>& visited,
                                              std::vector<size_t>& current_path,
                                              std::vector<std::vector<std::string>>& all_paths) const {
    if (from_index >= nodes_.size()) return;
    
    if (from_index == to_index) {
        std::vector<std::string> path;
        for (size_t idx : current_path) {
            path.push_back(nodes_[idx].name);
        }
        path.push_back(nodes_[to_index].name);
        all_paths.push_back(path);
        return;
    }
    
    visited.insert(from_index);
    current_path.push_back(from_index);
    
    for (size_t dep_index : nodes_[from_index].dependency_indices) {
        if (dep_index < nodes_.size() && visited.find(dep_index) == visited.end()) {
            dfs_path_finding(dep_index, to_index, visited, current_path, all_paths);
        }
    }
    
    visited.erase(from_index);
    current_path.pop_back();
}

// DependencyGraphBuilder 实现
DependencyGraphBuilder::DependencyGraphBuilder() {
    graph_ = std::make_unique<OptimizedDependencyGraph>();
}

bool DependencyGraphBuilder::build_from_packages(const std::map<std::string, std::string>& packages) {
    try {
        for (const auto& [package, version] : packages) {
            if (!resolve_package_dependencies(package, version)) {
                LOG(WARNING) << "Failed to resolve dependencies for " << package;
            }
        }
        
        LOG(INFO) << "Built dependency graph with " << graph_->get_node_count() << " nodes";
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to build dependency graph: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::build_from_json(const std::string& json_file) {
    try {
        std::ifstream file(json_file);
        if (!file) {
            LOG(ERROR) << "Cannot open JSON file: " << json_file;
            return false;
        }
        
        json j;
        file >> j;
        
        if (j.contains("dependencies")) {
            std::map<std::string, std::string> packages;
            for (const auto& [name, version] : j["dependencies"].items()) {
                packages[name] = version;
            }
            return build_from_packages(packages);
        }
        
        return false;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to build from JSON: " << e.what();
        return false;
    }
}

std::unique_ptr<OptimizedDependencyGraph> DependencyGraphBuilder::get_graph() {
    return std::move(graph_);
}

const OptimizedDependencyGraph* DependencyGraphBuilder::get_graph() const {
    return graph_.get();
}

void DependencyGraphBuilder::set_repositories(const std::map<std::string, std::string>& repos) {
    repositories_ = repos;
}

void DependencyGraphBuilder::add_repository(const std::string& name, const std::string& url) {
    repositories_[name] = url;
}

bool DependencyGraphBuilder::resolve_package_dependencies(const std::string& package, const std::string& version) {
    // 创建节点
    LightweightDependencyNode node(package, version);
    
    // 设置仓库URL
    auto repo_it = repositories_.find(package);
    if (repo_it != repositories_.end()) {
        node.repository = repo_it->second;
    }
    
    // 添加到图
    graph_->add_node(node);
    
    // 这里可以添加实际的依赖解析逻辑
    // 目前简化实现
    
    return true;
}

bool DependencyGraphBuilder::read_package_metadata(const std::string& package_path, LightweightDependencyNode& node) {
    // 这里可以添加读取包元数据的逻辑
    // 目前简化实现
    return true;
}

// DependencyGraphAnalyzer 实现
DependencyGraphAnalyzer::DependencyGraphAnalyzer(const OptimizedDependencyGraph* graph) 
    : graph_(graph) {
}

DependencyGraphAnalyzer::AnalysisResult DependencyGraphAnalyzer::analyze_structure() const {
    AnalysisResult result;
    result.total_packages = graph_->get_node_count();
    
    std::map<size_t, size_t> depth_dist;
    std::map<size_t, size_t> breadth_dist;
    
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (!node) continue;
        
        // 计算深度
        std::unordered_set<size_t> visited;
        size_t depth = calculate_depth(i, visited);
        depth_dist[depth]++;
        result.max_depth = std::max(result.max_depth, depth);
        
        // 计算广度
        size_t breadth = calculate_breadth(i);
        breadth_dist[breadth]++;
        result.max_breadth = std::max(result.max_breadth, breadth);
        
        // 检查是否为叶子节点
        if (node->dependency_indices.empty()) {
            result.leaf_packages.push_back(node->name);
        }
        
        // 检查是否为根节点
        if (node->dependent_indices.empty()) {
            result.root_packages.push_back(node->name);
        }
    }
    
    result.depth_distribution = depth_dist;
    result.breadth_distribution = breadth_dist;
    
    return result;
}

DependencyGraphAnalyzer::PerformanceMetrics DependencyGraphAnalyzer::analyze_performance() const {
    PerformanceMetrics metrics;
    
    double total_depth = 0;
    double total_dependent_count = 0;
    size_t max_connections = 0;
    std::string most_connected;
    
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (!node) continue;
        
        // 计算平均深度
        std::unordered_set<size_t> visited;
        size_t depth = calculate_depth(i, visited);
        total_depth += depth;
        
        // 计算平均依赖数
        total_dependent_count += node->dependent_indices.size();
        
        // 找到最多连接的包
        if (node->dependent_indices.size() > max_connections) {
            max_connections = node->dependent_indices.size();
            most_connected = node->name;
        }
    }
    
    metrics.average_dependency_depth = graph_->get_node_count() > 0 ? 
        total_depth / graph_->get_node_count() : 0;
    metrics.average_dependent_count = graph_->get_node_count() > 0 ? 
        total_dependent_count / graph_->get_node_count() : 0;
    metrics.most_connected_package_count = max_connections;
    metrics.most_connected_package = most_connected;
    
    // 找到关键包（被最多包依赖的包）
    std::map<std::string, size_t> dependent_counts;
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (!node) continue;
        
        dependent_counts[node->name] = node->dependent_indices.size();
    }
    
    // 按依赖数排序，取前10个
    std::vector<std::pair<std::string, size_t>> sorted_deps(
        dependent_counts.begin(), dependent_counts.end());
    std::sort(sorted_deps.begin(), sorted_deps.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(10), sorted_deps.size()); ++i) {
        metrics.critical_packages.push_back(sorted_deps[i].first);
    }
    
    return metrics;
}

std::vector<std::string> DependencyGraphAnalyzer::find_critical_dependencies() const {
    std::vector<std::string> critical;
    
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (!node) continue;
        
        // 如果被超过一半的包依赖，认为是关键依赖
        if (node->dependent_indices.size() > graph_->get_node_count() / 2) {
            critical.push_back(node->name);
        }
    }
    
    return critical;
}

std::vector<std::string> DependencyGraphAnalyzer::find_orphaned_packages() const {
    std::vector<std::string> orphaned;
    
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (!node) continue;
        
        // 如果没有被任何包依赖，认为是孤儿包
        if (node->dependent_indices.empty()) {
            orphaned.push_back(node->name);
        }
    }
    
    return orphaned;
}

std::vector<std::vector<std::string>> DependencyGraphAnalyzer::find_dependency_chains() const {
    std::vector<std::vector<std::string>> chains;
    
    // 找到所有根节点
    std::vector<size_t> root_nodes;
    for (size_t i = 0; i < graph_->get_node_count(); ++i) {
        const auto* node = graph_->get_node_by_index(i);
        if (node && node->dependent_indices.empty()) {
            root_nodes.push_back(i);
        }
    }
    
    // 从每个根节点开始，找到最长的依赖链
    for (size_t root : root_nodes) {
        std::unordered_set<size_t> visited;
        std::vector<size_t> current_chain;
        find_longest_chain(root, visited, current_chain, chains);
    }
    
    return chains;
}

size_t DependencyGraphAnalyzer::calculate_depth(size_t node_index, std::unordered_set<size_t>& visited) const {
    if (visited.find(node_index) != visited.end()) {
        return 0; // 避免循环
    }
    
    visited.insert(node_index);
    
    const auto* node = graph_->get_node_by_index(node_index);
    if (!node || node->dependency_indices.empty()) {
        return 0;
    }
    
    size_t max_depth = 0;
    for (size_t dep_index : node->dependency_indices) {
        size_t depth = calculate_depth(dep_index, visited) + 1;
        max_depth = std::max(max_depth, depth);
    }
    
    visited.erase(node_index);
    return max_depth;
}

size_t DependencyGraphAnalyzer::calculate_breadth(size_t node_index) const {
    const auto* node = graph_->get_node_by_index(node_index);
    if (!node) return 0;
    
    return node->dependency_indices.size();
}

} // namespace Paker
