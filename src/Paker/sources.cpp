#include "Paker/sources.h"
#include <fstream>
#include <map>
#include <string>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include "nlohmann/json.hpp"
// 获取内置仓库映射表（由 builtin_repos.cpp 提供）
extern const std::map<std::string, std::string>& get_builtin_repos();
namespace fs = std::filesystem;
using json = nlohmann::json;

// 读取 Paker.sources.json，返回自定义依赖源映射表（name -> url）
std::map<std::string, std::string> get_custom_repos() {
    std::map<std::string, std::string> repos;
    fs::path sources_file = "Paker.sources.json";
    if (!fs::exists(sources_file)) return repos;
    std::ifstream ifs(sources_file);
    try {
        json j; ifs >> j;
        // 解析 sources 数组
        if (j.contains("sources")) {
            for (const auto& src : j["sources"]) {
                // 只处理同时包含 name 和 url 的项
                if (src.contains("name") && src.contains("url")) {
                    repos[src["name"].get<std::string>()] = src["url"].get<std::string>();
                }
            }
        }
    } catch (...) {
        std::cerr << "Warning: failed to parse Paker.sources.json\n";
    }
    return repos;
}

// 合并自定义依赖源和内置仓库，优先自定义源，返回总映射表
std::map<std::string, std::string> get_all_repos() {
    auto custom = get_custom_repos();
    auto builtin = get_builtin_repos();
    // 内置仓库中未被自定义源覆盖的部分补充进来
    for (const auto& [k, v] : builtin) {
        if (custom.count(k) == 0) custom[k] = v;
    }
    return custom;
}

// 添加自定义依赖源到Paker.json的remotes
void add_remote(const std::string& name, const std::string& url) {
    fs::path json_file = "Paker.json";
    json j;
    if (fs::exists(json_file)) {
        std::ifstream ifs(json_file);
        ifs >> j;
    }
    if (!j.contains("remotes")) j["remotes"] = json::array();
    // 检查是否已存在，存在则更新url
    bool found = false;
    for (auto& src : j["remotes"]) {
        if (src["name"] == name) {
            src["url"] = url;
            found = true;
            break;
        }
    }
    if (!found) {
        j["remotes"].push_back({{"name", name}, {"url", url}});
    }
    std::ofstream ofs(json_file);
    ofs << j.dump(4);
    std::cout << "Added/Updated remote: " << name << " -> " << url << "\n";
}

// 从Paker.json的remotes移除指定源
void remove_remote(const std::string& name) {
    fs::path json_file = "Paker.json";
    if (!fs::exists(json_file)) return;
    json j;
    {
        std::ifstream ifs(json_file);
        ifs >> j;
    }
    if (!j.contains("remotes")) return;
    auto& arr = j["remotes"];
    auto it = std::remove_if(arr.begin(), arr.end(), [&](const json& src) {
        return src.contains("name") && src["name"] == name;
    });
    if (it != arr.end()) {
        arr.erase(it, arr.end());
        std::ofstream ofs(json_file);
        ofs << j.dump(4);
        std::cout << "Removed remote: " << name << "\n";
    } else {
        std::cout << "Remote not found: " << name << "\n";
    }
} 