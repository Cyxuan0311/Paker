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
ProgressBar::ProgressBar(int total, int width, const std::string& prefix, bool show_percentage)
    : total_(total), current_(0), width_(width), prefix_(prefix), show_percentage_(show_percentage) {}

void ProgressBar::update(int current) {
    current_ = current;
    if (total_ <= 0) return;
    
    float percentage = static_cast<float>(current_) / total_;
    int filled_width = static_cast<int>(width_ * percentage);
    
    std::cout << "\r" << prefix_;
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
    
    if (show_percentage_) {
        std::cout << " " << std::fixed << std::setprecision(1) << (percentage * 100) << "%";
    }
    
    std::cout << " (" << current_ << "/" << total_ << ")";
    std::cout.flush();
}

void ProgressBar::finish() {
    update(total_);
    std::cout << std::endl;
}

void ProgressBar::reset(int total) {
    total_ = total;
    current_ = 0;
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