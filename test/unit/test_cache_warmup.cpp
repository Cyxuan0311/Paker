#include <gtest/gtest.h>
#include "Paker/cache/cache_warmup.h"
#include "Paker/core/service_container.h"
#include "Paker/core/core_services.h"
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class CacheWarmupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化服务管理器
        Paker::initialize_service_manager();
        Paker::ServiceFactory::register_all_core_services();
        
        // 创建测试目录
        test_dir_ = fs::temp_directory_path() / "paker_warmup_test";
        fs::create_directories(test_dir_);
        
        // 获取预热服务
        warmup_service_ = Paker::get_cache_warmup_service();
        ASSERT_NE(warmup_service_, nullptr);
    }
    
    void TearDown() override {
        // 清理测试目录
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        
        // 清理服务
        Paker::cleanup_service_manager();
    }
    
    fs::path test_dir_;
    Paker::CacheWarmupService* warmup_service_;
};

TEST_F(CacheWarmupTest, ServiceInitialization) {
    EXPECT_NE(warmup_service_, nullptr);
    EXPECT_EQ(warmup_service_->get_name(), "CacheWarmupService");
}

TEST_F(CacheWarmupTest, PackageRegistration) {
    // 注册测试包
    bool success = warmup_service_->register_package(
        "test-package", "1.0.0", "https://github.com/test/package", 
        Paker::WarmupPriority::HIGH
    );
    
    EXPECT_TRUE(success);
    
    // 验证包已注册
    auto packages = warmup_service_->get_preload_queue();
    EXPECT_FALSE(packages.empty());
    
    bool found = false;
    for (const auto& pkg : packages) {
        if (pkg.package_name == "test-package" && pkg.version == "1.0.0") {
            found = true;
            EXPECT_EQ(pkg.priority, Paker::WarmupPriority::HIGH);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CacheWarmupTest, PackageUnregistration) {
    // 先注册包
    warmup_service_->register_package("test-package", "1.0.0", "https://github.com/test/package");
    
    // 验证包已注册
    auto packages_before = warmup_service_->get_preload_queue();
    EXPECT_FALSE(packages_before.empty());
    
    // 取消注册
    bool success = warmup_service_->unregister_package("test-package", "1.0.0");
    EXPECT_TRUE(success);
    
    // 验证包已移除
    auto packages_after = warmup_service_->get_preload_queue();
    bool found = false;
    for (const auto& pkg : packages_after) {
        if (pkg.package_name == "test-package" && pkg.version == "1.0.0") {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
}

TEST_F(CacheWarmupTest, PriorityManagement) {
    // 注册不同优先级的包
    warmup_service_->register_package("critical-pkg", "1.0.0", "https://github.com/test/critical", 
                                     Paker::WarmupPriority::CRITICAL);
    warmup_service_->register_package("high-pkg", "1.0.0", "https://github.com/test/high", 
                                     Paker::WarmupPriority::HIGH);
    warmup_service_->register_package("normal-pkg", "1.0.0", "https://github.com/test/normal", 
                                     Paker::WarmupPriority::NORMAL);
    
    // 优化预热顺序
    warmup_service_->optimize_preload_order();
    
    auto packages = warmup_service_->get_preload_queue();
    EXPECT_EQ(packages.size(), 3);
    
    // 验证优先级顺序
    EXPECT_EQ(packages[0].priority, Paker::WarmupPriority::CRITICAL);
    EXPECT_EQ(packages[0].package_name, "critical-pkg");
    
    EXPECT_EQ(packages[1].priority, Paker::WarmupPriority::HIGH);
    EXPECT_EQ(packages[1].package_name, "high-pkg");
    
    EXPECT_EQ(packages[2].priority, Paker::WarmupPriority::NORMAL);
    EXPECT_EQ(packages[2].package_name, "normal-pkg");
}

TEST_F(CacheWarmupTest, ProgressTracking) {
    // 注册测试包
    warmup_service_->register_package("test-package", "1.0.0", "https://github.com/test/package");
    
    // 设置进度回调
    bool callback_called = false;
    warmup_service_->set_progress_callback([&callback_called](const std::string& package, 
                                                             const std::string& version, 
                                                             size_t current, 
                                                             size_t total, 
                                                             bool success) {
        callback_called = true;
        EXPECT_EQ(package, "test-package");
        EXPECT_EQ(version, "1.0.0");
        EXPECT_EQ(current, 1);
        EXPECT_EQ(total, 1);
    });
    
    // 开始预热（异步模式）
    bool success = warmup_service_->start_preload(Paker::WarmupStrategy::ASYNC);
    
    // 等待预热完成
    int attempts = 0;
    while (warmup_service_->is_preloading() && attempts < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }
    
    // 验证预热完成
    EXPECT_FALSE(warmup_service_->is_preloading());
    
    // 验证统计信息
    auto stats = warmup_service_->get_statistics();
    EXPECT_GT(stats.total_packages, 0);
}

TEST_F(CacheWarmupTest, StatisticsCollection) {
    // 注册多个包
    warmup_service_->register_package("pkg1", "1.0.0", "https://github.com/test/pkg1");
    warmup_service_->register_package("pkg2", "1.0.0", "https://github.com/test/pkg2");
    warmup_service_->register_package("pkg3", "1.0.0", "https://github.com/test/pkg3");
    
    // 获取统计信息
    auto stats = warmup_service_->get_statistics();
    EXPECT_EQ(stats.total_packages, 3);
    EXPECT_EQ(stats.preloaded_packages, 0);
    EXPECT_EQ(stats.failed_packages, 0);
    EXPECT_EQ(stats.success_rate, 0.0);
}

TEST_F(CacheWarmupTest, ConfigurationManagement) {
    // 测试配置保存和加载
    std::string config_path = (test_dir_ / "warmup_config.json").string();
    
    // 注册一些包
    warmup_service_->register_package("config-test", "1.0.0", "https://github.com/test/config");
    
    // 保存配置
    bool save_success = warmup_service_->save_preload_config(config_path);
    EXPECT_TRUE(save_success);
    EXPECT_TRUE(fs::exists(config_path));
    
    // 创建新的预热服务实例来测试加载
    auto new_warmup = std::make_unique<Paker::CacheWarmupService>();
    new_warmup->initialize();
    
    // 加载配置
    bool load_success = new_warmup->load_preload_config(config_path);
    EXPECT_TRUE(load_success);
    
    // 验证配置已加载
    auto packages = new_warmup->get_preload_queue();
    EXPECT_FALSE(packages.empty());
    
    bool found = false;
    for (const auto& pkg : packages) {
        if (pkg.package_name == "config-test" && pkg.version == "1.0.0") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CacheWarmupTest, SmartPreload) {
    // 创建测试项目配置文件
    std::string project_config = (test_dir_ / ".paker" / "paker.json").string();
    fs::create_directories(fs::path(project_config).parent_path());
    
    nlohmann::json config;
    config["dependencies"]["test-dependency"] = "1.0.0";
    config["dependencies"]["another-dependency"] = "2.0.0";
    
    std::ofstream file(project_config);
    file << config.dump(4);
    file.close();
    
    // 切换到测试目录
    auto old_cwd = fs::current_path();
    fs::current_path(test_dir_);
    
    // 执行智能预热分析
    bool success = warmup_service_->analyze_usage_patterns();
    EXPECT_TRUE(success);
    
    // 恢复工作目录
    fs::current_path(old_cwd);
    
    // 验证依赖已被注册
    auto packages = warmup_service_->get_preload_queue();
    EXPECT_GT(packages.size(), 0);
}

TEST_F(CacheWarmupTest, EssentialPackages) {
    // 预加载核心包
    bool success = warmup_service_->preload_essential_packages();
    EXPECT_TRUE(success);
    
    // 验证核心包已注册
    auto packages = warmup_service_->get_preload_queue();
    EXPECT_FALSE(packages.empty());
    
    // 检查是否包含核心包
    bool has_essential = false;
    for (const auto& pkg : packages) {
        if (pkg.is_essential) {
            has_essential = true;
            break;
        }
    }
    EXPECT_TRUE(has_essential);
}

TEST_F(CacheWarmupTest, PopularPackages) {
    // 预加载流行包
    bool success = warmup_service_->preload_popular_packages(5);
    EXPECT_TRUE(success);
    
    // 验证包已注册
    auto packages = warmup_service_->get_preload_queue();
    EXPECT_GT(packages.size(), 0);
}

TEST_F(CacheWarmupTest, StopPreload) {
    // 注册包
    warmup_service_->register_package("test-package", "1.0.0", "https://github.com/test/package");
    
    // 开始异步预热
    warmup_service_->start_preload(Paker::WarmupStrategy::ASYNC);
    
    // 立即停止
    bool success = warmup_service_->stop_preload();
    EXPECT_TRUE(success);
    
    // 验证已停止
    EXPECT_FALSE(warmup_service_->is_preloading());
}

TEST_F(CacheWarmupTest, ProgressPercentage) {
    // 注册包
    warmup_service_->register_package("pkg1", "1.0.0", "https://github.com/test/pkg1");
    warmup_service_->register_package("pkg2", "1.0.0", "https://github.com/test/pkg2");
    
    // 验证进度百分比
    double percentage = warmup_service_->get_progress_percentage();
    EXPECT_EQ(percentage, 0.0);  // 初始状态
    
    // 开始预热
    warmup_service_->start_preload(Paker::WarmupStrategy::ASYNC);
    
    // 等待完成
    int attempts = 0;
    while (warmup_service_->is_preloading() && attempts < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }
    
    // 验证最终进度
    percentage = warmup_service_->get_progress_percentage();
    EXPECT_EQ(percentage, 100.0);
}
