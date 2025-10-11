#!/bin/bash
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
