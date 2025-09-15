#include "Paker/commands/install.h"
#include "Paker/commands/list.h"
#include "Paker/commands/lock.h"
#include "Paker/commands/info.h"
#include "Paker/commands/update.h"
#include "Paker/commands/monitor.h"
#include "Paker/commands/cache.h"
#include "Paker/commands/rollback.h"
#include "Paker/commands/warmup.h"
#include "Paker/commands/incremental_parse.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/sources.h"
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
        Paker::Output::set_colored_output(!no_color);
        Paker::Output::set_verbose_mode(verbose);
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
                Paker::Output::info("Using built-in url: " + all_repos[add_pkg]);
                pm_add(add_pkg);
            } else {
                Paker::Output::error("No url found for package: " + add_pkg + ". Please add a remote using 'add-remote'.");
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

    // add-parallel
    std::vector<std::string> add_parallel_pkgs;
    auto add_parallel = app.add_subcommand("add-parallel", "Add multiple dependencies in parallel");
    add_parallel->add_option("packages", add_parallel_pkgs, "Package names")->required();
    add_parallel->callback([&]() {
        pm_add_parallel(add_parallel_pkgs);
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
        Paker::pm_performance_report(perf_output);
    });

    // analyze-dependencies
    std::string analyze_output;
    auto analyze_deps = app.add_subcommand("analyze-dependencies", "Analyze dependencies");
    analyze_deps->add_option("-o,--output", analyze_output, "Output file (optional)");
    analyze_deps->callback([&]() {
        Paker::pm_analyze_dependencies(analyze_output);
    });

    // diagnose
    std::string diagnose_output;
    auto diagnose = app.add_subcommand("diagnose", "Run diagnostic checks");
    diagnose->add_option("-o,--output", diagnose_output, "Output file (optional)");
    diagnose->callback([&]() {
        Paker::pm_diagnose(diagnose_output);
    });

    // monitor-enable
    bool monitor_enable = true;
    auto monitor_enable_cmd = app.add_subcommand("monitor-enable", "Enable performance monitoring");
    monitor_enable_cmd->add_flag("--disable", monitor_enable, "Disable monitoring")->default_val(true);
    monitor_enable_cmd->callback([&]() {
        Paker::pm_monitor_enable(monitor_enable);
    });

    // monitor-clear
    auto monitor_clear = app.add_subcommand("monitor-clear", "Clear performance monitoring data");
    monitor_clear->callback([]() {
        Paker::pm_monitor_clear();
    });

    // cache commands
    std::string cache_pkg, cache_version;
    
    // cache-install
    auto cache_install = app.add_subcommand("cache-install", "Install package to global cache");
    cache_install->add_option("package", cache_pkg, "Package name")->required();
    cache_install->add_option("version", cache_version, "Package version (optional)");
    cache_install->callback([&]() {
        Paker::pm_cache_install(cache_pkg, cache_version);
    });
    
    // cache-remove
    auto cache_remove = app.add_subcommand("cache-remove", "Remove package from global cache");
    cache_remove->add_option("package", cache_pkg, "Package name")->required();
    cache_remove->add_option("version", cache_version, "Package version (optional)");
    cache_remove->callback([&]() {
        Paker::pm_cache_remove(cache_pkg, cache_version);
    });
    
    // cache-list
    auto cache_list = app.add_subcommand("cache-list", "List all cached packages");
    cache_list->callback([]() {
        Paker::pm_cache_list();
    });
    
    // cache-info
    auto cache_info = app.add_subcommand("cache-info", "Show cached package information");
    cache_info->add_option("package", cache_pkg, "Package name")->required();
    cache_info->callback([&]() {
        Paker::pm_cache_info(cache_pkg);
    });
    
    // cache-cleanup
    auto cache_cleanup = app.add_subcommand("cache-cleanup", "Clean up unused packages from cache");
    cache_cleanup->callback([]() {
        Paker::pm_cache_cleanup();
    });

    // LRU缓存管理命令
    auto cache_init_lru = app.add_subcommand("cache-init-lru", "Initialize LRU cache manager");
    cache_init_lru->callback([]() {
        Paker::pm_cache_init_lru();
    });

    auto cache_lru_stats = app.add_subcommand("cache-lru-stats", "Show LRU cache statistics");
    cache_lru_stats->callback([]() {
        Paker::pm_cache_lru_stats();
    });

    auto cache_lru_status = app.add_subcommand("cache-lru-status", "Show LRU cache status and health");
    cache_lru_status->callback([]() {
        Paker::pm_cache_lru_status();
    });

    auto cache_smart_cleanup = app.add_subcommand("cache-smart-cleanup", "Perform smart cache cleanup");
    cache_smart_cleanup->callback([]() {
        Paker::pm_cache_smart_cleanup();
    });

    auto cache_most_accessed = app.add_subcommand("cache-most-accessed", "Show most accessed packages");
    cache_most_accessed->callback([]() {
        Paker::pm_cache_most_accessed();
    });

    auto cache_oldest_items = app.add_subcommand("cache-oldest-items", "Show oldest cached items");
    cache_oldest_items->callback([]() {
        Paker::pm_cache_oldest_items();
    });

    auto cache_optimization_advice = app.add_subcommand("cache-optimization-advice", "Get cache optimization advice");
    cache_optimization_advice->callback([]() {
        Paker::pm_cache_optimization_advice();
    });

    // warmup commands
    auto warmup = app.add_subcommand("warmup", "Start cache warmup process");
    warmup->callback([]() {
        Paker::pm_warmup();
    });

    auto warmup_analyze = app.add_subcommand("warmup-analyze", "Analyze project dependencies for warmup");
    warmup_analyze->callback([]() {
        Paker::pm_warmup_analyze();
    });

    auto warmup_stats = app.add_subcommand("warmup-stats", "Show cache warmup statistics");
    warmup_stats->callback([]() {
        Paker::pm_warmup_stats();
    });

    auto warmup_config = app.add_subcommand("warmup-config", "Show cache warmup configuration");
    warmup_config->callback([]() {
        Paker::pm_warmup_config();
    });
    
    // incremental parse commands
    auto incremental_parse = app.add_subcommand("incremental-parse", "Start incremental dependency parsing");
    incremental_parse->callback([]() {
        Paker::pm_incremental_parse();
    });
    
    auto incremental_parse_stats = app.add_subcommand("incremental-parse-stats", "Show incremental parse statistics");
    incremental_parse_stats->callback([]() {
        Paker::pm_incremental_parse_stats();
    });
    
    auto incremental_parse_config = app.add_subcommand("incremental-parse-config", "Show incremental parse configuration");
    incremental_parse_config->callback([]() {
        Paker::pm_incremental_parse_config();
    });
    
    auto incremental_parse_clear = app.add_subcommand("incremental-parse-clear-cache", "Clear incremental parse cache");
    incremental_parse_clear->callback([]() {
        Paker::pm_incremental_parse_clear_cache();
    });
    
    auto incremental_parse_optimize = app.add_subcommand("incremental-parse-optimize", "Optimize incremental parse cache");
    incremental_parse_optimize->callback([]() {
        Paker::pm_incremental_parse_optimize();
    });
    
    auto incremental_parse_validate = app.add_subcommand("incremental-parse-validate", "Validate incremental parse cache integrity");
    incremental_parse_validate->callback([]() {
        Paker::pm_incremental_parse_validate();
    });
    
    // cache-stats
    auto cache_stats = app.add_subcommand("cache-stats", "Show cache statistics");
    cache_stats->callback([]() {
        Paker::pm_cache_stats();
    });
    
    // cache-status
    auto cache_status = app.add_subcommand("cache-status", "Show detailed cache status and health");
    cache_status->callback([]() {
        Paker::pm_cache_status();
    });
    
    // cache-optimize
    auto cache_optimize = app.add_subcommand("cache-optimize", "Optimize cache performance and storage");
    cache_optimize->callback([]() {
        Paker::pm_cache_optimize();
    });
    
    // cache-migrate
    std::string migrate_path;
    auto cache_migrate = app.add_subcommand("cache-migrate", "Migrate project from legacy mode to cache mode");
    cache_migrate->add_option("project_path", migrate_path, "Project path (optional, defaults to current directory)");
    cache_migrate->callback([&]() {
        Paker::pm_cache_migrate(migrate_path);
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

    // Rollback commands
    std::string rollback_pkg, rollback_version, rollback_timestamp;
    bool force_rollback = false;
    
    // rollback-to-version
    auto rollback_version_cmd = app.add_subcommand("rollback-to-version", "Rollback package to specific version");
    rollback_version_cmd->add_option("package", rollback_pkg, "Package name")->required();
    rollback_version_cmd->add_option("version", rollback_version, "Target version")->required();
    rollback_version_cmd->add_flag("--force", force_rollback, "Force rollback (skip safety checks)");
    rollback_version_cmd->callback([&]() {
        Paker::pm_rollback_to_version(rollback_pkg, rollback_version, force_rollback);
    });
    
    // rollback-to-previous
    auto rollback_previous_cmd = app.add_subcommand("rollback-to-previous", "Rollback package to previous version");
    rollback_previous_cmd->add_option("package", rollback_pkg, "Package name")->required();
    rollback_previous_cmd->add_flag("--force", force_rollback, "Force rollback (skip safety checks)");
    rollback_previous_cmd->callback([&]() {
        Paker::pm_rollback_to_previous(rollback_pkg, force_rollback);
    });
    
    // rollback-to-timestamp
    auto rollback_timestamp_cmd = app.add_subcommand("rollback-to-timestamp", "Rollback all packages to specific timestamp");
    rollback_timestamp_cmd->add_option("timestamp", rollback_timestamp, "Target timestamp (YYYY-MM-DD HH:MM:SS)")->required();
    rollback_timestamp_cmd->add_flag("--force", force_rollback, "Force rollback (skip safety checks)");
    rollback_timestamp_cmd->callback([&]() {
        Paker::pm_rollback_to_timestamp(rollback_timestamp, force_rollback);
    });
    
    // history-show
    auto history_show_cmd = app.add_subcommand("history-show", "Show version history");
    history_show_cmd->add_option("package", rollback_pkg, "Package name (optional)");
    history_show_cmd->callback([&]() {
        Paker::pm_history_show(rollback_pkg);
    });
    
    // rollback-list
    auto rollback_list_cmd = app.add_subcommand("rollback-list", "List rollbackable versions for a package");
    rollback_list_cmd->add_option("package", rollback_pkg, "Package name")->required();
    rollback_list_cmd->callback([&]() {
        Paker::pm_rollback_list(rollback_pkg);
    });
    
    // rollback-check
    auto rollback_check_cmd = app.add_subcommand("rollback-check", "Check rollback safety for a package");
    rollback_check_cmd->add_option("package", rollback_pkg, "Package name")->required();
    rollback_check_cmd->add_option("version", rollback_version, "Target version")->required();
    rollback_check_cmd->callback([&]() {
        Paker::pm_rollback_check(rollback_pkg, rollback_version);
    });
    
    // history-cleanup
    size_t max_entries = 50;
    auto history_cleanup_cmd = app.add_subcommand("history-cleanup", "Clean up old history records");
    history_cleanup_cmd->add_option("max_entries", max_entries, "Maximum entries to keep (default: 50)");
    history_cleanup_cmd->callback([&]() {
        Paker::pm_history_cleanup(max_entries);
    });
    
    // history-export
    std::string export_path;
    auto history_export_cmd = app.add_subcommand("history-export", "Export history records");
    history_export_cmd->add_option("path", export_path, "Export file path")->required();
    history_export_cmd->callback([&]() {
        Paker::pm_history_export(export_path);
    });
    
    // history-import
    std::string import_path;
    auto history_import_cmd = app.add_subcommand("history-import", "Import history records");
    history_import_cmd->add_option("path", import_path, "Import file path")->required();
    history_import_cmd->callback([&]() {
        Paker::pm_history_import(import_path);
    });
    
    // rollback-stats
    auto rollback_stats_cmd = app.add_subcommand("rollback-stats", "Show rollback statistics");
    rollback_stats_cmd->callback([]() {
        Paker::pm_rollback_stats();
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
} 