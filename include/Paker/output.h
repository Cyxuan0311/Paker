#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace Paker {

// 颜色代码
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
}

// 输出类型
enum class OutputType {
    INFO,
    SUCCESS,
    WARNING,
    ERROR,
    DEBUG
};

// 表格列定义
struct TableColumn {
    std::string name;
    int width;
    bool align_right;
    
    TableColumn(const std::string& n, int w = 0, bool right = false) 
        : name(n), width(w), align_right(right) {}
};

// 进度条类
class ProgressBar {
private:
    int total_;
    int current_;
    int width_;
    std::string prefix_;
    bool show_percentage_;
    
public:
    ProgressBar(int total, int width = 50, const std::string& prefix = "", bool show_percentage = true);
    void update(int current);
    void finish();
    void reset(int total);
};

// 表格类
class Table {
private:
    std::vector<TableColumn> columns_;
    std::vector<std::vector<std::string>> rows_;
    
public:
    void add_column(const std::string& name, int width = 0, bool align_right = false);
    void add_row(const std::vector<std::string>& row);
    void print(std::ostream& os = std::cout) const;
    void clear();
};

// 主要输出类
class Output {
private:
    static bool colored_output_;
    static bool verbose_mode_;
    
public:
    // 基础输出方法
    static void print(const std::string& message, OutputType type = OutputType::INFO);
    static void println(const std::string& message, OutputType type = OutputType::INFO);
    static void error(const std::string& message);
    static void warning(const std::string& message);
    static void success(const std::string& message);
    static void info(const std::string& message);
    static void debug(const std::string& message);
    
    // 格式化输出
    static void printf(const std::string& format, ...);
    static void print_table(const Table& table);
    static void print_progress_bar(ProgressBar& bar);
    
    // 依赖树输出
    static void print_dependency_tree(const std::string& root, 
                                    const std::map<std::string, std::vector<std::string>>& deps,
                                    const std::map<std::string, std::string>& versions = {});
    
    // 设置
    static void set_colored_output(bool enabled);
    static void set_verbose_mode(bool enabled);
    static bool is_colored_output_enabled();
    static bool is_verbose_mode_enabled();
    
    // 工具方法
    static std::string colorize(const std::string& text, const std::string& color);
    static std::string truncate(const std::string& text, int max_length);
    static std::string pad_right(const std::string& text, int width);
    static std::string pad_left(const std::string& text, int width);
    static std::string center(const std::string& text, int width);
};

} // namespace Paker 