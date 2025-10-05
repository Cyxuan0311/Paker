#!/bin/bash
# Paker Bash 智能补全脚本
# 支持动态补全、上下文感知、智能建议

# 补全函数
_paker_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # 获取当前命令上下文
    local cmd_context=""
    local subcmd=""
    
    # 解析命令上下文
    for ((i=1; i<COMP_CWORD; i++)); do
        if [[ "${COMP_WORDS[i]}" =~ ^(add|remove|list|tree|search|info|update|upgrade|lock|install-l|resolve|check|fix|validate|perf|analyze|diagnose|monitor-enable|monitor-clear|cache|rollback|history|record|parse|io|warmup|remote-add|remote-rm|version|remove-project)$ ]]; then
            subcmd="${COMP_WORDS[i]}"
            break
        fi
    done
    
    # 主命令补全
    if [[ $COMP_CWORD -eq 1 ]]; then
        opts="init add remove list tree search info update upgrade lock install-l resolve check fix validate perf analyze diagnose monitor-enable monitor-clear cache rollback history record parse io warmup remote-add remote-rm version remove-project suggestion --help --version --no-color --dev"
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
    
    # 子命令补全
    case "${subcmd}" in
        "add")
            _paker_add_completion
            ;;
        "remove")
            _paker_remove_completion
            ;;
        "cache")
            _paker_cache_completion
            ;;
        "rollback")
            _paker_rollback_completion
            ;;
        "history")
            _paker_history_completion
            ;;
        "record")
            _paker_record_completion
            ;;
        "parse")
            _paker_parse_completion
            ;;
        "io")
            _paker_io_completion
            ;;
        "remote-add")
            _paker_remote_add_completion
            ;;
        "remote-rm")
            _paker_remote_rm_completion
            ;;
        "suggestion")
            _paker_suggestion_completion
            ;;
        *)
            _paker_general_completion
            ;;
    esac
}

# 添加包补全
_paker_add_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "add")
            # 从缓存中获取可用包列表
            local packages
            packages=$(Paker cache list --names-only 2>/dev/null | head -20)
            if [[ -n "$packages" ]]; then
                COMPREPLY=( $(compgen -W "${packages}" -- ${cur}) )
            else
                # 提供常用包建议
                local common_packages="fmt spdlog nlohmann-json boost catch2 gtest benchmark"
                COMPREPLY=( $(compgen -W "${common_packages}" -- ${cur}) )
            fi
            ;;
        *)
            # 检查是否是URL
            if [[ "$cur" =~ ^(http://|https://|git@|git://) ]]; then
                # URL补全 - 提供常用Git托管服务
                local git_services="github.com gitlab.com bitbucket.org"
                COMPREPLY=( $(compgen -W "${git_services}" -- ${cur}) )
            else
                # 包名补全
                local packages
                packages=$(Paker cache list --names-only 2>/dev/null | head -20)
                if [[ -n "$packages" ]]; then
                    COMPREPLY=( $(compgen -W "${packages}" -- ${cur}) )
                fi
            fi
            ;;
    esac
}

# 移除包补全
_paker_remove_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    
    # 获取当前项目已安装的包
    local installed_packages
    installed_packages=$(Paker list --names-only 2>/dev/null)
    if [[ -n "$installed_packages" ]]; then
        COMPREPLY=( $(compgen -W "${installed_packages}" -- ${cur}) )
    fi
}

# 缓存管理补全
_paker_cache_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "cache")
            local cache_commands="add remove status clean lru"
            COMPREPLY=( $(compgen -W "${cache_commands}" -- ${cur}) )
            ;;
        "add"|"remove")
            # 获取缓存中的包列表
            local cached_packages
            cached_packages=$(Paker cache list --names-only 2>/dev/null)
            if [[ -n "$cached_packages" ]]; then
                COMPREPLY=( $(compgen -W "${cached_packages}" -- ${cur}) )
            fi
            ;;
        "status")
            local status_flags="--detailed"
            COMPREPLY=( $(compgen -W "${status_flags}" -- ${cur}) )
            ;;
        "clean")
            local clean_flags="--smart --force"
            COMPREPLY=( $(compgen -W "${clean_flags}" -- ${cur}) )
            ;;
        "lru")
            local lru_flags="--stats --status"
            COMPREPLY=( $(compgen -W "${lru_flags}" -- ${cur}) )
            ;;
    esac
}

# 回滚补全
_paker_rollback_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "rollback")
            local rollback_flags="--previous --timestamp --force --list --check --stats"
            COMPREPLY=( $(compgen -W "${rollback_flags}" -- ${cur}) )
            ;;
        "--list"|"--check")
            # 获取可回滚的包列表
            local rollbackable_packages
            rollbackable_packages=$(Paker list --names-only 2>/dev/null)
            if [[ -n "$rollbackable_packages" ]]; then
                COMPREPLY=( $(compgen -W "${rollbackable_packages}" -- ${cur}) )
            fi
            ;;
        *)
            # 包名补全
            local installed_packages
            installed_packages=$(Paker list --names-only 2>/dev/null)
            if [[ -n "$installed_packages" ]]; then
                COMPREPLY=( $(compgen -W "${installed_packages}" -- ${cur}) )
            fi
            ;;
    esac
}

# 历史管理补全
_paker_history_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "history")
            local history_flags="--clean --export --import --max-entries"
            COMPREPLY=( $(compgen -W "${history_flags}" -- ${cur}) )
            ;;
        "--export"|"--import")
            # 文件路径补全
            COMPREPLY=( $(compgen -f -- ${cur}) )
            ;;
        "--max-entries")
            # 数字补全
            local numbers="10 20 50 100 200"
            COMPREPLY=( $(compgen -W "${numbers}" -- ${cur}) )
            ;;
        *)
            # 包名补全
            local installed_packages
            installed_packages=$(Paker list --names-only 2>/dev/null)
            if [[ -n "$installed_packages" ]]; then
                COMPREPLY=( $(compgen -W "${installed_packages}" -- ${cur}) )
            fi
            ;;
    esac
}

# 记录管理补全
_paker_record_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "record")
            local record_flags="--list --files"
            COMPREPLY=( $(compgen -W "${record_flags}" -- ${cur}) )
            ;;
        "--files")
            # 获取已安装包列表
            local installed_packages
            installed_packages=$(Paker list --names-only 2>/dev/null)
            if [[ -n "$installed_packages" ]]; then
                COMPREPLY=( $(compgen -W "${installed_packages}" -- ${cur}) )
            fi
            ;;
        *)
            # 包名补全
            local installed_packages
            installed_packages=$(Paker list --names-only 2>/dev/null)
            if [[ -n "$installed_packages" ]]; then
                COMPREPLY=( $(compgen -W "${installed_packages}" -- ${cur}) )
            fi
            ;;
    esac
}

# 解析管理补全
_paker_parse_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    
    local parse_flags="--stats --config --clear --opt --validate"
    COMPREPLY=( $(compgen -W "${parse_flags}" -- ${cur}) )
}

# I/O管理补全
_paker_io_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    
    local io_flags="--stats --config --test --bench --opt"
    COMPREPLY=( $(compgen -W "${io_flags}" -- ${cur}) )
}

# 远程源添加补全
_paker_remote_add_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "remote-add")
            # 提供常用远程源名称
            local common_remotes="github gitlab bitbucket custom"
            COMPREPLY=( $(compgen -W "${common_remotes}" -- ${cur}) )
            ;;
        *)
            # URL补全
            if [[ "$cur" =~ ^(http://|https://|git@|git://) ]]; then
                local git_services="github.com gitlab.com bitbucket.org"
                COMPREPLY=( $(compgen -W "${git_services}" -- ${cur}) )
            fi
            ;;
    esac
}

# 远程源移除补全
_paker_remote_rm_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    
    # 获取已配置的远程源
    local configured_remotes
    configured_remotes=$(Paker remote-list --names-only 2>/dev/null)
    if [[ -n "$configured_remotes" ]]; then
        COMPREPLY=( $(compgen -W "${configured_remotes}" -- ${cur}) )
    else
        # 提供常用远程源名称
        local common_remotes="github gitlab bitbucket"
        COMPREPLY=( $(compgen -W "${common_remotes}" -- ${cur}) )
    fi
}

# 智能推荐补全
_paker_suggestion_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    case "${prev}" in
        "--category")
            local categories="web desktop embedded game scientific machine_learning"
            COMPREPLY=( $(compgen -W "${categories}" -- ${cur}) )
            ;;
        "--performance")
            local performance_levels="low medium high"
            COMPREPLY=( $(compgen -W "${performance_levels}" -- ${cur}) )
            ;;
        "--security")
            local security_levels="low medium high"
            COMPREPLY=( $(compgen -W "${security_levels}" -- ${cur}) )
            ;;
        "--export")
            # 文件路径补全
            COMPREPLY=( $(compgen -f -- ${cur}) )
            ;;
        "suggestion")
            local suggestion_flags="--category --performance --security --detailed --auto-install --export"
            COMPREPLY=( $(compgen -W "${suggestion_flags}" -- ${cur}) )
            ;;
        *)
            local suggestion_flags="--category --performance --security --detailed --auto-install --export"
            COMPREPLY=( $(compgen -W "${suggestion_flags}" -- ${cur}) )
            ;;
    esac
}

# 通用补全
_paker_general_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    
    # 根据当前目录上下文提供建议
    if [[ -f "Paker.json" ]]; then
        # 在Paker项目中，提供项目相关补全
        local project_commands="add remove list tree resolve check fix validate suggestion"
        COMPREPLY=( $(compgen -W "${project_commands}" -- ${cur}) )
    else
        # 不在项目中，提供初始化相关补全
        local init_commands="init --help --version"
        COMPREPLY=( $(compgen -W "${init_commands}" -- ${cur}) )
    fi
}

# 注册补全函数
complete -F _paker_completion Paker
complete -F _paker_completion ./Paker
complete -F _paker_completion paker

# 智能建议功能
_paker_smart_suggestions() {
    local cmd="$1"
    local context="$2"
    
    case "$cmd" in
        "add")
            echo "💡 提示: 使用 'Paker add <package>' 添加依赖包"
            echo "   示例: Paker add fmt spdlog nlohmann-json"
            ;;
        "cache")
            echo "💡 提示: 使用 'Paker cache status' 查看缓存状态"
            echo "   使用 'Paker cache clean --smart' 智能清理缓存"
            ;;
        "rollback")
            echo "💡 提示: 使用 'Paker rollback --list <package>' 查看可回滚版本"
            echo "   使用 'Paker rollback <package> <version>' 回滚到指定版本"
            ;;
        "perf")
            echo "💡 提示: 使用 'Paker perf' 生成性能报告"
            echo "   使用 'Paker analyze' 分析依赖结构"
            ;;
    esac
}

# 错误处理和建议
_paker_error_handling() {
    local error_code="$1"
    local command="$2"
    
    case "$error_code" in
        "command_not_found")
            echo "❌ 命令未找到: $command"
            echo "💡 建议: 使用 'Paker --help' 查看可用命令"
            ;;
        "package_not_found")
            echo "❌ 包未找到: $command"
            echo "💡 建议: 使用 'Paker search <package>' 搜索包"
            ;;
        "not_in_project")
            echo "❌ 当前目录不是Paker项目"
            echo "💡 建议: 使用 'Paker init' 初始化项目"
            ;;
    esac
}

# 静默加载，不显示提示信息
# 只有在调试模式下才显示加载信息
if [[ "${PAKER_COMPLETION_DEBUG:-false}" == "true" ]]; then
    echo "✅ Paker Bash 补全已加载"
    echo "💡 使用 'Paker <TAB>' 开始智能补全"
fi
