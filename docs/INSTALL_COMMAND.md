# Paker Install 命令详解

本文档详细介绍了 Paker 的包安装和卸载功能。

## 目录

- [命令概述](#命令概述)
- [install 命令](#install-命令)
- [install-p 命令](#install-p-命令)
- [uninstall 命令](#uninstall-命令)
- [构建系统支持](#构建系统支持)
- [安装记录管理](#安装记录管理)
- [使用示例](#使用示例)
- [故障排除](#故障排除)

## 命令概述

Paker 提供了三个核心的包安装命令：

| 命令 | 功能 | 说明 |
|------|------|------|
| `install` | 单包安装 | 编译并安装单个包到系统 |
| `install-p` | 并行安装 | 同时编译并安装多个包 |
| `uninstall` | 包卸载 | 从系统中卸载已安装的包 |

## install 命令

### 语法
```bash
Paker install <package>
```

### 功能
- 自动检测包的构建系统（CMake、Make、Ninja、Meson、Autotools）
- 编译包到临时目录
- 安装包到系统目录（~/.local 或系统目录）
- 记录安装信息到 `.paker/record/Record_Installing.json`

### 支持的构建系统
- **CMake**：检测 `CMakeLists.txt`
- **Make**：检测 `Makefile` 或 `makefile`
- **Ninja**：检测 `build.ninja`
- **Meson**：检测 `meson.build`
- **Autotools**：检测 `configure` 或 `configure.ac`

### 安装流程
1. 检查包是否存在于 `packages/` 目录
2. 检测构建系统类型
3. 创建构建和安装目录
4. 配置和编译包（静默模式）
5. 安装包到系统目录
6. 收集安装的文件列表
7. 记录安装信息

## install-p 命令

### 语法
```bash
Paker install-p <package1> <package2> [package3...]
```

### 功能
- 并行编译和安装多个包
- 使用 `ParallelExecutor` 实现真正的并行处理
- 每个包的安装过程独立进行
- 自动处理包之间的依赖关系

### 性能优势
- **并行编译**：同时编译多个包，充分利用多核CPU
- **异步处理**：不阻塞其他包的安装过程
- **智能调度**：根据系统负载自动调整并发数

## uninstall 命令

### 语法
```bash
Paker uninstall <package>
```

### 功能
- 从安装记录中读取包信息
- 删除所有已安装的文件
- 清理安装目录
- 更新安装记录

### 安全特性
- **精确删除**：只删除记录中的文件，避免误删
- **目录清理**：删除空的安装目录
- **记录更新**：从记录中移除包信息

## 构建系统支持

### CMake 项目
```bash
# 自动检测 CMakeLists.txt
Paker install fmt
```

**构建流程：**
1. 创建 `build` 目录
2. 运行 `cmake -DCMAKE_INSTALL_PREFIX=...`
3. 运行 `make -j$(nproc)`
4. 运行 `make install`

### Make 项目
```bash
# 自动检测 Makefile
Paker install mylib
```

**构建流程：**
1. 运行 `make -j$(nproc)`
2. 运行 `make install`

### Ninja 项目
```bash
# 自动检测 build.ninja
Paker install ninja-project
```

**构建流程：**
1. 运行 `ninja`
2. 运行 `ninja install`

### Meson 项目
```bash
# 自动检测 meson.build
Paker install meson-project
```

**构建流程：**
1. 运行 `meson setup --prefix=...`
2. 运行 `ninja`
3. 运行 `ninja install`

### Autotools 项目
```bash
# 自动检测 configure
Paker install autotools-project
```

**构建流程：**
1. 运行 `./configure --prefix=...`
2. 运行 `make -j$(nproc)`
3. 运行 `make install`

## 安装记录管理

### 记录文件位置
```
.paker/record/Record_Installing.json
```

### 记录格式
```json
{
    "package_name": {
        "build_system": "detected",
        "install_path": "/home/user/.local",
        "install_time": 1760433279,
        "installed_files": [
            "/home/user/.local/include/package/header.h",
            "/home/user/.local/lib/libpackage.a"
        ]
    }
}
```

### 记录字段说明
- **build_system**：检测到的构建系统类型
- **install_path**：系统安装路径
- **install_time**：安装时间戳
- **installed_files**：安装的文件列表

## 使用示例

### 基本安装
```bash
# 安装单个包
Paker install fmt

# 安装多个包
Paker install-p fmt spdlog nlohmann-json
```

### 包管理
```bash
# 查看已安装的包
cat .paker/record/Record_Installing.json

# 卸载包
Paker uninstall fmt
```

### 开发工作流
```bash
# 1. 添加依赖
Paker add fmt

# 2. 编译并安装到系统
Paker install fmt

# 3. 在项目中使用
# #include <fmt/format.h>

# 4. 清理时卸载
Paker uninstall fmt
```

## 故障排除

### 常见问题

#### 1. 包未找到
```
Package not found: fmt, please use 'Paker add' to download first
```
**解决方案：** 先使用 `Paker add fmt` 下载包

#### 2. 构建失败
```
Build failed: fmt
```
**解决方案：** 
- 检查包是否完整下载
- 检查系统是否安装了必要的构建工具
- 查看包的构建要求

#### 3. 权限问题
```
Failed to install to system: Permission denied
```
**解决方案：** 
- 检查目标目录的写权限
- 使用用户目录安装（~/.local）

#### 4. 路径问题
```
CMake Error: The source directory does not exist
```
**解决方案：** 
- 确保包在 `packages/` 目录下
- 检查包目录结构是否完整

### 调试模式
```bash
# 启用详细输出
Paker --verbose install fmt

# 查看构建日志
Paker install fmt 2>&1 | tee build.log
```

### 清理和重置
```bash
# 清理构建目录
rm -rf packages/*/build packages/*/install

# 重置安装记录
rm -f .paker/record/Record_Installing.json
```

## 最佳实践

### 1. 包管理策略
- 使用 `Paker add` 下载包到项目
- 使用 `Paker install` 安装到系统
- 定期清理不需要的包

### 2. 性能优化
- 使用 `install-p` 并行安装多个包
- 避免重复安装相同的包
- 定期清理构建缓存

### 3. 系统集成
- 将 `~/.local/bin` 添加到 PATH
- 将 `~/.local/lib` 添加到 LD_LIBRARY_PATH
- 配置 pkg-config 路径

### 4. 版本管理
- 记录安装的包版本
- 使用锁定文件确保一致性
- 定期更新和升级包

## 技术实现

### 核心组件
- **BuildSystem 检测器**：自动识别构建系统
- **ParallelExecutor**：并行任务执行器
- **FileTracker**：文件跟踪和记录
- **SystemInstaller**：系统安装管理器

### 性能特性
- **静默构建**：隐藏构建输出，提供清洁界面
- **并行处理**：多包同时安装
- **智能路径**：自动选择最优安装路径
- **增量安装**：避免重复安装

### 安全特性
- **精确跟踪**：记录所有安装的文件
- **安全删除**：只删除记录的文件
- **权限检查**：验证安装权限
- **冲突检测**：避免文件覆盖冲突
