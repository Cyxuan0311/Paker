#!/usr/bin/env python3
"""
编译优化脚本
优化CMake配置，启用编译优化选项
"""

import os
import sys
from pathlib import Path

class BuildOptimizer:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.cmake_file = self.project_root / "CMakeLists.txt"
        
    def optimize_cmake(self):
        """优化CMake配置"""
        print("🔧 优化CMake配置...")
        
        if not self.cmake_file.exists():
            print("❌ 未找到CMakeLists.txt文件")
            return False
        
        # 读取现有配置
        with open(self.cmake_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 添加编译优化选项
        optimizations = [
            "# 编译优化选项",
            "set(CMAKE_CXX_FLAGS_RELEASE \"-O3 -DNDEBUG -march=native -mtune=native\")",
            "set(CMAKE_CXX_FLAGS_DEBUG \"-O0 -g -Wall -Wextra -Wpedantic\")",
            "",
            "# 启用链接时优化",
            "set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)",
            "",
            "# 启用预编译头文件",
            "target_precompile_headers(Paker PRIVATE include/Paker/pch.h)",
            "",
            "# 启用并行编译",
            "set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -j$(nproc)\")",
            "",
            "# 启用警告",
            "set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic\")",
            "",
            "# 启用调试信息",
            "set(CMAKE_CXX_FLAGS_DEBUG \"${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0\")",
        ]
        
        # 检查是否已经包含优化选项
        if "CMAKE_CXX_FLAGS_RELEASE" not in content:
            print("✅ 添加编译优化选项...")
            with open(self.cmake_file, 'a', encoding='utf-8') as f:
                f.write("\n" + "\n".join(optimizations) + "\n")
        else:
            print("ℹ️  编译优化选项已存在")
        
        return True
    
    def create_build_script(self):
        """创建优化构建脚本"""
        print("📝 创建优化构建脚本...")
        
        build_script = """#!/bin/bash
# 优化构建脚本

set -e

echo "🚀 开始优化构建..."

# 创建构建目录
mkdir -p build
cd build

# 配置项目
echo "📋 配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release \\
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \\
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# 并行编译
echo "🔨 开始编译..."
make -j$(nproc)

echo "✅ 构建完成!"
"""
        
        script_file = self.project_root / "scripts" / "build_optimized.sh"
        with open(script_file, 'w', encoding='utf-8') as f:
            f.write(build_script)
        
        # 设置执行权限
        os.chmod(script_file, 0o755)
        print(f"✅ 创建构建脚本: {script_file}")
    
    def create_performance_test(self):
        """创建性能测试脚本"""
        print("📊 创建性能测试脚本...")
        
        perf_script = """#!/bin/bash
# 性能测试脚本

set -e

echo "📈 开始性能测试..."

# 编译性能测试
echo "⏱️  测试编译性能..."
time make clean
time make -j$(nproc)

# 运行时性能测试
echo "🏃 测试运行时性能..."
./Paker --help > /dev/null 2>&1

# 内存使用测试
echo "💾 测试内存使用..."
valgrind --tool=massif --pages-as-heap=yes ./Paker --help > /dev/null 2>&1 || true

echo "✅ 性能测试完成!"
"""
        
        script_file = self.project_root / "scripts" / "performance_test.sh"
        with open(script_file, 'w', encoding='utf-8') as f:
            f.write(perf_script)
        
        # 设置执行权限
        os.chmod(script_file, 0o755)
        print(f"✅ 创建性能测试脚本: {script_file}")
    
    def run(self):
        """运行构建优化"""
        print("🚀 开始构建优化...")
        
        self.optimize_cmake()
        self.create_build_script()
        self.create_performance_test()
        
        print("\n✅ 构建优化完成!")
        print("\n💡 使用说明:")
        print("  ./scripts/build_optimized.sh  - 优化构建")
        print("  ./scripts/performance_test.sh - 性能测试")

def main():
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    else:
        project_root = "."
    
    optimizer = BuildOptimizer(project_root)
    optimizer.run()

if __name__ == "__main__":
    main()
