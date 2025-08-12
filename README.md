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
</p>

Paker 是一个用 C++ 编写的轻量级 C++ 包管理器，支持本地依赖管理、自定义依赖源、包安装记录跟踪和丰富的命令行操作。具备精确的文件跟踪功能，确保依赖包的完整安装和清理。提供友好的彩色 CLI 输出，包括表格化显示、进度条和优化的依赖树可视化。

## 目录结构
```
Paker/
├── include/
│   ├── Paker/           # Paker模块头文件
│   │   ├── output.h     # CLI输出系统
│   ├── Recorder/        # Recorder模块头文件
│   └── third_party/     # 第三方库头文件
├── src/
│   ├── Paker/           # Paker模块实现
│   │   ├── output.cpp   # CLI输出系统实现
│   ├── Recorder/        # Recorder模块实现
│   ├── builtin_repos.cpp
│   └── main.cpp
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
| 初始化项目       | `./Paker init`                   | 初始化依赖管理               | Initialized Paker project. |
| 添加依赖         | `./Paker add fmt`                | 添加依赖包（支持自定义源）   | Added dependency: fmt<br>Installing: [==============>] 100% (3/3)<br>Successfully installed fmt (156 files recorded) |
| 递归添加依赖     | `./Paker add-recursive fmt`      | 递归安装依赖及其依赖         | Added dependency: fmt ... |
| 列出依赖         | `./Paker list`                   | 列出所有依赖（表格化显示）   | Package \| Version \| Status<br>fmt \| 9.1.0 \| installed |
| 移除依赖         | `./Paker remove fmt`             | 移除依赖包                   | Removed dependency: fmt<br>Found 156 files to remove for package: fmt |
| 显示依赖树       | `./Paker tree`                   | 以树状结构展示依赖关系       | my-project (1.0.0)<br>├── fmt (9.1.0)<br>└── spdlog (1.11.0) |
| 依赖锁定         | `./Paker lock`                   | 生成/更新 Paker.lock         | Generated Paker.lock |
| 按锁文件安装     | `./Paker install-lock`           | 按 lock 文件安装依赖         | Installed dependencies from Paker.lock |
| 升级所有依赖     | `./Paker upgrade`                | 升级所有依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 升级指定依赖     | `./Paker upgrade fmt`            | 升级指定依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 搜索依赖包       | `./Paker search fmt`             | 搜索可用依赖包（表格化显示） | Package \| Repository<br>fmt \| https://github.com/fmtlib/fmt.git |
| 查看依赖信息     | `./Paker info fmt`               | 查看依赖包详细信息           | Package: fmt<br>Repository: https://github.com/fmtlib/fmt.git |
| 添加依赖源       | `./Paker add-remote mylib https://git.example.com/mylib.git` | 添加/更新自定义依赖源 | Added/Updated remote: mylib -> ... |
| 移除依赖源       | `./Paker remove-remote mylib`    | 移除自定义依赖源             | Removed remote: mylib |
| 同步/刷新依赖    | `./Paker update`                 | git pull 同步本地依赖         | Updating fmt...\nUpdate complete. |
| 清理无用/损坏依赖| `./Paker clean`                  | 清理未声明或损坏的依赖包      | Removing unused package: ...\nClean complete. |
| **显示包安装记录** | `./Paker record-show fmt`        | 显示指定包的安装记录         | Package: fmt<br>Install Path: packages/fmt<br>Files (156): ... |
| **列出所有包记录** | `./Paker record-list`            | 列出所有已安装的包记录       | Installed packages (3):<br>  fmt (156 files)<br>  spdlog (89 files) |
| **获取包文件列表** | `./Paker record-files fmt`       | 获取指定包的所有文件列表     | Files for package 'fmt':<br>  packages/fmt/src/format.cc<br>  packages/fmt/include/fmt/format.h |

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
# 1. 初始化项目
./Paker init

# 2. 添加自定义依赖源
./Paker add-remote mylib https://github.com/example/mylib.git

# 3. 添加依赖包（显示进度条）
./Paker add fmt
./Paker add spdlog

# 4. 查看依赖列表（表格化显示）
./Paker list

# 5. 查看依赖树（优化显示）
./Paker tree

# 6. 查看安装记录
./Paker record-list
./Paker record-show fmt

# 7. 使用 CLI 选项
./Paker --no-color list          # 禁用彩色输出
./Paker -v add fmt              # 启用详细模式
./Paker --no-color -v search json  # 组合使用

# 5. 查看依赖树
./Paker tree

# 6. 锁定依赖版本
./Paker lock

# 7. 查看包文件列表
./Paker record-files fmt

# 8. 移除不需要的包
./Paker remove spdlog
```

### 高级功能示例
```bash
# 递归安装依赖
./Paker add-recursive mylib

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

# 使用详细模式查看安装过程
./Paker -v add fmt

# 禁用彩色输出（适用于脚本或管道）
./Paker --no-color list | grep "installed"
```

## 依赖
- [CLI11](https://github.com/CLIUtils/CLI11) (已集成头文件)
- [nlohmann/json](https://github.com/nlohmann/json) (已集成头文件)
- [glog](https://github.com/google/glog)
- [GoogleTest](https://github.com/google/googletest)

## License
MIT 
