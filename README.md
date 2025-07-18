# Paker

<!-- GitHub Badges -->
<p align="left">
  <a href="https://github.com/Cyxuan0311/Paker.git/stargazers"><img src="https://img.shields.io/github/stars/Cyxuan0311/Paker?style=social" alt="Stars"></a>
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

## 使用示例与命令行功能

| 功能             | 命令示例                        | 说明                         | 示例输出 |
|------------------|----------------------------------|------------------------------|----------|
| 初始化项目       | `./Paker init`                   | 初始化依赖管理               | Initialized Paker project. |
| 添加依赖         | `./Paker add fmt`                | 添加依赖包                   | Added dependency: fmt |
| 递归添加依赖     | `./Paker add-recursive fmt`      | 递归安装依赖及其依赖         | Added dependency: fmt ... |
| 列出依赖         | `./Paker list`                   | 列出所有依赖                 | Dependencies:  fmt: * |
| 移除依赖         | `./Paker remove fmt`             | 移除依赖包                   | Removed dependency: fmt |
| 显示依赖树       | `./Paker tree`                   | 以树状结构展示依赖关系       | - fmt\n  - spdlog |
| 依赖锁定         | `./Paker lock`                   | 生成/更新 Paker.lock         | Generated Paker.lock |
| 按锁文件安装     | `./Paker install-lock`           | 按 lock 文件安装依赖         | Installed dependencies from Paker.lock |
| 升级所有依赖     | `./Paker upgrade`                | 升级所有依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 升级指定依赖     | `./Paker upgrade fmt`            | 升级指定依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 搜索依赖包       | `./Paker search fmt`             | 搜索可用依赖包               | fmt    https://github.com/fmtlib/fmt.git |
| 查看依赖信息     | `./Paker info fmt`               | 查看依赖包详细信息           | Package: fmt\nRepo: ... |
| 同步/刷新依赖    | `./Paker update`                 | git pull 同步本地依赖         | Updating fmt...\nUpdate complete. |
| 清理无用/损坏依赖| `./Paker clean`                  | 清理未声明或损坏的依赖包      | Removing unused package: ...\nClean complete. |

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
