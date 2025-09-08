#!/bin/bash

# 构建脚本入口
# 自动选择可用的脚本类型

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 检查Python脚本是否可用
if command -v python3 &> /dev/null && [ -f "$SCRIPT_DIR/python/build.py" ]; then
    echo "Using Python build script..."
    python3 "$SCRIPT_DIR/python/build.py"
elif [ -f "$SCRIPT_DIR/bash/build.sh" ]; then
    echo "Using Bash build script..."
    bash "$SCRIPT_DIR/bash/build.sh"
else
    echo "Error: No build script found"
    exit 1
fi 