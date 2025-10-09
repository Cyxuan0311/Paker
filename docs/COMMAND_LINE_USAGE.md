# Paker 命令行使用指南

本文档详细介绍了 Paker 包管理器的8个核心命令组及其使用方法。

## 8个核心命令组

Paker采用8个核心命令组，每个命令组包含相关的子命令，让用户更容易找到需要的功能：

1. **核心包管理命令** - 包的增删改查和更新
2. **依赖锁定命令** - 依赖关系管理和版本锁定
3. **缓存管理命令** - 缓存优化和I/O性能
4. **性能监控命令** - 性能优化和问题诊断
5. **版本控制命令** - 版本管理和回滚
6. **项目管理命令** - 项目初始化和配置
7. **依赖源管理命令** - 自定义包源管理
8. **系统管理命令** - 系统维护和优化

## 目录

- [1. 核心包管理命令](#1-核心包管理命令)
- [2. 依赖锁定命令](#2-依赖锁定命令)
- [3. 缓存管理命令](#3-缓存管理命令)
- [4. 性能监控命令](#4-性能监控命令)
- [5. 版本控制命令](#5-版本控制命令)
- [6. 项目管理命令](#6-项目管理命令)
- [7. 依赖源管理命令](#7-依赖源管理命令)
- [8. 系统管理命令](#8-系统管理命令)
- [全局选项](#全局选项)
- [使用示例](#使用示例)

## 基本命令

### 项目初始化
```bash
# 初始化项目（自动启用全局缓存）
Paker init
```

### 依赖管理
```bash
# 添加依赖包
Paker add fmt

# 并行安装多个包（性能优化）
Paker add-p fmt spdlog nlohmann-json

# 递归添加依赖
Paker add-r fmt

# 移除依赖包
Paker remove fmt

# 列出所有依赖（表格化显示）
Paker list

# 显示依赖树
Paker tree

# 搜索依赖包（表格化显示）
Paker search fmt

# 查看依赖包详细信息
Paker info fmt
```

### 依赖源管理
```bash
# 添加自定义依赖源
Paker source-add mylib https://github.com/example/mylib.git

# 移除依赖源
Paker source-rm mylib
```

### 版本管理
```bash
# 升级所有依赖
Paker upgrade

# 升级指定依赖
Paker upgrade fmt

# 同步本地依赖
Paker update

# 锁定依赖版本
Paker lock

# 从锁文件安装
Paker lock install
```

### 依赖解析
```bash
# 解析项目依赖
Paker lock resolve

# 检查依赖冲突
Paker lock check

# 解决依赖冲突
Paker lock fix

# 验证依赖完整性
Paker lock validate
```

## 缓存管理命令

Paker 提供了统一的缓存管理命令，通过子命令和参数控制不同功能：

### 缓存操作
```bash
# 安装包到缓存
Paker cache add fmt
Paker cache add fmt 8.1.1

# 从缓存删除包
Paker cache remove fmt
Paker cache remove fmt 8.1.1

# 显示缓存状态
Paker cache status
Paker cache status --detailed

# 清理缓存
Paker cache clean
Paker cache clean --smart
Paker cache clean --force
```

### 缓存预热
```bash
# 缓存预热
Paker cache warmup
```

## 性能监控命令

### 性能监控管理
```bash
# 启用性能监控
Paker monitor enable

# 清除监控数据
Paker monitor clear

# 生成性能报告
Paker monitor perf

# 分析依赖结构
Paker monitor analyze

# 运行系统诊断
Paker monitor diagnose
```

## 版本控制命令

Paker 提供了统一的版本管理功能，包括版本信息、回滚和历史管理：

### 版本信息
```bash
# 显示版本信息
Paker version

# 显示简短版本
Paker version --short

# 显示构建信息
Paker version --build

# 检查版本兼容性
Paker version --check 1.0.0
```

### 版本回滚
```bash
# 回滚到指定版本
Paker version rollback fmt 1.0.0

# 回滚到上一版本
Paker version rollback --previous fmt

# 回滚到指定时间点
Paker version rollback --timestamp "2024-01-15 10:30:00"
```

### 回滚信息查询
```bash
# 列出可回滚版本
Paker version rollback --list fmt

# 检查回滚安全性
Paker version rollback --check fmt 1.0.0

# 显示回滚统计
Paker version rollback --stats
```

### 强制回滚
```bash
# 强制回滚（跳过安全检查）
Paker version rollback fmt 1.0.0 --force
Paker version rollback --previous fmt --force
```

### 历史管理
```bash
# 显示所有历史记录
Paker version history

# 显示指定包的历史记录
Paker version history fmt

# 清理历史记录
Paker version history --clean
Paker version history --clean --max-entries 50

# 导出历史记录
Paker version history --export backup.json

# 导入历史记录
Paker version history --import backup.json
```

### 安装记录
```bash
# 显示所有包记录
Paker version record --list

# 显示指定包记录
Paker version record fmt

# 显示包文件列表
Paker version record --files fmt
```

## 高级功能命令

### 增量解析
```bash
# 启动增量解析
Paker parse

# 显示解析统计
Paker parse --stats

# 显示解析配置
Paker parse --config

# 清除解析缓存
Paker parse --clear

# 优化解析缓存
Paker parse --opt

# 验证解析缓存
Paker parse --validate
```

### 异步I/O管理
```bash
# 显示I/O统计
Paker io --stats

# 显示I/O配置
Paker io --config

# 运行I/O测试
Paker io --test

# 运行I/O基准测试
Paker io --bench

# 优化I/O性能
Paker io --opt
```

### 缓存预热
```bash
# 启动缓存预热
Paker warmup

# 分析项目依赖
Paker warmup-analyze

# 显示预热统计
Paker warmup-stats

# 显示预热配置
Paker warmup-config
```

## 开发模式命令

开发模式提供了高级功能，需要 `--dev` 标志：

### 缓存迁移
```bash
# 迁移到缓存模式
Paker --dev cache-migrate
Paker --dev cache-migrate /path/to/project
```

### 高级测试
```bash
# I/O性能测试
Paker --dev io --test

# 解析缓存验证
Paker --dev parse --validate
```

## 全局选项

### 基本选项
```bash
# 禁用彩色输出
Paker --no-color list

# 显示版本信息
Paker --version

# 显示帮助信息
Paker --help
```

### 开发模式
```bash
# 启用开发模式
Paker --dev --help
Paker --dev cache-migrate
```

## 使用示例

### 项目初始化流程
```bash
# 1. 初始化项目
Paker init

# 2. 添加依赖源
Paker source-add mylib https://github.com/example/mylib.git

# 3. 并行安装依赖
Paker add-p fmt spdlog nlohmann-json

# 4. 解析依赖
Paker resolve

# 5. 检查冲突
Paker check

# 6. 锁定版本
Paker lock
```

### 性能优化流程
```bash
# 1. 分析项目依赖
Paker warmup-analyze

# 2. 启动缓存预热
Paker warmup

# 3. 优化解析缓存
Paker parse --opt

# 4. 优化I/O性能
Paker io --opt

# 5. 智能清理缓存
Paker cache clean --smart
```

### 故障排除流程
```bash
# 1. 运行系统诊断
Paker diagnose

# 2. 检查依赖冲突
Paker check

# 3. 查看缓存状态
Paker cache status --detailed

# 4. 生成性能报告
Paker perf

# 5. 分析依赖结构
Paker analyze
```

### 版本管理流程
```bash
# 1. 查看版本历史
Paker history fmt

# 2. 检查回滚安全性
Paker rollback --check fmt 1.0.0

# 3. 执行回滚
Paker rollback fmt 1.0.0

# 4. 验证回滚结果
Paker list
```

## 输出格式说明

### 颜色编码
- 🔵 **蓝色**: 一般信息 (INFO)
- 🟢 **绿色**: 成功信息 (SUCCESS)
- 🟡 **黄色**: 警告信息 (WARNING)
- 🔴 **红色**: 错误信息 (ERROR)

### 表格格式
- 自动列宽调整
- 左对齐/右对齐支持
- Unicode分隔线
- 表头样式

### 进度条
- 实时进度显示
- 百分比显示
- 自定义宽度
- 前缀文本支持

## 性能优化建议

1. **使用并行安装**: `add-p` 比单独 `add` 快2-5倍
2. **启用缓存预热**: `warmup` 提升首次使用体验70%+
3. **使用增量解析**: `parse` 提升解析速度60-80%
4. **定期优化缓存**: `cache clean --smart` 保持最佳性能
5. **监控系统性能**: `perf` 识别性能瓶颈

## 故障排除

### 常见问题解决
```bash
# 依赖冲突
Paker check
Paker fix

# 缓存问题
Paker cache status
Paker cache clean --smart

# 性能问题
Paker perf
Paker diagnose

# 版本问题
Paker rollback --list <package>
Paker rollback --previous <package>
```

### 获取详细帮助
```bash
# 查看所有命令
Paker --help

# 查看特定命令帮助
Paker <command> --help

# 开发模式命令
Paker --dev --help
```