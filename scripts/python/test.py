#!/usr/bin/env python3
"""
Paker 测试脚本 (Python版本)

提供与test.sh相同的功能，但使用Python实现
"""

import os
import sys
import subprocess
import multiprocessing
from pathlib import Path


def run_command(cmd, description, check=True, show_output=True):
    """运行命令并处理错误"""
    print(f"=== {description} ===")
    print(f"Running: {cmd}")
    
    try:
        result = subprocess.run(cmd, shell=True, check=check, 
                              capture_output=True, text=True)
        if show_output and result.stdout:
            print(result.stdout)
        if result.stderr:
            print(f"stderr: {result.stderr}")
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        if e.stdout:
            print(f"stdout: {e.stdout}")
        if e.stderr:
            print(f"stderr: {e.stderr}")
        return False


def check_project_root():
    """检查是否在项目根目录"""
    if not Path("CMakeLists.txt").exists():
        print("Error: Please run this script from the project root directory")
        print("Current directory:", os.getcwd())
        return False
    return True


def create_build_directory():
    """创建build目录"""
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    return build_dir


def configure_project(build_dir):
    """配置项目"""
    os.chdir(build_dir)
    return run_command("cmake ..", "Configuring project")


def build_project():
    """编译项目"""
    cpu_count = multiprocessing.cpu_count()
    return run_command(f"make -j{cpu_count}", "Building project")


def build_tests():
    """编译测试"""
    print("=== Building Tests ===")
    
    # 编译单元测试
    unit_success = run_command("make PakerUnitTests", "Building Unit Tests")
    
    # 编译集成测试
    integration_success = run_command("make PakerIntegrationTests", "Building Integration Tests")
    
    return unit_success and integration_success


def run_unit_tests():
    """运行单元测试"""
    print("\n=== Running Unit Tests ===")
    return run_command("./PakerUnitTests", "Running Unit Tests", show_output=True)


def run_integration_tests():
    """运行集成测试"""
    print("\n=== Running Integration Tests ===")
    return run_command("./PakerIntegrationTests", "Running Integration Tests", show_output=True)


def check_test_executables():
    """检查测试可执行文件是否存在"""
    unit_test = Path("PakerUnitTests")
    integration_test = Path("PakerIntegrationTests")
    
    if not unit_test.exists():
        print(f"Warning: {unit_test} not found")
        return False
    
    if not integration_test.exists():
        print(f"Warning: {integration_test} not found")
        return False
    
    return True


def main():
    """主函数"""
    print("=== Running Paker Tests (Python) ===")
    
    # 检查项目根目录
    if not check_project_root():
        sys.exit(1)
    
    # 创建build目录
    build_dir = create_build_directory()
    original_dir = os.getcwd()
    
    try:
        # 配置项目
        if not configure_project(build_dir):
            print("Configuration failed!")
            sys.exit(1)
        
        # 编译项目
        if not build_project():
            print("Build failed!")
            sys.exit(1)
        
        # 编译测试
        if not build_tests():
            print("Test build failed!")
            sys.exit(1)
        
        # 检查测试可执行文件
        if not check_test_executables():
            print("Test executables not found!")
            sys.exit(1)
        
        # 运行单元测试
        unit_success = run_unit_tests()
        
        # 运行集成测试
        integration_success = run_integration_tests()
        
        # 输出结果
        print("\n=== Test Results ===")
        print(f"Unit Tests: {'PASSED' if unit_success else 'FAILED'}")
        print(f"Integration Tests: {'PASSED' if integration_success else 'FAILED'}")
        
        if unit_success and integration_success:
            print("\n=== All tests completed successfully ===")
        else:
            print("\n=== Some tests failed ===")
            sys.exit(1)
        
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)
    finally:
        # 返回原目录
        os.chdir(original_dir)


if __name__ == "__main__":
    main() 