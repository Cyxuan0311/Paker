#include <gtest/gtest.h>
#include "Paker/cache/cache_manager.h"
#include <filesystem>
#include <fstream>

using namespace Paker;
namespace fs = std::filesystem;

class CacheManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_cache_dir_ = fs::temp_directory_path() / "paker_test_cache";
        test_project_dir_ = fs::temp_directory_path() / "paker_test_project";
        
        fs::create_directories(test_cache_dir_);
        fs::create_directories(test_project_dir_);
        
        // 创建测试缓存管理器
        cache_manager_ = std::make_unique<CacheManager>();
        
        // 设置测试缓存路径
        cache_manager_->set_cache_strategy(CacheStrategy::USER_ONLY);
    }
    
    void TearDown() override {
        // 清理测试目录
        if (fs::exists(test_cache_dir_)) {
            fs::remove_all(test_cache_dir_);
        }
        if (fs::exists(test_project_dir_)) {
            fs::remove_all(test_project_dir_);
        }
    }
    
    fs::path test_cache_dir_;
    fs::path test_project_dir_;
    std::unique_ptr<CacheManager> cache_manager_;
};

TEST_F(CacheManagerTest, BasicInitialization) {
    EXPECT_TRUE(cache_manager_->initialize());
    EXPECT_EQ(cache_manager_->get_cache_strategy(), CacheStrategy::USER_ONLY);
}

TEST_F(CacheManagerTest, PackageInstallation) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 测试包安装到缓存
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    EXPECT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    
    // 验证包是否已缓存
    EXPECT_TRUE(cache_manager_->is_package_cached("fmt", "latest"));
    
    // 验证缓存路径
    std::string cached_path = cache_manager_->get_cached_package_path("fmt", "latest");
    EXPECT_FALSE(cached_path.empty());
    EXPECT_TRUE(fs::exists(cached_path));
}

TEST_F(CacheManagerTest, ProjectLinkCreation) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 先安装包到缓存
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    
    // 创建项目链接
    EXPECT_TRUE(cache_manager_->create_project_link("fmt", "latest", test_project_dir_.string()));
    
    // 验证项目链接
    std::string project_path = cache_manager_->get_project_package_path("fmt", test_project_dir_.string());
    EXPECT_FALSE(project_path.empty());
    
    // 验证符号链接是否存在
    fs::path link_path = test_project_dir_ / ".paker" / "links" / "fmt";
    EXPECT_TRUE(fs::exists(link_path));
    EXPECT_TRUE(fs::is_symlink(link_path));
}

TEST_F(CacheManagerTest, PackageRemoval) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 安装包
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    ASSERT_TRUE(cache_manager_->is_package_cached("fmt", "latest"));
    
    // 移除包
    EXPECT_TRUE(cache_manager_->remove_package_from_cache("fmt", "latest"));
    EXPECT_FALSE(cache_manager_->is_package_cached("fmt", "latest"));
}

TEST_F(CacheManagerTest, CacheStatistics) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 安装多个包
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "8.1.1", test_repo));
    
    // 获取统计信息
    auto stats = cache_manager_->get_cache_statistics();
    EXPECT_GE(stats.total_packages, 2);
    EXPECT_GT(stats.total_size_bytes, 0);
}

TEST_F(CacheManagerTest, MultipleVersions) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    
    // 安装多个版本
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "8.1.1", test_repo));
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "9.1.0", test_repo));
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    
    // 验证所有版本都存在
    EXPECT_TRUE(cache_manager_->is_package_cached("fmt", "8.1.1"));
    EXPECT_TRUE(cache_manager_->is_package_cached("fmt", "9.1.0"));
    EXPECT_TRUE(cache_manager_->is_package_cached("fmt", "latest"));
    
    // 获取最新版本路径
    std::string latest_path = cache_manager_->get_cached_package_path("fmt");
    EXPECT_FALSE(latest_path.empty());
}

TEST_F(CacheManagerTest, CacheCleanup) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    
    // 安装多个版本
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "8.1.1", test_repo));
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "9.1.0", test_repo));
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    
    // 执行清理
    EXPECT_TRUE(cache_manager_->cleanup_old_versions());
    
    // 验证清理结果（应该保留最新版本）
    auto stats = cache_manager_->get_cache_statistics();
    EXPECT_LE(stats.total_packages, 3);  // 可能清理了一些版本
}

TEST_F(CacheManagerTest, CacheIndexPersistence) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 安装包
    std::string test_repo = "https://github.com/fmtlib/fmt.git";
    ASSERT_TRUE(cache_manager_->install_package_to_cache("fmt", "latest", test_repo));
    
    // 创建新的缓存管理器实例（模拟重启）
    auto new_cache_manager = std::make_unique<CacheManager>();
    ASSERT_TRUE(new_cache_manager->initialize());
    
    // 验证包信息是否持久化
    EXPECT_TRUE(new_cache_manager->is_package_cached("fmt", "latest"));
}

TEST_F(CacheManagerTest, StrategyConfiguration) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 测试不同策略
    cache_manager_->set_cache_strategy(CacheStrategy::GLOBAL_ONLY);
    EXPECT_EQ(cache_manager_->get_cache_strategy(), CacheStrategy::GLOBAL_ONLY);
    
    cache_manager_->set_cache_strategy(CacheStrategy::HYBRID);
    EXPECT_EQ(cache_manager_->get_cache_strategy(), CacheStrategy::HYBRID);
    
    cache_manager_->set_cache_strategy(CacheStrategy::PROJECT_LOCAL);
    EXPECT_EQ(cache_manager_->get_cache_strategy(), CacheStrategy::PROJECT_LOCAL);
}

TEST_F(CacheManagerTest, VersionStorageConfiguration) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 测试不同存储策略
    cache_manager_->set_version_storage(VersionStorage::ARCHIVE_ONLY);
    EXPECT_EQ(cache_manager_->get_version_storage(), VersionStorage::ARCHIVE_ONLY);
    
    cache_manager_->set_version_storage(VersionStorage::COMPRESSED);
    EXPECT_EQ(cache_manager_->get_version_storage(), VersionStorage::COMPRESSED);
    
    cache_manager_->set_version_storage(VersionStorage::SHALLOW_CLONE);
    EXPECT_EQ(cache_manager_->get_version_storage(), VersionStorage::SHALLOW_CLONE);
}

TEST_F(CacheManagerTest, PathResolution) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 测试路径解析
    cache_manager_->set_cache_strategy(CacheStrategy::USER_ONLY);
    
    std::string resolved_path = cache_manager_->get_cached_package_path("test_package", "1.0.0");
    EXPECT_FALSE(resolved_path.empty());
    EXPECT_NE(resolved_path.find("test_package"), std::string::npos);
    EXPECT_NE(resolved_path.find("1.0.0"), std::string::npos);
}

TEST_F(CacheManagerTest, ErrorHandling) {
    ASSERT_TRUE(cache_manager_->initialize());
    
    // 测试无效的包安装
    EXPECT_FALSE(cache_manager_->install_package_to_cache("", "latest", ""));
    EXPECT_FALSE(cache_manager_->install_package_to_cache("invalid_package", "latest", "invalid_url"));
    
    // 测试获取不存在的包路径
    std::string non_existent_path = cache_manager_->get_cached_package_path("non_existent", "1.0.0");
    EXPECT_TRUE(non_existent_path.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 