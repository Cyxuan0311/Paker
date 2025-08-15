#include "Paker/commands/info.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
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
    Output::info("Search results for '" + keyword + "':");
    
    Table table;
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
        Output::info("  (none)");
    } else {
        Output::print_table(table);
    }
}

void pm_info(const std::string& pkg) {
    auto repos = get_all_repos();
    auto it = repos.find(pkg);
    if (it == repos.end()) {
        Output::error("No info for package: " + pkg);
        return;
    }
    
    Output::info("Package: " + pkg);
    Output::info("Repository: " + it->second);
    
    fs::path pkg_dir = fs::path("packages") / pkg;
    fs::path readme = pkg_dir / "README.md";
    if (!fs::exists(readme)) readme = pkg_dir / "README.rst";
    
    if (fs::exists(readme)) {
        Output::info("Description (from README):");
        std::ifstream ifs(readme);
        std::string line;
        int cnt = 0;
        while (std::getline(ifs, line) && cnt < 10) {
            Output::info("  " + line);
            ++cnt;
        }
        if (cnt == 10) {
            Output::info("  ... (truncated)");
        }
    } else {
        Output::warning("No README file found for this package");
    }
} 