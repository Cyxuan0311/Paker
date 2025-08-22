#include <gtest/gtest.h>
#include "Paker/core/version_history.h"
#include "Paker/core/output.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class RollbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = fs::temp_directory_path() / "paker_rollback_test";
        fs::create_directories(test_dir_);
        
        // 创建测试包目录
        test_package_dir_ = test_dir_ / "test_package";
        fs::create_directories(test_package_dir_);
        
        // 创建测试文件
        std::ofstream test_file(test_package_dir_ / "test.cpp");
        test_file << "#include <iostream>\nint main() { return 0; }\n";
        test_file.close();
    }
    
    void TearDown() override {
        // 清理测试目录
        fs::remove_all(test_dir_);
    }
    
    fs::path test_dir_;
    fs::path test_package_dir_;
};

TEST_F(RollbackTest, VersionHistoryManagerCreation) {
    // 测试版本历史管理器创建
    Paker::VersionHistoryManager manager(test_dir_.string());
    EXPECT_TRUE(fs::exists(test_dir_ / ".paker"));
    EXPECT_TRUE(fs::exists(test_dir_ / ".paker" / "version_history.json"));
}

TEST_F(RollbackTest, RecordVersionChange) {
    Paker::VersionHistoryManager manager(test_dir_.string());
    
    // 记录版本变更
    bool success = manager.record_version_change(
        "test_package", 
        "1.0.0", 
        "1.1.0", 
        "https://github.com/test/package.git",
        "Test version change"
    );
    
    EXPECT_TRUE(success);
    
    // 验证历史记录
    auto history = manager.get_package_history("test_package");
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].package_name, "test_package");
    EXPECT_EQ(history[0].old_version, "1.0.0");
    EXPECT_EQ(history[0].new_version, "1.1.0");
    EXPECT_EQ(history[0].reason, "Test version change");
}

TEST_F(RollbackTest, GetRollbackableVersions) {
    Paker::VersionHistoryManager manager(test_dir_.string());
    
    // 添加多个版本记录
    manager.record_version_change("test_package", "1.0.0", "1.1.0", "https://github.com/test/package.git");
    manager.record_version_change("test_package", "1.1.0", "1.2.0", "https://github.com/test/package.git");
    manager.record_version_change("test_package", "1.2.0", "1.3.0", "https://github.com/test/package.git");
    
    auto versions = manager.get_rollbackable_versions("test_package");
    EXPECT_EQ(versions.size(), 3);
    
    // 检查版本是否包含在列表中
    EXPECT_TRUE(std::find(versions.begin(), versions.end(), "1.1.0") != versions.end());
    EXPECT_TRUE(std::find(versions.begin(), versions.end(), "1.2.0") != versions.end());
    EXPECT_TRUE(std::find(versions.begin(), versions.end(), "1.3.0") != versions.end());
}

TEST_F(RollbackTest, RollbackSafetyCheck) {
    Paker::VersionHistoryManager manager(test_dir_.string());
    
    // 添加版本记录
    manager.record_version_change("test_package", "1.0.0", "1.1.0", "https://github.com/test/package.git");
    
    // 检查回滚安全性
    bool can_rollback = manager.can_safely_rollback("test_package", "1.0.0");
    // 由于没有实际的依赖检查，这里应该返回true
    EXPECT_TRUE(can_rollback);
}

TEST_F(RollbackTest, RollbackUtilsSafetyCheck) {
    // 测试回滚工具类的安全性检查
    bool is_safe = Paker::RollbackUtils::check_rollback_safety("test_package", "1.0.0");
    // 由于没有历史记录，应该返回false
    EXPECT_FALSE(is_safe);
}

TEST_F(RollbackTest, BackupCreation) {
    Paker::VersionHistoryManager manager(test_dir_.string());
    
    // 创建备份
    bool backup_created = manager.create_backup("test_package", "1.0.0");
    EXPECT_TRUE(backup_created);
    
    // 验证备份文件存在
    auto backup_files = fs::directory_iterator(test_dir_ / ".paker" / "backups");
    bool found_backup = false;
    for (const auto& entry : backup_files) {
        if (entry.path().filename().string().find("test_package_1.0.0_") == 0) {
            found_backup = true;
            break;
        }
    }
    EXPECT_TRUE(found_backup);
}

TEST_F(RollbackTest, RollbackReportGeneration) {
    Paker::RollbackResult result;
    result.success = true;
    result.message = "Test rollback completed";
    result.rolled_back_packages = {"test_package"};
    result.duration = std::chrono::milliseconds(1500);
    
    std::string report = Paker::RollbackUtils::generate_rollback_report(result);
    
    // 验证报告包含关键信息
    EXPECT_TRUE(report.find("✅ Success") != std::string::npos);
    EXPECT_TRUE(report.find("test_package") != std::string::npos);
    EXPECT_TRUE(report.find("1500ms") != std::string::npos);
}

TEST_F(RollbackTest, HistoryCleanup) {
    Paker::VersionHistoryManager manager(test_dir_.string());
    
    // 添加多个历史记录
    for (int i = 0; i < 10; ++i) {
        std::string version = "1." + std::to_string(i) + ".0";
        manager.record_version_change("test_package", version, version, "https://github.com/test/package.git");
    }
    
    // 清理历史记录，保留5个
    bool cleanup_success = manager.cleanup_old_history(5);
    EXPECT_TRUE(cleanup_success);
    
    // 验证只保留了最新的5个记录
    auto history = manager.get_recent_history();
    EXPECT_LE(history.size(), 5);
}

TEST_F(RollbackTest, HistoryExportImport) {
    Paker::VersionHistoryManager manager1(test_dir_.string());
    
    // 添加一些历史记录
    manager1.record_version_change("test_package", "1.0.0", "1.1.0", "https://github.com/test/package.git");
    manager1.record_version_change("test_package", "1.1.0", "1.2.0", "https://github.com/test/package.git");
    
    // 导出历史记录
    std::string export_path = (test_dir_ / "history_export.json").string();
    bool export_success = manager1.export_history(export_path);
    EXPECT_TRUE(export_success);
    EXPECT_TRUE(fs::exists(export_path));
    
    // 创建新的管理器并导入历史记录
    fs::path import_dir = test_dir_ / "import_test";
    fs::create_directories(import_dir);
    Paker::VersionHistoryManager manager2(import_dir.string());
    
    bool import_success = manager2.import_history(export_path);
    EXPECT_TRUE(import_success);
    
    // 验证导入的历史记录
    auto history = manager2.get_package_history("test_package");
    EXPECT_EQ(history.size(), 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 