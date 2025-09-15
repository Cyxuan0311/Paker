cmake_minimum_required(VERSION 3.10)
project(PakerTests)

set(CMAKE_CXX_STANDARD 17)

# 查找gtest
find_package(GTest REQUIRED)

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ../include
)

# 单元测试
add_executable(PakerUnitTests
    unit/test_package_manager.cpp
    unit/test_record.cpp
    unit/test_dependency_resolution.cpp
    unit/test_monitoring.cpp
    unit/test_cache_manager.cpp
    unit/test_rollback.cpp
    unit/test_memory_management.cpp
    unit/test_service_architecture.cpp
    unit/test_incremental_parser.cpp
)

# 集成测试
add_executable(PakerIntegrationTests
    integration/test_integration.cpp
)

target_link_libraries(PakerUnitTests
    GTest::GTest
    GTest::Main
    pthread
)

target_link_libraries(PakerIntegrationTests
    GTest::GTest
    GTest::Main
    pthread
) 