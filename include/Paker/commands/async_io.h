#pragma once

#include <string>
#include <vector>

namespace Paker {

// 异步I/O命令接口
void pm_async_io_stats();
void pm_async_io_config();
void pm_async_io_test();
void pm_async_io_benchmark();
void pm_async_io_optimize();

} // namespace Paker
