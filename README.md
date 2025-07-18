# Paker

<!-- 技术栈标签 -->
<p align="left">
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/build-CMake-brightgreen.svg" alt="CMake">
  <img src="https://img.shields.io/badge/cli-CLI11-orange.svg" alt="CLI11">
  <img src="https://img.shields.io/badge/json-nlohmann--json-yellow.svg" alt="nlohmann/json">
  <img src="https://img.shields.io/badge/log-glog-lightgrey.svg" alt="glog">
  <img src="https://img.shields.io/badge/test-GoogleTest-red.svg" alt="GoogleTest">
</p>

Paker 是一个用 C++ 编写的轻量级 C++ 包管理器，支持本地依赖管理、自定义依赖源和丰富的命令行操作。

## 目录结构
```
Paker/
  include/
    Paker/
      install.h  list.h  lock.h  info.h  update.h  utils.h  sources.h
    ...
  src/
    Paker/
      install.cpp  list.cpp  lock.cpp  info.cpp  update.cpp  utils.cpp  sources.cpp
    builtin_repos.cpp  cli.cpp  main.cpp  package_manager.cpp
  test/
  build/
  Paker.json           # 依赖、项目元信息和自定义依赖源都在此
  CMakeLists.txt
  README.md
  LICENSE
  .gitignore
  .clang-format
```

## 使用示例与命令行功能

| 功能             | 命令示例                        | 说明                         | 示例输出 |
|------------------|----------------------------------|------------------------------|----------|
| 初始化项目       | `./Paker init`                   | 初始化依赖管理               | Initialized Paker project. |
| 添加依赖         | `./Paker add fmt`                | 添加依赖包（支持自定义源）   | Added dependency: fmt |
| 递归添加依赖     | `./Paker add-recursive fmt`      | 递归安装依赖及其依赖         | Added dependency: fmt ... |
| 列出依赖         | `./Paker list`                   | 列出所有依赖                 | Dependencies:  fmt: * |
| 移除依赖         | `./Paker remove fmt`             | 移除依赖包                   | Removed dependency: fmt |
| 显示依赖树       | `./Paker tree`                   | 以树状结构展示依赖关系       | - fmt\n  - spdlog |
| 依赖锁定         | `./Paker lock`                   | 生成/更新 Paker.lock         | Generated Paker.lock |
| 按锁文件安装     | `./Paker install-lock`           | 按 lock 文件安装依赖         | Installed dependencies from Paker.lock |
| 升级所有依赖     | `./Paker upgrade`                | 升级所有依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 升级指定依赖     | `./Paker upgrade fmt`            | 升级指定依赖到最新           | Upgrading fmt to latest...\nUpgrade complete. |
| 搜索依赖包       | `./Paker search fmt`             | 搜索可用依赖包（含自定义源） | fmt    https://github.com/fmtlib/fmt.git |
| 查看依赖信息     | `./Paker info fmt`               | 查看依赖包详细信息           | Package: fmt\nRepo: ... |
| 添加依赖源       | `./Paker add-remote mylib https://git.example.com/mylib.git` | 添加/更新自定义依赖源 | Added/Updated remote: mylib -> ... |
| 移除依赖源       | `./Paker remove-remote mylib`    | 移除自定义依赖源             | Removed remote: mylib |
| 同步/刷新依赖    | `./Paker update`                 | git pull 同步本地依赖         | Updating fmt...\nUpdate complete. |
| 清理无用/损坏依赖| `./Paker clean`                  | 清理未声明或损坏的依赖包      | Removing unused package: ...\nClean complete. |

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

## 依赖
- [CLI11](https://github.com/CLIUtils/CLI11) (已集成头文件)
- [nlohmann/json](https://github.com/nlohmann/json) (已集成头文件)
- [glog](https://github.com/google/glog)
- [GoogleTest](https://github.com/google/googletest)

## License
MIT 
