#include "Paker/commands/cli.h"
#include "Paker/core/service_container.h"
#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // 初始化glog，设置生产环境日志级别
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 0;  // 不输出到stderr
    FLAGS_minloglevel = 2;  // 只显示ERROR和FATAL级别日志
    
    int result = run_cli(argc, argv);
    
    // 只在服务管理器被初始化时才清理
    if (Paker::g_service_manager) {
        Paker::cleanup_service_manager();
    }
    
    return result;
} 