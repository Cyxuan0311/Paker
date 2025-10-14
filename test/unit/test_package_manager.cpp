#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdio>
extern "C" {
#include "Paker/package_manager.h"
}

namespace fs = std::filesystem;

class PakerTest : public ::testing::Test {
protected:
    std::string test_dir = "test_tmp";
    void SetUp() override {
        fs::remove_all(test_dir);
        fs::create_directory(test_dir);
        fs::current_path(test_dir);
        pm_init();
    }
    void TearDown() override {
        fs::current_path("..");
        fs::remove_all(test_dir);
    }
};

TEST_F(PakerTest, InitCreatesJson) {
    ASSERT_TRUE(fs::exists("test_tmp.json"));
}

TEST_F(PakerTest, AddDependency) {
    pm_add("fmt");
    std::ifstream ifs("test_tmp.json");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_NE(content.find("fmt"), std::string::npos);
}

TEST_F(PakerTest, RemoveDependency) {
    pm_add("fmt");
    pm_remove("fmt");
    std::ifstream ifs("test_tmp.json");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_EQ(content.find("fmt"), std::string::npos);
}

TEST_F(PakerTest, ListDependency) {
    pm_add("fmt");
    testing::internal::CaptureStdout();
    pm_list();
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_NE(output.find("fmt"), std::string::npos);
}

TEST_F(PakerTest, LockFileGeneration) {
    pm_add("fmt");
    pm_lock();
    ASSERT_TRUE(fs::exists(".paker/lock/Paker.lock"));
    std::ifstream ifs(".paker/lock/Paker.lock");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_NE(content.find("fmt"), std::string::npos);
}

TEST_F(PakerTest, UpgradeDependency) {
    pm_add("fmt");
    pm_upgrade("fmt"); // 只要不报错即可
    std::ifstream ifs("test_tmp.json");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_NE(content.find("fmt"), std::string::npos);
}
// 可继续添加pm_list、pm_lock等更多测试 