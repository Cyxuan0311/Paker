#!/bin/bash

# Paker 构建脚本

set -e

echo "=== Building Paker ==="

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# 创建build目录
mkdir -p build
cd build

# 配置项目
echo "Configuring project..."
cmake ..

# 编译项目
echo "Building project..."
make -j$(nproc)

echo "=== Build completed successfully ==="
echo "Executable location: build/Paker" 