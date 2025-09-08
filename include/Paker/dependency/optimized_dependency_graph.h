#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace Paker {

// 轻量级依赖节点
struct LightweightDependencyNode {
    std::string name;
    std::string version;
    std::string repository;
    bool is_installed;
    std::string install_path;
    
    // 使用索引而不是直接存储依赖关系
    std::vector<size_t> dependency_indices;
    std::vector<size_t> dependent_indices;
    
    // 缓存信息
    mutable std::chrono::system_clock::time_point last_access;
    mutable bool is_cached;
    
    LightweightDependencyNode() : is_installed(false), is_cached(false) {}
    LightweightDependencyNode(const std::string& name, const std::string& version = "")
        : name(name), version(version), is_installed(false), is_cached(false) {}
};

// 内存优化的依赖图
class OptimizedDependencyGraph {
private:
    // 使用向量存储节点，通过索引访问
    std::vector<LightweightDependencyNode> nodes_;
    
    // 名称到索引的映射
    std::unordered_map<std::string, size_t> name_to_index_;
    
    // 访问统计
    mutable std::unordered_map<size_t, size_t> access_counts_;
    
    // 缓存配置
    size_t max_cached_nodes_;
    size_t cache_cleanup_threshold_;
    
    // 内存管理
    void cleanup_cache();
    void update_access_time(size_t index) const;
    void evict_least_used_nodes();
    
public:
    OptimizedDependencyGraph(size_t max_cached_nodes = 1000, 
                           size_t cache_cleanup_threshold = 800);
    ~OptimizedDependencyGraph();
    
    // 节点管理
    size_t add_node(const LightweightDependencyNode& node);
    bool remove_node(const std::string& name);
    bool has_node(const std::string& name) const;
    
    // 节点访问
    const LightweightDependencyNode* get_node(const std::string& name) const;
    LightweightDependencyNode* get_node(const std::string& name);
    const LightweightDependencyNode* get_node_by_index(size_t index) const;
    LightweightDependencyNode* get_node_by_index(size_t index);
    
    // 依赖关系管理
    bool add_dependency(const std::string& from, const std::string& to);
    bool remove_dependency(const std::string& from, const std::string& to);
    std::vector<std::string> get_dependencies(const std::string& name) const;
    std::vector<std::string> get_dependents(const std::string& name) const;
    
    // 图算法
    std::vector<std::string> topological_sort() const;
    std::vector<std::vector<std::string>> detect_cycles() const;
    std::vector<std::vector<std::string>> get_all_paths(const std::string& from, 
                                                       const std::string& to) const;
    
    // 内存管理
    void optimize_memory();
    void clear_cache();
    size_t get_memory_usage() const;
    size_t get_cached_nodes_count() const;
    
    // 统计信息
    size_t get_node_count() const { return nodes_.size(); }
    size_t get_edge_count() const;
    std::map<std::string, size_t> get_access_statistics() const;
    
    // 序列化
    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);
    
    // 批量操作
    void add_nodes_batch(const std::vector<LightweightDependencyNode>& nodes);
    void remove_nodes_batch(const std::vector<std::string>& names);
    
private:
    // 内部辅助方法
    size_t get_node_index(const std::string& name) const;
    void dfs_cycle_detection(size_t node_index, 
                           std::unordered_set<size_t>& visited,
                           std::unordered_set<size_t>& rec_stack,
                           std::vector<size_t>& path,
                           std::vector<std::vector<std::string>>& cycles) const;
    void dfs_path_finding(size_t from_index, size_t to_index,
                         std::unordered_set<size_t>& visited,
                         std::vector<size_t>& current_path,
                         std::vector<std::vector<std::string>>& all_paths) const;
};

// 依赖图构建器
class DependencyGraphBuilder {
private:
    std::unique_ptr<OptimizedDependencyGraph> graph_;
    std::map<std::string, std::string> repositories_;
    
public:
    DependencyGraphBuilder();
    
    // 构建图
    bool build_from_packages(const std::map<std::string, std::string>& packages);
    bool build_from_json(const std::string& json_file);
    
    // 获取构建的图
    std::unique_ptr<OptimizedDependencyGraph> get_graph();
    const OptimizedDependencyGraph* get_graph() const;
    
    // 设置仓库映射
    void set_repositories(const std::map<std::string, std::string>& repos);
    void add_repository(const std::string& name, const std::string& url);
    
private:
    bool resolve_package_dependencies(const std::string& package, const std::string& version);
    bool read_package_metadata(const std::string& package_path, LightweightDependencyNode& node);
};

// 依赖图分析器
class DependencyGraphAnalyzer {
private:
    const OptimizedDependencyGraph* graph_;
    
public:
    DependencyGraphAnalyzer(const OptimizedDependencyGraph* graph);
    
    // 分析功能
    struct AnalysisResult {
        size_t total_packages;
        size_t max_depth;
        size_t max_breadth;
        std::vector<std::string> leaf_packages;
        std::vector<std::string> root_packages;
        std::map<size_t, size_t> depth_distribution;
        std::map<size_t, size_t> breadth_distribution;
    };
    
    AnalysisResult analyze_structure() const;
    
    // 性能分析
    struct PerformanceMetrics {
        double average_dependency_depth;
        double average_dependent_count;
        size_t most_connected_package_count;
        std::string most_connected_package;
        std::vector<std::string> critical_packages; // 被最多包依赖的包
    };
    
    PerformanceMetrics analyze_performance() const;
    
    // 依赖关系分析
    std::vector<std::string> find_critical_dependencies() const;
    std::vector<std::string> find_orphaned_packages() const;
    std::vector<std::vector<std::string>> find_dependency_chains() const;
    
private:
    size_t calculate_depth(size_t node_index, std::unordered_set<size_t>& visited) const;
    size_t calculate_breadth(size_t node_index) const;
    void find_longest_chain(size_t node_index, std::unordered_set<size_t>& visited, 
                           std::vector<size_t>& current_chain, 
                           std::vector<std::vector<std::string>>& chains) const;
};

} // namespace Paker
