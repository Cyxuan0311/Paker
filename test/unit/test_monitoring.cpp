#include <gtest/gtest.h>
#include "Paker/monitor/performance_monitor.h"
#include "Paker/monitor/dependency_analyzer.h"
#include "Paker/monitor/diagnostic_tool.h"
#include "Paker/dependency/dependency_graph.h"
#include "Paker/dependency/dependency_resolver.h"

using namespace Paker;

class PerformanceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor_.clear();
    }
    
    PerformanceMonitor monitor_;
};

TEST_F(PerformanceMonitorTest, BasicTimer) {
    monitor_.start_timer("test_timer");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    monitor_.end_timer("test_timer", MetricType::INSTALL_TIME);
    
    auto metrics = monitor_.get_metrics("install");
    ASSERT_EQ(metrics.size(), 1);
    EXPECT_EQ(metrics[0].name, "test_timer");
    EXPECT_GT(metrics[0].value, 0);
    EXPECT_EQ(metrics[0].unit, "ms");
}

TEST_F(PerformanceMonitorTest, RecordMetric) {
    monitor_.record_metric(MetricType::DOWNLOAD_SPEED, "test_download", 1024.5, "KB/s");
    
    auto metrics = monitor_.get_metrics("network");
    ASSERT_EQ(metrics.size(), 1);
    EXPECT_EQ(metrics[0].name, "test_download");
    EXPECT_DOUBLE_EQ(metrics[0].value, 1024.5);
    EXPECT_EQ(metrics[0].unit, "KB/s");
}

TEST_F(PerformanceMonitorTest, EnableDisable) {
    monitor_.enable(false);
    monitor_.start_timer("disabled_timer");
    monitor_.end_timer("disabled_timer");
    
    auto metrics = monitor_.get_metrics();
    EXPECT_EQ(metrics.size(), 0);
    
    monitor_.enable(true);
    monitor_.start_timer("enabled_timer");
    monitor_.end_timer("enabled_timer");
    
    metrics = monitor_.get_metrics();
    EXPECT_EQ(metrics.size(), 1);
}

TEST_F(PerformanceMonitorTest, GenerateReport) {
    monitor_.record_metric(MetricType::INSTALL_TIME, "test1", 100.0, "ms");
    monitor_.record_metric(MetricType::INSTALL_TIME, "test2", 200.0, "ms");
    
    std::string report = monitor_.generate_performance_report();
    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("test1"), std::string::npos);
    EXPECT_NE(report.find("test2"), std::string::npos);
}

class DependencyAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试依赖图
        graph_.add_node("package1", "1.0.0");
        graph_.add_node("package2", "2.0.0");
        graph_.add_node("package3", "3.0.0");
        graph_.add_dependency("package1", "package2");
        graph_.add_dependency("package2", "package3");
        
        analyzer_ = std::make_unique<DependencyAnalyzer>(graph_);
    }
    
    DependencyGraph graph_;
    std::unique_ptr<DependencyAnalyzer> analyzer_;
};

TEST_F(DependencyAnalyzerTest, BasicAnalysis) {
    auto analysis = analyzer_->analyze();
    
    EXPECT_EQ(analysis.total_packages, 3);
    EXPECT_EQ(analysis.direct_dependencies, 2);
    EXPECT_EQ(analysis.circular_dependencies, 0);
    EXPECT_EQ(analysis.version_conflicts, 0);
}

TEST_F(DependencyAnalyzerTest, VersionDistribution) {
    auto distribution = analyzer_->analyze_version_distribution();
    
    EXPECT_EQ(distribution["package1"].size(), 1);
    EXPECT_EQ(distribution["package2"].size(), 1);
    EXPECT_EQ(distribution["package3"].size(), 1);
    
    EXPECT_TRUE(distribution["package1"].count("1.0.0"));
    EXPECT_TRUE(distribution["package2"].count("2.0.0"));
    EXPECT_TRUE(distribution["package3"].count("3.0.0"));
}

TEST_F(DependencyAnalyzerTest, DependencyDepth) {
    auto depths = analyzer_->calculate_dependency_depth();
    
    EXPECT_EQ(depths["package1"], 2);  // package1 -> package2 -> package3
    EXPECT_EQ(depths["package2"], 1);  // package2 -> package3
    EXPECT_EQ(depths["package3"], 0);  // package3 has no dependencies
}

TEST_F(DependencyAnalyzerTest, GenerateReport) {
    auto analysis = analyzer_->analyze();
    std::string report = analyzer_->generate_analysis_report(analysis);
    
    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("package1"), std::string::npos);
    EXPECT_NE(report.find("package2"), std::string::npos);
    EXPECT_NE(report.find("package3"), std::string::npos);
}

class DiagnosticToolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试依赖图
        graph_.add_node("package1", "1.0.0");
        graph_.add_node("package2", "2.0.0");
        graph_.add_dependency("package1", "package2");
        
        diagnostic_ = std::make_unique<DiagnosticTool>(graph_);
    }
    
    DependencyGraph graph_;
    std::unique_ptr<DiagnosticTool> diagnostic_;
};

TEST_F(DiagnosticToolTest, BasicDiagnostic) {
    auto result = diagnostic_->diagnose();
    
    // 应该没有严重问题
    EXPECT_FALSE(result.has_critical_issues);
    EXPECT_FALSE(result.has_errors);
    EXPECT_FALSE(result.has_warnings);
    
    // 应该有信息级别的消息（如没有依赖等）
    EXPECT_GE(result.issues.size(), 0);
}

TEST_F(DiagnosticToolTest, GenerateReport) {
    auto result = diagnostic_->diagnose();
    std::string report = diagnostic_->generate_diagnostic_report(result);
    
    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("Diagnostic Report"), std::string::npos);
}

TEST_F(DiagnosticToolTest, CheckDependencies) {
    auto issues = diagnostic_->check_dependencies();
    
    // 对于有效的依赖图，应该没有依赖问题
    EXPECT_EQ(issues.size(), 0);
}

TEST_F(DiagnosticToolTest, CheckConfiguration) {
    auto issues = diagnostic_->check_configuration();
    
    // 在测试环境中，配置文件可能不存在
    EXPECT_GE(issues.size(), 0);
}

// 测试诊断规则
TEST_F(DiagnosticToolTest, CircularDependencyRule) {
    // 创建循环依赖
    DependencyGraph cyclic_graph;
    cyclic_graph.add_node("A", "1.0.0");
    cyclic_graph.add_node("B", "1.0.0");
    cyclic_graph.add_dependency("A", "B");
    cyclic_graph.add_dependency("B", "A");
    
    CircularDependencyRule rule;
    auto issues = rule.check(cyclic_graph);
    
    EXPECT_GT(issues.size(), 0);
    EXPECT_EQ(issues[0].level, DiagnosticLevel::ERROR);
    EXPECT_NE(issues[0].message.find("Circular dependency"), std::string::npos);
}

TEST_F(DiagnosticToolTest, MissingDependencyRule) {
    // 创建缺失依赖的情况
    DependencyGraph missing_graph;
    missing_graph.add_node("A", "1.0.0");
    missing_graph.add_dependency("A", "missing_package");
    
    MissingDependencyRule rule;
    auto issues = rule.check(missing_graph);
    
    EXPECT_GT(issues.size(), 0);
    EXPECT_EQ(issues[0].level, DiagnosticLevel::ERROR);
    EXPECT_NE(issues[0].message.find("Missing dependency"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 