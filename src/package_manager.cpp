#include "package_manager.h"
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

static const char* PKG_FILE = "cpppm.json";

void pm_init() {
    if (fs::exists(PKG_FILE)) {
        std::cout << "Project already initialized.\n";
        return;
    }
    json j = {
        {"name", "myproject"},
        {"version", "0.1.0"},
        {"dependencies", json::object()}
    };
    std::ofstream ofs(PKG_FILE);
    ofs << j.dump(4);
    std::cout << "Initialized Paker project.\n";
}

void pm_add(const std::string& pkg) {
    if (!fs::exists(PKG_FILE)) {
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(PKG_FILE);
    json j;
    ifs >> j;
    j["dependencies"][pkg] = "*";
    std::ofstream ofs(PKG_FILE);
    ofs << j.dump(4);
    std::cout << "Added dependency: " << pkg << "\n";
}

void pm_remove(const std::string& pkg) {
    if (!fs::exists(PKG_FILE)) {
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(PKG_FILE);
    json j;
    ifs >> j;
    if (j["dependencies"].contains(pkg)) {
        j["dependencies"].erase(pkg);
        std::ofstream ofs(PKG_FILE);
        ofs << j.dump(4);
        std::cout << "Removed dependency: " << pkg << "\n";
    } else {
        std::cout << "Dependency not found: " << pkg << "\n";
    }
}

void pm_list() {
    if (!fs::exists(PKG_FILE)) {
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(PKG_FILE);
    json j;
    ifs >> j;
    std::cout << "Dependencies:\n";
    for (auto& [k, v] : j["dependencies"].items()) {
        std::cout << "  " << k << ": " << v << "\n";
    }
} 