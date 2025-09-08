#include "Paker/commands/monitor.h"
#include "Paker/monitor/performance_monitor.h"
#include "Paker/monitor/dependency_analyzer.h"
#include "Paker/monitor/diagnostic_tool.h"
#include "Paker/core/package_manager.h"
#include "Paker/core/output.h"
#include "Paker/dependency/dependency_resolver.h"
#include <glog/logging.h>

namespace Paker {

int pm_performance_report(const std::string& output_file) {
    try {
        Output::info("Generating performance report...");
        
        // 生成性能报告
        std::string report = g_performance_monitor.generate_performance_report();
        
        if (output_file.empty()) {
            // 输出到控制台
            Output::success("Performance Report:");
            std::cout << report << std::endl;
        } else {
            // 保存到文件
            if (g_performance_monitor.save_to_file(output_file)) {
                Output::success("Performance report saved to: " + output_file);
            } else {
                Output::error("Failed to save performance report to: " + output_file);
                return 1;
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error generating performance report: " + std::string(e.what()));
        LOG(ERROR) << "Error generating performance report: " << e.what();
        return 1;
    }
}

int pm_analyze_dependencies(const std::string& output_file) {
    try {
        Output::info("Analyzing dependencies...");
        
        // 解析依赖
        // 创建依赖解析器
        DependencyResolver resolver;
        if (!resolver.resolve_project_dependencies()) {
            Output::error("Failed to resolve project dependencies");
            return 1;
        }
        
        // 获取依赖图
        auto graph = resolver.get_dependency_graph();
        if (graph.empty()) {
            Output::warning("No dependencies found to analyze");
            return 0;
        }
        
        // 创建分析器
        DependencyAnalyzer analyzer(graph);
        
        // 执行分析
        auto analysis = analyzer.analyze();
        
        // 生成报告
        std::string report = analyzer.generate_analysis_report(analysis);
        
        if (output_file.empty()) {
            // 输出到控制台
            Output::success("Dependency Analysis Report:");
            std::cout << report << std::endl;
            
            // 显示依赖树可视化
            std::string tree_viz = analyzer.generate_dependency_tree_visualization();
            std::cout << tree_viz << std::endl;
        } else {
            // 保存到文件
            if (analyzer.export_analysis(analysis, output_file)) {
                Output::success("Dependency analysis saved to: " + output_file);
            } else {
                Output::error("Failed to save dependency analysis to: " + output_file);
                return 1;
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error analyzing dependencies: " + std::string(e.what()));
        LOG(ERROR) << "Error analyzing dependencies: " << e.what();
        return 1;
    }
}

int pm_diagnose(const std::string& output_file) {
    try {
        Output::info("Running diagnostic...");
        
        // 解析依赖
        // 创建依赖解析器
        DependencyResolver resolver;
        if (!resolver.resolve_project_dependencies()) {
            Output::error("Failed to resolve project dependencies");
            return 1;
        }
        
        // 获取依赖图
        auto graph = resolver.get_dependency_graph();
        
        // 创建诊断工具
        DiagnosticTool diagnostic(graph);
        
        // 执行诊断
        auto result = diagnostic.diagnose();
        
        // 生成报告
        std::string report = diagnostic.generate_diagnostic_report(result);
        
        if (output_file.empty()) {
            // 输出到控制台
            Output::success("Diagnostic Report:");
            std::cout << report << std::endl;
            
            // 显示修复建议
            auto suggestions = diagnostic.generate_fix_suggestions(result);
            if (!suggestions.empty()) {
                Output::info("Fix Suggestions:");
                for (const auto& suggestion : suggestions) {
                    std::cout << "  - " << suggestion << std::endl;
                }
            }
        } else {
            // 保存到文件
            if (diagnostic.export_diagnostic_result(result, output_file)) {
                Output::success("Diagnostic result saved to: " + output_file);
            } else {
                Output::error("Failed to save diagnostic result to: " + output_file);
                return 1;
            }
        }
        
        // 返回适当的退出码
        if (result.has_critical_issues) {
            Output::error("Critical issues detected!");
            return 3;
        } else if (result.has_errors) {
            Output::warning("Errors detected");
            return 2;
        } else if (result.has_warnings) {
            Output::info("Warnings detected");
            return 1;
        } else {
            Output::success("No issues found");
            return 0;
        }
        
    } catch (const std::exception& e) {
        Output::error("Error running diagnostic: " + std::string(e.what()));
        LOG(ERROR) << "Error running diagnostic: " << e.what();
        return 1;
    }
}

int pm_monitor_enable(bool enable) {
    try {
        g_performance_monitor.enable(enable);
        
        if (enable) {
            Output::success("Performance monitoring enabled");
        } else {
            Output::info("Performance monitoring disabled");
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error configuring performance monitoring: " + std::string(e.what()));
        LOG(ERROR) << "Error configuring performance monitoring: " << e.what();
        return 1;
    }
}

int pm_monitor_clear() {
    try {
        g_performance_monitor.clear();
        Output::success("Performance monitoring data cleared");
        return 0;
        
    } catch (const std::exception& e) {
        Output::error("Error clearing performance monitoring data: " + std::string(e.what()));
        LOG(ERROR) << "Error clearing performance monitoring data: " << e.what();
        return 1;
    }
}

} // namespace Paker 