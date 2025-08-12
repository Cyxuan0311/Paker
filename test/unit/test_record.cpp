#include <gtest/gtest.h>
#include "Recorder/record.h"
#include <filesystem>
#include <fstream>

class RecordTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前清理测试文件
        if (std::filesystem::exists(test_record_file_)) {
            std::filesystem::remove(test_record_file_);
        }
    }
    
    void TearDown() override {
        // 每个测试后清理测试文件
        if (std::filesystem::exists(test_record_file_)) {
            std::filesystem::remove(test_record_file_);
        }
    }
    
    std::string test_record_file_ = "test_record_gtest.json";
};

// 测试构造函数和基本功能
TEST_F(RecordTest, ConstructorAndBasicFunctionality) {
    Recorder::Record record(test_record_file_);
    
    // 测试初始状态
    EXPECT_TRUE(record.getAllPackages().empty());
    EXPECT_FALSE(record.isPackageInstalled("nonexistent"));
}

// 测试添加包记录
TEST_F(RecordTest, AddPackageRecord) {
    Recorder::Record record(test_record_file_);
    
    std::vector<std::string> files = {
        "/usr/local/lib/libcurl.so",
        "/usr/local/lib/libcurl.a",
        "/usr/local/include/curl/curl.h"
    };
    
    record.addPackageRecord("libcurl", "/usr/local/lib", files);
    
    // 验证包已安装
    EXPECT_TRUE(record.isPackageInstalled("libcurl"));
    EXPECT_FALSE(record.isPackageInstalled("nonexistent"));
    
    // 验证文件列表
    auto recorded_files = record.getPackageFiles("libcurl");
    EXPECT_EQ(recorded_files.size(), files.size());
    
    // 验证安装路径
    EXPECT_EQ(record.getPackageInstallPath("libcurl"), "/usr/local/lib");
}

// 测试添加单个文件记录
TEST_F(RecordTest, AddFileRecord) {
    Recorder::Record record(test_record_file_);
    
    // 先添加包记录
    record.addPackageRecord("libcurl", "/usr/local/lib");
    
    // 添加单个文件
    record.addFileRecord("libcurl", "/usr/local/bin/curl");
    record.addFileRecord("libcurl", "/usr/local/share/curl/ca-bundle.crt");
    
    // 验证文件已添加
    auto files = record.getPackageFiles("libcurl");
    EXPECT_EQ(files.size(), 2);
    
    // 验证文件内容
    EXPECT_TRUE(std::find(files.begin(), files.end(), "/usr/local/bin/curl") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "/usr/local/share/curl/ca-bundle.crt") != files.end());
}

// 测试重复添加文件
TEST_F(RecordTest, DuplicateFileHandling) {
    Recorder::Record record(test_record_file_);
    
    record.addPackageRecord("libcurl", "/usr/local/lib");
    
    // 添加相同文件多次
    record.addFileRecord("libcurl", "/usr/local/bin/curl");
    record.addFileRecord("libcurl", "/usr/local/bin/curl");
    record.addFileRecord("libcurl", "/usr/local/bin/curl");
    
    // 验证文件只被添加一次
    auto files = record.getPackageFiles("libcurl");
    EXPECT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], "/usr/local/bin/curl");
}

// 测试删除包记录
TEST_F(RecordTest, RemovePackageRecord) {
    Recorder::Record record(test_record_file_);
    
    // 添加包
    record.addPackageRecord("libcurl", "/usr/local/lib", {
        "/usr/local/lib/libcurl.so",
        "/usr/local/include/curl/curl.h"
    });
    
    // 验证包存在
    EXPECT_TRUE(record.isPackageInstalled("libcurl"));
    
    // 删除包
    EXPECT_TRUE(record.removePackageRecord("libcurl"));
    
    // 验证包已删除
    EXPECT_FALSE(record.isPackageInstalled("libcurl"));
    
    // 尝试删除不存在的包
    EXPECT_FALSE(record.removePackageRecord("nonexistent"));
}

// 测试获取所有包
TEST_F(RecordTest, GetAllPackages) {
    Recorder::Record record(test_record_file_);
    
    // 添加多个包
    record.addPackageRecord("libcurl", "/usr/local/lib");
    record.addPackageRecord("openssl", "/usr/local/ssl");
    record.addPackageRecord("zlib", "/usr/local/zlib");
    
    auto packages = record.getAllPackages();
    EXPECT_EQ(packages.size(), 3);
    
    // 验证包名
    EXPECT_TRUE(std::find(packages.begin(), packages.end(), "libcurl") != packages.end());
    EXPECT_TRUE(std::find(packages.begin(), packages.end(), "openssl") != packages.end());
    EXPECT_TRUE(std::find(packages.begin(), packages.end(), "zlib") != packages.end());
}

// 测试文件持久化
TEST_F(RecordTest, FilePersistence) {
    {
        // 创建记录并添加数据
        Recorder::Record record(test_record_file_);
        record.addPackageRecord("libcurl", "/usr/local/lib", {
            "/usr/local/lib/libcurl.so",
            "/usr/local/include/curl/curl.h"
        });
    } // record析构，数据保存到文件
    
    {
        // 创建新的记录实例，从文件加载数据
        Recorder::Record record(test_record_file_);
        
        // 验证数据已加载
        EXPECT_TRUE(record.isPackageInstalled("libcurl"));
        EXPECT_EQ(record.getPackageInstallPath("libcurl"), "/usr/local/lib");
        
        auto files = record.getPackageFiles("libcurl");
        EXPECT_EQ(files.size(), 2);
    }
}

// 测试空包处理
TEST_F(RecordTest, EmptyPackageHandling) {
    Recorder::Record record(test_record_file_);
    
    // 添加空文件列表的包
    record.addPackageRecord("empty-pkg", "/usr/local/empty");
    
    EXPECT_TRUE(record.isPackageInstalled("empty-pkg"));
    EXPECT_EQ(record.getPackageFiles("empty-pkg").size(), 0);
    EXPECT_EQ(record.getPackageInstallPath("empty-pkg"), "/usr/local/empty");
}

// 测试不存在的包查询
TEST_F(RecordTest, NonExistentPackageQueries) {
    Recorder::Record record(test_record_file_);
    
    // 查询不存在的包
    EXPECT_FALSE(record.isPackageInstalled("nonexistent"));
    EXPECT_EQ(record.getPackageFiles("nonexistent").size(), 0);
    EXPECT_EQ(record.getPackageInstallPath("nonexistent"), "");
}

// 测试JSON文件格式
TEST_F(RecordTest, JsonFileFormat) {
    Recorder::Record record(test_record_file_);
    
    record.addPackageRecord("libcurl", "/usr/local/lib", {
        "/usr/local/lib/libcurl.so",
        "/usr/local/include/curl/curl.h"
    });
    
    // 验证文件存在且格式正确
    EXPECT_TRUE(std::filesystem::exists(test_record_file_));
    
    std::ifstream file(test_record_file_);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // 验证JSON格式
    EXPECT_FALSE(content.empty());
    EXPECT_TRUE(content.find("libcurl") != std::string::npos);
    EXPECT_TRUE(content.find("/usr/local/lib") != std::string::npos);
    EXPECT_TRUE(content.find("libcurl.so") != std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 