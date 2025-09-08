#!/usr/bin/env python3
"""
Paker 构建脚本 (Python版本)

提供与build.sh相同的功能，但使用Python实现
"""

import os
import sys
import subprocess
import multiprocessing
from pathlib import Path


def run_command(cmd, description, check=True):
    """运行命令并处理错误"""
    print(f"=== {description} ===")
    print(f"Running: {cmd}")
    
    try:
        result = subprocess.run(cmd, shell=True, check=check, 
                              capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
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
    # 获取CPU核心数
    cpu_count = multiprocessing.cpu_count()
    return run_command(f"make -j{cpu_count}", "Building project")


def main():
    """主函数"""
    print("=== Building Paker (Python) ===")
    
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
        
        print("=== Build completed successfully ===")
        print(f"Executable location: {build_dir}/Paker")
        
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)
    finally:
        # 返回原目录
        os.chdir(original_dir)


if __name__ == "__main__":
    main() 