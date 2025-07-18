#pragma once
#include <string>
std::string get_project_name();
std::string get_json_file();
std::pair<std::string, std::string> parse_name_version(const std::string& input); 