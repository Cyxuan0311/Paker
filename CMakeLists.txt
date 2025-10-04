cmake_minimum_required(VERSION 3.10)
project(Paker)

set(CMAKE_CXX_STANDARD 17)

# 启用预编译头文件
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 启用OpenMP支持
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()

# 检查编译器是否支持预编译头文件
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winvalid-pch")
endif()

# 启用SIMD指令集支持
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 检查CPU特性并启用相应的SIMD指令集
    include(CheckCXXCompilerFlag)
    
    # 检查SSE2支持
    check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
    if(COMPILER_SUPPORTS_SSE2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    endif()
    
    # 检查SSE4.2支持
    check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
    if(COMPILER_SUPPORTS_SSE42)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
    endif()
    
    # 检查AVX2支持
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    if(COMPILER_SUPPORTS_AVX2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    endif()
    
    # 检查AVX512支持
    check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
    if(COMPILER_SUPPORTS_AVX512)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f")
    endif()
endif()

# 查找glog库
find_package(glog REQUIRED)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

# 查找CURL库
find_package(CURL REQUIRED)

# 查找zlib库
find_package(ZLIB REQUIRED)

# 查找OpenMP库
find_package(OpenMP REQUIRED)

file(GLOB_RECURSE PAKER_SRCS src/Paker/*.cpp)
file(GLOB RECORDER_SRCS src/Recorder/*.cpp)
file(GLOB NETWORK_SRCS src/Paker/network/*.cpp)
file(GLOB SIMD_SRCS src/Paker/simd/*.cpp)

add_executable(Paker
    src/main.cpp
    src/builtin_repos.cpp
    ${PAKER_SRCS}
    ${RECORDER_SRCS}
    ${NETWORK_SRCS}
    ${SIMD_SRCS}
)

target_include_directories(Paker PRIVATE include include/Paker include/Recorder include/third_party)

# 设置预编译头文件
target_precompile_headers(Paker PRIVATE include/Paker/pch.h)

# 修复 std::filesystem 链接错误
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(Paker stdc++fs glog::glog OpenSSL::SSL OpenSSL::Crypto CURL::libcurl ZLIB::ZLIB OpenMP::OpenMP_CXX)
else()
    target_link_libraries(Paker glog::glog OpenSSL::SSL OpenSSL::Crypto CURL::libcurl ZLIB::ZLIB OpenMP::OpenMP_CXX)
endif()

# 设置安装目标
install(TARGETS Paker
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/Paker/
    DESTINATION include/Paker
    FILES_MATCHING PATTERN "*.h"
)

# 安装文档
install(DIRECTORY docs/
    DESTINATION share/doc/paker
    FILES_MATCHING PATTERN "*.md"
)

# 安装示例
install(DIRECTORY examples/
    DESTINATION share/paker/examples
    FILES_MATCHING PATTERN "*.cpp"
)

# 安装脚本
install(DIRECTORY scripts/
    DESTINATION share/paker/scripts
    FILES_MATCHING PATTERN "*.sh" PATTERN "*.py"
)

# 安装图标
install(FILES icon/logo.png
    DESTINATION share/paker/icons
)

# 安装配置文件模板
install(FILES Paker.json.template
    DESTINATION share/paker/templates
)

# 设置包信息
set(CPACK_PACKAGE_NAME "Paker")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "现代化 C++ 包管理器")
set(CPACK_PACKAGE_VENDOR "Paker Team")
set(CPACK_PACKAGE_CONTACT "paker@example.com")

# 设置包类型
set(CPACK_GENERATOR "DEB;RPM;TGZ;ZIP")

# 设置包组件
set(CPACK_COMPONENTS_ALL runtime development documentation examples)

# 设置组件描述
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Paker Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Paker 运行时组件")

set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Paker Development")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Paker 开发组件")

set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Paker Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Paker 文档")

set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Paker Examples")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Paker 示例代码")

# 设置安装权限
install(CODE "
    execute_process(
        COMMAND chmod +x /usr/local/share/paker/scripts/*.sh
        RESULT_VARIABLE chmod_result
    )
    if(chmod_result)
        message(WARNING \"Failed to set execute permissions on scripts\")
    endif()
")

# 创建卸载脚本
install(CODE "
    file(WRITE \${CMAKE_INSTALL_PREFIX}/share/paker/scripts/uninstall.sh \"#!/bin/bash
# Paker 卸载脚本
echo \"正在卸载 Paker...\"

# 删除可执行文件
if [ -f \${CMAKE_INSTALL_PREFIX}/bin/Paker ]; then
    rm -f \${CMAKE_INSTALL_PREFIX}/bin/Paker
    echo \"已删除可执行文件\"
fi

# 删除头文件
if [ -d \${CMAKE_INSTALL_PREFIX}/include/Paker ]; then
    rm -rf \${CMAKE_INSTALL_PREFIX}/include/Paker
    echo \"已删除头文件\"
fi

# 删除文档
if [ -d \${CMAKE_INSTALL_PREFIX}/share/doc/paker ]; then
    rm -rf \${CMAKE_INSTALL_PREFIX}/share/doc/paker
    echo \"已删除文档\"
fi

# 删除示例和脚本
if [ -d \${CMAKE_INSTALL_PREFIX}/share/paker ]; then
    rm -rf \${CMAKE_INSTALL_PREFIX}/share/paker
    echo \"已删除示例和脚本\"
fi

echo \"Paker 卸载完成\"
\")
    execute_process(COMMAND chmod +x \${CMAKE_INSTALL_PREFIX}/share/paker/scripts/uninstall.sh)
")

# 包含 CPack
include(CPack) 