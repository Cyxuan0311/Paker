cmake_minimum_required(VERSION 3.10)
project(Paker)

set(CMAKE_CXX_STANDARD 17)

# 查找glog库
find_package(glog REQUIRED)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE PAKER_SRCS src/Paker/*.cpp)
file(GLOB RECORDER_SRCS src/Recorder/*.cpp)

add_executable(Paker
    src/main.cpp
    src/builtin_repos.cpp
    ${PAKER_SRCS}
    ${RECORDER_SRCS}
)

target_include_directories(Paker PRIVATE include include/Paker include/Recorder include/third_party)

# 修复 std::filesystem 链接错误
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(Paker stdc++fs glog::glog OpenSSL::SSL OpenSSL::Crypto)
else()
    target_link_libraries(Paker glog::glog OpenSSL::SSL OpenSSL::Crypto)
endif() 