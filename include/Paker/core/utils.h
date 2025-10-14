#pragma once
#include <string>
#include <vector>

std::string get_project_name();
std::string get_json_file();
std::pair<std::string, std::string> parse_name_version(const std::string& input);

// Record-related utility functions
std::string get_record_file_path();
std::string get_lock_file_path();
std::string get_warmup_config_path();
std::vector<std::string> collect_package_files(const std::string& package_path); 