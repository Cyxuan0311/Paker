#include "Paker/commands/incremental_parse.h"
#include "Paker/dependency/incremental_parser.h"
#include "Paker/core/package_manager.h"
#include "Paker/core/output.h"
#include <glog/logging.h>

namespace Paker {

void pm_incremental_parse(const std::vector<std::string>& packages) {
    LOG(INFO) << "Starting incremental parse";
    
    // 获取增量解析器
    auto* parser = get_incremental_parser();
    if (!parser) {
        LOG(ERROR) << "Incremental parser not initialized";
        Output::error("Incremental parser not initialized. Run 'paker init' first.");
        return;
    }
    
    try {
        bool success;
        if (packages.empty()) {
            // 解析整个项目
            Output::info("🔍 开始增量解析项目依赖...");
            success = parser->parse_project_dependencies();
        } else {
            // 解析指定包
            Output::info("🔍 开始增量解析指定包: " + std::to_string(packages.size()) + " 个包");
            success = parser->parse_packages(packages);
        }
        
        if (success) {
            Output::success("✅ 增量解析完成！");
            
            // 显示统计信息
            auto stats = parser->get_stats();
            Output::info("📊 解析统计:");
            Output::info("  总解析包数: " + std::to_string(stats.total_packages_parsed));
            Output::info("  缓存命中: " + std::to_string(stats.cache_hits));
            Output::info("  缓存未命中: " + std::to_string(stats.cache_misses));
            Output::info("  平均解析时间: " + std::to_string(stats.avg_parse_time.count()) + "ms");
            
            if (stats.cache_hits + stats.cache_misses > 0) {
                double hit_rate = (double)stats.cache_hits / (stats.cache_hits + stats.cache_misses) * 100;
                Output::info("  缓存命中率: " + std::to_string(hit_rate) + "%");
            }
        } else {
            Output::error("❌ 增量解析失败");
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Incremental parse failed: " << e.what();
        Output::error("增量解析失败: " + std::string(e.what()));
    }
}

void pm_incremental_parse_stats() {
    LOG(INFO) << "Displaying incremental parse statistics";
    
    auto* parser = get_incremental_parser();
    if (!parser) {
        Output::error("增量解析器未初始化");
        return;
    }
    
    try {
        auto stats = parser->get_stats();
        auto cache_info = parser->get_cache_info();
        auto performance_report = parser->get_performance_report();
        
        Output::info("📊 增量解析统计信息");
        Output::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        Output::info("📈 性能统计:");
        Output::info("  总解析包数: " + std::to_string(stats.total_packages_parsed));
        Output::info("  缓存命中: " + std::to_string(stats.cache_hits));
        Output::info("  缓存未命中: " + std::to_string(stats.cache_misses));
        Output::info("  增量更新: " + std::to_string(stats.incremental_updates));
        Output::info("  完整解析: " + std::to_string(stats.full_parses));
        
        if (stats.cache_hits + stats.cache_misses > 0) {
            double hit_rate = (double)stats.cache_hits / (stats.cache_hits + stats.cache_misses) * 100;
            Output::info("  缓存命中率: " + std::to_string(hit_rate) + "%");
        }
        
        Output::info("⏱️ 时间统计:");
        Output::info("  平均解析时间: " + std::to_string(stats.avg_parse_time.count()) + "ms");
        Output::info("  总解析时间: " + std::to_string(stats.total_parse_time.count()) + "ms");
        Output::info("  缓存加载时间: " + std::to_string(stats.cache_load_time.count()) + "ms");
        Output::info("  缓存保存时间: " + std::to_string(stats.cache_save_time.count()) + "ms");
        
        Output::info("💾 缓存信息:");
        Output::info("  缓存大小: " + std::to_string(parser->get_cache_size()) + " 条目");
        
        // 显示缓存详细信息
        std::istringstream cache_stream(cache_info);
        std::string line;
        while (std::getline(cache_stream, line)) {
            if (!line.empty()) {
                Output::info("  " + line);
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to get parse statistics: " << e.what();
        Output::error("获取统计信息失败: " + std::string(e.what()));
    }
}

void pm_incremental_parse_config() {
    LOG(INFO) << "Displaying incremental parse configuration";
    
    auto* parser = get_incremental_parser();
    if (!parser) {
        Output::error("增量解析器未初始化");
        return;
    }
    
    try {
        auto config = parser->get_config();
        
        Output::info("⚙️ 增量解析配置");
        Output::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        Output::info("🔧 功能设置:");
        Output::info("  启用缓存: " + std::string(config.enable_caching ? "✅ 是" : "❌ 否"));
        Output::info("  启用增量解析: " + std::string(config.enable_incremental ? "✅ 是" : "❌ 否"));
        Output::info("  启用并行解析: " + std::string(config.enable_parallel ? "✅ 是" : "❌ 否"));
        Output::info("  启用预测: " + std::string(config.enable_prediction ? "✅ 是" : "❌ 否"));
        
        Output::info("📊 性能设置:");
        Output::info("  最大缓存大小: " + std::to_string(config.max_cache_size) + " 条目");
        Output::info("  最大并行任务: " + std::to_string(config.max_parallel_tasks) + " 个");
        Output::info("  缓存TTL: " + std::to_string(config.cache_ttl.count()) + " 分钟");
        Output::info("  预测窗口: " + std::to_string(config.prediction_window.count()) + " 分钟");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to get parse configuration: " << e.what();
        Output::error("获取配置信息失败: " + std::string(e.what()));
    }
}

void pm_incremental_parse_clear_cache() {
    LOG(INFO) << "Clearing incremental parse cache";
    
    auto* parser = get_incremental_parser();
    if (!parser) {
        Output::error("增量解析器未初始化");
        return;
    }
    
    try {
        size_t cache_size = parser->get_cache_size();
        parser->clear_cache();
        
        Output::success("✅ 缓存清理完成！");
        Output::info("清理了 " + std::to_string(cache_size) + " 个缓存条目");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to clear cache: " << e.what();
        Output::error("清理缓存失败: " + std::string(e.what()));
    }
}

void pm_incremental_parse_optimize() {
    LOG(INFO) << "Optimizing incremental parse cache";
    
    auto* parser = get_incremental_parser();
    if (!parser) {
        Output::error("增量解析器未初始化");
        return;
    }
    
    try {
        Output::info("🔧 开始优化增量解析缓存...");
        
        // 执行缓存优化
        parser->optimize_cache();
        
        // 预加载常用依赖
        parser->preload_common_dependencies();
        
        Output::success("✅ 缓存优化完成！");
        
        // 显示优化后的统计信息
        auto stats = parser->get_stats();
        Output::info("📊 优化后统计:");
        Output::info("  缓存大小: " + std::to_string(parser->get_cache_size()) + " 条目");
        Output::info("  缓存命中率: " + std::to_string(
            stats.cache_hits + stats.cache_misses > 0 ? 
            (double)stats.cache_hits / (stats.cache_hits + stats.cache_misses) * 100 : 0) + "%");
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to optimize cache: " << e.what();
        Output::error("缓存优化失败: " + std::string(e.what()));
    }
}

void pm_incremental_parse_validate() {
    LOG(INFO) << "Validating incremental parse cache integrity";
    
    auto* parser = get_incremental_parser();
    if (!parser) {
        Output::error("增量解析器未初始化");
        return;
    }
    
    try {
        Output::info("🔍 开始验证缓存完整性...");
        
        bool is_valid = parser->validate_cache_integrity();
        
        if (is_valid) {
            Output::success("✅ 缓存完整性验证通过！");
        } else {
            Output::warning("⚠️ 发现缓存完整性问题");
            Output::info("建议运行 'paker incremental-parse-clear-cache' 清理缓存");
        }
        
        // 显示详细报告
        auto performance_report = parser->get_performance_report();
        std::istringstream report_stream(performance_report);
        std::string line;
        while (std::getline(report_stream, line)) {
            if (!line.empty()) {
                Output::info("  " + line);
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to validate cache: " << e.what();
        Output::error("缓存验证失败: " + std::string(e.what()));
    }
}

} // namespace Paker
