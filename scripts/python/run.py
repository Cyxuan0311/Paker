#!/usr/bin/env python3
"""
Paker 脚本管理器

提供统一的接口来运行构建和测试脚本，支持bash和python两种实现
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path


def check_python_version():
    """检查Python版本"""
    if sys.version_info < (3, 6):
        print("Error: Python 3.6 or higher is required")
        sys.exit(1)


def check_requirements():
    """检查系统要求"""
    # 检查cmake
    try:
        subprocess.run(["cmake", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: cmake is not installed or not in PATH")
        sys.exit(1)
    
    # 检查make
    try:
        subprocess.run(["make", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: make is not installed or not in PATH")
        sys.exit(1)


def run_script(script_path, script_type):
    """运行脚本"""
    if not script_path.exists():
        print(f"Error: {script_path} not found")
        return False
    
    try:
        if script_type == "python":
            # 运行Python脚本
            result = subprocess.run([sys.executable, str(script_path)], check=True)
        else:
            # 运行bash脚本
            result = subprocess.run(["bash", str(script_path)], check=True)
        
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"Script execution failed with exit code: {e.returncode}")
        return False
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="Paker Script Manager")
    parser.add_argument("action", choices=["build", "test"], 
                       help="Action to perform: build or test")
    parser.add_argument("--type", choices=["bash", "python"], default="python",
                       help="Script type to use (default: python)")
    parser.add_argument("--check-requirements", action="store_true",
                       help="Check system requirements before running")
    
    args = parser.parse_args()
    
    # 检查Python版本
    check_python_version()
    
    # 检查系统要求
    if args.check_requirements:
        check_requirements()
    
    # 检查项目根目录
    if not Path("CMakeLists.txt").exists():
        print("Error: Please run this script from the project root directory")
        sys.exit(1)
    
    # 确定脚本路径
    scripts_dir = Path(__file__).parent
    if args.action == "build":
        if args.type == "python":
            script_path = scripts_dir / "build.py"
        else:
            script_path = scripts_dir.parent / "bash" / "build.sh"
    else:  # test
        if args.type == "python":
            script_path = scripts_dir / "test.py"
        else:
            script_path = scripts_dir.parent / "bash" / "test.sh"
    
    # 运行脚本
    print(f"=== Running {args.action} with {args.type} script ===")
    success = run_script(script_path, args.type)
    
    if success:
        print(f"\n=== {args.action.capitalize()} completed successfully ===")
    else:
        print(f"\n=== {args.action.capitalize()} failed ===")
        sys.exit(1)


if __name__ == "__main__":
    main() 