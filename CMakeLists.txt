cmake_minimum_required(VERSION 3.10)
project(Paker)

set(CMAKE_CXX_STANDARD 17)

# 查找glog库
find_package(glog REQUIRED)

file(GLOB PAKER_SRCS src/Paker/*.cpp)

add_executable(Paker
    src/main.cpp
    src/cli.cpp
    src/package_manager.cpp
    src/builtin_repos.cpp
    ${PAKER_SRCS}
)

target_include_directories(Paker PRIVATE include include/Paker)

# 修复 std::filesystem 链接错误
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(Paker stdc++fs glog::glog)
else()
    target_link_libraries(Paker glog::glog)
endif() 