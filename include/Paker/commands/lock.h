#pragma once
#include <string>

void pm_lock();
void pm_add_lock();
void pm_upgrade(const std::string& pkg);
void pm_validate_dependencies(); 