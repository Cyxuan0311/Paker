#include "Paker/info.h"
#include "Paker/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include "Paker/sources.h"
extern const std::map<std::string, std::string>& get_builtin_repos();
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_search(const std::string& keyword) {
    auto repos = get_all_repos();
    std::cout << "Search results for '" << keyword << "':\n";
    bool found = false;
    for (const auto& [name, url] : repos) {
        if (name.find(keyword) != std::string::npos) {
            std::cout << "  " << name << "\t" << url << "\n";
            found = true;
        }
    }
    if (!found) std::cout << "  (none)\n";
}

void pm_info(const std::string& pkg) {
    auto repos = get_all_repos();
    auto it = repos.find(pkg);
    if (it == repos.end()) {
        std::cout << "No info for package: " << pkg << "\n";
        return;
    }
    std::cout << "Package: " << pkg << "\n";
    std::cout << "Repo: " << it->second << "\n";
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path readme = pkg_dir / "README.md";
    if (!fs::exists(readme)) readme = pkg_dir / "README.rst";
    if (fs::exists(readme)) {
        std::ifstream ifs(readme);
        std::string line;
        std::cout << "Description (from README):\n";
        int cnt = 0;
        while (std::getline(ifs, line) && cnt < 10) {
            std::cout << line << "\n";
            ++cnt;
        }
    }
} 