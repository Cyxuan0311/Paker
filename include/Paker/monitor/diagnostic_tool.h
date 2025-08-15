#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "dependency/dependency_graph.h"

namespace Paker {

// 诊断级别
enum class DiagnosticLevel {
    INFO,       // 信息
    WARNING,    // 警告
    ERROR,      // 错误
    CRITICAL    // 严重错误
};

// 诊断问题
struct DiagnosticIssue {
    DiagnosticLevel level;
    std::string category;
    std::string message;
    std::string description;
    std::vector<std::string> suggestions;
    std::map<std::string, std::string> context;
    
    DiagnosticIssue(DiagnosticLevel l, const std::string& c, const std::string& m)
        : level(l), category(c), message(m) {}
};

// 诊断结果
struct DiagnosticResult {
    std::vector<DiagnosticIssue> issues;
    bool has_critical_issues;
    bool has_errors;
    bool has_warnings;
    std::string summary;
    
    DiagnosticResult() : has_critical_issues(false), has_errors(false), has_warnings(false) {}
};

// 诊断工具
class DiagnosticTool {
private:
    const DependencyGraph& graph_;
    std::vector<std::unique_ptr<class DiagnosticRule>> rules_;
    
public:
    explicit DiagnosticTool(const DependencyGraph& graph);
    
    // 执行完整诊断
    DiagnosticResult diagnose();
    
    // 生成诊断报告
    std::string generate_diagnostic_report(const DiagnosticResult& result);
    
    // 检查配置问题
    std::vector<DiagnosticIssue> check_configuration();
    
    // 检查依赖问题
    std::vector<DiagnosticIssue> check_dependencies();
    
    // 检查性能问题
    std::vector<DiagnosticIssue> check_performance();
    
    // 检查安全问题
    std::vector<DiagnosticIssue> check_security();
    
    // 检查文件系统问题
    std::vector<DiagnosticIssue> check_filesystem();
    
    // 检查网络问题
    std::vector<DiagnosticIssue> check_network();
    
    // 修复建议
    std::vector<std::string> generate_fix_suggestions(const DiagnosticResult& result);
    
    // 导出诊断结果
    bool export_diagnostic_result(const DiagnosticResult& result, const std::string& filename);
    
private:
    // 初始化诊断规则
    void initialize_rules();
    
    // 格式化诊断级别
    std::string format_level(DiagnosticLevel level) const;
    
    // 获取诊断级别颜色
    std::string get_level_color(DiagnosticLevel level) const;
};

// 诊断规则基类
class DiagnosticRule {
public:
    virtual ~DiagnosticRule() = default;
    virtual std::vector<DiagnosticIssue> check(const DependencyGraph& graph) = 0;
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
};

// 循环依赖检测规则
class CircularDependencyRule : public DiagnosticRule {
public:
    std::vector<DiagnosticIssue> check(const DependencyGraph& graph) override;
    std::string get_name() const override { return "Circular Dependency Check"; }
    std::string get_description() const override { return "Detects circular dependencies in the dependency graph"; }
};

// 版本冲突检测规则
class VersionConflictRule : public DiagnosticRule {
public:
    std::vector<DiagnosticIssue> check(const DependencyGraph& graph) override;
    std::string get_name() const override { return "Version Conflict Check"; }
    std::string get_description() const override { return "Detects version conflicts between dependencies"; }
};

// 缺失依赖检测规则
class MissingDependencyRule : public DiagnosticRule {
public:
    std::vector<DiagnosticIssue> check(const DependencyGraph& graph) override;
    std::string get_name() const override { return "Missing Dependency Check"; }
    std::string get_description() const override { return "Detects missing dependencies"; }
};

} // namespace Paker 