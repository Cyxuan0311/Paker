# Paker 智能包推荐指南

## 🧠 概述

Paker 智能包推荐系统是一个基于项目分析和GitHub集成的AI驱动推荐引擎，能够自动分析您的项目特征，结合GitHub热门项目和相似项目数据，提供个性化的包推荐建议。

## ✨ 核心特性

### 🎯 智能分析
- **项目类型检测**: 自动识别Web应用、桌面应用、游戏引擎等
- **构建系统分析**: 检测CMake、Make、Meson等构建系统
- **C++标准检测**: 自动识别C++11/14/17/20标准
- **依赖关系分析**: 分析现有依赖和潜在冲突

### 🧠 智能推荐
- **上下文感知**: 基于项目特征提供相关推荐
- **GitHub集成**: 基于GitHub热门项目和相似项目推荐
- **多维度过滤**: 支持按类别、性能、安全要求过滤
- **智能排序**: 基于兼容性、流行度、维护性排序
- **个性化建议**: 根据项目需求提供定制化推荐
- **实时数据**: 基于GitHub API实时获取最新推荐数据

## 🚀 使用方法

### 基本命令
```bash
# 分析项目并获取推荐
Paker suggestion

# 显示详细分析
Paker suggestion --detailed

# 自动安装推荐包
Paker suggestion --auto-install

# 导出分析结果
Paker suggestion --export analysis.json
```

### 高级过滤
```bash
# 按类别过滤
Paker suggestion --category web
Paker suggestion --category desktop
Paker suggestion --category game

# 按性能要求过滤
Paker suggestion --performance high
Paker suggestion --performance medium
Paker suggestion --performance low

# 按安全要求过滤
Paker suggestion --security high
Paker suggestion --security medium
Paker suggestion --security low

# 组合过滤
Paker suggestion --category web --performance high --security medium
```

## 📊 输出格式

### 简洁模式
```bash
Paker suggestion
# 输出:
# 🎯 智能包推荐
# ┌─────────────────────────────────────────────────────────────┐
# │ 包名           │ 推荐理由           │ 优先级 │ 兼容性 │ 流行度 │
# ├─────────────────────────────────────────────────────────────┤
# │ boost-beast    │ 高性能HTTP库       │ 高     │ 95%    │ 90%    │
# │ crow           │ 轻量级Web框架      │ 高     │ 90%    │ 85%    │
# │ gtest          │ 单元测试框架       │ 高     │ 95%    │ 95%    │
# └─────────────────────────────────────────────────────────────┘
```

### 详细模式
```bash
Paker suggestion --detailed
# 输出:
# 📋 项目分析结果:
# ┌─────────────────────────────────────┐
# │ 项目类型: Web应用                   │
# │ 构建系统: CMake                     │
# │ C++标准: C++17                      │
# │ 性能要求: 高                        │
# │ 安全要求: 中                        │
# │ 测试要求: 高                        │
# └─────────────────────────────────────┘
# 
# 🎯 智能包推荐:
# 1. boost-beast (推荐度: ⭐⭐⭐⭐⭐)
#    ├── 描述: 高性能HTTP和WebSocket库
#    ├── 推荐理由: 适合高性能Web应用
#    ├── 类别: web
#    ├── 兼容性: 95%
#    ├── 流行度: 90%
#    ├── 维护性: 85%
#    ├── 优先级: high
#    └── 安装: Paker add boost-beast
```

## 🔧 项目分析维度

### 项目类型检测
- **Web应用**: 检测HTTP库、Web框架使用
- **桌面应用**: 检测GUI框架、窗口系统
- **游戏引擎**: 检测图形库、游戏框架
- **嵌入式系统**: 检测RTOS、微控制器相关代码
- **科学计算**: 检测数学库、数值计算
- **机器学习**: 检测AI框架、计算机视觉库

### 构建系统分析
- **CMake**: 检测CMakeLists.txt文件
- **Make**: 检测Makefile文件
- **Meson**: 检测meson.build文件
- **Conan**: 检测conanfile.txt/py文件
- **Vcpkg**: 检测vcpkg.json文件

### C++标准检测
- **C++20**: 检测concepts、requires、ranges等特性
- **C++17**: 检测optional、variant、any等特性
- **C++14**: 检测auto、decltype、make_unique等特性
- **C++11**: 检测基础C++11特性

### 需求评估
- **性能要求**: 基于高性能库使用情况评估
- **安全要求**: 基于加密、认证库使用情况评估
- **测试要求**: 基于测试框架使用情况评估

## 🎯 推荐算法

### 多维度推荐策略
1. **基于项目类型**: 根据项目类型推荐相关包
2. **基于现有依赖**: 推荐与现有依赖相关的包
3. **基于性能要求**: 根据性能需求推荐高性能包
4. **基于安全要求**: 根据安全需求推荐安全包
5. **基于测试要求**: 根据测试需求推荐测试框架
6. **基于GitHub热门包**: 推荐GitHub上热门的包
7. **基于相似项目**: 推荐相似项目使用的包
8. **基于C++标准**: 根据C++标准推荐兼容包
9. **基于构建系统**: 根据构建系统推荐相关包
10. **基于项目复杂度**: 根据项目复杂度推荐包
11. **基于项目特征**: 基于项目具体特征推荐包
12. **基于代码模式**: 根据代码模式推荐相关包

### 智能排序算法
```python
# 推荐评分计算（十二维度权重系统）
score = (
    confidence * 0.3 +      # 置信度权重30%
    compatibility * 0.25 +   # 兼容性权重25%
    popularity * 0.2 +      # 流行度权重20%
    maintenance * 0.15 +    # 维护性权重15%
    priority_bonus * 0.1    # 优先级奖励10%
)

# 权重调整（基于推荐源）
if source == "github_trending":
    confidence *= 1.3      # GitHub热门包权重更高
elif source == "similar_project":
    confidence *= 1.2      # 相似项目推荐权重稍高
elif source == "project_type":
    confidence *= 1.4      # 项目类型推荐权重最高
```

### 过滤机制
- **类别过滤**: 按包类别过滤推荐
- **性能过滤**: 按性能要求过滤推荐
- **安全过滤**: 按安全要求过滤推荐
- **兼容性过滤**: 过滤不兼容的包

## 📈 使用场景

### 1. 新项目初始化
```bash
# 分析新项目并获取推荐
Paker suggestion --detailed
# 根据推荐安装包
Paker suggestion --auto-install
```

### 2. 现有项目优化
```bash
# 分析现有项目
Paker suggestion --category web --performance high
# 获取特定类别的推荐
Paker suggestion --category testing
```

### 3. 项目迁移
```bash
# 分析项目特征
Paker suggestion --export analysis.json
# 根据分析结果进行迁移规划
```

### 4. 团队协作
```bash
# 导出项目分析
Paker suggestion --export team-analysis.json
# 团队成员可以基于分析结果统一包选择
```

## 🛠️ 配置选项

### 环境变量
```bash
# 启用详细分析
export PAKER_SUGGESTION_DETAILED=true

# 设置推荐数量限制
export PAKER_SUGGESTION_LIMIT=10

# 启用自动安装
export PAKER_SUGGESTION_AUTO_INSTALL=true

# GitHub API配置
export GITHUB_TOKEN=your_github_token_here

# 启用GitHub集成
export PAKER_GITHUB_INTEGRATION=true

# 设置GitHub API超时
export PAKER_GITHUB_TIMEOUT=30
```

### 配置文件
```json
{
  "suggestion": {
    "enabled": true,
    "detailed": false,
    "auto_install": false,
    "export_format": "json",
    "github_integration": true,
    "github_token": "",
    "github_timeout": 30,
    "filters": {
      "category": "",
      "performance": "",
      "security": ""
    }
  }
}
```

## 🔍 故障排除

### 常见问题

#### 1. 分析失败
```bash
# 检查是否在Paker项目中
ls Paker.json

# 检查项目文件权限
ls -la
```

#### 2. 推荐不准确
```bash
# 更新包知识库
Paker cache update

# 重新分析项目
Paker suggestion --detailed
```

#### 3. 性能问题
```bash
# 限制分析深度
export PAKER_ANALYSIS_DEPTH=2

# 使用缓存
export PAKER_USE_CACHE=true

# 禁用GitHub集成（如果网络问题）
export PAKER_GITHUB_INTEGRATION=false
```

#### 4. GitHub API问题
```bash
# 检查GitHub Token
echo $GITHUB_TOKEN

# 测试GitHub连接
curl -H "Authorization: token $GITHUB_TOKEN" https://api.github.com/user

# 增加超时时间
export PAKER_GITHUB_TIMEOUT=60
```

### 调试模式
```bash
# 启用调试输出
export PAKER_DEBUG=true

# 详细分析
Paker suggestion --detailed --export debug.json
```

## 📚 最佳实践

### 1. 项目结构
- 保持清晰的目录结构
- 使用标准的构建系统
- 明确的项目类型标识

### 2. 依赖管理
- 定期更新依赖
- 使用版本锁定
- 避免循环依赖

### 3. 性能优化
- 根据需求选择合适的包
- 考虑包的维护性
- 平衡功能与性能

### 4. 安全考虑
- 选择活跃维护的包
- 定期安全审计
- 使用可信的包源

## 🚀 未来规划

### 短期目标
- **机器学习集成**: 基于用户行为优化推荐
- **实时分析**: 支持实时项目分析
- **团队协作**: 支持团队推荐共享
- **GitHub深度集成**: 更丰富的GitHub数据分析

### 长期目标
- **AI驱动**: 深度学习的智能推荐
- **云端分析**: 基于云端数据的推荐
- **生态系统**: 完整的包推荐生态系统
- **多平台集成**: 集成更多代码托管平台

## 📖 相关文档

- [命令行使用指南](COMMAND_LINE_USAGE.md)
- [命令参考](COMMAND_REFERENCE.md)
- [功能特性详解](FEATURES.md)
- [补全指南](COMPLETION_GUIDE.md)

---

**智能推荐让包选择更智能，让开发更高效！** 🚀

## 🔗 GitHub集成特性

### GitHub项目分析
- **自动检测**: 自动检测项目是否为GitHub项目
- **项目信息**: 获取stars、forks、watchers、语言等
- **热门包推荐**: 基于GitHub stars排序推荐热门包
- **相似项目**: 查找相似项目并推荐其使用的包

### GitHub API配置
```bash
# 设置GitHub Token（可选，提高API限制）
export GITHUB_TOKEN=your_github_token_here

# 启用GitHub集成
export PAKER_GITHUB_INTEGRATION=true
```

### 智能包名提取
- **前缀移除**: 自动移除lib-、boost-、cpp-等前缀
- **后缀处理**: 处理-cpp、-cxx、-library等后缀
- **用户名分离**: 从user/repo格式中提取包名
- **智能匹配**: 基于项目类型匹配相关包

**GitHub集成让推荐更智能，让包选择更精准！** 🌟
