#pragma once
#include <map>
#include <string>

std::map<std::string, std::string> get_custom_repos();
std::map<std::string, std::string> get_all_repos();
void add_remote(const std::string& name, const std::string& url);
void remove_remote(const std::string& name); 