# Paker 命令行使用指南

本文档详细介绍了 Paker 包管理器的所有命令行功能和使用方法。

## 目录

- [基本命令](#基本命令)
- [依赖管理命令](#依赖管理命令)
- [缓存管理命令](#缓存管理命令)
- [缓存预热命令](#缓存预热命令)
- [增量解析命令](#增量解析命令)
- [异步I/O命令](#异步io命令)
- [监控与诊断命令](#监控与诊断命令)
- [版本回滚命令](#版本回滚命令)
- [CLI选项](#cli选项)
- [使用示例](#使用示例)

## 基本命令

### 项目初始化
```bash
# 初始化项目（自动启用全局缓存）
./Paker init
```

### 依赖管理
```bash
# 添加依赖包
./Paker add fmt

# 并行安装多个包（性能优化）
./Paker add-parallel fmt spdlog nlohmann-json

# 递归添加依赖
./Paker add-recursive fmt

# 移除依赖包
./Paker remove fmt

# 列出所有依赖（表格化显示）
./Paker list

# 显示依赖树
./Paker tree

# 搜索依赖包（表格化显示）
./Paker search fmt

# 查看依赖包详细信息
./Paker info fmt
```

### 依赖源管理
```bash
# 添加自定义依赖源
./Paker add-remote mylib https://github.com/example/mylib.git

# 移除自定义依赖源
./Paker remove-remote mylib
```

### 版本管理
```bash
# 升级所有依赖
./Paker upgrade

# 升级指定依赖
./Paker upgrade fmt

# 同步/刷新依赖
./Paker update

# 锁定依赖版本
./Paker lock

# 按锁文件安装依赖
./Paker install-lock
```

### 清理操作
```bash
# 清理无用/损坏依赖
./Paker clean
```

## 依赖管理命令

### 依赖解析与冲突处理
```bash
# 解析项目依赖树
./Paker resolve-dependencies

# 检查依赖冲突
./Paker check-conflicts

# 解决依赖冲突
./Paker resolve-conflicts

# 验证依赖完整性
./Paker validate-dependencies
```

### 包安装记录
```bash
# 显示指定包的安装记录
./Paker record-show fmt

# 列出所有包记录
./Paker record-list

# 获取包文件列表
./Paker record-files fmt
```

## 缓存管理命令

### 基本缓存操作
```bash
# 显示缓存统计信息
./Paker cache-stats

# 显示详细缓存状态和健康度
./Paker cache-status

# 自动优化缓存性能和存储
./Paker cache-optimize

# 直接安装包到全局缓存
./Paker cache-install fmt

# 清理未使用的包和旧版本
./Paker cache-cleanup
```

### LRU智能缓存管理
```bash
# 初始化LRU智能缓存管理器
./Paker cache-init-lru

# 显示LRU缓存详细统计
./Paker cache-lru-stats

# 执行智能缓存清理策略
./Paker cache-smart-cleanup

# 获取缓存优化建议
./Paker cache-optimization-advice
```

## 缓存预热命令

### 缓存预热操作
```bash
# 启动缓存预热进程
./Paker warmup

# 分析项目依赖进行预热
./Paker warmup-analyze

# 显示缓存预热统计信息
./Paker warmup-stats

# 显示缓存预热配置
./Paker warmup-config
```

## 增量解析命令

### 增量解析操作
```bash
# 启动增量依赖解析
./Paker incremental-parse

# 显示增量解析统计信息
./Paker incremental-parse-stats

# 显示增量解析配置
./Paker incremental-parse-config

# 清理增量解析缓存
./Paker incremental-parse-clear-cache

# 优化增量解析缓存
./Paker incremental-parse-optimize

# 验证增量解析缓存完整性
./Paker incremental-parse-validate
```

## 异步I/O命令

### 异步I/O操作
```bash
# 显示异步I/O统计信息
./Paker async-io-stats

# 显示异步I/O配置
./Paker async-io-config

# 运行异步I/O测试
./Paker async-io-test

# 运行异步I/O性能基准测试
./Paker async-io-benchmark

# 优化异步I/O性能
./Paker async-io-optimize
```

## 监控与诊断命令

### 性能监控
```bash
# 生成性能监控报告
./Paker performance-report

# 分析依赖树和版本分布
./Paker analyze-dependencies

# 运行系统诊断检查
./Paker diagnose
```

## 版本回滚命令

### 回滚操作
```bash
# 回滚包到指定版本
./Paker rollback-to-version fmt 1.0.0

# 回滚包到上一个版本
./Paker rollback-to-previous fmt

# 回滚所有包到指定时间点
./Paker rollback-to-timestamp "2024-01-15 10:30:00"
```

### 版本历史管理
```bash
# 显示包的版本历史记录
./Paker history-show fmt

# 列出可回滚的版本
./Paker rollback-list fmt

# 检查回滚操作的安全性
./Paker rollback-check fmt 1.0.0
```

## CLI选项

### 全局选项
```bash
# 禁用彩色输出
./Paker --no-color <command>

# 启用详细模式（显示调试信息）
./Paker -v <command>
./Paker --verbose <command>

# 组合使用
./Paker --no-color -v <command>
```

### 输出格式
- **INFO**: 蓝色 - 一般信息
- **SUCCESS**: 绿色 - 成功信息  
- **WARNING**: 黄色 - 警告信息
- **ERROR**: 红色加粗 - 错误信息
- **DEBUG**: 灰色 - 调试信息（仅在详细模式下显示）

## 使用示例

### 基本工作流程
```bash
# 1. 初始化项目
./Paker init

# 2. 添加自定义依赖源
./Paker add-remote mylib https://github.com/example/mylib.git

# 3. 添加依赖包（推荐使用并行安装）
./Paker add-parallel fmt spdlog nlohmann-json

# 4. 解析项目依赖
./Paker resolve-dependencies

# 5. 检查依赖冲突
./Paker check-conflicts

# 6. 查看依赖列表
./Paker list

# 7. 查看依赖树
./Paker tree

# 8. 验证依赖完整性
./Paker validate-dependencies

# 9. 锁定依赖版本
./Paker lock
```

### 高级功能示例
```bash
# 递归安装依赖
./Paker add-recursive mylib

# 检测并解决冲突
./Paker check-conflicts
./Paker resolve-conflicts

# 升级所有依赖
./Paker upgrade

# 搜索可用包
./Paker search json

# 查看包详细信息
./Paker info fmt

# 同步本地依赖
./Paker update

# 清理无用包
./Paker clean
```

### 缓存管理示例
```bash
# 查看缓存状态
./Paker cache-status

# 优化缓存
./Paker cache-optimize

# 清理缓存
./Paker cache-cleanup

# LRU智能缓存管理
./Paker cache-init-lru
./Paker cache-lru-stats
./Paker cache-smart-cleanup
```

### 性能优化示例
```bash
# 缓存预热
./Paker warmup-analyze
./Paker warmup
./Paker warmup-stats

# 增量解析
./Paker incremental-parse
./Paker incremental-parse-stats
./Paker incremental-parse-optimize

# 异步I/O测试
./Paker async-io-test
./Paker async-io-benchmark
./Paker async-io-optimize
```

### 监控与诊断示例
```bash
# 生成性能报告
./Paker performance-report

# 分析依赖结构
./Paker analyze-dependencies

# 运行系统诊断
./Paker diagnose
```

### 版本回滚示例
```bash
# 回滚到指定版本
./Paker rollback-to-version fmt 1.0.0

# 回滚到上一个版本
./Paker rollback-to-previous fmt

# 显示版本历史
./Paker history-show fmt

# 检查回滚安全性
./Paker rollback-check fmt 1.0.0
```

### 详细模式示例
```bash
# 使用详细模式查看安装过程
./Paker -v add fmt

# 禁用彩色输出（适用于脚本或管道）
./Paker --no-color list | grep "installed"

# 组合使用
./Paker --no-color -v search json
```

## 命令输出示例

### 依赖列表输出
```
Package │ Version │ Status
fmt     │ 9.1.0   │ installed
spdlog  │ 1.11.0  │ installed
```

### 依赖树输出
```
my-project (1.0.0)
├── fmt (9.1.0)
└── spdlog (1.11.0)
    └── fmt (9.1.0)
```

### 缓存统计输出
```
📊 Cache Statistics:
  Total packages: 15
  Total size: 2.3 GB
  Cache hit rate: 85%
```

### 性能报告输出
```
📈 Performance Report:
  Average install time: 2.3s
  Cache hit rate: 78%
  Total operations: 150
  Success rate: 98%
```

## 注意事项

1. **全局缓存模式**：Paker 默认启用全局缓存模式，多项目共享包，节省空间和时间
2. **并行安装**：推荐使用 `add-parallel` 命令安装多个包，性能更优
3. **依赖冲突**：定期运行 `check-conflicts` 和 `resolve-conflicts` 确保依赖一致性
4. **缓存管理**：定期运行 `cache-optimize` 和 `cache-cleanup` 保持缓存健康
5. **性能监控**：使用 `performance-report` 和 `diagnose` 监控系统性能
6. **版本控制**：使用 `lock` 命令锁定依赖版本，确保项目稳定性

## 故障排除

### 常见问题
1. **依赖冲突**：使用 `check-conflicts` 检测，`resolve-conflicts` 解决
2. **缓存问题**：使用 `cache-status` 检查，`cache-optimize` 优化
3. **性能问题**：使用 `performance-report` 分析，`diagnose` 诊断
4. **版本问题**：使用 `rollback-*` 命令回滚到稳定版本

### 获取帮助
```bash
# 查看命令帮助
./Paker --help

# 查看特定命令帮助
./Paker <command> --help

# 使用详细模式获取更多信息
./Paker -v <command>
```
