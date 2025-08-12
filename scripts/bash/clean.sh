#!/bin/bash

# Paker 清理脚本 (Bash版本)

set -e

echo "=== Paker Clean Script (Bash) ==="

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# 清理build目录
clean_build() {
    if [ -d "build" ]; then
        echo "Removing build directory: build"
        rm -rf build
    else
        echo "Build directory not found"
    fi
}

# 清理packages目录
clean_packages() {
    if [ -d "packages" ]; then
        echo "Removing packages directory: packages"
        rm -rf packages
    else
        echo "Packages directory not found"
    fi
}

# 清理记录文件
clean_records() {
    local count=0
    for file in *_install_record.json; do
        if [ -f "$file" ]; then
            echo "Removing record file: $file"
            rm -f "$file"
            ((count++))
        fi
    done
    
    if [ $count -eq 0 ]; then
        echo "No record files found"
    fi
}

# 清理测试文件
clean_tests() {
    local count=0
    
    # 清理测试文件
    local test_files=("test_record.json" "test_record_gtest.json")
    for file in "${test_files[@]}"; do
        if [ -f "$file" ]; then
            echo "Removing test file: $file"
            rm -f "$file"
            ((count++))
        fi
    done
    
    # 清理测试目录
    local test_dirs=("test_package_integration" "test_package_integration_2")
    for dir in "${test_dirs[@]}"; do
        if [ -d "$dir" ]; then
            echo "Removing test directory: $dir"
            rm -rf "$dir"
            ((count++))
        fi
    done
    
    if [ $count -eq 0 ]; then
        echo "No test files found"
    fi
}

# 清理所有
clean_all() {
    echo "=== Cleaning all build artifacts ==="
    clean_build
    clean_packages
    clean_records
    clean_tests
    echo "=== Clean completed ==="
}

# 解析命令行参数
case "${1:-all}" in
    "build")
        clean_build
        ;;
    "packages")
        clean_packages
        ;;
    "records")
        clean_records
        ;;
    "tests")
        clean_tests
        ;;
    "all")
        clean_all
        ;;
    *)
        echo "Usage: $0 [build|packages|records|tests|all]"
        echo "  build    - Clean build directory only"
        echo "  packages - Clean packages directory only"
        echo "  records  - Clean record files only"
        echo "  tests    - Clean test files only"
        echo "  all      - Clean all (default)"
        exit 1
        ;;
esac 