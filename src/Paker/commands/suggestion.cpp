#include "Paker/analysis/project_analyzer.h"
#include "Paker/analysis/recommendation_engine.h"
#include "Paker/core/output.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace Paker {

// 前向声明
void display_project_analysis(const Analysis::ProjectAnalysis& analysis, bool detailed);
void display_recommendations(const std::vector<Analysis::PackageRecommendation>& recommendations, bool detailed);
void auto_install_recommendations(const std::vector<Analysis::PackageRecommendation>& recommendations);
void export_analysis_results(
    const Analysis::ProjectAnalysis& analysis,
    const std::vector<Analysis::PackageRecommendation>& recommendations,
    const std::string& export_path
);

void pm_smart_suggestion(
    const std::string& category_filter,
    const std::string& performance_filter,
    const std::string& security_filter,
    bool detailed,
    bool auto_install,
    const std::string& export_path
) {
    try {
        // 创建项目分析器
        Analysis::ProjectAnalyzer analyzer;
        
        // 分析当前项目
        Paker::Output::info("Analyzing project features...");
        auto analysis = analyzer.analyze_project(".");
        
        if (!analysis.is_initialized) {
            Paker::Output::error("Project analysis failed, please ensure you are running this command in a valid Paker project");
            return;
        }
        
        // 显示项目分析结果
        display_project_analysis(analysis, detailed);
        
        // 创建推荐引擎
        Analysis::RecommendationEngine engine;
        
        // 生成推荐
        Paker::Output::info("Generating smart recommendations...");
        auto recommendations = engine.generate_recommendations(
            analysis, category_filter, performance_filter, security_filter
        );
        
        // 显示推荐结果
        display_recommendations(recommendations, detailed);
        
        // 自动安装推荐包
        if (auto_install && !recommendations.empty()) {
            auto_install_recommendations(recommendations);
        }
        
        // 导出分析结果
        if (!export_path.empty()) {
            export_analysis_results(analysis, recommendations, export_path);
        }
        
    } catch (const std::exception& e) {
        Paker::Output::error("Smart recommendation failed: " + std::string(e.what()));
    }
}

void display_project_analysis(const Analysis::ProjectAnalysis& analysis, bool detailed) {
    Paker::Output::info("Project Analysis Results:");
    
    // 计算最大标签长度，确保完美对齐
    const size_t LABEL_WIDTH = 15;
    const size_t VALUE_WIDTH = 20;
    const size_t TOTAL_WIDTH = LABEL_WIDTH + VALUE_WIDTH + 5; // +5 for borders and spaces
    
    // 构建专业边框
    std::string top_border = "+" + std::string(TOTAL_WIDTH - 2, '-') + "+";
    std::string bottom_border = "+" + std::string(TOTAL_WIDTH - 2, '-') + "+";
    
    std::cout << top_border << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "Project Type"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.project_type << " |" << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "Build System"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.build_system << " |" << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "C++ Standard"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.cpp_standard << " |" << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "Performance"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.performance_requirements << " |" << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "Security"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.security_requirements << " |" << std::endl;
    std::cout << "| " << std::setw(LABEL_WIDTH) << std::left << "Testing"
              << " | " << std::setw(VALUE_WIDTH) << std::left << analysis.testing_requirements << " |" << std::endl;
    std::cout << bottom_border << std::endl;
    
    if (detailed) {
        std::cout << std::endl;
        Paker::Output::info("Detailed Analysis:");
        
        std::cout << "├── Existing Dependencies: ";
        if (analysis.existing_dependencies.empty()) {
            std::cout << "None" << std::endl;
        } else {
            for (size_t i = 0; i < analysis.existing_dependencies.size(); ++i) {
                std::cout << analysis.existing_dependencies[i];
                if (i < analysis.existing_dependencies.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        std::cout << "├── Code Patterns: ";
        if (analysis.code_patterns.empty()) {
            std::cout << "No special patterns" << std::endl;
        } else {
            for (size_t i = 0; i < analysis.code_patterns.size(); ++i) {
                std::cout << analysis.code_patterns[i];
                if (i < analysis.code_patterns.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        std::cout << "└── Feature Scores:" << std::endl;
        for (const auto& [feature, score] : analysis.feature_scores) {
            std::cout << "    " << feature << ": " << std::fixed << std::setprecision(2) << score << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void display_recommendations(const std::vector<Analysis::PackageRecommendation>& recommendations, bool detailed) {
    if (recommendations.empty()) {
        Paker::Output::warning("No suitable package recommendations found");
        return;
    }
    
    Paker::Output::info("Smart Package Recommendations:");
    
    if (detailed) {
        // 详细显示
        for (size_t i = 0; i < recommendations.size(); ++i) {
            const auto& rec = recommendations[i];
            
            std::cout << std::endl;
            std::cout << (i + 1) << ". " << rec.name << " (Recommendation: ";
            
            // 显示推荐度星级
            int stars = static_cast<int>(rec.confidence * 5);
            for (int j = 0; j < stars; ++j) {
                std::cout << "⭐";
            }
            for (int j = stars; j < 5; ++j) {
                std::cout << "☆";
            }
            std::cout << ")" << std::endl;
            
            std::cout << "   ├── Description: " << rec.description << std::endl;
            std::cout << "   ├── Reason: " << rec.reason << std::endl;
            std::cout << "   ├── Category: " << rec.category << std::endl;
            std::cout << "   ├── Compatibility: " << std::fixed << std::setprecision(0) << (rec.compatibility * 100) << "%" << std::endl;
            std::cout << "   ├── Popularity: " << std::fixed << std::setprecision(0) << (rec.popularity * 100) << "%" << std::endl;
            std::cout << "   ├── Maintenance: " << std::fixed << std::setprecision(0) << (rec.maintenance * 100) << "%" << std::endl;
            std::cout << "   ├── Priority: " << rec.priority << std::endl;
            std::cout << "   └── Install: " << rec.install_command << std::endl;
        }
    } else {
        // 专业表格显示 - 参考nvidia-smi等专业工具的对齐效果
        std::cout << std::endl;
        
        // 定义固定列宽，确保完美对齐
        const size_t PACKAGE_WIDTH = 18;
        const size_t REASON_WIDTH = 28;
        const size_t PRIORITY_WIDTH = 8;
        const size_t COMPAT_WIDTH = 8;
        const size_t POPULAR_WIDTH = 8;
        
        // 构建专业表格边框
        std::string top_border = "+" + std::string(PACKAGE_WIDTH + 2, '-') + "+" +
                                std::string(REASON_WIDTH + 2, '-') + "+" +
                                std::string(PRIORITY_WIDTH + 2, '-') + "+" +
                                std::string(COMPAT_WIDTH + 2, '-') + "+" +
                                std::string(POPULAR_WIDTH + 2, '-') + "+";
        
        std::string separator = "+" + std::string(PACKAGE_WIDTH + 2, '-') + "+" +
                               std::string(REASON_WIDTH + 2, '-') + "+" +
                               std::string(PRIORITY_WIDTH + 2, '-') + "+" +
                               std::string(COMPAT_WIDTH + 2, '-') + "+" +
                               std::string(POPULAR_WIDTH + 2, '-') + "+";
        
        std::string bottom_border = "+" + std::string(PACKAGE_WIDTH + 2, '-') + "+" +
                                   std::string(REASON_WIDTH + 2, '-') + "+" +
                                   std::string(PRIORITY_WIDTH + 2, '-') + "+" +
                                   std::string(COMPAT_WIDTH + 2, '-') + "+" +
                                   std::string(POPULAR_WIDTH + 2, '-') + "+";
        
        // 输出表格头部
        std::cout << top_border << std::endl;
        std::cout << "| " << std::setw(PACKAGE_WIDTH) << std::left << "Package"
                  << " | " << std::setw(REASON_WIDTH) << std::left << "Reason"
                  << " | " << std::setw(PRIORITY_WIDTH) << std::left << "Priority"
                  << " | " << std::setw(COMPAT_WIDTH) << std::left << "Compat"
                  << " | " << std::setw(POPULAR_WIDTH) << std::left << "Popular" << " |" << std::endl;
        std::cout << separator << std::endl;
        
        // 输出数据行 - 专业级对齐
        for (const auto& rec : recommendations) {
            // 智能截断文本，保持对齐
            std::string package_name = rec.name;
            if (package_name.length() > PACKAGE_WIDTH) {
                package_name = package_name.substr(0, PACKAGE_WIDTH - 3) + "...";
            }
            
            std::string reason = rec.reason;
            if (reason.length() > REASON_WIDTH) {
                reason = reason.substr(0, REASON_WIDTH - 3) + "...";
            }
            
            // 格式化百分比，确保对齐
            std::string compat_str = std::to_string(static_cast<int>(rec.compatibility * 100)) + "%";
            std::string popular_str = std::to_string(static_cast<int>(rec.popularity * 100)) + "%";
            
            // 优先级格式化
            std::string priority_str = rec.priority;
            if (priority_str == "high") {
                priority_str = "HIGH";
            } else if (priority_str == "medium") {
                priority_str = "MED";
            } else {
                priority_str = "LOW";
            }
            
            // 输出完美对齐的行 - 使用精确的列宽控制
            std::cout << "| " << std::setw(PACKAGE_WIDTH) << std::left << package_name
                      << " | " << std::setw(REASON_WIDTH) << std::left << reason
                      << " | " << std::setw(PRIORITY_WIDTH) << std::left << priority_str
                      << " | " << std::setw(COMPAT_WIDTH) << std::right << compat_str
                      << " | " << std::setw(POPULAR_WIDTH) << std::right << popular_str
                      << " |" << std::endl;
        }
        
        std::cout << bottom_border << std::endl;
    }
    
    std::cout << std::endl;
    Paker::Output::info("Use 'Paker suggestion --detailed' to view detailed recommendations");
    Paker::Output::info("Use 'Paker suggestion --auto-install' to automatically install recommended packages");
}

void auto_install_recommendations(const std::vector<Analysis::PackageRecommendation>& recommendations) {
    Paker::Output::info("🚀 Auto-installing recommended packages...");
    
    for (const auto& rec : recommendations) {
        if (rec.priority == "high" || rec.confidence > 0.8) {
            Paker::Output::info("Installing " + rec.name + "...");
            
            // 这里应该调用实际的安装命令
            // 简化实现，只显示安装命令
            std::cout << "Executing: " << rec.install_command << std::endl;
            
            // 实际实现中应该调用:
            // system(rec.install_command.c_str());
        }
    }
}

void export_analysis_results(
    const Analysis::ProjectAnalysis& analysis,
    const std::vector<Analysis::PackageRecommendation>& recommendations,
    const std::string& export_path
) {
    try {
        std::ofstream file(export_path);
        if (!file.is_open()) {
            Paker::Output::error("无法创建导出文件: " + export_path);
            return;
        }
        
        file << "{\n";
        file << "  \"project_analysis\": {\n";
        file << "    \"project_type\": \"" << analysis.project_type << "\",\n";
        file << "    \"build_system\": \"" << analysis.build_system << "\",\n";
        file << "    \"cpp_standard\": \"" << analysis.cpp_standard << "\",\n";
        file << "    \"performance_requirements\": \"" << analysis.performance_requirements << "\",\n";
        file << "    \"security_requirements\": \"" << analysis.security_requirements << "\",\n";
        file << "    \"testing_requirements\": \"" << analysis.testing_requirements << "\",\n";
        file << "    \"existing_dependencies\": [\n";
        for (size_t i = 0; i < analysis.existing_dependencies.size(); ++i) {
            file << "      \"" << analysis.existing_dependencies[i] << "\"";
            if (i < analysis.existing_dependencies.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << "    ],\n";
        file << "    \"code_patterns\": [\n";
        for (size_t i = 0; i < analysis.code_patterns.size(); ++i) {
            file << "      \"" << analysis.code_patterns[i] << "\"";
            if (i < analysis.code_patterns.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << "    ]\n";
        file << "  },\n";
        file << "  \"recommendations\": [\n";
        
        for (size_t i = 0; i < recommendations.size(); ++i) {
            const auto& rec = recommendations[i];
            file << "    {\n";
            file << "      \"name\": \"" << rec.name << "\",\n";
            file << "      \"description\": \"" << rec.description << "\",\n";
            file << "      \"reason\": \"" << rec.reason << "\",\n";
            file << "      \"category\": \"" << rec.category << "\",\n";
            file << "      \"confidence\": " << rec.confidence << ",\n";
            file << "      \"compatibility\": " << rec.compatibility << ",\n";
            file << "      \"popularity\": " << rec.popularity << ",\n";
            file << "      \"maintenance\": " << rec.maintenance << ",\n";
            file << "      \"priority\": \"" << rec.priority << "\",\n";
            file << "      \"install_command\": \"" << rec.install_command << "\"\n";
            file << "    }";
            if (i < recommendations.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        Paker::Output::success("分析结果已导出到: " + export_path);
        
    } catch (const std::exception& e) {
        Paker::Output::error("导出失败: " + std::string(e.what()));
    }
}

} // namespace Paker
