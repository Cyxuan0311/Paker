#!/bin/bash

# 清理脚本入口
# 自动选择可用的脚本类型

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 检查Python脚本是否可用
if command -v python3 &> /dev/null && [ -f "$SCRIPT_DIR/python/clean.py" ]; then
    echo "Using Python clean script..."
    python3 "$SCRIPT_DIR/python/clean.py" "$@"
elif [ -f "$SCRIPT_DIR/bash/clean.sh" ]; then
    echo "Using Bash clean script..."
    bash "$SCRIPT_DIR/bash/clean.sh" "$@"
else
    echo "Error: No clean script found"
    exit 1
fi 