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