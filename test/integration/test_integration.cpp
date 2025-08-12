#include <gtest/gtest.h>
#include "Paker/utils.h"
#include "Recorder/record.h"
#include <filesystem>
#include <fstream>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前清理测试文件
        test_record_file_ = get_record_file_path();
        if (std::filesystem::exists(test_record_file_)) {
            std::filesystem::remove(test_record_file_);
        }
        
        // 创建测试目录
        test_package_dir_ = "./test_package_integration";
        if (std::filesystem::exists(test_package_dir_)) {
            std::filesystem::remove_all(test_package_dir_);
        }
        std::filesystem::create_directories(test_package_dir_);
        
        // 创建测试文件
        createTestFiles();
    }
    
    void TearDown() override {
        // 每个测试后清理
        if (std::filesystem::exists(test_record_file_)) {
            std::filesystem::remove(test_record_file_);
        }
        if (std::filesystem::exists(test_package_dir_)) {
            std::filesystem::remove_all(test_package_dir_);
        }
    }
    
    void createTestFiles() {
        // 创建测试文件结构
        std::filesystem::create_directories(test_package_dir_ + "/lib");
        std::filesystem::create_directories(test_package_dir_ + "/include");
        std::filesystem::create_directories(test_package_dir_ + "/src");
        
        // 创建测试文件
        std::ofstream lib_file(test_package_dir_ + "/lib/test.so");
        lib_file << "test library content";
        lib_file.close();
        
        std::ofstream header_file(test_package_dir_ + "/include/test.h");
        header_file << "#pragma once\nvoid test_function();";
        header_file.close();
        
        std::ofstream src_file(test_package_dir_ + "/src/main.cpp");
        src_file << "#include \"test.h\"\nint main() { return 0; }";
        src_file.close();
        
        std::ofstream readme_file(test_package_dir_ + "/README.md");
        readme_file << "# Test Package\nThis is a test package for integration testing.";
        readme_file.close();
    }
    
    std::string test_record_file_;
    std::string test_package_dir_;
};

// 测试工具函数
TEST_F(IntegrationTest, UtilityFunctions) {
    std::string project_name = get_project_name();
    std::string json_file = get_json_file();
    std::string record_file = get_record_file_path();
    
    // 验证工具函数返回值
    EXPECT_FALSE(project_name.empty());
    EXPECT_FALSE(json_file.empty());
    EXPECT_FALSE(record_file.empty());
    
    // 验证文件名格式
    EXPECT_EQ(json_file, project_name + ".json");
    EXPECT_EQ(record_file, project_name + "_install_record.json");
}

// 测试文件收集功能
TEST_F(IntegrationTest, FileCollection) {
    std::vector<std::string> files = collect_package_files(test_package_dir_);
    
    // 验证收集到的文件数量
    EXPECT_GT(files.size(), 0);
    
    // 验证特定文件是否存在
    bool found_lib = false, found_header = false, found_src = false, found_readme = false;
    for (const auto& file : files) {
        if (file.find("test.so") != std::string::npos) found_lib = true;
        if (file.find("test.h") != std::string::npos) found_header = true;
        if (file.find("main.cpp") != std::string::npos) found_src = true;
        if (file.find("README.md") != std::string::npos) found_readme = true;
    }
    
    EXPECT_TRUE(found_lib);
    EXPECT_TRUE(found_header);
    EXPECT_TRUE(found_src);
    EXPECT_TRUE(found_readme);
}

// 测试Record类与工具函数集成
TEST_F(IntegrationTest, RecordWithUtilityFunctions) {
    Recorder::Record record(test_record_file_);
    
    // 使用工具函数收集文件
    std::vector<std::string> files = collect_package_files(test_package_dir_);
    
    // 添加包记录
    record.addPackageRecord("test-lib", test_package_dir_, files);
    
    // 验证记录
    EXPECT_TRUE(record.isPackageInstalled("test-lib"));
    EXPECT_EQ(record.getPackageInstallPath("test-lib"), test_package_dir_);
    
    auto recorded_files = record.getPackageFiles("test-lib");
    EXPECT_EQ(recorded_files.size(), files.size());
}

// 测试包名版本解析
TEST_F(IntegrationTest, PackageNameVersionParsing) {
    // 测试不带版本的包名
    auto [pkg1, ver1] = parse_name_version("libcurl");
    EXPECT_EQ(pkg1, "libcurl");
    EXPECT_EQ(ver1, "");
    
    // 测试带版本的包名
    auto [pkg2, ver2] = parse_name_version("libcurl@7.68.0");
    EXPECT_EQ(pkg2, "libcurl");
    EXPECT_EQ(ver2, "7.68.0");
    
    // 测试带@但没有版本的情况
    auto [pkg3, ver3] = parse_name_version("lib@curl");
    EXPECT_EQ(pkg3, "lib");
    EXPECT_EQ(ver3, "curl");
}

// 测试Record类的持久化与工具函数
TEST_F(IntegrationTest, RecordPersistenceWithUtils) {
    std::string record_file = get_record_file_path();
    
    {
        // 创建记录并添加数据
        Recorder::Record record(record_file);
        std::vector<std::string> files = collect_package_files(test_package_dir_);
        record.addPackageRecord("test-lib", test_package_dir_, files);
    } // record析构，数据保存到文件
    
    {
        // 创建新的记录实例，从文件加载数据
        Recorder::Record record(record_file);
        
        // 验证数据已加载
        EXPECT_TRUE(record.isPackageInstalled("test-lib"));
        EXPECT_EQ(record.getPackageInstallPath("test-lib"), test_package_dir_);
        
        auto files = record.getPackageFiles("test-lib");
        EXPECT_GT(files.size(), 0);
    }
}

// 测试错误处理
TEST_F(IntegrationTest, ErrorHandling) {
    // 测试收集不存在的目录
    std::vector<std::string> files = collect_package_files("./nonexistent_directory");
    EXPECT_EQ(files.size(), 0);
    
    // 测试Record类处理不存在的包
    Recorder::Record record(test_record_file_);
    EXPECT_FALSE(record.isPackageInstalled("nonexistent"));
    EXPECT_EQ(record.getPackageFiles("nonexistent").size(), 0);
    EXPECT_EQ(record.getPackageInstallPath("nonexistent"), "");
}

// 测试多个包的集成
TEST_F(IntegrationTest, MultiplePackagesIntegration) {
    Recorder::Record record(test_record_file_);
    
    // 添加多个包
    std::vector<std::string> files1 = collect_package_files(test_package_dir_);
    record.addPackageRecord("test-lib-1", test_package_dir_ + "_1", files1);
    
    // 创建第二个测试目录
    std::string test_package_dir_2 = "./test_package_integration_2";
    std::filesystem::create_directories(test_package_dir_2);
    std::ofstream test_file(test_package_dir_2 + "/test.txt");
    test_file << "test content";
    test_file.close();
    
    std::vector<std::string> files2 = collect_package_files(test_package_dir_2);
    record.addPackageRecord("test-lib-2", test_package_dir_2, files2);
    
    // 验证两个包都存在
    EXPECT_TRUE(record.isPackageInstalled("test-lib-1"));
    EXPECT_TRUE(record.isPackageInstalled("test-lib-2"));
    
    auto all_packages = record.getAllPackages();
    EXPECT_EQ(all_packages.size(), 2);
    
    // 清理第二个测试目录
    std::filesystem::remove_all(test_package_dir_2);
}

// 测试文件路径处理
TEST_F(IntegrationTest, FilePathHandling) {
    Recorder::Record record(test_record_file_);
    
    // 测试相对路径和绝对路径
    std::string relative_path = "./relative_path";
    std::string absolute_path = std::filesystem::absolute(relative_path).string();
    
    record.addPackageRecord("test-pkg", relative_path, {"file1.txt", "file2.txt"});
    
    EXPECT_EQ(record.getPackageInstallPath("test-pkg"), relative_path);
    
    // 测试文件路径的存储和检索
    auto files = record.getPackageFiles("test-pkg");
    EXPECT_EQ(files.size(), 2);
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file1.txt") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file2.txt") != files.end());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 