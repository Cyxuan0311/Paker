# Paker

<!-- GitHub Badges -->
<p align="left">
  <a href="https://github.com/Cyxuan0311/Paker/stargazers"><img src="https://img.shields.io/github/stars/Cyxuan0311/Paker?style=social" alt="Stars"></a>
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
