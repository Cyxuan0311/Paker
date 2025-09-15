# Paker 命令参考

## 快速命令表

| 功能分类 | 命令 | 说明 | 示例 |
|---------|------|------|------|
| **项目初始化** | `init` | 初始化项目 | `./Paker init` |
| **依赖管理** | `add <package>` | 添加依赖包 | `./Paker add fmt` |
| | `add-parallel <pkg1> <pkg2>...` | 并行安装多个包 | `./Paker add-parallel fmt spdlog` |
| | `add-recursive <package>` | 递归安装依赖 | `./Paker add-recursive mylib` |
| | `remove <package>` | 移除依赖包 | `./Paker remove fmt` |
| | `list` | 列出所有依赖 | `./Paker list` |
| | `tree` | 显示依赖树 | `./Paker tree` |
| | `search <package>` | 搜索依赖包 | `./Paker search fmt` |
| | `info <package>` | 查看包信息 | `./Paker info fmt` |
| **依赖源管理** | `add-remote <name> <url>` | 添加依赖源 | `./Paker add-remote mylib https://github.com/example/mylib.git` |
| | `remove-remote <name>` | 移除依赖源 | `./Paker remove-remote mylib` |
| **版本管理** | `upgrade` | 升级所有依赖 | `./Paker upgrade` |
| | `upgrade <package>` | 升级指定依赖 | `./Paker upgrade fmt` |
| | `update` | 同步本地依赖 | `./Paker update` |
| | `lock` | 锁定依赖版本 | `./Paker lock` |
| | `install-lock` | 按锁文件安装 | `./Paker install-lock` |
| **依赖解析** | `resolve-dependencies` | 解析项目依赖 | `./Paker resolve-dependencies` |
| | `check-conflicts` | 检查依赖冲突 | `./Paker check-conflicts` |
| | `resolve-conflicts` | 解决依赖冲突 | `./Paker resolve-conflicts` |
| | `validate-dependencies` | 验证依赖完整性 | `./Paker validate-dependencies` |
| **缓存管理** | `cache-stats` | 显示缓存统计 | `./Paker cache-stats` |
| | `cache-status` | 显示缓存状态 | `./Paker cache-status` |
| | `cache-optimize` | 优化缓存 | `./Paker cache-optimize` |
| | `cache-install <package>` | 安装到缓存 | `./Paker cache-install fmt` |
| | `cache-cleanup` | 清理缓存 | `./Paker cache-cleanup` |
| **LRU缓存** | `cache-init-lru` | 初始化LRU缓存 | `./Paker cache-init-lru` |
| | `cache-lru-stats` | LRU缓存统计 | `./Paker cache-lru-stats` |
| | `cache-smart-cleanup` | 智能缓存清理 | `./Paker cache-smart-cleanup` |
| | `cache-optimization-advice` | 缓存优化建议 | `./Paker cache-optimization-advice` |
| **缓存预热** | `warmup` | 启动缓存预热 | `./Paker warmup` |
| | `warmup-analyze` | 分析项目依赖 | `./Paker warmup-analyze` |
| | `warmup-stats` | 预热统计信息 | `./Paker warmup-stats` |
| | `warmup-config` | 预热配置 | `./Paker warmup-config` |
| **增量解析** | `incremental-parse` | 启动增量解析 | `./Paker incremental-parse` |
| | `incremental-parse-stats` | 解析统计信息 | `./Paker incremental-parse-stats` |
| | `incremental-parse-config` | 解析配置 | `./Paker incremental-parse-config` |
| | `incremental-parse-clear-cache` | 清理解析缓存 | `./Paker incremental-parse-clear-cache` |
| | `incremental-parse-optimize` | 优化解析缓存 | `./Paker incremental-parse-optimize` |
| | `incremental-parse-validate` | 验证解析缓存 | `./Paker incremental-parse-validate` |
| **异步I/O** | `async-io-stats` | 异步I/O统计 | `./Paker async-io-stats` |
| | `async-io-config` | 异步I/O配置 | `./Paker async-io-config` |
| | `async-io-test` | 异步I/O测试 | `./Paker async-io-test` |
| | `async-io-benchmark` | 异步I/O基准测试 | `./Paker async-io-benchmark` |
| | `async-io-optimize` | 异步I/O优化 | `./Paker async-io-optimize` |
| **监控诊断** | `performance-report` | 性能报告 | `./Paker performance-report` |
| | `analyze-dependencies` | 依赖分析 | `./Paker analyze-dependencies` |
| | `diagnose` | 系统诊断 | `./Paker diagnose` |
| **版本回滚** | `rollback-to-version <pkg> <ver>` | 回滚到指定版本 | `./Paker rollback-to-version fmt 1.0.0` |
| | `rollback-to-previous <pkg>` | 回滚到上一版本 | `./Paker rollback-to-previous fmt` |
| | `rollback-to-timestamp <time>` | 回滚到时间点 | `./Paker rollback-to-timestamp "2024-01-15 10:30:00"` |
| | `history-show <pkg>` | 显示版本历史 | `./Paker history-show fmt` |
| | `rollback-list <pkg>` | 列出可回滚版本 | `./Paker rollback-list fmt` |
| | `rollback-check <pkg> <ver>` | 检查回滚安全性 | `./Paker rollback-check fmt 1.0.0` |
| **安装记录** | `record-show <pkg>` | 显示包安装记录 | `./Paker record-show fmt` |
| | `record-list` | 列出所有包记录 | `./Paker record-list` |
| | `record-files <pkg>` | 获取包文件列表 | `./Paker record-files fmt` |
| **清理操作** | `clean` | 清理无用依赖 | `./Paker clean` |

## 全局选项

| 选项 | 说明 | 示例 |
|------|------|------|
| `--no-color` | 禁用彩色输出 | `./Paker --no-color list` |
| `-v, --verbose` | 启用详细模式 | `./Paker -v add fmt` |
| `--help` | 显示帮助信息 | `./Paker --help` |

## 常用命令组合

### 项目初始化流程
```bash
./Paker init
./Paker add-remote mylib https://github.com/example/mylib.git
./Paker add-parallel fmt spdlog nlohmann-json
./Paker resolve-dependencies
./Paker check-conflicts
./Paker lock
```

### 性能优化流程
```bash
./Paker warmup-analyze
./Paker warmup
./Paker incremental-parse
./Paker async-io-test
./Paker cache-optimize
```

### 故障排除流程
```bash
./Paker diagnose
./Paker check-conflicts
./Paker cache-status
./Paker performance-report
```

### 版本管理流程
```bash
./Paker history-show fmt
./Paker rollback-check fmt 1.0.0
./Paker rollback-to-version fmt 1.0.0
```

## 输出格式说明

### 颜色编码
- 🔵 **蓝色**: 一般信息 (INFO)
- 🟢 **绿色**: 成功信息 (SUCCESS)
- 🟡 **黄色**: 警告信息 (WARNING)
- 🔴 **红色**: 错误信息 (ERROR)
- ⚪ **灰色**: 调试信息 (DEBUG)

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

1. **使用并行安装**: `add-parallel` 比单独 `add` 快2-5倍
2. **启用缓存预热**: `warmup` 提升首次使用体验70%+
3. **使用增量解析**: `incremental-parse` 提升解析速度60-80%
4. **定期优化缓存**: `cache-optimize` 保持最佳性能
5. **监控系统性能**: `performance-report` 识别性能瓶颈

## 故障排除

### 常见问题解决
```bash
# 依赖冲突
./Paker check-conflicts
./Paker resolve-conflicts

# 缓存问题
./Paker cache-status
./Paker cache-optimize

# 性能问题
./Paker performance-report
./Paker diagnose

# 版本问题
./Paker rollback-list <package>
./Paker rollback-to-previous <package>
```

### 获取详细帮助
```bash
# 查看所有命令
./Paker --help

# 查看特定命令帮助
./Paker <command> --help

# 详细模式运行
./Paker -v <command>
```
