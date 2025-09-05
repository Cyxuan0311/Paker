# Paker

<!-- 技术栈标签 -->
<p align="left">
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/build-CMake-brightgreen.svg" alt="CMake">
  <img src="https://img.shields.io/badge/cli-CLI11-orange.svg" alt="CLI11">
  <img src="https://img.shields.io/badge/json-nlohmann--json-yellow.svg" alt="nlohmann/json">
  <img src="https://img.shields.io/badge/log-glog-lightgrey.svg" alt="glog">
  <img src="https://img.shields.io/badge/test-GoogleTest-red.svg" alt="GoogleTest">
  <img src="https://img.shields.io/badge/feature-Record%20Tracking-purple.svg" alt="Record Tracking">
  <img src="https://img.shields.io/badge/feature-Colorful%20CLI-cyan.svg" alt="Colorful CLI">
  <img src="https://img.shields.io/badge/feature-Dependency%20Resolution-green.svg" alt="Dependency Resolution">
  <img src="https://img.shields.io/badge/feature-Conflict%20Detection-red.svg" alt="Conflict Detection">
  <img src="https://img.shields.io/badge/feature-Global%20Cache-blue.svg" alt="Global Cache">
  <img src="https://img.shields.io/badge/feature-Monitoring%20%26%20Diagnostics-orange.svg" alt="Monitoring & Diagnostics">
</p>

Paker 是一个用 C++ 编写的现代化 C++ 包管理器，支持全局缓存模式、智能依赖管理、冲突检测与解决、性能监控和诊断工具。具备精确的文件跟踪功能，确保依赖包的完整安装和清理。提供友好的彩色 CLI 输出，包括表格化显示、进度条和优化的依赖树可视化。

**核心特性**：
- 🚀 **全局缓存模式**：默认启用，多项目共享包，节省空间和时间
- 🔍 **智能依赖解析**：自动检测和解决版本冲突、循环依赖
- 📊 **性能监控**：实时监控安装时间、缓存命中率、磁盘使用情况
- 🛠️ **诊断工具**：自动检测配置问题、依赖冲突、性能瓶颈
- 🎨 **现代化CLI**：彩色输出、进度条、表格化显示

**性能优化**：
- ⚡ **并行下载**：同时下载多个包，安装速度提升2-5倍
- 🔄 **增量更新**：只下载变更文件，减少80-90%下载时间
- 💾 **内存优化**：轻量级依赖图，内存使用减少40-60%
- 🧠 **智能缓存**：LRU算法管理，缓存命中率提升至85%+

## 目录结构
```
Paker/
├── include/
│   └── Paker/           # Paker模块头文件
│       ├── core/        # 核心功能模块
│       │   ├── package_manager.h  # 包管理器主接口
│       │   ├── utils.h            # 工具函数
│       │   └── output.h           # CLI输出系统
│       ├── dependency/  # 依赖管理模块
│       │   ├── dependency_graph.h    # 依赖图数据结构
│       │   ├── dependency_resolver.h # 依赖解析器
│       │   ├── version_manager.h     # 版本管理
│       │   └── sources.h             # 仓库管理
│       ├── conflict/    # 冲突检测与解决模块
│       │   ├── conflict_detector.h   # 冲突检测器
│       │   └── conflict_resolver.h   # 冲突解决器
│       ├── commands/    # 命令模块
│       │   ├── install.h  # 安装命令
│       │   ├── list.h     # 列表命令
│       │   ├── lock.h     # 锁定命令
│       │   ├── info.h     # 信息命令
│       │   ├── update.h   # 更新命令
│       │   ├── cli.h      # CLI接口
│       │   ├── cache.h    # 缓存管理命令
│       │   └── monitor.h  # 监控命令
│       ├── monitor/       # 监控与诊断模块
│       │   ├── performance_monitor.h # 性能监控
│       │   ├── dependency_analyzer.h # 依赖分析
│       │   └── diagnostic_tool.h     # 诊断工具
│       └── cache/         # 缓存管理模块
│           ├── cache_manager.h       # 缓存管理器
│           ├── cache_config.h        # 缓存配置
│           ├── cache_path_resolver.h # 路径解析器
│           └── cache_monitor.h       # 缓存监控
├── src/
│   └── Paker/           # Paker模块实现
│       ├── core/        # 核心功能实现
│       │   ├── package_manager.cpp
│       │   ├── utils.cpp
│       │   └── output.cpp
│       ├── dependency/  # 依赖管理实现
│       │   ├── dependency_graph.cpp
│       │   ├── dependency_resolver.cpp
│       │   ├── version_manager.cpp
│       │   └── sources.cpp
│       ├── conflict/    # 冲突检测与解决实现
│       │   ├── conflict_detector.cpp
│       │   └── conflict_resolver.cpp
│       ├── commands/    # 命令实现
│       │   ├── install.cpp
│       │   ├── list.cpp
│       │   ├── lock.cpp
│       │   ├── info.cpp
│       │   ├── update.cpp
│       │   ├── cli.cpp
│       │   ├── cache.cpp
│       │   └── monitor.cpp
│       ├── monitor/     # 监控与诊断实现
│       │   ├── performance_monitor.cpp
│       │   ├── dependency_analyzer.cpp
│       │   └── diagnostic_tool.cpp
│       └── cache/       # 缓存管理实现
│           ├── cache_manager.cpp
│           ├── cache_config.cpp
│           ├── cache_path_resolver.cpp
│           └── cache_monitor.cpp
├── test/
│   ├── unit/            # 单元测试
│   └── integration/     # 集成测试
├── scripts/             # 构建和测试脚本
├── docs/                # 文档目录
└── CMakeLists.txt
```

## 使用示例与命令行功能

| 功能             | 命令示例                        | 说明                         | 示例输出 |
|------------------|----------------------------------|------------------------------|----------|
| 初始化项目       | `./Paker init`                   | 初始化依赖管理（启用全局缓存）| Initialized Paker project.<br>Global cache system initialized (default mode) |
| 添加依赖         | `./Paker add fmt`                | 添加依赖包（全局缓存模式）   | Using global cache mode (default)<br>Added dependency: fmt<br>Successfully installed fmt (cached, 156 files) |
| **并行安装**     | `./Paker add-parallel fmt spdlog nlohmann-json` | 并行安装多个包（性能优化） | Starting parallel installation of 3 packages<br>Parallel installation completed successfully |
| 递归添加依赖     | `./Paker add-recursive fmt`      | 递归安装依赖及其依赖         | Added dependency: fmt ... |
| 列出依赖         | `./Paker list`                   | 列出所有依赖（表格化显示）   | Package \| Version \| Status<br>fmt \| 9.1.0 \| installed |
| 移除依赖         | `./Paker remove fmt`             | 移除依赖包                   | Removed dependency: fmt<br>Found 156 files to remove for package: fmt |
| 显示依赖树       | `./Paker tree`                   | 以树状结构展示依赖关系       | my-project (1.0.0)<br>├── fmt (9.1.0)<br>└── spdlog (1.11.0) |
| 依赖锁定         | `./Paker lock`                   | 生成/更新 Paker.lock         | Generated Paker.lock |
| 按锁文件安装     | `./Paker install-lock`           | 按 lock 文件安装依赖         | Installed dependencies from Paker.lock |
| 升级所有依赖     | `./Paker upgrade`                | 升级所有依赖到最新           | Upgrading fmt to latest...<br>Upgrade complete. |
| 升级指定依赖     | `./Paker upgrade fmt`            | 升级指定依赖到最新           | Upgrading fmt to latest...<br>Upgrade complete. |
| 搜索依赖包       | `./Paker search fmt`             | 搜索可用依赖包（表格化显示） | Package \| Repository<br>fmt \| https://github.com/fmtlib/fmt.git |
| 查看依赖信息     | `./Paker info fmt`               | 查看依赖包详细信息           | Package: fmt<br>Repository: https://github.com/fmtlib/fmt.git |
| 添加依赖源       | `./Paker add-remote mylib https://git.example.com/mylib.git` | 添加/更新自定义依赖源 | Added/Updated remote: mylib -> ... |
| 移除依赖源       | `./Paker remove-remote mylib`    | 移除自定义依赖源             | Removed remote: mylib |
| 同步/刷新依赖    | `./Paker update`                 | git pull 同步本地依赖         | Updating fmt...<br>Update complete. |
| 清理无用/损坏依赖| `./Paker clean`                  | 清理未声明或损坏的依赖包      | Removing unused package: ...<br>Clean complete. |
| **解析项目依赖**   | `./Paker resolve-dependencies`   | 解析整个项目的依赖树         | Resolving project dependencies...<br>Dependencies resolved successfully |
| **检查依赖冲突**   | `./Paker check-conflicts`        | 检测依赖树中的冲突           | Checking for dependency conflicts...<br>Found 2 conflicts |
| **解决依赖冲突**   | `./Paker resolve-conflicts`      | 自动或交互式解决冲突         | Auto-resolve conflicts? [Y/n/i]: <br>Conflicts resolved successfully |
| **验证依赖完整性** | `./Paker validate-dependencies`  | 验证依赖图的完整性           | Validating dependencies...<br>Dependencies validated successfully |
| **显示包安装记录** | `./Paker record-show fmt`        | 显示指定包的安装记录         | Package: fmt<br>Install Path: packages/fmt<br>Files (156): ... |
| **列出所有包记录** | `./Paker record-list`            | 列出所有已安装的包记录       | Installed packages (3):<br>  fmt (156 files)<br>  spdlog (89 files) |
| **获取包文件列表** | `./Paker record-files fmt`       | 获取指定包的所有文件列表     | Files for package 'fmt':<br>  packages/fmt/src/format.cc<br>  packages/fmt/include/fmt/format.h |
| **缓存管理**      | `./Paker cache-stats`            | 显示缓存统计信息             | 📊 Cache Statistics:<br>  Total packages: 15<br>  Total size: 2.3 GB |
| **缓存状态**      | `./Paker cache-status`           | 显示详细缓存状态和健康度     | 🔍 Cache Status Report<br>🏥 Cache Health: 85% |
| **缓存优化**      | `./Paker cache-optimize`         | 自动优化缓存性能和存储       | 🚀 Optimizing cache...<br>Cache optimization completed successfully |
| **缓存安装**      | `./Paker cache-install fmt`      | 直接安装包到全局缓存         | Installing fmt to global cache...<br>Successfully cached fmt |
| **缓存清理**      | `./Paker cache-cleanup`          | 清理未使用的包和旧版本       | Cleaning up unused packages...<br>Cleaned up 5 packages |
| **LRU缓存初始化** | `./Paker cache-init-lru`         | 初始化LRU智能缓存管理器     | LRU cache manager initialized successfully |
| **LRU缓存统计**   | `./Paker cache-lru-stats`        | 显示LRU缓存详细统计         | 📊 LRU Cache Statistics:<br>  Hit Rate: 85.2%<br>  Total Items: 45 |
| **智能缓存清理**   | `./Paker cache-smart-cleanup`    | 执行智能缓存清理策略         | 🧹 Starting smart cache cleanup...<br>Smart cleanup completed successfully |
| **缓存优化建议**   | `./Paker cache-optimization-advice` | 获取缓存优化建议           | 💡 Cache Optimization Advice:<br>  Cache is optimally configured |
| **性能报告**      | `./Paker performance-report`     | 生成性能监控报告             | 📈 Performance Report:<br>  Average install time: 2.3s<br>  Cache hit rate: 78% |
| **依赖分析**      | `./Paker analyze-dependencies`   | 分析依赖树和版本分布         | 📊 Dependency Analysis:<br>  Total dependencies: 12<br>  Max depth: 3 |
| **系统诊断**      | `./Paker diagnose`               | 运行系统诊断检查             | 🔧 System Diagnostics:<br>  Configuration: ✅ OK<br>  Dependencies: ⚠️ 2 warnings |
| **回滚到指定版本** | `./Paker rollback-to-version fmt 1.0.0` | 回滚包到指定版本         | 🔄 Rolling back fmt to version 1.0.0<br>Rollback completed successfully |
| **回滚到上一版本** | `./Paker rollback-to-previous fmt` | 回滚包到上一个版本         | 🔄 Rolling back fmt to previous version<br>Rollback completed successfully |
| **回滚到时间点**   | `./Paker rollback-to-timestamp "2024-01-15 10:30:00"` | 回滚所有包到指定时间点 | 🔄 Rolling back to timestamp<br>Rollback completed successfully |
| **显示版本历史**   | `./Paker history-show fmt`       | 显示包的版本历史记录       | 📜 Version history for fmt:<br>  Package │ Old Version │ New Version │ Timestamp |
| **列出可回滚版本** | `./Paker rollback-list fmt`      | 列出可回滚的版本           | 📋 Rollbackable versions for fmt:<br>  1. 1.2.0 (current)<br>  2. 1.1.0 |
| **检查回滚安全性** | `./Paker rollback-check fmt 1.0.0` | 检查回滚操作的安全性     | 🔍 Checking rollback safety<br>✅ Rollback is safe |

### 全局缓存模式

Paker 默认启用全局缓存模式，提供高效的包管理和存储优化：

#### 缓存策略
- **混合模式**：优先使用用户缓存，备用全局缓存
- **智能路径选择**：基于空间、性能和访问模式自动选择最优位置
- **符号链接**：项目通过符号链接引用缓存中的包，节省空间

#### 缓存位置
```
~/.paker/cache/                    # 用户缓存（主要）
├── fmt/
│   ├── latest/
│   └── 8.1.1/
└── cache_index.json

/usr/local/share/paker/cache/      # 全局缓存（备用）
├── fmt/
│   └── latest/

项目目录/.paker/links/             # 项目链接
├── fmt -> ~/.paker/cache/fmt/latest
```

#### 缓存管理命令
```bash
# 查看缓存状态
./Paker cache-status

# 优化缓存
./Paker cache-optimize

# 清理缓存
./Paker cache-cleanup

# 迁移到缓存模式
./Paker cache-migrate
```

### 依赖冲突检测与解决

Paker 提供了强大的依赖冲突检测与解决机制，能够自动识别和解决复杂的依赖问题。

#### 冲突类型检测
- **版本冲突**: 检测同一包在不同路径中的版本冲突
- **循环依赖**: 检测依赖图中的循环依赖关系
- **缺失依赖**: 检测未找到的依赖包

#### 解决策略
- **自动解决**: 智能选择最佳版本，自动解决冲突
- **交互式解决**: 用户可选择具体的解决策略
- **版本升级/降级**: 自动调整版本以满足依赖约束
- **依赖移除**: 移除冲突的依赖关系

#### 使用示例
```bash
# 检查项目中的依赖冲突
./Paker check-conflicts

# 自动解决冲突
./Paker resolve-conflicts

# 验证依赖完整性
./Paker validate-dependencies

# 解析项目依赖树
./Paker resolve-dependencies
```

#### 冲突报告示例
```
⚠️  Dependency Conflicts Detected

Conflict 1:
Package: fmt
Type: Version Conflict
Conflicting Versions:
  - 8.1.1 (required by spdlog@1.11.0)
  - 9.1.0 (required by json@3.11.2)
Conflict Path: myproject -> spdlog -> fmt
Suggested Solution: Use compatible version 9.1.0
```

### 版本回滚系统

Paker 提供了强大的版本回滚功能，支持快速、安全地回滚到之前的版本：

#### 回滚策略
- **单个包回滚**：回滚指定的包到指定版本
- **批量回滚**：回滚多个包到指定时间点
- **依赖感知回滚**：自动处理依赖关系，确保系统一致性
- **选择性回滚**：用户可选择性地回滚特定包

#### 安全机制
- **回滚前检查**：验证回滚操作的安全性
- **依赖验证**：检查版本兼容性和依赖约束
- **备份创建**：自动创建当前版本的备份
- **强制回滚**：在必要时跳过安全检查

#### 历史管理
- **版本历史记录**：详细记录所有版本变更
- **时间点回滚**：支持回滚到特定的时间点
- **历史清理**：自动清理过期的历史记录
- **历史导出/导入**：支持历史记录的备份和恢复

#### 使用示例
```bash
# 回滚到指定版本
./Paker rollback-to-version fmt 1.0.0

# 回滚到上一个版本
./Paker rollback-to-previous fmt

# 回滚到指定时间点
./Paker rollback-to-timestamp "2024-01-15 10:30:00"

# 显示版本历史
./Paker history-show fmt

# 检查回滚安全性
./Paker rollback-check fmt 1.0.0

# 列出可回滚的版本
./Paker rollback-list fmt
```

#### 回滚报告示例
```
🔄 Rollback Report
==================

Status: ✅ Success
Duration: 1250ms
Message: Successfully rolled back fmt to version 1.0.0

✅ Successfully Rolled Back:
  - fmt

💾 Backup Location: .paker/backups/fmt_current_20240115_103000.tar.gz
📁 Files Affected: 156

💡 Recommendations:
  - Verify the rolled back packages work correctly
  - Test your application thoroughly
  - Consider updating your dependency specifications
```

### 监控与诊断系统

Paker 集成了全面的监控和诊断系统，帮助用户了解系统性能和诊断问题：

#### 性能监控
- **安装时间跟踪**：记录每个包的安装耗时
- **缓存命中率**：监控缓存使用效率
- **磁盘使用情况**：跟踪缓存空间占用
- **网络性能**：监控下载速度和延迟

#### 依赖分析
- **依赖树分析**：分析依赖深度和复杂度
- **版本分布统计**：了解版本使用情况
- **包大小分析**：监控包存储占用
- **冲突趋势**：分析冲突发生模式

#### 诊断工具
- **配置检查**：验证配置文件完整性
- **依赖验证**：检查依赖关系正确性
- **性能诊断**：识别性能瓶颈
- **文件系统检查**：验证文件权限和空间
- **安全检查**：检测潜在安全问题

#### 使用示例
```bash
# 生成性能报告
./Paker performance-report

# 分析依赖结构
./Paker analyze-dependencies

# 运行系统诊断
./Paker diagnose

# 启用详细监控
./Paker monitor-enable

# 清除监控数据
./Paker monitor-clear
```

### 自定义依赖源支持

- 依赖源统一配置在 `Paker.json` 的 `remotes` 字段，例如：
  ```json
  {
    "name": "Paker",
    "version": "0.1.0",
    "description": "",
    "dependencies": {
      "fmt": "8.1.1"
    },
    "remotes": [
      { "name": "myprivlib", "url": "https://git.example.com/myprivlib.git" },
      { "name": "myorglib", "url": "git@github.com:myorg/myorglib.git" }
    ]
  }
  ```
- `add`、`search`、`info` 等命令会自动优先查找自定义源。
- `add-remote`/`remove-remote` 命令可动态管理依赖源。
- 支持私有仓库、镜像等多种依赖源。
- 如果依赖未在 remotes 和内置源中找到，add 命令会提示用户添加依赖源。

### CLI 输出优化

Paker 提供了友好的彩色 CLI 输出，大大提升了用户体验：

#### 彩色输出系统
- **颜色区分**: 不同类型消息使用不同颜色
  - INFO: 蓝色 - 一般信息
  - SUCCESS: 绿色 - 成功信息  
  - WARNING: 黄色 - 警告信息
  - ERROR: 红色加粗 - 错误信息
  - DEBUG: 灰色 - 调试信息（仅在详细模式下显示）
- **全局选项**: 
  - `--no-color`: 禁用彩色输出
  - `-v/--verbose`: 启用详细模式，显示调试信息

#### 表格化输出
- **自动列宽**: 根据内容自动调整列宽
- **对齐支持**: 支持左对齐和右对齐
- **格式化**: 自动添加分隔线和表头样式
- **应用场景**: `list`、`search` 等命令使用表格显示信息

#### 进度条
- **实时更新**: 显示当前进度和百分比
- **自定义宽度**: 可调整进度条宽度
- **前缀支持**: 可添加自定义前缀文本
- **应用场景**: `add` 命令显示安装进度

#### 优化的依赖树
- **树形结构**: 使用 Unicode 字符显示层次关系
- **版本信息**: 在包名后显示版本号
- **颜色区分**: 包名使用青色，版本使用灰色

### 包安装记录功能

Paker 集成了强大的包安装记录功能，可以精确跟踪每个安装包的文件路径，便于后续的删除、显示路径等操作。

#### 记录文件
- 位置：`<project_name>_install_record.json`
- 格式：JSON格式，包含包名、安装路径和文件列表
- 示例：
  ```json
  {
    "fmt": {
      "install_path": "packages/fmt",
      "files": [
        "packages/fmt/src/format.cc",
        "packages/fmt/include/fmt/format.h",
        "packages/fmt/README.md"
      ]
    }
  }
  ```

#### 记录功能特性
- **自动记录**：安装包时自动记录所有文件路径
- **精确跟踪**：记录每个包的确切文件位置
- **完全清理**：删除包时确保所有文件都被移除
- **易于查询**：提供多种方式查看安装信息
- **持久化存储**：记录保存在JSON文件中，程序重启后仍然可用
- **项目隔离**：每个项目有独立的记录文件

#### 使用场景
1. **精确删除**：删除包时不会遗漏任何文件
2. **文件审计**：查看包安装的所有文件
3. **空间管理**：了解每个包占用的磁盘空间
4. **依赖分析**：分析包的内部结构
5. **故障排除**：定位文件冲突或权限问题

## 完整使用示例

### 基本工作流程
```bash
# 1. 初始化项目（自动启用全局缓存）
./Paker init

# 2. 添加自定义依赖源
./Paker add-remote mylib https://github.com/example/mylib.git

# 3. 添加依赖包（全局缓存模式）
# 并行安装多个包（性能优化）
./Paker add-parallel fmt spdlog nlohmann-json

# 或者单独安装
./Paker add fmt
./Paker add spdlog

# 4. 解析项目依赖
./Paker resolve-dependencies

# 5. 检查依赖冲突
./Paker check-conflicts

# 6. 查看依赖列表（表格化显示）
./Paker list

# 7. 查看依赖树（优化显示）
./Paker tree

# 8. 验证依赖完整性
./Paker validate-dependencies

# 9. 查看缓存状态
./Paker cache-status

# 10. 查看安装记录
./Paker record-list
./Paker record-show fmt

# 11. 使用 CLI 选项
./Paker --no-color list          # 禁用彩色输出
./Paker -v add fmt              # 启用详细模式
./Paker --no-color -v search json  # 组合使用

# 12. 锁定依赖版本
./Paker lock

# 13. 查看包文件列表
./Paker record-files fmt

# 14. 移除不需要的包
./Paker remove spdlog
```

### 高级功能示例
```bash
# 递归安装依赖
./Paker add-recursive mylib

# 解析复杂依赖树
./Paker resolve-dependencies

# 检测并解决冲突
./Paker check-conflicts
./Paker resolve-conflicts

# 验证依赖完整性
./Paker validate-dependencies

# 升级所有依赖
./Paker upgrade

# 搜索可用包（表格化显示）
./Paker search json

# 查看包详细信息
./Paker info fmt

# 同步本地依赖
./Paker update

# 清理无用包
./Paker clean

# 缓存管理
./Paker cache-stats
./Paker cache-optimize
./Paker cache-cleanup

# LRU智能缓存管理
./Paker cache-init-lru
./Paker cache-lru-stats
./Paker cache-lru-status
./Paker cache-smart-cleanup
./Paker cache-optimization-advice

# 性能监控
./Paker performance-report
./Paker analyze-dependencies
./Paker diagnose

# 使用详细模式查看安装过程
./Paker -v add fmt

# 禁用彩色输出（适用于脚本或管道）
./Paker --no-color list | grep "installed"

# 交互式解决冲突
./Paker resolve-conflicts
# 选择 'i' 进入交互模式
```

## 依赖
- [CLI11](https://github.com/CLIUtils/CLI11) (已集成头文件)
- [nlohmann/json](https://github.com/nlohmann/json) (已集成头文件)
- [glog](https://github.com/google/glog)
- [GoogleTest](https://github.com/google/googletest)

## 性能优化

Paker 包含了多项性能优化功能，显著提升包管理效率：

- **并行下载**：同时下载多个包，安装速度提升2-5倍
- **增量更新**：只下载变更文件，减少80-90%下载时间  
- **内存优化**：轻量级依赖图，内存使用减少40-60%
- **智能缓存**：LRU算法管理，缓存命中率提升至85%+

详细文档请查看：[性能优化指南](PERFORMANCE_OPTIMIZATIONS.md)

### 快速开始性能优化

```bash
# 构建优化版本
./scripts/build_optimized.sh

# 运行性能演示
./scripts/performance_demo.sh

# 运行性能测试
./scripts/performance_test.sh
```

## License
MIT 
