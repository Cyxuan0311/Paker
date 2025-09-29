#!/bin/bash
# 优化构建脚本

set -e

echo "🚀 开始优化构建..."

# 创建构建目录
mkdir -p build
cd build

# 配置项目
echo "📋 配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# 并行编译
echo "🔨 开始编译..."
make -j$(nproc)

echo "✅ 构建完成!"
