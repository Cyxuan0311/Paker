#pragma once
#include <string>

void pm_lock();
void pm_install_lock();
void pm_upgrade(const std::string& pkg); 