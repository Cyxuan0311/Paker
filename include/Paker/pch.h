#pragma once

// 标准库头文件
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <utility>
#include <tuple>
#include <optional>
#include <variant>
#include <any>
#include <type_traits>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstddef>

// 第三方库头文件
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

// 项目内部头文件
#include "Paker/core/utils.h"
#include "Paker/core/output.h"

// 常用命名空间别名
namespace fs = std::filesystem;
using json = nlohmann::json;
