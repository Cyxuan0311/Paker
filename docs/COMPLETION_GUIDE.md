# Paker 智能命令补全指南

## 🚀 概述

Paker 智能命令补全系统提供了强大的命令行补全功能，支持 Bash 和 Zsh 两种主流 Shell，具备上下文感知、智能建议、错误处理等高级特性。

## ✨ 核心特性

### 🧠 智能补全
- **上下文感知**: 根据当前命令和项目状态提供相关建议
- **动态补全**: 实时获取可用包、远程源等信息
- **智能建议**: 基于使用模式提供个性化建议
- **错误处理**: 自动检测错误并提供解决方案

### 🎯 补全类型
- **命令补全**: 主命令和子命令智能补全
- **参数补全**: 标志和选项智能补全
- **包名补全**: 已安装包和缓存包补全
- **路径补全**: 文件和目录路径补全
- **URL补全**: Git仓库URL智能补全

## 📦 安装方法

### 自动安装
```bash
# 运行安装脚本
./scripts/completion/install-completion.sh

# 重新启动终端
source ~/.bashrc  # 或 source ~/.zshrc
```

### 手动安装

#### Bash 补全
```bash
# 复制补全脚本
cp scripts/completion/paker-completion.bash ~/.local/share/bash-completion/completions/paker

# 添加到配置文件
echo "source ~/.local/share/bash-completion/completions/paker" >> ~/.bashrc
```

#### Zsh 补全
```bash
# 复制补全脚本
cp scripts/completion/_paker ~/.zsh/completions/_paker

# 添加到配置文件
echo "fpath=(\$HOME/.zsh/completions \$fpath)" >> ~/.zshrc
echo "autoload -U compinit && compinit" >> ~/.zshrc
```

## 🎮 使用方法

### 基本补全
```bash
# 命令补全
Paker <TAB>          # 显示所有可用命令
Paker add <TAB>       # 显示包名建议
Paker cache <TAB>     # 显示缓存管理子命令

# 参数补全
Paker cache status --<TAB>    # 显示可用标志
Paker rollback --<TAB>        # 显示回滚选项
```

### 智能补全示例

#### 1. 添加包补全
```bash
# 在Paker项目中
Paker add <TAB>
# 显示: fmt spdlog nlohmann-json boost catch2 gtest benchmark

# 不在项目中
Paker add <TAB>
# 显示: 常用包列表和缓存包列表
```

#### 2. 缓存管理补全
```bash
Paker cache <TAB>
# 显示: add remove status clean lru

Paker cache add <TAB>
# 显示: 缓存中的包列表

Paker cache status --<TAB>
# 显示: --detailed
```

#### 3. 回滚管理补全
```bash
Paker rollback <TAB>
# 显示: --previous --timestamp --force --list --check --stats

Paker rollback --list <TAB>
# 显示: 已安装的包列表
```

#### 4. 远程源管理补全
```bash
Paker remote-add <TAB>
# 显示: github gitlab bitbucket custom

Paker remote-rm <TAB>
# 显示: 已配置的远程源列表
```

## 🔧 高级功能

### 智能建议系统
```bash
# 获取智能建议
paker-completion --suggest add fmt
# 输出: fmt spdlog nlohmann-json boost catch2 gtest benchmark

# 获取智能提示
paker-completion --tips cache
# 输出: 💡 提示: 使用 'Paker cache status' 查看缓存状态
```

### 错误处理
```bash
# 处理命令未找到错误
paker-completion --error command_not_found
# 输出: ❌ 命令未找到: command_not_found
#      💡 建议: 使用 'Paker --help' 查看可用命令
```

### 缓存管理
```bash
# 更新补全缓存
paker-completion --update-cache
# 输出: 🔄 更新补全缓存...
#      ✅ 补全缓存已更新
```

## 📊 补全上下文

### 项目状态感知
```bash
# 在Paker项目中
Paker <TAB>
# 显示: add remove list tree resolve check fix validate

# 不在项目中
Paker <TAB>
# 显示: init --help --version
```

### 命令上下文感知
```bash
# 根据当前命令提供相关补全
Paker cache add <TAB>     # 显示缓存包列表
Paker rollback <TAB>      # 显示回滚选项
Paker history <TAB>       # 显示历史管理选项
```

## 🎨 用户体验优化

### 彩色输出
- **命令**: 蓝色高亮
- **参数**: 绿色高亮
- **标志**: 黄色高亮
- **错误**: 红色高亮
- **提示**: 紫色高亮

### 智能提示
```bash
# 自动显示使用提示
Paker add <TAB>
# 显示: 💡 提示: 使用 'Paker add <package>' 添加依赖包
#      💡 示例: Paker add fmt spdlog nlohmann-json
```

### 错误诊断
```bash
# 自动检测常见错误
Paker add nonexistent-package
# 显示: ❌ 包未找到: nonexistent-package
#      💡 建议: 使用 'Paker search <package>' 搜索包
```

## 🛠️ 配置选项

### 补全缓存配置
```json
{
  "packages": ["fmt", "spdlog", "nlohmann-json"],
  "remotes": ["github", "gitlab", "bitbucket"],
  "installed_packages": ["fmt", "spdlog"],
  "cached_packages": ["boost", "catch2", "gtest"],
  "last_updated": 1640995200
}
```

### 智能建议配置
```bash
# 启用智能建议
export PAKER_SMART_SUGGESTIONS=true

# 启用错误处理
export PAKER_ERROR_HANDLING=true

# 启用缓存优化
export PAKER_CACHE_OPTIMIZATION=true

# 启用调试模式（显示加载信息）
export PAKER_COMPLETION_DEBUG=true

# 或者使用配置管理命令
paker-completion-config --debug
```

## 🔍 故障排除

### 常见问题

#### 1. 补全不工作
```bash
# 检查补全脚本是否正确加载
bash -c "source ~/.local/share/bash-completion/completions/paker && complete -p Paker"

# 检查Zsh补全
zsh -c "autoload -U compinit && compinit && which _paker"
```

#### 2. 补全速度慢
```bash
# 更新补全缓存
paker-completion --update-cache

# 检查缓存文件
ls -la ~/.paker/completion_cache.json
```

#### 3. 补全建议不准确
```bash
# 清除缓存并重新生成
rm ~/.paker/completion_cache.json
paker-completion --update-cache
```

### 调试模式
```bash
# 启用调试模式
export PAKER_COMPLETION_DEBUG=true

# 或者使用配置管理命令
paker-completion-config --debug

# 查看补全状态
paker-completion-config --status

# 重置补全配置
paker-completion-config --reset
```

## 📈 性能优化

### 缓存策略
- **本地缓存**: 缓存常用包和命令
- **智能更新**: 根据使用频率更新缓存
- **增量补全**: 只加载必要的补全数据

### 性能指标
- **补全响应时间**: < 100ms
- **缓存命中率**: > 90%
- **内存使用**: < 10MB
- **CPU使用**: < 5%

## 🎯 最佳实践

### 1. 定期更新缓存
```bash
# 设置自动更新
echo "0 2 * * * paker-completion --update-cache" | crontab -
```

### 2. 自定义补全
```bash
# 添加自定义包到补全列表
echo '["my-custom-package"]' >> ~/.paker/custom_packages.json
```

### 3. 团队共享配置
```bash
# 共享补全配置
cp ~/.paker/completion_cache.json ./team-completion-config.json
```

## 🔮 未来规划

### 计划功能
- **AI辅助补全**: 基于机器学习的智能建议
- **多语言支持**: 支持多种编程语言的包补全
- **云端同步**: 跨设备同步补全配置
- **插件系统**: 支持第三方补全插件

### 社区贡献
- **补全脚本**: 欢迎贡献新的补全脚本
- **语言支持**: 欢迎添加新的Shell支持
- **功能建议**: 欢迎提出功能改进建议

## 📚 相关文档

- [命令行使用指南](COMMAND_LINE_USAGE.md)
- [命令参考](COMMAND_REFERENCE.md)
- [功能特性详解](FEATURES.md)
- [安装指南](INSTALLATION.md)

---

**智能补全让Paker更智能，让开发更高效！** 🚀
