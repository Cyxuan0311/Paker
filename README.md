# Paker

<!-- GitHub Badges -->
<p align="left">
  <a href="https://github.com/Cyxuan0311/Paker.git/stargazers"><img src="https://img.shields.io/github/stars/Cyxuan0311/Paker?style=social" alt="Stars"></a>
  <a href="https://github.com/Cyxuan0311/Paker/issues"><img src="https://img.shields.io/github/issues/Cyxuan0311/Paker" alt="Issues"></a>
  <a href="https://github.com/Cyxuan0311/Paker/pulls"><img src="https://img.shields.io/github/issues-pr/Cyxuan0311/Paker" alt="Pull Requests"></a>
  <a href="https://github.com/Cyxuan0311/Paker/network/members"><img src="https://img.shields.io/github/forks/Cyxuan0311/Paker?style=social" alt="Forks"></a>
  <a href="https://github.com/Cyxuan0311/Paker/blob/main/LICENSE"><img src="https://img.shields.io/github/license/Cyxuan0311/Paker" alt="License"></a>
</p>

Paker 是一个用 C++ 编写的轻量级 C++ 包管理器，支持本地依赖管理和命令行操作。

## 特性
- 初始化 C++ 项目依赖管理
- 添加/移除/列出依赖
- 采用 CLI11 作为命令行解析库
- 采用 nlohmann/json 作为配置文件解析

## 快速开始

```bash
# 构建
mkdir build && cd build
cmake ..
make
```

## 使用示例

### 初始化项目
```bash
./Paker init
```
输出：
```
Initialized Paker project.
```

### 添加依赖
```bash
./Paker add fmt
```
输出：
```
Added dependency: fmt
```

### 列出依赖
```bash
./Paker list
```
输出：
```
Dependencies:
  fmt: *
```

### 移除依赖
```bash
./Paker remove fmt
```
输出：
```
Removed dependency: fmt
```

## 命令行功能

### 初始化项目
```bash
./Paker init
```

### 添加依赖
```bash
./Paker add fmt
```

### 递归添加依赖及其依赖
```bash
./Paker add-recursive fmt
```

### 列出依赖
```bash
./Paker list
```

### 移除依赖
```bash
./Paker remove fmt
```

### 显示依赖树
```bash
./Paker tree
```

### 依赖版本锁定与复现
- 生成/更新 lock 文件
```bash
./Paker lock
```
- 按 lock 文件安装依赖
```bash
./Paker install-lock
```

### 升级依赖
- 升级所有依赖到最新
```bash
./Paker upgrade
```
- 升级指定依赖
```bash
./Paker upgrade fmt
```

### 搜索可用依赖包
```bash
./Paker search fmt
```

### 查看依赖包信息
```bash
./Paker info fmt
```

### 同步/刷新本地依赖（git pull）
```bash
./Paker update
```

### 清理未使用或损坏的依赖包
```bash
./Paker clean
```

## 目录结构
```
Paker/
  src/
  include/
  test/
  CMakeLists.txt
  README.md
  .gitignore
  .clang-format
  LICENSE
```

## 依赖
- [CLI11](https://github.com/CLIUtils/CLI11) (已集成头文件)
- [nlohmann/json](https://github.com/nlohmann/json) (已集成头文件)

## License
MIT 