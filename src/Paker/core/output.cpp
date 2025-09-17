#include "Paker/core/output.h"
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <functional>
#include <set>

namespace Paker {

// 静态成员初始化
bool Output::colored_output_ = true;
bool Output::verbose_mode_ = false;

// ProgressBar 实现
ProgressBar::ProgressBar(int total, int width, const std::string& prefix, 
                        bool show_percentage, bool show_eta, bool show_speed, ProgressStyle style)
    : total_(total), current_(0), width_(width), prefix_(prefix), suffix_(""),
      show_percentage_(show_percentage), show_eta_(show_eta), show_speed_(show_speed), style_(style),
      start_time_(std::chrono::steady_clock::now()), last_update_time_(start_time_) {
    recent_speeds_.reserve(SPEED_HISTORY_SIZE);
}

void ProgressBar::update(int current) {
    update(current, "");
}

void ProgressBar::update(int current, const std::string& custom_suffix) {
    current_ = std::min(current, total_);
    auto now = std::chrono::steady_clock::now();
    
    // 更新速度历史
    if (current_ > 0) {
        update_speed_history();
    }
    
    last_update_time_ = now;
    
    if (total_ <= 0) return;
    
    double percentage = static_cast<double>(current_) / total_;
    int filled_width = static_cast<int>(width_ * percentage);
    
    std::cout << "\r" << prefix_;
    
    // 根据样式绘制进度条
    switch (style_) {
        case ProgressStyle::BASIC: {
            std::cout << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < filled_width) {
                    std::cout << "=";
                } else if (i == filled_width) {
                    std::cout << ">";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "]";
            break;
        }
            
        case ProgressStyle::BLOCK: {
            std::cout << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < filled_width) {
                    std::cout << "█";
                } else {
                    std::cout << "░";
                }
            }
            std::cout << "]";
            break;
        }
            
        case ProgressStyle::ROTATING: {
            std::cout << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < filled_width) {
                    std::cout << "=";
                } else if (i == filled_width) {
                    std::cout << ">";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "]";
            // 添加旋转指示器
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
            const char* spinners[] = {"⏳", "⏰", "⌛", "⏲️"};
            std::cout << " " << spinners[(elapsed / 200) % 4];
            break;
        }
            
        case ProgressStyle::SMOOTH: {
            std::cout << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < filled_width) {
                    std::cout << "█";
                } else {
                    std::cout << "░";
                }
            }
            std::cout << "]";
            if (filled_width < width_) {
                std::cout << " █";
            }
            break;
        }
            
        case ProgressStyle::MINIMAL:
            // 最小样式不显示进度条
            break;
    }
    
    // 添加百分比
    if (show_percentage_) {
        std::cout << " " << std::fixed << std::setprecision(1) << (percentage * 100) << "%";
    }
    
    // 添加计数
    std::cout << " (" << current_ << "/" << total_ << ")";
    
    // 添加ETA
    if (show_eta_ && current_ > 0 && current_ < total_) {
        double eta = get_eta_seconds();
        if (eta > 0) {
            std::cout << " ETA: " << format_eta(eta);
        }
    }
    
    // 添加速度
    if (show_speed_ && current_ > 0) {
        double speed = get_speed();
        if (speed > 0) {
            std::cout << " " << format_speed(speed);
        }
    }
    
    // 添加自定义后缀
    if (!custom_suffix.empty()) {
        std::cout << " " << custom_suffix;
    } else if (!suffix_.empty()) {
        std::cout << " " << suffix_;
    }
    
    std::cout.flush();
}

void ProgressBar::finish() {
    finish("");
}

void ProgressBar::finish(const std::string& final_message) {
    update(total_);
    if (!final_message.empty()) {
        std::cout << " " << final_message;
    }
    std::cout << std::endl;
}

void ProgressBar::reset(int total) {
    total_ = total;
    current_ = 0;
    start_time_ = std::chrono::steady_clock::now();
    last_update_time_ = start_time_;
    recent_speeds_.clear();
}

double ProgressBar::get_percentage() const {
    if (total_ <= 0) return 0.0;
    return static_cast<double>(current_) / total_;
}

double ProgressBar::get_eta_seconds() const {
    if (current_ <= 0 || current_ >= total_) return 0.0;
    
    double speed = calculate_smoothed_speed();
    if (speed <= 0) return 0.0;
    
    int remaining = total_ - current_;
    return remaining / speed;
}

double ProgressBar::get_speed() const {
    return calculate_smoothed_speed();
}

std::string ProgressBar::format_eta(double seconds) const {
    if (seconds < 60) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    } else if (seconds < 3600) {
        int minutes = static_cast<int>(seconds / 60);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + "m" + std::to_string(secs) + "s";
    } else {
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        return std::to_string(hours) + "h" + std::to_string(minutes) + "m";
    }
}

std::string ProgressBar::format_speed(double items_per_second) const {
    if (items_per_second < 1) {
        return std::to_string(static_cast<int>(items_per_second * 60)) + "/min";
    } else if (items_per_second < 60) {
        return std::to_string(static_cast<int>(items_per_second)) + "/s";
    } else {
        return std::to_string(static_cast<int>(items_per_second / 60)) + "/min";
    }
}

double ProgressBar::calculate_smoothed_speed() const {
    if (recent_speeds_.empty()) return 0.0;
    
    double sum = 0.0;
    for (double speed : recent_speeds_) {
        sum += speed;
    }
    return sum / recent_speeds_.size();
}

void ProgressBar::update_speed_history() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time_).count();
    
    if (elapsed > 0) {
        double speed = static_cast<double>(current_) / std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
        
        recent_speeds_.push_back(speed);
        if (recent_speeds_.size() > SPEED_HISTORY_SIZE) {
            recent_speeds_.erase(recent_speeds_.begin());
        }
    }
}

// Table 实现
void Table::add_column(const std::string& name, int width, bool align_right) {
    columns_.emplace_back(name, width, align_right);
}

void Table::add_row(const std::vector<std::string>& row) {
    rows_.push_back(row);
}

void Table::print(std::ostream& os) const {
    if (columns_.empty()) return;
    
    // 计算列宽
    std::vector<int> col_widths;
    for (size_t i = 0; i < columns_.size(); ++i) {
        int width = columns_[i].width;
        if (width == 0) {
            // 自动计算宽度
            width = columns_[i].name.length();
            for (const auto& row : rows_) {
                if (i < row.size()) {
                    width = std::max(width, static_cast<int>(row[i].length()));
                }
            }
        }
        col_widths.push_back(width);
    }
    
    // 打印表头
    os << Colors::BOLD;
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) os << " | ";
        if (columns_[i].align_right) {
            os << Output::pad_left(columns_[i].name, col_widths[i]);
        } else {
            os << Output::pad_right(columns_[i].name, col_widths[i]);
        }
    }
    os << Colors::RESET << std::endl;
    
    // 打印分隔线
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) os << "-+-";
        os << std::string(col_widths[i], '-');
    }
    os << std::endl;
    
    // 打印数据行
    for (const auto& row : rows_) {
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0) os << " | ";
            std::string cell = (i < row.size()) ? row[i] : "";
            if (columns_[i].align_right) {
                os << Output::pad_left(cell, col_widths[i]);
            } else {
                os << Output::pad_right(cell, col_widths[i]);
            }
        }
        os << std::endl;
    }
}

void Table::clear() {
    columns_.clear();
    rows_.clear();
}

// Output 实现
void Output::print(const std::string& message, OutputType type) {
    std::string color;
    switch (type) {
        case OutputType::ERROR:
            color = Colors::RED + Colors::BOLD;
            break;
        case OutputType::WARNING:
            color = Colors::YELLOW;
            break;
        case OutputType::SUCCESS:
            color = Colors::GREEN;
            break;
        case OutputType::INFO:
            color = Colors::BLUE;
            break;
        case OutputType::DEBUG:
            color = Colors::DIM;
            break;
    }
    
    if (colored_output_) {
        std::cout << color << message << Colors::RESET;
    } else {
        std::cout << message;
    }
}

void Output::println(const std::string& message, OutputType type) {
    print(message, type);
    std::cout << std::endl;
}

void Output::error(const std::string& message) {
    println(message, OutputType::ERROR);
}

void Output::warning(const std::string& message) {
    println(message, OutputType::WARNING);
}

void Output::success(const std::string& message) {
    println(message, OutputType::SUCCESS);
}

void Output::info(const std::string& message) {
    println(message, OutputType::INFO);
}

void Output::debug(const std::string& message) {
    if (verbose_mode_) {
        println(message, OutputType::DEBUG);
    }
}

void Output::printf(const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    
    // 计算需要的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, format.c_str(), args_copy);
    va_end(args_copy);
    
    if (size > 0) {
        std::vector<char> buffer(size + 1);
        vsnprintf(buffer.data(), buffer.size(), format.c_str(), args);
        std::cout << buffer.data();
    }
    
    va_end(args);
}

void Output::print_table(const Table& table) {
    table.print();
}

void Output::print_progress_bar(ProgressBar& bar) {
    // ProgressBar 自己处理输出
}

// 便捷的进度条创建函数实现
ProgressBar Output::create_progress_bar(int total, const std::string& task_name) {
    std::string prefix = task_name.empty() ? "Progress: " : task_name + ": ";
    return ProgressBar(total, 50, prefix, true, true, false, ProgressStyle::SMOOTH);
}

ProgressBar Output::create_download_progress(int total, const std::string& filename) {
    std::string prefix = "Downloading " + (filename.empty() ? "file" : filename) + ": ";
    return ProgressBar(total, 50, prefix, true, true, true, ProgressStyle::SMOOTH);
}

ProgressBar Output::create_install_progress(int total, const std::string& package_name) {
    std::string prefix = "Installing " + (package_name.empty() ? "package" : package_name) + ": ";
    return ProgressBar(total, 50, prefix, true, true, false, ProgressStyle::BLOCK);
}

ProgressBar Output::create_parse_progress(int total, const std::string& file_name) {
    std::string prefix = "Parsing " + (file_name.empty() ? "file" : file_name) + ": ";
    return ProgressBar(total, 50, prefix, true, true, false, ProgressStyle::ROTATING);
}

void Output::print_dependency_tree(const std::string& root, 
                                  const std::map<std::string, std::vector<std::string>>& deps,
                                  const std::map<std::string, std::string>& versions) {
    std::function<void(const std::string&, int, std::set<std::string>&)> print_node;
    
    print_node = [&](const std::string& node, int depth, std::set<std::string>& visited) {
        // 缩进
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        
        // 连接符
        if (depth > 0) {
            std::cout << (depth == 1 ? "├── " : "│   ");
        }
        
        // 节点名称
        std::cout << Colors::CYAN << node << Colors::RESET;
        
        // 版本信息
        auto version_it = versions.find(node);
        if (version_it != versions.end() && !version_it->second.empty()) {
            std::cout << Colors::DIM << " (" << version_it->second << ")" << Colors::RESET;
        }
        
        std::cout << std::endl;
        
        // 避免循环依赖
        if (visited.count(node)) {
            return;
        }
        visited.insert(node);
        
        // 递归打印子依赖
        auto deps_it = deps.find(node);
        if (deps_it != deps.end()) {
            for (size_t i = 0; i < deps_it->second.size(); ++i) {
                const std::string& child = deps_it->second[i];
                bool is_last = (i == deps_it->second.size() - 1);
                
                // 调整缩进
                for (int j = 0; j < depth; ++j) {
                    std::cout << "  ";
                }
                if (depth > 0) {
                    std::cout << (is_last ? "└── " : "├── ");
                }
                
                print_node(child, depth + 1, visited);
            }
        }
    };
    
    std::set<std::string> visited;
    print_node(root, 0, visited);
}

void Output::set_colored_output(bool enabled) {
    colored_output_ = enabled;
}

void Output::set_verbose_mode(bool enabled) {
    verbose_mode_ = enabled;
}

bool Output::is_colored_output_enabled() {
    return colored_output_;
}

bool Output::is_verbose_mode_enabled() {
    return verbose_mode_;
}

std::string Output::colorize(const std::string& text, const std::string& color) {
    if (colored_output_) {
        return color + text + Colors::RESET;
    }
    return text;
}

std::string Output::truncate(const std::string& text, int max_length) {
    if (text.length() <= max_length) {
        return text;
    }
    return text.substr(0, max_length - 3) + "...";
}

std::string Output::pad_right(const std::string& text, int width) {
    if (text.length() >= width) {
        return text;
    }
    return text + std::string(width - text.length(), ' ');
}

std::string Output::pad_left(const std::string& text, int width) {
    if (text.length() >= width) {
        return text;
    }
    return std::string(width - text.length(), ' ') + text;
}

std::string Output::center(const std::string& text, int width) {
    if (text.length() >= width) {
        return text;
    }
    int padding = width - text.length();
    int left_padding = padding / 2;
    int right_padding = padding - left_padding;
    return std::string(left_padding, ' ') + text + std::string(right_padding, ' ');
}

} // namespace Paker 