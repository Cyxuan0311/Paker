#!/bin/bash

# 统一脚本管理器入口
# 自动选择可用的脚本类型

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 检查Python脚本是否可用
if command -v python3 &> /dev/null && [ -f "$SCRIPT_DIR/python/run.py" ]; then
    echo "Using Python script manager..."
    python3 "$SCRIPT_DIR/python/run.py" "$@"
else
    echo "Error: Python script manager not available"
    echo "Please ensure python3 is installed and run.py exists"
    exit 1
fi 