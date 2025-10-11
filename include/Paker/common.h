#pragma once

// 常用类型定义
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <filesystem>

namespace Paker {

// 常用类型别名
using String = std::string;
using StringList = std::vector<std::string>;
using StringMap = std::map<std::string, std::string>;
using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::milliseconds;

// 文件系统别名
namespace fs = std::filesystem;

// 智能指针别名
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// 常用常量
constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
constexpr size_t MAX_CONCURRENT_OPERATIONS = 16;
constexpr size_t DEFAULT_CACHE_SIZE = 1024 * 1024 * 1024; // 1GB

// 常用枚举
enum class Status {
    SUCCESS,
    FAILED,
    PENDING,
    CANCELLED
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

} // namespace Paker
