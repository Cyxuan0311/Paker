#include "Paker/dependency/incremental_parser.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <glog/logging.h>

namespace Paker {

SmartParseStrategy::Strategy SmartParseStrategy::select_strategy(
    const std::string& package, const std::string& version) const {
    
    // 检查是否有历史数据
    auto pattern_it = package_patterns_.find(package);
    auto freq_it = package_frequencies_.find(package);
    auto last_used_it = last_used_.find(package);
    
    if (pattern_it == package_patterns_.end()) {
        // 没有历史数据，使用完整解析
        return Strategy::FULL_PARSE;
    }
    
    // 检查最后使用时间
    if (last_used_it != last_used_.end()) {
        auto now = std::chrono::system_clock::now();
        auto time_since_last_use = now - last_used_it->second;
        
        // 如果超过30分钟未使用，使用完整解析
        if (time_since_last_use > std::chrono::minutes(30)) {
            return Strategy::FULL_PARSE;
        }
    }
    
    // 检查使用频率
    if (freq_it != package_frequencies_.end()) {
        double frequency = freq_it->second;
        
        // 高频使用的包，使用预测解析
        if (frequency > 0.8) {
            return Strategy::PREDICTIVE;
        }
        
        // 中频使用的包，使用增量解析
        if (frequency > 0.3) {
            return Strategy::INCREMENTAL;
        }
    }
    
    // 默认使用增量解析
    return Strategy::INCREMENTAL;
}

void SmartParseStrategy::learn_pattern(const std::string& package, 
                                      const std::vector<std::string>& dependencies) {
    package_patterns_[package] = dependencies;
    
    // 更新使用频率
    update_frequency(package);
    
    LOG(INFO) << "Learned pattern for package " << package 
              << " with " << dependencies.size() << " dependencies";
}

void SmartParseStrategy::update_frequency(const std::string& package) {
    auto now = std::chrono::system_clock::now();
    last_used_[package] = now;
    
    // 计算使用频率（基于时间衰减）
    auto it = package_frequencies_.find(package);
    if (it != package_frequencies_.end()) {
        // 时间衰减因子
        auto time_since_last_use = now - last_used_[package];
        double decay_factor = std::exp(-std::chrono::duration<double>(time_since_last_use).count() / 3600.0); // 1小时衰减
        
        it->second = it->second * decay_factor + 1.0;
    } else {
        package_frequencies_[package] = 1.0;
    }
}

std::vector<std::string> SmartParseStrategy::predict_dependencies(const std::string& package) const {
    auto it = package_patterns_.find(package);
    if (it != package_patterns_.end()) {
        return it->second;
    }
    
    // 基于相似包的模式进行预测
    std::vector<std::string> predicted;
    
    // 简单的相似性匹配（可以进一步优化）
    for (const auto& [other_package, dependencies] : package_patterns_) {
        if (other_package != package && 
            other_package.find(package) != std::string::npos) {
            // 如果包名相似，使用其依赖模式
            predicted.insert(predicted.end(), dependencies.begin(), dependencies.end());
            break;
        }
    }
    
    return predicted;
}

double SmartParseStrategy::get_confidence(const std::string& package) const {
    auto pattern_it = package_patterns_.find(package);
    auto freq_it = package_frequencies_.find(package);
    
    if (pattern_it == package_patterns_.end()) {
        return 0.0; // 没有历史数据
    }
    
    double confidence = 0.5; // 基础置信度
    
    // 基于使用频率调整置信度
    if (freq_it != package_frequencies_.end()) {
        double frequency = freq_it->second;
        confidence += frequency * 0.3; // 频率贡献30%
    }
    
    // 基于模式稳定性调整置信度
    if (pattern_it->second.size() > 0) {
        confidence += 0.2; // 有依赖模式贡献20%
    }
    
    return std::min(confidence, 1.0);
}

std::vector<std::string> SmartParseStrategy::get_optimization_suggestions() const {
    std::vector<std::string> suggestions;
    
    // 分析使用模式
    std::vector<std::pair<std::string, double>> sorted_packages;
    for (const auto& [package, frequency] : package_frequencies_) {
        sorted_packages.emplace_back(package, frequency);
    }
    
    std::sort(sorted_packages.begin(), sorted_packages.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 建议预加载高频包
    if (!sorted_packages.empty() && sorted_packages[0].second > 0.7) {
        suggestions.push_back("Consider preloading high-frequency package: " + sorted_packages[0].first);
    }
    
    // 建议清理低频包
    size_t low_freq_count = 0;
    for (const auto& [package, frequency] : sorted_packages) {
        if (frequency < 0.1) {
            low_freq_count++;
        }
    }
    
    if (low_freq_count > 5) {
        suggestions.push_back("Consider cleaning up " + std::to_string(low_freq_count) + " low-frequency packages");
    }
    
    // 建议优化缓存策略
    if (package_patterns_.size() > 100) {
        suggestions.push_back("Consider implementing more aggressive cache eviction for large pattern sets");
    }
    
    return suggestions;
}

} // namespace Paker
