#include "Paker/utils.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <glog/logging.h>
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_init() {
    std::string json_file = get_json_file();
    if (fs::exists(json_file)) {
        LOG(INFO) << "Project already initialized.";
        std::cout << "Project already initialized.\n";
        return;
    }
    std::string project_name = get_project_name();
    json j = {
        {"name", project_name},
        {"version", "0.1.0"},
        {"description", ""},
        {"dependencies", json::object()}
    };
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Initialized Paker project: " << project_name;
    std::cout << "Initialized Paker project: " << project_name << "\n";
}

void pm_add_desc(const std::string& desc) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["description"] = desc;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project description.";
    std::cout << "Updated project description.\n";
}

void pm_add_version(const std::string& vers) {
    std::string json_file = get_json_file();
    if (!fs::exists(json_file)) {
        LOG(ERROR) << "Not a Paker project. Run 'paker init' first.";
        std::cout << "Not a Paker project. Run 'paker init' first.\n";
        return;
    }
    std::ifstream ifs(json_file);
    json j;
    ifs >> j;
    j["version"] = vers;
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    LOG(INFO) << "Updated project version.";
    std::cout << "Updated project version.\n";
} 