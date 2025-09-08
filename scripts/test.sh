#!/bin/bash

# 测试脚本入口
# 自动选择可用的脚本类型

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 检查Python脚本是否可用
if command -v python3 &> /dev/null && [ -f "$SCRIPT_DIR/python/test.py" ]; then
    echo "Using Python test script..."
    python3 "$SCRIPT_DIR/python/test.py"
elif [ -f "$SCRIPT_DIR/bash/test.sh" ]; then
    echo "Using Bash test script..."
    bash "$SCRIPT_DIR/bash/test.sh"
else
    echo "Error: No test script found"
    exit 1
fi 