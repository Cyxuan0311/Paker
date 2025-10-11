#!/bin/bash
# Paker 智能补全安装脚本
# 支持 Bash 和 Zsh 自动安装和配置

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 打印函数
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_header() {
    echo -e "${PURPLE}🚀 $1${NC}"
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查Python3
    if ! command -v python3 &> /dev/null; then
        print_error "Python3 未安装，请先安装 Python3"
        exit 1
    fi
    
    # 检查Paker命令
    if ! command -v Paker &> /dev/null; then
        print_warning "Paker 命令未找到，请确保 Paker 已正确安装"
    fi
    
    print_success "依赖检查完成"
}

# 检测Shell类型
detect_shell() {
    local shell_name=$(basename "$SHELL")
    case "$shell_name" in
        "bash")
            echo "bash"
            ;;
        "zsh")
            echo "zsh"
            ;;
        *)
            print_warning "未识别的Shell: $shell_name，将尝试自动检测"
            if [[ "$SHELL" == *"zsh"* ]]; then
                echo "zsh"
            else
                echo "bash"
            fi
            ;;
    esac
}

# 获取Shell配置文件路径
get_shell_config() {
    local shell_type="$1"
    case "$shell_type" in
        "bash")
            if [[ -f "$HOME/.bashrc" ]]; then
                echo "$HOME/.bashrc"
            elif [[ -f "$HOME/.bash_profile" ]]; then
                echo "$HOME/.bash_profile"
            else
                echo "$HOME/.bashrc"
            fi
            ;;
        "zsh")
            if [[ -f "$HOME/.zshrc" ]]; then
                echo "$HOME/.zshrc"
            else
                echo "$HOME/.zshrc"
            fi
            ;;
    esac
}

# 安装Bash补全
install_bash_completion() {
    print_info "安装 Bash 补全..."
    
    local completion_dir="$HOME/.local/share/bash-completion/completions"
    local completion_file="$completion_dir/paker"
    
    # 创建目录
    mkdir -p "$completion_dir"
    
    # 复制补全脚本
    cp "$(dirname "$0")/paker-completion.bash" "$completion_file"
    chmod +x "$completion_file"
    
    # 添加到配置文件
    local config_file=$(get_shell_config "bash")
    if ! grep -q "paker-completion" "$config_file" 2>/dev/null; then
        echo "" >> "$config_file"
        echo "# Paker 智能补全" >> "$config_file"
        echo "if [[ -f '$completion_file' ]]; then" >> "$config_file"
        echo "    source '$completion_file'" >> "$config_file"
        echo "fi" >> "$config_file"
        echo "# 补全配置管理" >> "$config_file"
        echo "if [[ -f '$(dirname "$0")/completion-config.sh' ]]; then" >> "$config_file"
        echo "    source '$(dirname "$0")/completion-config.sh'" >> "$config_file"
        echo "fi" >> "$config_file"
    fi
    
    print_success "Bash 补全安装完成"
}

# 安装Zsh补全
install_zsh_completion() {
    print_info "安装 Zsh 补全..."
    
    local completion_dir="$HOME/.zsh/completions"
    local completion_file="$completion_dir/_paker"
    
    # 创建目录
    mkdir -p "$completion_dir"
    
    # 复制补全脚本
    cp "$(dirname "$0")/_paker" "$completion_file"
    chmod +x "$completion_file"
    
    # 添加到配置文件
    local config_file=$(get_shell_config "zsh")
    if ! grep -q "_paker" "$config_file" 2>/dev/null; then
        echo "" >> "$config_file"
        echo "# Paker 智能补全" >> "$config_file"
        echo "fpath=(\$HOME/.zsh/completions \$fpath)" >> "$config_file"
        echo "autoload -U compinit && compinit" >> "$config_file"
        echo "# 补全配置管理" >> "$config_file"
        echo "if [[ -f '$(dirname "$0")/completion-config.sh' ]]; then" >> "$config_file"
        echo "    source '$(dirname "$0")/completion-config.sh'" >> "$config_file"
        echo "fi" >> "$config_file"
    fi
    
    print_success "Zsh 补全安装完成"
}

# 安装智能补全逻辑
install_smart_completion() {
    print_info "安装智能补全逻辑..."
    
    local script_dir="$HOME/.local/bin"
    local script_file="$script_dir/paker-completion"
    
    # 创建目录
    mkdir -p "$script_dir"
    
    # 复制智能补全脚本
    cp "$(dirname "$0")/smart-completion.py" "$script_file"
    chmod +x "$script_file"
    
    # 添加到PATH
    if [[ ":$PATH:" != *":$script_dir:"* ]]; then
        local shell_type=$(detect_shell)
        local config_file=$(get_shell_config "$shell_type")
        if ! grep -q "$script_dir" "$config_file" 2>/dev/null; then
            echo "" >> "$config_file"
            echo "# 添加本地bin目录到PATH" >> "$config_file"
            echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "$config_file"
        fi
    fi
    
    print_success "智能补全逻辑安装完成"
}

# 创建补全缓存目录
create_cache_directory() {
    print_info "创建补全缓存目录..."
    
    local cache_dir="$HOME/.paker"
    mkdir -p "$cache_dir"
    
    # 创建初始缓存文件
    local cache_file="$cache_dir/completion_cache.json"
    if [[ ! -f "$cache_file" ]]; then
        cat > "$cache_file" << EOF
{
  "packages": [],
  "remotes": [],
  "installed_packages": [],
  "cached_packages": [],
  "last_updated": 0
}
EOF
    fi
    
    print_success "补全缓存目录创建完成"
}

# 测试补全安装
test_completion() {
    print_info "测试补全安装..."
    
    local shell_type=$(detect_shell)
    
    if [[ "$shell_type" == "bash" ]]; then
        if bash -c "source ~/.local/share/bash-completion/completions/paker 2>/dev/null && complete -p Paker" 2>/dev/null; then
            print_success "Bash 补全测试通过"
        else
            print_warning "Bash 补全测试失败，请检查安装"
        fi
    elif [[ "$shell_type" == "zsh" ]]; then
        if zsh -c "autoload -U compinit && compinit && which _paker" 2>/dev/null; then
            print_success "Zsh 补全测试通过"
        else
            print_warning "Zsh 补全测试失败，请检查安装"
        fi
    fi
}

# 显示使用说明
show_usage() {
    print_header "Paker 智能补全安装完成！"
    
    echo ""
    echo -e "${CYAN}📖 使用说明:${NC}"
    echo "1. 重新启动终端或运行: source ~/.$(detect_shell)rc"
    echo "2. 使用 'Paker <TAB>' 开始智能补全"
    echo "3. 使用 'Paker --help' 查看所有命令"
    echo ""
    
    echo -e "${CYAN}🔧 高级功能:${NC}"
    echo "• 智能建议: 根据上下文提供相关建议"
    echo "• 错误处理: 自动检测错误并提供解决方案"
    echo "• 缓存优化: 自动缓存常用包和命令"
    echo "• 上下文感知: 根据项目状态提供不同建议"
    echo ""
    
    echo -e "${CYAN}🛠️  管理命令:${NC}"
    echo "• 查看状态: paker-completion-config --status"
    echo "• 调试模式: paker-completion-config --debug"
    echo "• 重置配置: paker-completion-config --reset"
    echo "• 更新缓存: paker-completion --update-cache"
    echo "• 获取建议: paker-completion --suggest <command>"
    echo "• 获取提示: paker-completion --tips <command>"
    echo "• 错误处理: paker-completion --error <error_type>"
    echo ""
    
    echo -e "${CYAN}📁 文件位置:${NC}"
    echo "• Bash补全: ~/.local/share/bash-completion/completions/paker"
    echo "• Zsh补全: ~/.zsh/completions/_paker"
    echo "• 智能逻辑: ~/.local/bin/paker-completion"
    echo "• 缓存文件: ~/.paker/completion_cache.json"
    echo ""
    
    print_success "安装完成！请重新启动终端以使用智能补全功能。"
}

# 卸载补全
uninstall_completion() {
    print_info "卸载 Paker 补全..."
    
    # 删除补全文件
    rm -f "$HOME/.local/share/bash-completion/completions/paker"
    rm -f "$HOME/.zsh/completions/_paker"
    rm -f "$HOME/.local/bin/paker-completion"
    rm -f "$HOME/.paker/completion_cache.json"
    
    # 从配置文件中移除相关行
    local shell_type=$(detect_shell)
    local config_file=$(get_shell_config "$shell_type")
    
    if [[ -f "$config_file" ]]; then
        # 创建临时文件
        local temp_file=$(mktemp)
        
        # 过滤掉Paker相关行
        grep -v "paker-completion\|_paker\|Paker 智能补全" "$config_file" > "$temp_file"
        
        # 替换原文件
        mv "$temp_file" "$config_file"
    fi
    
    print_success "Paker 补全已卸载"
    
    # 清理环境变量
    unset PAKER_COMPLETION_DEBUG
    unset PAKER_SMART_SUGGESTIONS
    unset PAKER_ERROR_HANDLING
    unset PAKER_CACHE_OPTIMIZATION
}

# 主函数
main() {
    print_header "Paker 智能补全安装程序"
    
    # 检查参数
    if [[ "$1" == "--uninstall" ]]; then
        uninstall_completion
        exit 0
    fi
    
    # 检查依赖
    check_dependencies
    
    # 检测Shell类型
    local shell_type=$(detect_shell)
    print_info "检测到Shell类型: $shell_type"
    
    # 安装补全
    if [[ "$shell_type" == "bash" ]]; then
        install_bash_completion
    elif [[ "$shell_type" == "zsh" ]]; then
        install_zsh_completion
    fi
    
    # 安装智能补全逻辑
    install_smart_completion
    
    # 创建缓存目录
    create_cache_directory
    
    # 测试安装
    test_completion
    
    # 显示使用说明
    show_usage
}

# 运行主函数
main "$@"
