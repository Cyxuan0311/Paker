# Paker 命令参考

## 快速命令表

| 功能分类 | 命令 | 说明 | 示例 |
|---------|------|------|------|
| **项目初始化** | `init` | 初始化项目 | `./Paker init` |
| **依赖管理** | `add <package>` | 添加依赖包 | `./Paker add fmt` |
| | `add-p <pkg1> <pkg2>...` | 并行安装多个包 | `./Paker add-p fmt spdlog` |
| | `add-r <package>` | 递归安装依赖 | `./Paker add-r mylib` |
| | `remove <package>` | 移除依赖包 | `./Paker remove fmt` |
| | `list` | 列出所有依赖 | `./Paker list` |
| | `tree` | 显示依赖树 | `./Paker tree` |
| | `search <package>` | 搜索依赖包 | `./Paker search fmt` |
| | `info <package>` | 查看包信息 | `./Paker info fmt` |
| **依赖源管理** | `remote-add <name> <url>` | 添加依赖源 | `./Paker remote-add mylib https://github.com/example/mylib.git` |
| | `remote-rm <name>` | 移除依赖源 | `./Paker remote-rm mylib` |
| **版本管理** | `upgrade` | 升级所有依赖 | `./Paker upgrade` |
| | `upgrade <package>` | 升级指定依赖 | `./Paker upgrade fmt` |
| | `update` | 同步本地依赖 | `./Paker update` |
| | `lock` | 锁定依赖版本 | `./Paker lock` |
| | `install-l` | 按锁文件安装 | `./Paker install-l` |
| **依赖解析** | `resolve` | 解析项目依赖 | `./Paker resolve` |
| | `check` | 检查依赖冲突 | `./Paker check` |
| | `fix` | 解决依赖冲突 | `./Paker fix` |
| | `validate` | 验证依赖完整性 | `./Paker validate` |

## 缓存管理命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `cache add <package> [version]` | 安装包到缓存 | `./Paker cache add fmt` |
| `cache remove <package> [version]` | 从缓存删除包 | `./Paker cache remove fmt` |
| `cache status [--detailed]` | 显示缓存状态 | `./Paker cache status --detailed` |
| `cache clean [--smart] [--force]` | 清理缓存 | `./Paker cache clean --smart` |
| `cache lru [--stats] [--status]` | LRU缓存管理 | `./Paker cache lru --stats` |

## 性能监控命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `perf` | 生成性能报告 | `./Paker perf` |
| `analyze` | 分析依赖结构 | `./Paker analyze` |
| `diagnose` | 运行系统诊断 | `./Paker diagnose` |
| `monitor-enable` | 启用性能监控 | `./Paker monitor-enable` |
| `monitor-clear` | 清除监控数据 | `./Paker monitor-clear` |

## 回滚管理命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `rollback <package> [version]` | 回滚到指定版本 | `./Paker rollback fmt 1.0.0` |
| `rollback --previous <package>` | 回滚到上一版本 | `./Paker rollback --previous fmt` |
| `rollback --timestamp <time>` | 回滚到时间点 | `./Paker rollback --timestamp "2024-01-15 10:30:00"` |
| `rollback --list <package>` | 列出可回滚版本 | `./Paker rollback --list fmt` |
| `rollback --check <package> <version>` | 检查回滚安全性 | `./Paker rollback --check fmt 1.0.0` |
| `rollback --stats` | 显示回滚统计 | `./Paker rollback --stats` |

## 历史管理命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `history [package]` | 显示版本历史 | `./Paker history fmt` |
| `history --clean [--max-entries]` | 清理历史记录 | `./Paker history --clean --max-entries 50` |
| `history --export <path>` | 导出历史记录 | `./Paker history --export backup.json` |
| `history --import <path>` | 导入历史记录 | `./Paker history --import backup.json` |

## 记录管理命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `record [package]` | 显示包安装记录 | `./Paker record fmt` |
| `record --list` | 列出所有包记录 | `./Paker record --list` |
| `record --files <package>` | 获取包文件列表 | `./Paker record --files fmt` |

## 高级功能命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `parse [--stats] [--config] [--clear] [--opt] [--validate]` | 增量解析管理 | `./Paker parse --stats` |
| `io [--stats] [--config] [--test] [--bench] [--opt]` | 异步I/O管理 | `./Paker io --test` |
| `warmup` | 启动缓存预热 | `./Paker warmup` |
| `warmup-analyze` | 分析项目依赖 | `./Paker warmup-analyze` |
| `warmup-stats` | 显示预热统计 | `./Paker warmup-stats` |
| `warmup-config` | 显示预热配置 | `./Paker warmup-config` |

## 开发模式命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `--dev cache-migrate` | 缓存迁移 | `./Paker --dev cache-migrate` |
| `--dev io --test` | I/O测试 | `./Paker --dev io --test` |
| `--dev parse --validate` | 验证解析缓存 | `./Paker --dev parse --validate` |

## 全局选项

| 选项 | 说明 | 示例 |
|------|------|------|
| `--no-color` | 禁用彩色输出 | `./Paker --no-color list` |
| `--version` | 显示版本信息 | `./Paker --version` |
| `--dev` | 启用开发模式 | `./Paker --dev cache-migrate` |
| `--help` | 显示帮助信息 | `./Paker --help` |

## 常用命令组合

### 项目初始化流程
```bash
./Paker init
./Paker remote-add mylib https://github.com/example/mylib.git
./Paker add-p fmt spdlog nlohmann-json
./Paker resolve
./Paker check
./Paker lock
```

### 性能优化流程
```bash
./Paker warmup-analyze
./Paker warmup
./Paker parse --opt
./Paker io --opt
./Paker cache clean --smart
```

### 故障排除流程
```bash
./Paker diagnose
./Paker check
./Paker cache status
./Paker perf
```

### 版本管理流程
```bash
./Paker history fmt
./Paker rollback --check fmt 1.0.0
./Paker rollback fmt 1.0.0
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
./Paker check
./Paker fix

# 缓存问题
./Paker cache status
./Paker cache clean --smart

# 性能问题
./Paker perf
./Paker diagnose

# 版本问题
./Paker rollback --list <package>
./Paker rollback --previous <package>
```

### 获取详细帮助
```bash
# 查看所有命令
./Paker --help

# 查看特定命令帮助
./Paker <command> --help

# 开发模式命令
./Paker --dev --help
```