#!/usr/bin/env python3
"""
Paker 清理脚本 (Python版本)

清理构建产物和临时文件
"""

import os
import sys
import shutil
import argparse
from pathlib import Path


def clean_build_directory():
    """清理build目录"""
    build_dir = Path("build")
    if build_dir.exists():
        print(f"Removing build directory: {build_dir}")
        shutil.rmtree(build_dir)
        return True
    else:
        print("Build directory not found")
        return False


def clean_packages_directory():
    """清理packages目录"""
    packages_dir = Path("packages")
    if packages_dir.exists():
        print(f"Removing packages directory: {packages_dir}")
        shutil.rmtree(packages_dir)
        return True
    else:
        print("Packages directory not found")
        return False


def clean_record_files():
    """清理记录文件"""
    record_files = list(Path(".").glob("*_install_record.json"))
    removed_count = 0
    
    for record_file in record_files:
        print(f"Removing record file: {record_file}")
        record_file.unlink()
        removed_count += 1
    
    if removed_count == 0:
        print("No record files found")
    
    return removed_count


def clean_test_files():
    """清理测试相关文件"""
    test_files = [
        Path("test_record.json"),
        Path("test_record_gtest.json"),
        Path("test_package_integration"),
        Path("test_package_integration_2")
    ]
    
    removed_count = 0
    for test_file in test_files:
        if test_file.exists():
            if test_file.is_dir():
                print(f"Removing test directory: {test_file}")
                shutil.rmtree(test_file)
            else:
                print(f"Removing test file: {test_file}")
                test_file.unlink()
            removed_count += 1
    
    if removed_count == 0:
        print("No test files found")
    
    return removed_count


def clean_all():
    """清理所有文件"""
    print("=== Cleaning all build artifacts ===")
    
    build_cleaned = clean_build_directory()
    packages_cleaned = clean_packages_directory()
    record_count = clean_record_files()
    test_count = clean_test_files()
    
    total_cleaned = sum([
        1 if build_cleaned else 0,
        1 if packages_cleaned else 0,
        record_count,
        test_count
    ])
    
    print(f"\n=== Clean completed ===")
    print(f"Total items cleaned: {total_cleaned}")
    
    return total_cleaned > 0


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="Paker Clean Script")
    parser.add_argument("--build", action="store_true", 
                       help="Clean build directory only")
    parser.add_argument("--packages", action="store_true", 
                       help="Clean packages directory only")
    parser.add_argument("--records", action="store_true", 
                       help="Clean record files only")
    parser.add_argument("--tests", action="store_true", 
                       help="Clean test files only")
    parser.add_argument("--all", action="store_true", 
                       help="Clean all (default)")
    
    args = parser.parse_args()
    
    # 检查项目根目录
    if not Path("CMakeLists.txt").exists():
        print("Error: Please run this script from the project root directory")
        sys.exit(1)
    
    print("=== Paker Clean Script (Python) ===")
    
    if args.build:
        clean_build_directory()
    elif args.packages:
        clean_packages_directory()
    elif args.records:
        clean_record_files()
    elif args.tests:
        clean_test_files()
    else:
        # 默认清理所有
        clean_all()


if __name__ == "__main__":
    main() 