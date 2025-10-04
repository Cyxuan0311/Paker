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
#include "Paker/commands/async_io.h"
#include "Paker/commands/version.h"
#include "Paker/commands/remove_project.h"
#include "Paker/core/utils.h"
#include "Paker/core/output.h"
#include "Paker/core/package_manager.h"
#include "Paker/dependency/sources.h"
#include "Paker/version.h"
#include "Recorder/record.h"
#include <iostream>
#include "third_party/CLI11.hpp"

int run_cli(int argc, char* argv[]) {
    CLI::App app{"Paker - C++ Package Manager"};

    // 全局选项
    bool no_color = false;
    bool version = false;
    bool dev_mode = false;
    app.add_flag("--no-color", no_color, "Disable colored output");
    app.add_flag("--version", version, "Show version information");
    app.add_flag("--dev", dev_mode, "Enable development mode (show advanced commands)");
    
    // 设置输出选项
    app.preparse_callback([&](size_t) {
        Paker::Output::set_colored_output(!no_color);
        
        // 处理版本信息
        if (version) {
            std::cout << Paker::Version::get_detailed_version() << std::endl;
            exit(0);
        }
    });
    

    // init
    auto init = app.add_subcommand("init", "Initialize a new Paker project");
    init->callback([]() {
        pm_init();
    });

    // add-remote
    std::string remote_name, remote_url;
    auto add_remote_cmd = app.add_subcommand("remote-add", "Add or update a custom dependency source");
    add_remote_cmd->add_option("name", remote_name, "Remote name")->required();
    add_remote_cmd->add_option("url", remote_url, "Remote url")->required();
    add_remote_cmd->callback([&]() {
        add_remote(remote_name, remote_url);
    });

    // remove-remote
    std::string remove_name;
    auto remove_remote_cmd = app.add_subcommand("remote-rm", "Remove a custom dependency source");
    remove_remote_cmd->add_option("name", remove_name, "Remote name")->required();
    remove_remote_cmd->callback([&]() {
        remove_remote(remove_name);
    });

    // add
    std::string add_pkg;
    auto add = app.add_subcommand("add", "Add a dependency or project info");
    add->add_option("package", add_pkg, "Package name or URL to add");
    add->callback([&]() {
        if (!add_pkg.empty()) {
            // 检查是否是URL（以http://或https://或git@开头）
            if (add_pkg.find("http://") == 0 || 
                add_pkg.find("https://") == 0 || 
                add_pkg.find("git@") == 0 ||
                add_pkg.find("git://") == 0) {
                // 直接使用URL添加
                Paker::Output::info("Adding package from URL: " + add_pkg);
                pm_add_url(add_pkg);
            } else {
                // 从预配置的源添加
                auto custom_repos = get_custom_repos();
                auto all_repos = get_all_repos();
                if (custom_repos.count(add_pkg)) {
                    pm_add(add_pkg);
                } else if (all_repos.count(add_pkg)) {
                    Paker::Output::info("Using built-in url: " + all_repos[add_pkg]);
                    pm_add(add_pkg);
                } else {
                    Paker::Output::error("No url found for package: " + add_pkg + ". Please add a remote using 'remote-add' or use a direct URL.");
                }
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
    auto add_parallel = app.add_subcommand("add-p", "Add multiple dependencies in parallel");
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
    auto add_rec = app.add_subcommand("add-r", "Recursively add a dependency and its dependencies");
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
    auto install_lock = app.add_subcommand("install-l", "Install dependencies from Paker.lock");
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
    auto resolve_deps = app.add_subcommand("resolve", "Resolve project dependencies");
    resolve_deps->callback([]() {
        pm_resolve_dependencies();
    });

    // check-conflicts
    auto check_conflicts = app.add_subcommand("check", "Check for dependency conflicts");
    check_conflicts->callback([]() {
        pm_check_conflicts();
    });

    // resolve-conflicts
    auto resolve_conflicts = app.add_subcommand("fix", "Resolve dependency conflicts");
    resolve_conflicts->callback([]() {
        pm_resolve_conflicts();
    });

    // validate-dependencies
    auto validate_deps = app.add_subcommand("validate", "Validate dependencies");
    validate_deps->callback([]() {
        pm_validate_dependencies();
    });

    // performance-report
    std::string perf_output;
    auto perf_report = app.add_subcommand("perf", "Generate performance report");
    perf_report->add_option("-o,--output", perf_output, "Output file (optional)");
    perf_report->callback([&]() {
        Paker::pm_performance_report(perf_output);
    });

    // analyze-dependencies
    std::string analyze_output;
    auto analyze_deps = app.add_subcommand("analyze", "Analyze dependencies");
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

    // 统一的缓存管理命令
    std::string cache_pkg, cache_version;
    bool cache_smart = false, cache_force = false, cache_detailed = false;
    bool cache_lru_stats = false, cache_lru_status = false;
    
    auto cache_cmd = app.add_subcommand("cache", "Cache management");
    
    // cache add <package> [version]
    auto cache_add = cache_cmd->add_subcommand("add", "Install package to global cache");
    cache_add->add_option("package", cache_pkg, "Package name")->required();
    cache_add->add_option("version", cache_version, "Package version (optional)");
    cache_add->callback([&]() {
        Paker::pm_cache_install(cache_pkg, cache_version);
    });
    
    // cache remove <package> [version]
    auto cache_remove = cache_cmd->add_subcommand("remove", "Remove package from global cache");
    cache_remove->add_option("package", cache_pkg, "Package name")->required();
    cache_remove->add_option("version", cache_version, "Package version (optional)");
    cache_remove->callback([&]() {
        Paker::pm_cache_remove(cache_pkg, cache_version);
    });
    
    // cache status [--detailed]
    auto cache_status = cache_cmd->add_subcommand("status", "Show cache status and statistics");
    cache_status->add_flag("--detailed", cache_detailed, "Show detailed information");
    cache_status->callback([&]() {
        if (cache_detailed) {
            Paker::pm_cache_status();
        } else {
            Paker::pm_cache_stats();
        }
    });
    
    // cache clean [--smart] [--force]
    auto cache_clean = cache_cmd->add_subcommand("clean", "Clean up cache");
    cache_clean->add_flag("--smart", cache_smart, "Use smart cleanup strategy");
    cache_clean->add_flag("--force", cache_force, "Force cleanup without confirmation");
    cache_clean->callback([&]() {
        if (cache_smart) {
            Paker::pm_cache_smart_cleanup();
        } else {
            Paker::pm_cache_cleanup();
        }
    });
    
    // cache lru [--stats] [--status]
    auto cache_lru = cache_cmd->add_subcommand("lru", "LRU cache management");
    cache_lru->add_flag("--stats", cache_lru_stats, "Show LRU statistics");
    cache_lru->add_flag("--status", cache_lru_status, "Show LRU status");
    cache_lru->callback([&]() {
        if (cache_lru_stats) {
            Paker::pm_cache_lru_stats();
        } else if (cache_lru_status) {
            Paker::pm_cache_lru_status();
        } else {
            Paker::pm_cache_init_lru();
        }
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
    
    // 统一的增量解析命令
    bool parse_stats = false, parse_config = false, parse_clear = false, parse_opt = false, parse_validate = false;
    
    auto parse_cmd = app.add_subcommand("parse", "Incremental dependency parsing");
    parse_cmd->add_flag("--stats", parse_stats, "Show parse statistics");
    parse_cmd->add_flag("--config", parse_config, "Show parse configuration");
    parse_cmd->add_flag("--clear", parse_clear, "Clear parse cache");
    parse_cmd->add_flag("--opt", parse_opt, "Optimize parse cache");
    parse_cmd->add_flag("--validate", parse_validate, "Validate parse cache integrity");
    parse_cmd->callback([&]() {
        if (parse_stats) {
            Paker::pm_incremental_parse_stats();
        } else if (parse_config) {
            Paker::pm_incremental_parse_config();
        } else if (parse_clear) {
            Paker::pm_incremental_parse_clear_cache();
        } else if (parse_opt) {
            Paker::pm_incremental_parse_optimize();
        } else if (parse_validate) {
            Paker::pm_incremental_parse_validate();
        } else {
            Paker::pm_incremental_parse();
        }
    });
    
    // 统一的异步I/O命令
    bool io_stats = false, io_config = false, io_test = false, io_bench = false, io_opt = false;
    
    auto io_cmd = app.add_subcommand("io", "Async I/O management");
    io_cmd->add_flag("--stats", io_stats, "Show I/O statistics");
    io_cmd->add_flag("--config", io_config, "Show I/O configuration");
    io_cmd->add_flag("--test", io_test, "Run I/O tests");
    io_cmd->add_flag("--bench", io_bench, "Run I/O benchmark");
    io_cmd->add_flag("--opt", io_opt, "Optimize I/O performance");
    io_cmd->callback([&]() {
        if (io_stats) {
            Paker::pm_async_io_stats();
        } else if (io_config) {
            Paker::pm_async_io_config();
        } else if (io_test) {
            Paker::pm_async_io_test();
        } else if (io_bench) {
            Paker::pm_async_io_benchmark();
        } else if (io_opt) {
            Paker::pm_async_io_optimize();
        } else {
            Paker::pm_async_io_stats(); // 默认显示统计信息
        }
    });
    
    // cache-migrate (开发模式命令)
    std::string migrate_path;
    auto cache_migrate = app.add_subcommand("cache-migrate", "Migrate project from legacy mode to cache mode");
    cache_migrate->add_option("project_path", migrate_path, "Project path (optional, defaults to current directory)");
    cache_migrate->callback([&]() {
        if (!dev_mode) {
            std::cout << "This command is only available in development mode. Use --dev flag." << std::endl;
            return;
        }
        Paker::pm_cache_migrate(migrate_path);
    });

    // 统一的记录管理命令
    std::string record_pkg;
    bool record_list = false, record_files = false;
    
    auto record_cmd = app.add_subcommand("record", "Package installation records");
    record_cmd->add_option("package", record_pkg, "Package name (optional)");
    record_cmd->add_flag("--list", record_list, "List all packages");
    record_cmd->add_flag("--files", record_files, "Show package files");
    record_cmd->callback([&]() {
        Recorder::Record record(get_record_file_path());
        if (record_list) {
            record.showAllPackages();
        } else if (record_files && !record_pkg.empty()) {
            if (record.isPackageInstalled(record_pkg)) {
                std::vector<std::string> files = record.getPackageFiles(record_pkg);
                std::cout << "Files for package '" << record_pkg << "':" << std::endl;
                for (const auto& file : files) {
                    std::cout << "  " << file << std::endl;
                }
            } else {
                std::cout << "Package '" << record_pkg << "' not found in installation records." << std::endl;
            }
        } else if (!record_pkg.empty()) {
            if (record.isPackageInstalled(record_pkg)) {
                record.showPackageFiles(record_pkg);
            } else {
                std::cout << "Package '" << record_pkg << "' not found in installation records." << std::endl;
            }
        } else {
            record.showAllPackages();
        }
    });

    // 统一的回滚管理命令
    std::string rollback_pkg, rollback_version, rollback_timestamp;
    bool force_rollback = false;
    bool rollback_previous = false, rollback_timestamp_flag = false;
    bool rollback_list = false, rollback_check = false, rollback_stats = false;
    
    auto rollback_cmd = app.add_subcommand("rollback", "Package rollback management");
    rollback_cmd->add_option("package", rollback_pkg, "Package name");
    rollback_cmd->add_option("version", rollback_version, "Target version");
    rollback_cmd->add_option("timestamp", rollback_timestamp, "Target timestamp (YYYY-MM-DD HH:MM:SS)");
    rollback_cmd->add_flag("--previous", rollback_previous, "Rollback to previous version");
    rollback_cmd->add_flag("--timestamp", rollback_timestamp_flag, "Rollback to timestamp");
    rollback_cmd->add_flag("--force", force_rollback, "Force rollback (skip safety checks)");
    rollback_cmd->add_flag("--list", rollback_list, "List rollbackable versions");
    rollback_cmd->add_flag("--check", rollback_check, "Check rollback safety");
    rollback_cmd->add_flag("--stats", rollback_stats, "Show rollback statistics");
    rollback_cmd->callback([&]() {
        if (rollback_list && !rollback_pkg.empty()) {
            Paker::pm_rollback_list(rollback_pkg);
        } else if (rollback_check && !rollback_pkg.empty() && !rollback_version.empty()) {
            Paker::pm_rollback_check(rollback_pkg, rollback_version);
        } else if (rollback_stats) {
            Paker::pm_rollback_stats();
        } else if (rollback_previous && !rollback_pkg.empty()) {
            Paker::pm_rollback_to_previous(rollback_pkg, force_rollback);
        } else if (rollback_timestamp_flag && !rollback_timestamp.empty()) {
            Paker::pm_rollback_to_timestamp(rollback_timestamp, force_rollback);
        } else if (!rollback_pkg.empty() && !rollback_version.empty()) {
            Paker::pm_rollback_to_version(rollback_pkg, rollback_version, force_rollback);
        } else {
            std::cout << "Usage: rollback <package> [version] [options]" << std::endl;
            std::cout << "       rollback --list <package>" << std::endl;
            std::cout << "       rollback --check <package> <version>" << std::endl;
            std::cout << "       rollback --stats" << std::endl;
        }
    });
    
    // 统一的历史管理命令
    std::string export_path, import_path;
    bool history_clean = false, history_export = false, history_import = false;
    size_t max_entries = 50;
    
    auto history_cmd = app.add_subcommand("history", "Version history management");
    history_cmd->add_option("package", rollback_pkg, "Package name (optional)");
    history_cmd->add_flag("--clean", history_clean, "Clean up old history records");
    history_cmd->add_flag("--export", history_export, "Export history records");
    history_cmd->add_flag("--import", history_import, "Import history records");
    history_cmd->add_option("--export-path", export_path, "Export file path");
    history_cmd->add_option("--import-path", import_path, "Import file path");
    history_cmd->add_option("--max-entries", max_entries, "Maximum entries to keep (default: 50)");
    history_cmd->callback([&]() {
        if (history_clean) {
            Paker::pm_history_cleanup(max_entries);
        } else if (history_export && !export_path.empty()) {
            Paker::pm_history_export(export_path);
        } else if (history_import && !import_path.empty()) {
            Paker::pm_history_import(import_path);
        } else {
            Paker::pm_history_show(rollback_pkg);
        }
    });
    
    // version command
    bool version_short = false;
    bool version_build = false;
    std::string version_check;
    
    auto version_cmd = app.add_subcommand("version", "Show version information");
    version_cmd->add_flag("--short", version_short, "Show short version");
    version_cmd->add_flag("--build", version_build, "Show build information");
    version_cmd->add_option("--check", version_check, "Check version compatibility");
    
    version_cmd->callback([&]() {
        if (version_short) {
            Paker::pm_version_short();
        } else if (version_build) {
            Paker::pm_version_build();
        } else if (!version_check.empty()) {
            Paker::pm_version_check(version_check);
        } else {
            Paker::pm_version();
        }
    });

    // remove project command
    bool force_remove = false;
    auto remove_project_cmd = app.add_subcommand("remove-project", "Remove Paker project completely");
    remove_project_cmd->add_flag("--force", force_remove, "Force removal without confirmation");
    remove_project_cmd->callback([&]() {
        if (force_remove) {
            Paker::pm_remove_project(true);
        } else {
            Paker::pm_remove_project_confirm();
        }
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
} 