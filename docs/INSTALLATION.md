# Paker 安装指南

## 系统要求

### 最低要求
- **操作系统**: Linux, macOS, Windows (WSL)
- **编译器**: C++17 支持 (GCC 7+, Clang 5+, MSVC 2019+)
- **CMake**: 3.10 或更高版本
- **内存**: 至少 2GB RAM
- **磁盘空间**: 至少 1GB 可用空间

### 推荐配置
- **CPU**: 多核处理器 (4核或更多)
- **内存**: 8GB RAM 或更多
- **磁盘空间**: 5GB 可用空间
- **网络**: 稳定的互联网连接

## 依赖库

### 必需依赖
- **glog**: 日志记录库
- **OpenSSL**: 加密和哈希计算
- **CURL**: 网络下载和HTTP客户端
- **zlib**: 数据压缩和解压缩
- **OpenMP**: 多核并行处理

### 安装依赖

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libglog-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    libomp-dev
```

#### CentOS/RHEL/Fedora
```bash
# CentOS/RHEL
sudo yum install -y \
    gcc-c++ \
    cmake \
    glog-devel \
    openssl-devel \
    libcurl-devel \
    zlib-devel \
    libomp-devel

# Fedora
sudo dnf install -y \
    gcc-c++ \
    cmake \
    glog-devel \
    openssl-devel \
    libcurl-devel \
    zlib-devel \
    libomp-devel
```

#### macOS
```bash
# 使用 Homebrew
brew install cmake glog openssl curl zlib libomp
```

## 安装方法

### 方法1: 从源码编译安装

#### 1. 克隆仓库
```bash
git clone https://github.com/your-username/paker.git
cd paker
```

#### 2. 创建构建目录
```bash
mkdir build
cd build
```

#### 3. 配置构建
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### 4. 编译
```bash
make -j$(nproc)
```

#### 5. 安装
```bash
sudo make install
```

### 方法2: 使用包管理器

#### 创建包
```bash
# 在构建目录中
cpack -G DEB  # 创建 .deb 包
cpack -G RPM  # 创建 .rpm 包
cpack -G TGZ  # 创建 .tar.gz 包
```

#### 安装包
```bash
# Ubuntu/Debian
sudo dpkg -i Paker-0.1.0-Linux.deb

# CentOS/RHEL
sudo rpm -i Paker-0.1.0-Linux.rpm
```

## 安装后配置

### 1. 验证安装
```bash
Paker --version
```

### 2. 初始化配置
```bash
# 创建项目目录
mkdir my-project
cd my-project

# 初始化 Paker 项目
Paker init

# 查看生成的配置文件
cat Paker.json
```

### 3. 配置环境变量
```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export PAKER_CACHE_DIR="$HOME/.paker/cache"
export PAKER_CONFIG_DIR="$HOME/.paker/config"
export PAKER_LOG_LEVEL="INFO"
```

## 安装目录结构

安装完成后，Paker 的文件将分布在以下位置：

```
/usr/local/
├── bin/
│   └── Paker                    # 主可执行文件
├── include/
│   └── Paker/                   # 头文件
│       ├── core/               # 核心功能
│       ├── cache/              # 缓存管理
│       ├── commands/           # 命令模块
│       ├── dependency/         # 依赖管理
│       ├── network/            # 网络优化
│       └── simd/               # SIMD优化
├── share/
│   ├── doc/
│   │   └── paker/              # 文档
│   └── paker/
│       ├── examples/           # 示例代码
│       ├── scripts/            # 脚本工具
│       ├── icons/              # 图标文件
│       └── templates/          # 配置模板
```

## 组件安装

### 选择性安装组件
```bash
# 只安装运行时组件
sudo make install DESTDIR=/tmp/paker-install COMPONENT=runtime

# 只安装开发组件
sudo make install DESTDIR=/tmp/paker-install COMPONENT=development

# 只安装文档
sudo make install DESTDIR=/tmp/paker-install COMPONENT=documentation

# 只安装示例
sudo make install DESTDIR=/tmp/paker-install COMPONENT=examples
```

### 查看可用组件
```bash
cmake --build . --target help | grep install
```

## 卸载

### 方法1: 使用卸载脚本
```bash
sudo /usr/local/share/paker/scripts/uninstall.sh
```

### 方法2: 手动卸载
```bash
# 删除可执行文件
sudo rm -f /usr/local/bin/Paker

# 删除头文件
sudo rm -rf /usr/local/include/Paker

# 删除文档
sudo rm -rf /usr/local/share/doc/paker

# 删除示例和脚本
sudo rm -rf /usr/local/share/paker

# 删除用户配置（可选）
rm -rf ~/.paker
```

## 故障排除

### 常见问题

#### 1. 编译错误
```bash
# 检查编译器版本
gcc --version
g++ --version

# 检查 CMake 版本
cmake --version

# 检查依赖库
pkg-config --cflags --libs glog
pkg-config --cflags --libs openssl
```

#### 2. 链接错误
```bash
# 检查库路径
ldconfig -p | grep glog
ldconfig -p | grep ssl
ldconfig -p | grep curl
```

#### 3. 运行时错误
```bash
# 检查依赖库
ldd /usr/local/bin/Paker

# 设置库路径
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### 调试模式

#### 启用详细日志
```bash
export PAKER_LOG_LEVEL=DEBUG
Paker --verbose
```

#### 检查配置
```bash
Paker config --show
```

## 高级配置

### 自定义安装路径
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/paker
make -j$(nproc)
sudo make install
```

### 交叉编译
```bash
# 设置交叉编译工具链
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
cmake .. -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake
```

### 静态链接
```bash
cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_EXE_LINKER_FLAGS="-static"
```

## 性能优化

### 编译优化
```bash
# 启用所有优化
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
         -DENABLE_SIMD=ON \
         -DENABLE_OPENMP=ON
```

### 运行时优化
```bash
# 设置线程数
export OMP_NUM_THREADS=4

# 启用 SIMD
export PAKER_ENABLE_SIMD=1

# 设置缓存大小
export PAKER_CACHE_SIZE=1GB
```

## 更新

### 从源码更新
```bash
cd paker
git pull
cd build
make -j$(nproc)
sudo make install
```

### 从包更新
```bash
# Ubuntu/Debian
sudo apt update
sudo apt upgrade paker

# CentOS/RHEL
sudo yum update paker
```

## 支持

如果遇到安装问题，请：

1. 检查系统要求是否满足
2. 查看错误日志
3. 参考故障排除部分
4. 提交 Issue 到 GitHub 仓库

## 许可证

Paker 使用 MIT 许可证。详情请参阅 LICENSE 文件。
