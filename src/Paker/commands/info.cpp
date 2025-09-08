#include "Paker/commands/info.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <glog/logging.h>
#include "nlohmann/json.hpp"
#include "Paker/dependency/sources.h"
extern const std::map<std::string, std::string>& get_builtin_repos();
using json = nlohmann::json;
namespace fs = std::filesystem;

void pm_search(const std::string& keyword) {
    auto repos = get_all_repos();
    Paker::Output::info("Search results for '" + keyword + "':");
    
    Paker::Table table;
    table.add_column("Package", 20);
    table.add_column("Repository", 50);
    
    bool found = false;
    for (const auto& [name, url] : repos) {
        if (name.find(keyword) != std::string::npos) {
            table.add_row({name, url});
            found = true;
        }
    }
    
    if (!found) {
        Paker::Output::info("  (none)");
    } else {
        Paker::Output::print_table(table);
    }
}

void pm_info(const std::string& pkg) {
    auto repos = get_all_repos();
    auto it = repos.find(pkg);
    if (it == repos.end()) {
        Paker::Output::error("No info for package: " + pkg);
        return;
    }
    
    Paker::Output::info("Package: " + pkg);
    Paker::Output::info("Repository: " + it->second);
    
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path readme = pkg_dir / "README.md";
    if (!fs::exists(readme)) readme = pkg_dir / "README.rst";
    
    if (fs::exists(readme)) {
        Paker::Output::info("Description (from README):");
        std::ifstream ifs(readme);
        std::string line;
        int cnt = 0;
        while (std::getline(ifs, line) && cnt < 10) {
            Paker::Output::info("  " + line);
            ++cnt;
        }
        if (cnt == 10) {
            Paker::Output::info("  ... (truncated)");
        }
    } else {
        Paker::Output::warning("No README file found for this package");
    }
} 