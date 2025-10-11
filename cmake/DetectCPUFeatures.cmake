# CPU特性检测脚本
# 智能检测CPU支持的指令集，避免在虚拟化环境中使用不支持的指令集

function(detect_cpu_features)
    message(STATUS "Starting CPU feature detection...")
    
    # 创建CPU特性检测程序
    set(CPU_DETECTION_SOURCE ${CMAKE_BINARY_DIR}/cpu_feature_detection.cpp)
    file(WRITE ${CPU_DETECTION_SOURCE} "
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {
    std::ifstream cpuinfo(\"/proc/cpuinfo\");
    std::string line;
    std::vector<std::string> flags;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find(\"flags\") == 0) {
            size_t pos = line.find(\":\");
            if (pos != std::string::npos) {
                std::string flags_str = line.substr(pos + 1);
                size_t start = 0;
                size_t end = flags_str.find(' ');
                while (end != std::string::npos) {
                    if (end > start) {
                        flags.push_back(flags_str.substr(start, end - start));
                    }
                    start = end + 1;
                    end = flags_str.find(' ', start);
                }
                if (start < flags_str.length()) {
                    flags.push_back(flags_str.substr(start));
                }
                break;
            }
        }
    }
    
    // 检查各种指令集支持
    bool has_sse2 = false, has_sse42 = false, has_avx = false, has_avx2 = false, has_avx512 = false;
    bool is_hypervisor = false;
    
    for (const auto& flag : flags) {
        if (flag == \"sse2\") has_sse2 = true;
        else if (flag == \"sse4_2\") has_sse42 = true;
        else if (flag == \"avx\") has_avx = true;
        else if (flag == \"avx2\") has_avx2 = true;
        else if (flag.find(\"avx512\") == 0) has_avx512 = true;
        else if (flag == \"hypervisor\") is_hypervisor = true;
    }
    
    std::cout << \"SSE2:\" << (has_sse2 ? \"1\" : \"0\") << std::endl;
    std::cout << \"SSE42:\" << (has_sse42 ? \"1\" : \"0\") << std::endl;
    std::cout << \"AVX:\" << (has_avx ? \"1\" : \"0\") << std::endl;
    std::cout << \"AVX2:\" << (has_avx2 ? \"1\" : \"0\") << std::endl;
    std::cout << \"AVX512:\" << (has_avx512 ? \"1\" : \"0\") << std::endl;
    std::cout << \"HYPERVISOR:\" << (is_hypervisor ? \"1\" : \"0\") << std::endl;
    
    return 0;
}
")
    
    # 编译CPU特性检测程序
    try_compile(CPU_DETECTION_SUCCESS 
                ${CMAKE_BINARY_DIR} 
                ${CPU_DETECTION_SOURCE}
                COPY_FILE ${CMAKE_BINARY_DIR}/cpu_detector
                OUTPUT_VARIABLE CPU_DETECTION_OUTPUT)
    
    if(CPU_DETECTION_SUCCESS)
        message(STATUS "CPU feature detection program compiled successfully")
        
        # 运行CPU特性检测
        execute_process(COMMAND ${CMAKE_BINARY_DIR}/cpu_detector
                        OUTPUT_VARIABLE CPU_FEATURES
                        RESULT_VARIABLE CPU_DETECTION_RESULT)
        
        if(CPU_DETECTION_RESULT EQUAL 0)
            message(STATUS "CPU feature detection results:")
            message(STATUS "${CPU_FEATURES}")
            
            # 解析CPU特性
            string(REGEX MATCH "SSE2:1" SSE2_SUPPORTED "${CPU_FEATURES}")
            string(REGEX MATCH "SSE42:1" SSE42_SUPPORTED "${CPU_FEATURES}")
            string(REGEX MATCH "AVX:1" AVX_SUPPORTED "${CPU_FEATURES}")
            string(REGEX MATCH "AVX2:1" AVX2_SUPPORTED "${CPU_FEATURES}")
            string(REGEX MATCH "AVX512:1" AVX512_SUPPORTED "${CPU_FEATURES}")
            string(REGEX MATCH "HYPERVISOR:1" HYPERVISOR_DETECTED "${CPU_FEATURES}")
            
            # 设置全局变量
            set(CPU_SSE2_SUPPORTED ${SSE2_SUPPORTED} PARENT_SCOPE)
            set(CPU_SSE42_SUPPORTED ${SSE42_SUPPORTED} PARENT_SCOPE)
            set(CPU_AVX_SUPPORTED ${AVX_SUPPORTED} PARENT_SCOPE)
            set(CPU_AVX2_SUPPORTED ${AVX2_SUPPORTED} PARENT_SCOPE)
            set(CPU_AVX512_SUPPORTED ${AVX512_SUPPORTED} PARENT_SCOPE)
            set(CPU_HYPERVISOR_DETECTED ${HYPERVISOR_DETECTED} PARENT_SCOPE)
            
            message(STATUS "  SSE2: ${SSE2_SUPPORTED}")
            message(STATUS "  SSE4.2: ${SSE42_SUPPORTED}")
            message(STATUS "  AVX: ${AVX_SUPPORTED}")
            message(STATUS "  AVX2: ${AVX2_SUPPORTED}")
            message(STATUS "  AVX512: ${AVX512_SUPPORTED}")
            message(STATUS "  Hypervisor: ${HYPERVISOR_DETECTED}")
            
        else()
            message(WARNING "CPU feature detection program execution failed")
            set_cpu_features_default()
        endif()
    else()
        message(WARNING "CPU feature detection program compilation failed: ${CPU_DETECTION_OUTPUT}")
        set_cpu_features_default()
    endif()
    
    # 清理临时文件
    file(REMOVE ${CPU_DETECTION_SOURCE})
    if(EXISTS ${CMAKE_BINARY_DIR}/cpu_detector)
        file(REMOVE ${CMAKE_BINARY_DIR}/cpu_detector)
    endif()
endfunction()

function(set_cpu_features_default)
    message(STATUS "Using default CPU feature settings")
    set(CPU_SSE2_SUPPORTED "" PARENT_SCOPE)
    set(CPU_SSE42_SUPPORTED "" PARENT_SCOPE)
    set(CPU_AVX_SUPPORTED "" PARENT_SCOPE)
    set(CPU_AVX2_SUPPORTED "" PARENT_SCOPE)
    set(CPU_AVX512_SUPPORTED "" PARENT_SCOPE)
    set(CPU_HYPERVISOR_DETECTED "" PARENT_SCOPE)
endfunction()

function(apply_cpu_optimizations)
    include(CheckCXXCompilerFlag)
    
    message(STATUS "Applying CPU optimization settings...")
    
    # SSE2 - 几乎所有现代CPU都支持
    if(CPU_SSE2_SUPPORTED)
        check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
        if(COMPILER_SUPPORTS_SSE2)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2" PARENT_SCOPE)
            message(STATUS "Enabling SSE2 instruction set")
        endif()
    endif()
    
    # SSE4.2
    if(CPU_SSE42_SUPPORTED)
        check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
        if(COMPILER_SUPPORTS_SSE42)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2" PARENT_SCOPE)
            message(STATUS "Enabling SSE4.2 instruction set")
        endif()
    endif()
    
    # AVX
    if(CPU_AVX_SUPPORTED)
        check_cxx_compiler_flag("-mavx" COMPILER_SUPPORTS_AVX)
        if(COMPILER_SUPPORTS_AVX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx" PARENT_SCOPE)
            message(STATUS "Enabling AVX instruction set")
        endif()
    endif()
    
    # AVX2
    if(CPU_AVX2_SUPPORTED)
        check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
        if(COMPILER_SUPPORTS_AVX2)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2" PARENT_SCOPE)
            message(STATUS "Enabling AVX2 instruction set")
        endif()
    endif()
    
    # AVX512 - 在虚拟化环境中需要特别小心
    if(CPU_AVX512_SUPPORTED AND NOT CPU_HYPERVISOR_DETECTED)
        check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
        if(COMPILER_SUPPORTS_AVX512)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f" PARENT_SCOPE)
            message(STATUS "Enabling AVX512 instruction set")
        endif()
    elseif(CPU_AVX512_SUPPORTED AND CPU_HYPERVISOR_DETECTED)
        message(WARNING "Hypervisor detected, skipping AVX512 instruction set to avoid stability issues")
    endif()
    
    # 如果没有检测到任何支持的指令集，至少启用SSE2
    if(NOT CPU_SSE2_SUPPORTED AND NOT CPU_SSE42_SUPPORTED AND NOT CPU_AVX_SUPPORTED AND NOT CPU_AVX2_SUPPORTED)
        check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
        if(COMPILER_SUPPORTS_SSE2)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2" PARENT_SCOPE)
            message(STATUS "Using default SSE2 instruction set")
        endif()
    endif()
endfunction()
