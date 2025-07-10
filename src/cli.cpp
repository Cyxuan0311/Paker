#include "cli.h"
#include "package_manager.h"
#include "CLI11.hpp"
#include <iostream>

int run_cli(int argc, char* argv[]) {
    CLI::App app{"Paker - C++ Package Manager"};

    // init
    auto init = app.add_subcommand("init", "Initialize a new Paker project");
    init->callback([]() {
        pm_init();
    });

    // add
    std::string add_pkg;
    auto add = app.add_subcommand("add", "Add a dependency");
    add->add_option("package", add_pkg, "Package name to add")->required();
    add->callback([&]() {
        pm_add(add_pkg);
    });

    // remove
    std::string rm_pkg;
    auto remove = app.add_subcommand("remove", "Remove a dependency");
    remove->add_option("package", rm_pkg, "Package name to remove")->required();
    remove->callback([&]() {
        pm_remove(rm_pkg);
    });

    // list
    auto list = app.add_subcommand("list", "List dependencies");
    list->callback([]() {
        pm_list();
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
} 