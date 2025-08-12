#!/bin/bash

# Paker 测试脚本

set -e

echo "=== Running Paker Tests ==="

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# 创建build目录并编译
mkdir -p build
cd build

# 配置项目
echo "Configuring project..."
cmake ..

# 编译项目
echo "Building project..."
make -j$(nproc)

# 编译测试
echo "Building tests..."
make PakerUnitTests
make PakerIntegrationTests

# 运行单元测试
echo ""
echo "=== Running Unit Tests ==="
./PakerUnitTests

# 运行集成测试
echo ""
echo "=== Running Integration Tests ==="
./PakerIntegrationTests

echo ""
echo "=== All tests completed successfully ===" 