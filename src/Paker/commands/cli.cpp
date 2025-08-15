#include "Paker/commands/install.h"
#include "Paker/commands/list.h"
#include "Paker/commands/lock.h"
#include "Paker/commands/info.h"
#include "Paker/commands/update.h"
#include "Paker/commands/monitor.h"
#include "Paker/commands/cache.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
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

    // resolve-dependencies
    auto resolve_deps = app.add_subcommand("resolve-dependencies", "Resolve project dependencies");
    resolve_deps->callback([]() {
        pm_resolve_dependencies();
    });

    // check-conflicts
    auto check_conflicts = app.add_subcommand("check-conflicts", "Check for dependency conflicts");
    check_conflicts->callback([]() {
        pm_check_conflicts();
    });

    // resolve-conflicts
    auto resolve_conflicts = app.add_subcommand("resolve-conflicts", "Resolve dependency conflicts");
    resolve_conflicts->callback([]() {
        pm_resolve_conflicts();
    });

    // validate-dependencies
    auto validate_deps = app.add_subcommand("validate-dependencies", "Validate dependencies");
    validate_deps->callback([]() {
        pm_validate_dependencies();
    });

    // performance-report
    std::string perf_output;
    auto perf_report = app.add_subcommand("performance-report", "Generate performance report");
    perf_report->add_option("-o,--output", perf_output, "Output file (optional)");
    perf_report->callback([&]() {
        pm_performance_report(perf_output);
    });

    // analyze-dependencies
    std::string analyze_output;
    auto analyze_deps = app.add_subcommand("analyze-dependencies", "Analyze dependencies");
    analyze_deps->add_option("-o,--output", analyze_output, "Output file (optional)");
    analyze_deps->callback([&]() {
        pm_analyze_dependencies(analyze_output);
    });

    // diagnose
    std::string diagnose_output;
    auto diagnose = app.add_subcommand("diagnose", "Run diagnostic checks");
    diagnose->add_option("-o,--output", diagnose_output, "Output file (optional)");
    diagnose->callback([&]() {
        pm_diagnose(diagnose_output);
    });

    // monitor-enable
    bool monitor_enable = true;
    auto monitor_enable_cmd = app.add_subcommand("monitor-enable", "Enable performance monitoring");
    monitor_enable_cmd->add_flag("--disable", monitor_enable, "Disable monitoring")->default_val(true);
    monitor_enable_cmd->callback([&]() {
        pm_monitor_enable(monitor_enable);
    });

    // monitor-clear
    auto monitor_clear = app.add_subcommand("monitor-clear", "Clear performance monitoring data");
    monitor_clear->callback([]() {
        pm_monitor_clear();
    });

    // cache commands
    std::string cache_pkg, cache_version;
    
    // cache-install
    auto cache_install = app.add_subcommand("cache-install", "Install package to global cache");
    cache_install->add_option("package", cache_pkg, "Package name")->required();
    cache_install->add_option("version", cache_version, "Package version (optional)");
    cache_install->callback([&]() {
        pm_cache_install(cache_pkg, cache_version);
    });
    
    // cache-remove
    auto cache_remove = app.add_subcommand("cache-remove", "Remove package from global cache");
    cache_remove->add_option("package", cache_pkg, "Package name")->required();
    cache_remove->add_option("version", cache_version, "Package version (optional)");
    cache_remove->callback([&]() {
        pm_cache_remove(cache_pkg, cache_version);
    });
    
    // cache-list
    auto cache_list = app.add_subcommand("cache-list", "List all cached packages");
    cache_list->callback([]() {
        pm_cache_list();
    });
    
    // cache-info
    auto cache_info = app.add_subcommand("cache-info", "Show cached package information");
    cache_info->add_option("package", cache_pkg, "Package name")->required();
    cache_info->callback([&]() {
        pm_cache_info(cache_pkg);
    });
    
    // cache-cleanup
    auto cache_cleanup = app.add_subcommand("cache-cleanup", "Clean up unused packages from cache");
    cache_cleanup->callback([]() {
        pm_cache_cleanup();
    });
    
    // cache-stats
    auto cache_stats = app.add_subcommand("cache-stats", "Show cache statistics");
    cache_stats->callback([]() {
        pm_cache_stats();
    });
    
    // cache-status
    auto cache_status = app.add_subcommand("cache-status", "Show detailed cache status and health");
    cache_status->callback([]() {
        pm_cache_status();
    });
    
    // cache-optimize
    auto cache_optimize = app.add_subcommand("cache-optimize", "Optimize cache performance and storage");
    cache_optimize->callback([]() {
        pm_cache_optimize();
    });
    
    // cache-migrate
    std::string migrate_path;
    auto cache_migrate = app.add_subcommand("cache-migrate", "Migrate project from legacy mode to cache mode");
    cache_migrate->add_option("project_path", migrate_path, "Project path (optional, defaults to current directory)");
    cache_migrate->callback([&]() {
        pm_cache_migrate(migrate_path);
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