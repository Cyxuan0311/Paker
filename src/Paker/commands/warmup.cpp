#include "Paker/commands/warmup.h"
#include "Paker/core/core_services.h"
#include "Paker/cache/cache_warmup.h"
#include "Paker/core/output.h"
#include "Paker/core/utils.h"
#include <iostream>
#include <iomanip>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Paker {

void pm_warmup() {
    try {
        std::cout << "🔥 启动缓存预热..." << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return;
        }
        
        // 设置进度回调
        warmup_service->set_progress_callback([](const std::string& package, 
                                                const std::string& version, 
                                                size_t current, 
                                                size_t total, 
                                                bool success) {
            std::cout << "[" << current << "/" << total << "] ";
            if (success) {
                std::cout << "✅ ";
            } else {
                std::cout << "❌ ";
            }
            std::cout << package << "@" << version << std::endl;
        });
        
        // 开始智能预热
        bool success = warmup_service->start_smart_preload();
        
        if (success) {
            std::cout << "🎉 缓存预热完成！" << std::endl;
            
            // 显示统计信息
            auto stats = warmup_service->get_statistics();
            std::cout << "\n📊 预热统计:" << std::endl;
            std::cout << "  总包数: " << stats.total_packages << std::endl;
            std::cout << "  成功预热: " << stats.preloaded_packages << std::endl;
            std::cout << "  失败: " << stats.failed_packages << std::endl;
            std::cout << "  成功率: " << std::fixed << std::setprecision(1) 
                      << (stats.success_rate * 100) << "%" << std::endl;
            std::cout << "  总耗时: " << stats.total_time.count() << "ms" << std::endl;
            std::cout << "  平均耗时: " << stats.average_time_per_package.count() << "ms/包" << std::endl;
        } else {
            std::cout << "❌ 缓存预热失败" << std::endl;
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in pm_warmup: " << e.what();
        std::cout << "❌ 预热过程中发生错误: " << e.what() << std::endl;
    }
}

void pm_warmup_analyze() {
    try {
        std::cout << "🔍 分析项目依赖和使用模式..." << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return;
        }
        
        // 分析使用模式
        bool success = warmup_service->analyze_usage_patterns();
        if (!success) {
            std::cout << "⚠️ 无法分析项目依赖，使用默认配置" << std::endl;
        }
        
        // 更新流行度分数
        warmup_service->update_popularity_scores();
        
        // 优化预热顺序
        warmup_service->optimize_preload_order();
        
        // 显示分析结果
        auto packages = warmup_service->get_preload_queue();
        
        std::cout << "\n📋 预热队列分析:" << std::endl;
        std::cout << "  总包数: " << packages.size() << std::endl;
        
        // 按优先级分组显示
        std::map<WarmupPriority, std::vector<PackageWarmupInfo>> priority_groups;
        for (const auto& pkg : packages) {
            priority_groups[pkg.priority].push_back(pkg);
        }
        
        const char* priority_names[] = {"关键", "高", "普通", "低", "后台"};
        
        for (int i = 0; i < 5; ++i) {
            WarmupPriority priority = static_cast<WarmupPriority>(i);
            if (priority_groups.find(priority) != priority_groups.end()) {
                const auto& group = priority_groups[priority];
                std::cout << "  " << priority_names[i] << "优先级 (" << group.size() << "个):" << std::endl;
                
                for (const auto& pkg : group) {
                    std::cout << "    • " << pkg.package_name << "@" << pkg.version;
                    if (pkg.is_essential) {
                        std::cout << " [核心]";
                    }
                    std::cout << " (流行度: " << std::fixed << std::setprecision(2) 
                              << pkg.popularity_score << ")" << std::endl;
                }
            }
        }
        
        std::cout << "\n✅ 分析完成！" << std::endl;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in pm_warmup_analyze: " << e.what();
        std::cout << "❌ 分析过程中发生错误: " << e.what() << std::endl;
    }
}

void pm_warmup_stats() {
    try {
        std::cout << "📊 缓存预热统计信息" << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return;
        }
        
        auto stats = warmup_service->get_statistics();
        
        std::cout << "\n📈 总体统计:" << std::endl;
        std::cout << "  总包数: " << stats.total_packages << std::endl;
        std::cout << "  已预热: " << stats.preloaded_packages << std::endl;
        std::cout << "  失败: " << stats.failed_packages << std::endl;
        std::cout << "  跳过: " << stats.skipped_packages << std::endl;
        std::cout << "  成功率: " << std::fixed << std::setprecision(1) 
                  << (stats.success_rate * 100) << "%" << std::endl;
        
        std::cout << "\n⏱️ 性能统计:" << std::endl;
        std::cout << "  总耗时: " << stats.total_time.count() << "ms" << std::endl;
        std::cout << "  平均耗时: " << stats.average_time_per_package.count() << "ms/包" << std::endl;
        std::cout << "  预热大小: " << (stats.total_size_preloaded / (1024 * 1024)) << " MB" << std::endl;
        
        // 显示当前进度
        if (warmup_service->is_preloading()) {
            std::cout << "\n🔄 当前进度:" << std::endl;
            std::cout << "  进度: " << warmup_service->get_current_progress() 
                      << "/" << warmup_service->get_total_progress() << std::endl;
            std::cout << "  完成率: " << std::fixed << std::setprecision(1) 
                      << warmup_service->get_progress_percentage() << "%" << std::endl;
        }
        
        // 显示已预热的包
        auto preloaded = warmup_service->get_preloaded_packages();
        if (!preloaded.empty()) {
            std::cout << "\n✅ 已预热的包:" << std::endl;
            for (const auto& pkg : preloaded) {
                std::cout << "  • " << pkg.package_name << "@" << pkg.version << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in pm_warmup_stats: " << e.what();
        std::cout << "❌ 获取统计信息时发生错误: " << e.what() << std::endl;
    }
}

void pm_warmup_config() {
    try {
        std::cout << "⚙️ 缓存预热配置" << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return;
        }
        
        // 显示当前配置
        std::cout << "\n📋 当前配置:" << std::endl;
        std::cout << "  最大并发预热数: " << 4 << std::endl;  // 从服务获取
        std::cout << "  最大预热大小: " << (1024) << " MB" << std::endl;  // 从服务获取
        std::cout << "  预热超时: " << 300 << " 秒" << std::endl;  // 从服务获取
        
        // 显示预热队列
        auto packages = warmup_service->get_preload_queue();
        std::cout << "\n📦 预热队列 (" << packages.size() << "个包):" << std::endl;
        
        for (const auto& pkg : packages) {
            std::cout << "  • " << pkg.package_name << "@" << pkg.version;
            if (pkg.is_essential) {
                std::cout << " [核心包]";
            }
            std::cout << std::endl;
        }
        
        std::cout << "\n💡 提示: 使用 'paker warmup analyze' 分析项目依赖" << std::endl;
        std::cout << "💡 提示: 使用 'paker warmup' 开始预热" << std::endl;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in pm_warmup_config: " << e.what();
        std::cout << "❌ 获取配置信息时发生错误: " << e.what() << std::endl;
    }
}

bool configure_warmup_settings() {
    try {
        std::cout << "⚙️ 配置缓存预热设置" << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return false;
        }
        
        // 这里可以实现交互式配置
        // 暂时使用默认配置
        std::cout << "✅ 使用默认预热配置" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in configure_warmup_settings: " << e.what();
        std::cout << "❌ 配置过程中发生错误: " << e.what() << std::endl;
        return false;
    }
}

bool show_warmup_configuration() {
    try {
        pm_warmup_config();
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in show_warmup_configuration: " << e.what();
        return false;
    }
}

bool reset_warmup_configuration() {
    try {
        std::cout << "🔄 重置缓存预热配置" << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            std::cout << "❌ 预热服务未初始化" << std::endl;
            return false;
        }
        
        // 重置配置
        warmup_service->load_default_config();
        
        std::cout << "✅ 配置已重置为默认值" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in reset_warmup_configuration: " << e.what();
        std::cout << "❌ 重置过程中发生错误: " << e.what() << std::endl;
        return false;
    }
}

bool analyze_project_dependencies() {
    try {
        pm_warmup_analyze();
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in analyze_project_dependencies: " << e.what();
        return false;
    }
}

bool generate_warmup_recommendations() {
    try {
        std::cout << "💡 生成预热建议..." << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            return false;
        }
        
        // 分析项目依赖
        warmup_service->analyze_usage_patterns();
        
        // 更新流行度分数
        warmup_service->update_popularity_scores();
        
        // 优化预热顺序
        warmup_service->optimize_preload_order();
        
        std::cout << "✅ 预热建议已生成" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in generate_warmup_recommendations: " << e.what();
        return false;
    }
}

bool optimize_warmup_strategy() {
    try {
        std::cout << "🎯 优化预热策略..." << std::endl;
        
        auto warmup_service = get_cache_warmup_service();
        if (!warmup_service) {
            return false;
        }
        
        // 优化预热顺序
        warmup_service->optimize_preload_order();
        
        std::cout << "✅ 预热策略已优化" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in optimize_warmup_strategy: " << e.what();
        return false;
    }
}

} // namespace Paker
