#!/bin/bash
# Paker 补全配置文件
# 支持静默加载、调试模式、性能优化

# 补全配置
export PAKER_COMPLETION_DEBUG="${PAKER_COMPLETION_DEBUG:-false}"
export PAKER_SMART_SUGGESTIONS="${PAKER_SMART_SUGGESTIONS:-true}"
export PAKER_ERROR_HANDLING="${PAKER_ERROR_HANDLING:-true}"
export PAKER_CACHE_OPTIMIZATION="${PAKER_CACHE_OPTIMIZATION:-true}"

# 性能优化配置
export PAKER_COMPLETION_CACHE_TTL="${PAKER_COMPLETION_CACHE_TTL:-3600}"  # 1小时
export PAKER_COMPLETION_MAX_SUGGESTIONS="${PAKER_COMPLETION_MAX_SUGGESTIONS:-20}"
export PAKER_COMPLETION_TIMEOUT="${PAKER_COMPLETION_TIMEOUT:-5}"

# 静默加载函数
load_completion_silently() {
    local completion_file="$1"
    local shell_type="$2"
    
    if [[ -f "$completion_file" ]]; then
        if [[ "$PAKER_COMPLETION_DEBUG" == "true" ]]; then
            echo "🔄 加载补全: $completion_file"
            source "$completion_file"
            echo "✅ 补全加载完成"
        else
            # 静默加载
            source "$completion_file" 2>/dev/null || true
        fi
    fi
}

# 智能补全加载
load_paker_completion() {
    local shell_type=$(basename "$SHELL")
    
    case "$shell_type" in
        "bash")
            local completion_file="$HOME/.local/share/bash-completion/completions/paker"
            load_completion_silently "$completion_file" "bash"
            ;;
        "zsh")
            local completion_file="$HOME/.zsh/completions/_paker"
            load_completion_silently "$completion_file" "zsh"
            ;;
    esac
}

# 性能优化
optimize_completion_performance() {
    if [[ "$PAKER_CACHE_OPTIMIZATION" == "true" ]]; then
        # 启用补全缓存
        if command -v paker-completion &> /dev/null; then
            # 静默后台更新缓存，不显示任何输出
            (paker-completion --update-cache --silent >/dev/null 2>&1 &) 2>/dev/null || true
        fi
    fi
}

# 错误处理
handle_completion_error() {
    local error_code="$1"
    local error_message="$2"
    
    if [[ "$PAKER_ERROR_HANDLING" == "true" ]]; then
        case "$error_code" in
            "load_failed")
                if [[ "$PAKER_COMPLETION_DEBUG" == "true" ]]; then
                    echo "❌ 补全加载失败: $error_message"
                fi
                ;;
            "cache_failed")
                if [[ "$PAKER_COMPLETION_DEBUG" == "true" ]]; then
                    echo "⚠️  补全缓存更新失败: $error_message"
                fi
                ;;
        esac
    fi
}

# 自动加载补全
auto_load_completion() {
    # 检查是否在Paker项目中
    if [[ -f "Paker.json" ]] || [[ -f ".paker/config.json" ]]; then
        load_paker_completion
        optimize_completion_performance
    fi
}

# 调试模式切换
toggle_debug_mode() {
    if [[ "$PAKER_COMPLETION_DEBUG" == "true" ]]; then
        export PAKER_COMPLETION_DEBUG="false"
        echo "🔇 补全调试模式已关闭"
    else
        export PAKER_COMPLETION_DEBUG="true"
        echo "🔊 补全调试模式已开启"
    fi
}

# 补全状态检查
check_completion_status() {
    local shell_type=$(basename "$SHELL")
    local status="❌ 未加载"
    
    case "$shell_type" in
        "bash")
            if bash -c "source ~/.local/share/bash-completion/completions/paker 2>/dev/null && complete -p Paker" 2>/dev/null; then
                status="✅ 已加载"
            fi
            ;;
        "zsh")
            if zsh -c "autoload -U compinit && compinit && which _paker" 2>/dev/null; then
                status="✅ 已加载"
            fi
            ;;
    esac
    
    echo "📊 补全状态: $status"
    echo "🐚 Shell类型: $shell_type"
    echo "🔧 调试模式: $PAKER_COMPLETION_DEBUG"
    echo "🧠 智能建议: $PAKER_SMART_SUGGESTIONS"
    echo "🛠️  错误处理: $PAKER_ERROR_HANDLING"
    echo "⚡ 缓存优化: $PAKER_CACHE_OPTIMIZATION"
}

# 补全重置
reset_completion() {
    echo "🔄 重置补全配置..."
    
    # 清除缓存
    rm -f "$HOME/.paker/completion_cache.json" 2>/dev/null || true
    
    # 重新加载补全
    load_paker_completion
    
    echo "✅ 补全配置已重置"
}

# 帮助信息
show_completion_help() {
    echo "🔧 Paker 补全管理命令:"
    echo "  paker-completion-config --status     # 查看补全状态"
    echo "  paker-completion-config --debug      # 切换调试模式"
    echo "  paker-completion-config --reset     # 重置补全配置"
    echo "  paker-completion-config --help      # 显示帮助信息"
    echo ""
    echo "🔧 环境变量配置:"
    echo "  export PAKER_COMPLETION_DEBUG=true   # 启用调试模式"
    echo "  export PAKER_SMART_SUGGESTIONS=false # 禁用智能建议"
    echo "  export PAKER_ERROR_HANDLING=false   # 禁用错误处理"
    echo "  export PAKER_CACHE_OPTIMIZATION=false # 禁用缓存优化"
}

# 主函数
main() {
    case "${1:-}" in
        "--status")
            check_completion_status
            ;;
        "--debug")
            toggle_debug_mode
            ;;
        "--reset")
            reset_completion
            ;;
        "--help")
            show_completion_help
            ;;
        *)
            # 默认行为：静默加载补全
            load_paker_completion
            optimize_completion_performance
            ;;
    esac
}

# 如果直接运行此脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
else
    # 如果被source，则自动加载补全
    auto_load_completion
fi
