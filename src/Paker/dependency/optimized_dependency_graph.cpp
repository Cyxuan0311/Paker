#include "Paker/dependency/optimized_dependency_graph.h"
#include "Paker/core/output.h"
#include <glog/logging.h>
#include <fstream>
#include <algorithm>
#include <queue>
#include <stack>
#include <filesystem>
#include <sstream>
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
    
    LOG(INFO) << "Added node: " << node.name << " at index " << index;
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
    
    LOG(INFO) << "Removed node: " << name;
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
    
    LOG(INFO) << "Added dependency: " << from << " -> " << to;
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
    
    LOG(INFO) << "Removed dependency: " << from << " -> " << to;
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
    try {
        LOG(INFO) << "Resolving dependencies for package: " << package << " version: " << version;
        
        // 创建节点
        LightweightDependencyNode node(package, version);
        
        // 设置仓库URL
        auto repo_it = repositories_.find(package);
        if (repo_it != repositories_.end()) {
            node.repository = repo_it->second;
            VLOG(1) << "Found repository for " << package << ": " << repo_it->second;
        } else {
            LOG(WARNING) << "No repository found for package: " << package;
        }
        
        // 读取包元数据
        std::string package_path = find_package_path(package, version);
        if (!package_path.empty()) {
            if (!read_package_metadata(package_path, node)) {
                LOG(WARNING) << "Failed to read metadata for package: " << package;
            }
        } else {
            LOG(WARNING) << "Package path not found for: " << package << " version: " << version;
        }
        
        // 添加到图
        size_t node_index = graph_->add_node(node);
        VLOG(1) << "Added node at index: " << node_index;
        
        // 解析依赖关系
        std::vector<std::string> dependencies = extract_dependencies(node);
        LOG(INFO) << "Found " << dependencies.size() << " dependencies for " << package;
        
        // 递归解析依赖
        for (const auto& dep : dependencies) {
            VLOG(1) << "Processing dependency: " << dep;
            
            // 检查依赖是否已存在
            if (!graph_->has_node(dep)) {
                // 尝试解析依赖版本
                std::string dep_version = resolve_dependency_version(package, dep);
                if (!dep_version.empty()) {
                    LOG(INFO) << "Resolving dependency: " << dep << " version: " << dep_version;
                    
                    // 递归解析依赖的依赖
                    if (!resolve_package_dependencies(dep, dep_version)) {
                        LOG(ERROR) << "Failed to resolve dependencies for: " << dep;
                        return false;
                    }
                } else {
                    LOG(WARNING) << "Could not resolve version for dependency: " << dep;
                }
            }
            
            // 添加依赖关系
            if (graph_->has_node(dep)) {
                graph_->add_dependency(package, dep);
                VLOG(1) << "Added dependency relationship: " << package << " -> " << dep;
            }
        }
        
        LOG(INFO) << "Successfully resolved dependencies for: " << package;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error resolving dependencies for " << package << ": " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_package_metadata(const std::string& package_path, LightweightDependencyNode& node) {
    try {
        LOG(INFO) << "Reading package metadata from: " << package_path;
        
        if (package_path.empty() || !std::filesystem::exists(package_path)) {
            LOG(WARNING) << "Package path does not exist: " << package_path;
            return false;
        }
        
        // 尝试读取C++包的元数据文件
        std::vector<std::string> cpp_metadata_files = {
            "CMakeLists.txt",
            "Makefile",
            "configure.ac",
            "configure.in",
            "autogen.sh",
            "pkg-config.pc",
            "config.h",
            "version.h",
            "dependencies.txt",
            "requirements.txt",  // C++项目也可能使用这个名称
            "vcpkg.json",
            "conanfile.txt",
            "conanfile.py"
        };
        
        for (const auto& metadata_file : cpp_metadata_files) {
            std::string full_path = package_path + "/" + metadata_file;
            if (std::filesystem::exists(full_path)) {
                VLOG(1) << "Found C++ metadata file: " << metadata_file;
                
                if (metadata_file == "CMakeLists.txt") {
                    return read_cmake_metadata(full_path, node);
                } else if (metadata_file == "Makefile") {
                    return read_makefile_metadata(full_path, node);
                } else if (metadata_file == "configure.ac" || metadata_file == "configure.in") {
                    return read_autotools_metadata(full_path, node);
                } else if (metadata_file == "pkg-config.pc") {
                    return read_pkgconfig_metadata(full_path, node);
                } else if (metadata_file == "vcpkg.json") {
                    return read_vcpkg_metadata(full_path, node);
                } else if (metadata_file == "conanfile.txt" || metadata_file == "conanfile.py") {
                    return read_conan_metadata(full_path, node);
                } else if (metadata_file == "dependencies.txt" || metadata_file == "requirements.txt") {
                    return read_cpp_requirements(full_path, node);
                }
            }
        }
        
        // 如果没有找到标准的元数据文件，尝试从目录结构推断
        LOG(INFO) << "No standard metadata files found, analyzing directory structure";
        return analyze_package_structure(package_path, node);
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading package metadata from " << package_path << ": " << e.what();
        return false;
    }
}

// 辅助函数实现
std::string DependencyGraphBuilder::find_package_path(const std::string& package, const std::string& version) const {
    (void)version; // 避免未使用参数警告
    try {
        // 在多个可能的位置查找包
        std::vector<std::string> search_paths = {
            "packages/" + package,
            "node_modules/" + package,
            "vendor/" + package,
            "lib/" + package,
            "src/" + package,
            ".paker/packages/" + package
        };
        
        for (const auto& path : search_paths) {
            if (std::filesystem::exists(path)) {
                VLOG(1) << "Found package at: " << path;
                return path;
            }
        }
        
        LOG(WARNING) << "Package path not found for: " << package;
        return "";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error finding package path for " << package << ": " << e.what();
        return "";
    }
}

std::vector<std::string> DependencyGraphBuilder::extract_dependencies(const LightweightDependencyNode& node) const {
    std::vector<std::string> dependencies;
    
    try {
        // 从节点的元数据中提取依赖
        if (!node.metadata.empty()) {
            // 这里可以解析不同格式的依赖信息
            // 目前简化实现
            VLOG(1) << "Extracting dependencies from metadata";
        }
        
        return dependencies;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error extracting dependencies: " << e.what();
        return dependencies;
    }
}

std::string DependencyGraphBuilder::resolve_dependency_version(const std::string& parent_package, const std::string& dependency) const {
    try {
        // 尝试从多个来源解析依赖版本
        // 1. 从父包的约束中获取
        // 2. 从仓库中查询最新版本
        // 3. 使用默认版本策略
        
        VLOG(1) << "Resolving version for dependency: " << dependency << " from parent: " << parent_package;
        
        // 简化实现：返回默认版本
        return "latest";
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error resolving version for " << dependency << ": " << e.what();
        return "";
    }
}

bool DependencyGraphBuilder::read_cmake_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        
        while (std::getline(file, line)) {
            // 查找项目名称
            if (line.find("project(") != std::string::npos) {
                size_t start = line.find("project(") + 8;
                size_t end = line.find(")", start);
                if (end != std::string::npos) {
                    node.name = line.substr(start, end - start);
                    // 移除可能的版本信息
                    size_t space_pos = node.name.find(' ');
                    if (space_pos != std::string::npos) {
                        node.version = node.name.substr(space_pos + 1);
                        node.name = node.name.substr(0, space_pos);
                    }
                }
            }
            
            // 查找依赖
            if (line.find("find_package(") != std::string::npos) {
                size_t start = line.find("find_package(") + 12;
                size_t end = line.find(")", start);
                if (end != std::string::npos) {
                    std::string dep = line.substr(start, end - start);
                    // 移除可能的版本约束
                    size_t space_pos = dep.find(' ');
                    if (space_pos != std::string::npos) {
                        dep = dep.substr(0, space_pos);
                    }
                    node.dependencies.push_back(dep);
                    VLOG(1) << "Found CMake dependency: " << dep;
                }
            }
            
            // 查找pkg-config依赖
            if (line.find("pkg_check_modules(") != std::string::npos) {
                size_t start = line.find("pkg_check_modules(") + 18;
                size_t end = line.find(")", start);
                if (end != std::string::npos) {
                    std::string dep = line.substr(start, end - start);
                    // 移除可能的版本约束
                    size_t space_pos = dep.find(' ');
                    if (space_pos != std::string::npos) {
                        dep = dep.substr(0, space_pos);
                    }
                    node.dependencies.push_back(dep);
                    VLOG(1) << "Found pkg-config dependency: " << dep;
                }
            }
        }
        
        LOG(INFO) << "Successfully read CMake metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading CMake metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_makefile_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 查找项目名称（通常在PROJECT_NAME或TARGET变量中）
            if (line.find("PROJECT_NAME") != std::string::npos || line.find("TARGET") != std::string::npos) {
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    node.name = line.substr(eq_pos + 1);
                    // 移除空格和制表符
                    node.name.erase(0, node.name.find_first_not_of(" \t"));
                    node.name.erase(node.name.find_last_not_of(" \t") + 1);
                }
            }
            
            // 查找链接库依赖
            if (line.find("LIBS") != std::string::npos || line.find("LDFLAGS") != std::string::npos) {
                std::istringstream iss(line);
                std::string token;
                while (iss >> token) {
                    // 查找 -l 开头的库
                    if (token.find("-l") == 0) {
                        std::string lib = token.substr(2);
                        node.dependencies.push_back(lib);
                        VLOG(1) << "Found Makefile library dependency: " << lib;
                    }
                }
            }
            
            // 查找包含路径依赖
            if (line.find("INCLUDES") != std::string::npos || line.find("CPPFLAGS") != std::string::npos) {
                std::istringstream iss(line);
                std::string token;
                while (iss >> token) {
                    // 查找 -I 开头的包含路径
                    if (token.find("-I") == 0) {
                        std::string include_path = token.substr(2);
                        // 从路径中提取可能的库名
                        size_t last_slash = include_path.find_last_of("/\\");
                        if (last_slash != std::string::npos) {
                            std::string lib_name = include_path.substr(last_slash + 1);
                            if (!lib_name.empty()) {
                                node.dependencies.push_back(lib_name);
                                VLOG(1) << "Found Makefile include dependency: " << lib_name;
                            }
                        }
                    }
                }
            }
        }
        
        LOG(INFO) << "Successfully read Makefile metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading Makefile metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_autotools_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 查找项目名称
            if (line.find("AC_INIT(") != std::string::npos) {
                size_t start = line.find("AC_INIT(") + 8;
                size_t end = line.find(",", start);
                if (end != std::string::npos) {
                    node.name = line.substr(start, end - start);
                    // 移除引号和空格
                    node.name.erase(0, node.name.find_first_not_of(" \t\""));
                    node.name.erase(node.name.find_last_not_of(" \t\"") + 1);
                }
            }
            
            // 查找PKG_CHECK_MODULES依赖
            if (line.find("PKG_CHECK_MODULES(") != std::string::npos) {
                size_t start = line.find("PKG_CHECK_MODULES(") + 18;
                size_t end = line.find(",", start);
                if (end != std::string::npos) {
                    std::string dep = line.substr(start, end - start);
                    // 移除空格
                    dep.erase(0, dep.find_first_not_of(" \t"));
                    dep.erase(dep.find_last_not_of(" \t") + 1);
                    node.dependencies.push_back(dep);
                    VLOG(1) << "Found Autotools pkg-config dependency: " << dep;
                }
            }
            
            // 查找AC_CHECK_LIB依赖
            if (line.find("AC_CHECK_LIB(") != std::string::npos) {
                size_t start = line.find("AC_CHECK_LIB(") + 13;
                size_t end = line.find(",", start);
                if (end != std::string::npos) {
                    std::string dep = line.substr(start, end - start);
                    // 移除空格
                    dep.erase(0, dep.find_first_not_of(" \t"));
                    dep.erase(dep.find_last_not_of(" \t") + 1);
                    node.dependencies.push_back(dep);
                    VLOG(1) << "Found Autotools library dependency: " << dep;
                }
            }
        }
        
        LOG(INFO) << "Successfully read Autotools metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading Autotools metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_pkgconfig_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 查找包名
            if (line.find("Name:") == 0) {
                node.name = line.substr(5);
                // 移除空格
                node.name.erase(0, node.name.find_first_not_of(" \t"));
                node.name.erase(node.name.find_last_not_of(" \t") + 1);
            }
            
            // 查找版本
            if (line.find("Version:") == 0) {
                node.version = line.substr(8);
                // 移除空格
                node.version.erase(0, node.version.find_first_not_of(" \t"));
                node.version.erase(node.version.find_last_not_of(" \t") + 1);
            }
            
            // 查找描述
            if (line.find("Description:") == 0) {
                node.description = line.substr(12);
                // 移除空格
                node.description.erase(0, node.description.find_first_not_of(" \t"));
                node.description.erase(node.description.find_last_not_of(" \t") + 1);
            }
            
            // 查找依赖
            if (line.find("Requires:") == 0) {
                std::string deps = line.substr(9);
                // 移除空格
                deps.erase(0, deps.find_first_not_of(" \t"));
                deps.erase(deps.find_last_not_of(" \t") + 1);
                
                // 分割依赖列表
                std::istringstream iss(deps);
                std::string dep;
                while (std::getline(iss, dep, ' ')) {
                    if (!dep.empty()) {
                        node.dependencies.push_back(dep);
                        VLOG(1) << "Found pkg-config dependency: " << dep;
                    }
                }
            }
        }
        
        LOG(INFO) << "Successfully read pkg-config metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading pkg-config metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_vcpkg_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        json j;
        file >> j;
        
        // 提取基本信息
        if (j.contains("name")) {
            node.name = j["name"];
        }
        if (j.contains("version")) {
            node.version = j["version"];
        }
        if (j.contains("description")) {
            node.description = j["description"];
        }
        
        // 提取依赖
        if (j.contains("dependencies")) {
            for (const auto& dep : j["dependencies"]) {
                if (dep.is_string()) {
                    node.dependencies.push_back(dep);
                    VLOG(1) << "Found vcpkg dependency: " << dep.get<std::string>();
                } else if (dep.is_object() && dep.contains("name")) {
                    node.dependencies.push_back(dep["name"]);
                    VLOG(1) << "Found vcpkg dependency: " << dep["name"];
                }
            }
        }
        
        LOG(INFO) << "Successfully read vcpkg metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading vcpkg metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::analyze_package_structure(const std::string& package_path, LightweightDependencyNode& node) const {
    try {
        LOG(INFO) << "Analyzing package structure for: " << package_path;
        
        // 分析目录结构来推断包类型和依赖
        std::filesystem::path path(package_path);
        
        // 检查常见的包结构模式
        if (std::filesystem::exists(path / "src")) {
            node.package_type = "source_code";
            VLOG(1) << "Detected source code package";
        } else if (std::filesystem::exists(path / "lib")) {
            node.package_type = "library";
            VLOG(1) << "Detected library package";
        } else if (std::filesystem::exists(path / "bin")) {
            node.package_type = "executable";
            VLOG(1) << "Detected executable package";
        }
        
        // 尝试从文件扩展名推断C++项目类型
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                if (extension == ".cpp" || extension == ".cc" || extension == ".cxx" || extension == ".c++") {
                    node.language = "cpp";
                    node.package_type = "source_code";
                    break;
                } else if (extension == ".h" || extension == ".hpp" || extension == ".hxx" || extension == ".h++") {
                    node.language = "cpp";
                    node.package_type = "header_only";
                    break;
                } else if (extension == ".c") {
                    node.language = "c";
                    node.package_type = "source_code";
                    break;
                } else if (extension == ".so" || extension == ".a" || extension == ".lib" || extension == ".dll") {
                    node.language = "cpp";
                    node.package_type = "library";
                    break;
                }
            }
        }
        
        LOG(INFO) << "Package analysis completed. Type: " << node.package_type 
                  << ", Language: " << node.language;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error analyzing package structure: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_conan_metadata(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 查找项目名称
            if (line.find("name =") != std::string::npos) {
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    node.name = line.substr(eq_pos + 1);
                    // 移除引号和空格
                    node.name.erase(0, node.name.find_first_not_of(" \t\""));
                    node.name.erase(node.name.find_last_not_of(" \t\"") + 1);
                }
            }
            
            // 查找版本
            if (line.find("version =") != std::string::npos) {
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    node.version = line.substr(eq_pos + 1);
                    // 移除引号和空格
                    node.version.erase(0, node.version.find_first_not_of(" \t\""));
                    node.version.erase(node.version.find_last_not_of(" \t\"") + 1);
                }
            }
            
            // 查找依赖
            if (line.find("requires =") != std::string::npos) {
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    std::string deps = line.substr(eq_pos + 1);
                    // 移除引号和空格
                    deps.erase(0, deps.find_first_not_of(" \t\""));
                    deps.erase(deps.find_last_not_of(" \t\"") + 1);
                    
                    // 分割依赖列表
                    std::istringstream iss(deps);
                    std::string dep;
                    while (std::getline(iss, dep, ',')) {
                        // 移除空格
                        dep.erase(0, dep.find_first_not_of(" \t"));
                        dep.erase(dep.find_last_not_of(" \t") + 1);
                        if (!dep.empty()) {
                            node.dependencies.push_back(dep);
                            VLOG(1) << "Found Conan dependency: " << dep;
                        }
                    }
                }
            }
        }
        
        LOG(INFO) << "Successfully read Conan metadata for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading Conan metadata: " << e.what();
        return false;
    }
}

bool DependencyGraphBuilder::read_cpp_requirements(const std::string& file_path, LightweightDependencyNode& node) const {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file_path;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 跳过注释和空行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 解析依赖行
            std::istringstream iss(line);
            std::string dependency;
            iss >> dependency;
            
            if (!dependency.empty()) {
                node.dependencies.push_back(dependency);
                VLOG(1) << "Found C++ dependency: " << dependency;
            }
        }
        
        LOG(INFO) << "Successfully read C++ requirements for: " << node.name;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error reading C++ requirements: " << e.what();
        return false;
    }
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

void DependencyGraphAnalyzer::find_longest_chain(size_t node_index, std::unordered_set<size_t>& visited, 
                                                std::vector<size_t>& current_chain, 
                                                std::vector<std::vector<std::string>>& chains) const {
    if (visited.find(node_index) != visited.end()) {
        // 发现循环，记录当前链
        if (current_chain.size() > 1) {
            std::vector<std::string> chain_names;
            for (size_t idx : current_chain) {
                const auto* node = graph_->get_node_by_index(idx);
                if (node) {
                    chain_names.push_back(node->name);
                }
            }
            chains.push_back(chain_names);
        }
        return;
    }
    
    visited.insert(node_index);
    current_chain.push_back(node_index);
    
    const auto* node = graph_->get_node_by_index(node_index);
    if (node) {
        // 递归处理所有依赖
        for (size_t dep_index : node->dependency_indices) {
            find_longest_chain(dep_index, visited, current_chain, chains);
        }
        
        // 如果没有依赖，这是一个叶子节点，记录链
        if (node->dependency_indices.empty() && current_chain.size() > 1) {
            std::vector<std::string> chain_names;
            for (size_t idx : current_chain) {
                const auto* chain_node = graph_->get_node_by_index(idx);
                if (chain_node) {
                    chain_names.push_back(chain_node->name);
                }
            }
            chains.push_back(chain_names);
        }
    }
    
    current_chain.pop_back();
    visited.erase(node_index);
}

} // namespace Paker
