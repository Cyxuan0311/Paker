cmake_minimum_required(VERSION 3.10)
project(PakerTests)

set(CMAKE_CXX_STANDARD 17)

# 查找gtest
find_package(GTest REQUIRED)

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ../include
)

add_executable(PakerTests
    test_package_manager.cpp
)

target_link_libraries(PakerTests
    GTest::GTest
    GTest::Main
    pthread
) 