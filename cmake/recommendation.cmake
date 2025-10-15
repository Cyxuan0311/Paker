# 智能推荐系统CMake配置

# 添加推荐引擎源文件
set(RECOMMENDATION_SOURCES
    src/Paker/analysis/recommendation_engine.cpp
    src/Paker/analysis/smart_recommendation_service.cpp
)

# 添加推荐引擎头文件
set(RECOMMENDATION_HEADERS
    include/Paker/analysis/recommendation_engine.h
    include/Paker/analysis/smart_recommendation_service.h
)

# 添加推荐引擎依赖
set(RECOMMENDATION_DEPENDENCIES
    Paker::Analysis::ProjectAnalyzer
    Paker::Analysis::ProjectTypeConfig
    Paker::Core::Output
    ${CURL_LIBRARIES}
    jsoncpp
)

# 创建推荐引擎库
add_library(PakerRecommendation
    ${RECOMMENDATION_SOURCES}
    ${RECOMMENDATION_HEADERS}
)

# 设置目标属性
set_target_properties(PakerRecommendation PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# 链接依赖库
target_link_libraries(PakerRecommendation
    ${RECOMMENDATION_DEPENDENCIES}
)

# 包含目录
target_include_directories(PakerRecommendation
    PUBLIC
        include
    PRIVATE
        src
)

# 编译选项
target_compile_options(PakerRecommendation
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O2
)

# 如果启用测试
if(BUILD_TESTS)
    # 添加推荐系统测试
    add_executable(test_enhanced_recommendation
        test/analysis/test_enhanced_recommendation.cpp
    )
    
    target_link_libraries(test_enhanced_recommendation
        PakerRecommendation
        gtest
        gtest_main
    )
    
    # 运行测试
    add_test(NAME EnhancedRecommendationTest
        COMMAND test_enhanced_recommendation
    )
endif()

# 如果启用示例
if(BUILD_EXAMPLES)
    # 添加智能推荐示例
    add_executable(smart_recommendation_example
        examples/smart_recommendation_example.cpp
    )
    
    target_link_libraries(smart_recommendation_example
        PakerRecommendation
    )
    
    # 设置示例输出目录
    set_target_properties(smart_recommendation_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
endif()

# 安装配置
install(TARGETS PakerRecommendation
    EXPORT PakerTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${RECOMMENDATION_HEADERS}
    DESTINATION include/Paker/analysis
)

# 导出目标
export(TARGETS PakerRecommendation
    FILE ${CMAKE_CURRENT_BINARY_DIR}/PakerRecommendationTargets.cmake
)

# 创建配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PakerRecommendationConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PakerRecommendationConfig.cmake
    @ONLY
)

# 安装配置文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PakerRecommendationConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PakerRecommendationTargets.cmake
    DESTINATION lib/cmake/PakerRecommendation
)
