#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>

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

// 进度条样式枚举
enum class ProgressStyle {
    BASIC,      // 基本样式: [====>    ] 50%
    BLOCK,      // 块样式: [████████░░] 80%
    ROTATING,   // 旋转样式: [====>    ] 50% ⏳
    SMOOTH,     // 平滑样式: [████████░░] 80% █
    MINIMAL,    // 最小样式: 50% (100/200)
    SPINNER,    // 旋转器样式: ⏳ Installing... 50%
    NPM_STYLE   // NPM风格: ⏳ Installing package... 50%
};

// 进度条类
class ProgressBar {
private:
    int total_;
    int current_;
    int width_;
    std::string prefix_;
    std::string suffix_;
    bool show_percentage_;
    bool show_eta_;
    bool show_speed_;
    ProgressStyle style_;
    
    // ETA计算相关
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_update_time_;
    std::vector<double> recent_speeds_;  // 最近的速度记录，用于平滑计算
    static constexpr size_t SPEED_HISTORY_SIZE = 10;
    
    // 旋转字符相关
    mutable int spinner_index_;
    static constexpr const char* SPINNER_CHARS = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
    static constexpr const char* NPM_SPINNER_CHARS = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
    
    // 内部方法
    std::string format_eta(double seconds) const;
    std::string format_speed(double items_per_second) const;
    double calculate_smoothed_speed() const;
    void update_speed_history();
    std::string get_spinner_char() const;
    
public:
    ProgressBar(int total, int width = 50, const std::string& prefix = "", 
                bool show_percentage = true, bool show_eta = true, bool show_speed = false,
                ProgressStyle style = ProgressStyle::SMOOTH);
    
    void update(int current);
    void update(int current, const std::string& custom_suffix);
    void finish();
    void finish(const std::string& final_message);
    void reset(int total);
    
    // 配置方法
    void set_style(ProgressStyle style) { style_ = style; }
    void set_prefix(const std::string& prefix) { prefix_ = prefix; }
    void set_suffix(const std::string& suffix) { suffix_ = suffix; }
    void set_show_eta(bool show) { show_eta_ = show; }
    void set_show_speed(bool show) { show_speed_ = show; }
    void set_show_percentage(bool show) { show_percentage_ = show; }
    
    // 获取状态
    int get_current() const { return current_; }
    int get_total() const { return total_; }
    double get_percentage() const;
    double get_eta_seconds() const;
    double get_speed() const;
    bool is_complete() const { return current_ >= total_; }
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
    
    // 便捷的进度条创建函数
    static ProgressBar create_progress_bar(int total, const std::string& task_name = "");
    static ProgressBar create_download_progress(int total, const std::string& filename = "");
    static ProgressBar create_install_progress(int total, const std::string& package_name = "");
    static ProgressBar create_parse_progress(int total, const std::string& file_name = "");
    
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