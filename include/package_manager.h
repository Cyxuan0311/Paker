#include <string>
#pragma once

void pm_init();
void pm_add(const std::string& pkg);
void pm_remove(const std::string& pkg);
void pm_list(); 