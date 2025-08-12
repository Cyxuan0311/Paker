#include "Paker/install.h"
#include "Paker/list.h"
#include "Paker/lock.h"
#include "Paker/info.h"
#include "Paker/update.h"
#include "Paker/utils.h"
#include "Paker/output.h"
#include "Recorder/record.h"
#include <iostream>
#include "third_party/CLI11.hpp"

int run_cli(int argc, char* argv[]) {
    CLI::App app{"Paker - C++ Package Manager"};

    // 全局选项
    bool no_color = false;
    bool verbose = false;
    app.add_flag("--no-color", no_color, "Disable colored output");
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
    
    // 设置输出选项
    app.preparse_callback([&](size_t) {
        Output::set_colored_output(!no_color);
        Output::set_verbose_mode(verbose);
    });

    // init
    auto init = app.add_subcommand("init", "Initialize a new Paker project");
    init->callback([]() {
        pm_init();
    });

    // add-remote
    std::string remote_name, remote_url;
    auto add_remote_cmd = app.add_subcommand("add-remote", "Add or update a custom dependency source");
    add_remote_cmd->add_option("name", remote_name, "Remote name")->required();
    add_remote_cmd->add_option("url", remote_url, "Remote url")->required();
    add_remote_cmd->callback([&]() {
        add_remote(remote_name, remote_url);
    });

    // remove-remote
    std::string remove_name;
    auto remove_remote_cmd = app.add_subcommand("remove-remote", "Remove a custom dependency source");
    remove_remote_cmd->add_option("name", remove_name, "Remote name")->required();
    remove_remote_cmd->callback([&]() {
        remove_remote(remove_name);
    });

    // add
    std::string add_pkg;
    auto add = app.add_subcommand("add", "Add a dependency or project info");
    add->add_option("package", add_pkg, "Package name to add");
    add->callback([&]() {
        if (!add_pkg.empty()) {
            auto custom_repos = get_custom_repos();
            auto all_repos = get_all_repos();
            if (custom_repos.count(add_pkg)) {
                pm_add(add_pkg);
            } else if (all_repos.count(add_pkg)) {
                Output::info("Using built-in url: " + all_repos[add_pkg]);
                pm_add(add_pkg);
            } else {
                Output::error("No url found for package: " + add_pkg + ". Please add a remote using 'add-remote'.");
            }
        }
    });

    // add desc
    std::string desc;
    auto add_desc = add->add_subcommand("desc", "Set project description");
    add_desc->add_option("desc", desc, "Project description")->required();
    add_desc->callback([&]() {
        pm_add_desc(desc);
    });

    // add vers
    std::string vers;
    auto add_vers = add->add_subcommand("vers", "Set project version");
    add_vers->add_option("vers", vers, "Project version")->required();
    add_vers->callback([&]() {
        pm_add_version(vers);
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

    // add-recursive
    std::string add_rec_pkg;
    auto add_rec = app.add_subcommand("add-recursive", "Recursively add a dependency and its dependencies");
    add_rec->add_option("package", add_rec_pkg, "Package name to add recursively")->required();
    add_rec->callback([&]() {
        pm_add_recursive(add_rec_pkg);
    });

    // tree
    auto tree = app.add_subcommand("tree", "Show dependency tree");
    tree->callback([]() {
        pm_tree();
    });

    // lock
    auto lock = app.add_subcommand("lock", "Generate or update Paker.lock file");
    lock->callback([]() {
        pm_lock();
    });

    // install-lock
    auto install_lock = app.add_subcommand("install-lock", "Install dependencies from Paker.lock");
    install_lock->callback([]() {
        pm_install_lock();
    });

    // upgrade
    std::string upgrade_pkg;
    auto upgrade = app.add_subcommand("upgrade", "Upgrade all dependencies or a specific dependency");
    upgrade->add_option("package", upgrade_pkg, "Package name to upgrade (optional)");
    upgrade->callback([&]() {
        pm_upgrade(upgrade_pkg);
    });

    // search
    std::string search_kw;
    auto search = app.add_subcommand("search", "Search available packages");
    search->add_option("keyword", search_kw, "Keyword to search")->required();
    search->callback([&]() {
        pm_search(search_kw);
    });

    // info
    std::string info_pkg;
    auto info = app.add_subcommand("info", "Show package info");
    info->add_option("package", info_pkg, "Package name")->required();
    info->callback([&]() {
        pm_info(info_pkg);
    });

    // update
    auto update = app.add_subcommand("update", "Update all local packages");
    update->callback([]() {
        pm_update();
    });

    // clean
    auto clean = app.add_subcommand("clean", "Clean unused or broken packages");
    clean->callback([]() {
        pm_clean();
    });

    // record commands
    std::string record_pkg;
    auto record_show = app.add_subcommand("record-show", "Show package installation record");
    record_show->add_option("package", record_pkg, "Package name")->required();
    record_show->callback([&]() {
        Recorder::Record record(get_record_file_path());
        if (record.isPackageInstalled(record_pkg)) {
            record.showPackageFiles(record_pkg);
        } else {
            std::cout << "Package '" << record_pkg << "' not found in installation records." << std::endl;
        }
    });

    auto record_list = app.add_subcommand("record-list", "List all installed packages with records");
    record_list->callback([]() {
        Recorder::Record record(get_record_file_path());
        record.showAllPackages();
    });

    auto record_files = app.add_subcommand("record-files", "Get all files for a package");
    record_files->add_option("package", record_pkg, "Package name")->required();
    record_files->callback([&]() {
        Recorder::Record record(get_record_file_path());
        if (record.isPackageInstalled(record_pkg)) {
            std::vector<std::string> files = record.getPackageFiles(record_pkg);
            std::cout << "Files for package '" << record_pkg << "':" << std::endl;
            for (const auto& file : files) {
                std::cout << "  " << file << std::endl;
            }
        } else {
            std::cout << "Package '" << record_pkg << "' not found in installation records." << std::endl;
        }
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
} 