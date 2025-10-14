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
#include "Paker/commands/suggestion.h"
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
    
    // 自定义帮助信息
    app.set_help_flag("-h,--help", "Print this help message and exit");
    app.set_help_all_flag("--help-all", "Print help for all commands and subcommands");
    app.footer("For more information, visit: https://github.com/Cyxuan0311/Paker");
    
    // 设置子命令显示
    app.require_subcommand(0, 1);  // 允许0或1个子命令
    
    // 设置输出选项
    app.preparse_callback([&](size_t) {
        Paker::Output::set_colored_output(!no_color);
        
        // 处理版本信息
        if (version) {
            std::cout << Paker::Version::get_detailed_version() << std::endl;
            exit(0);
        }
    });
    
    // ============================================================================
    // 1. 核心包管理命令 (Core Package Management)
    // ============================================================================
    
    // add
    std::string add_pkg;
    auto add = app.add_subcommand("add", "Add a dependency or project info");
    add->group("Core Package Management");
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
                    Paker::Output::error("No url found for package: " + add_pkg + ". Please add a remote using 'source add' or use a direct URL.");
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
    add_parallel->group("Core Package Management");
    add_parallel->add_option("packages", add_parallel_pkgs, "Package names")->required();
    add_parallel->callback([&]() {
        pm_add_parallel(add_parallel_pkgs);
    });

    // add-recursive
    std::string add_rec_pkg;
    auto add_rec = app.add_subcommand("add-r", "Recursively add a dependency and its dependencies");
    add_rec->group("Core Package Management");
    add_rec->add_option("package", add_rec_pkg, "Package name to add recursively")->required();
    add_rec->callback([&]() {
        pm_add_recursive(add_rec_pkg);
    });

    // remove
    std::string rm_pkg;
    auto remove = app.add_subcommand("remove", "Remove a dependency");
    remove->group("Core Package Management");
    remove->add_option("package", rm_pkg, "Package name to remove")->required();
    remove->callback([&]() {
        pm_remove(rm_pkg);
    });

    // list
    auto list = app.add_subcommand("list", "List dependencies");
    list->group("Core Package Management");
    list->callback([]() {
        pm_list();
    });

    // tree
    auto tree = app.add_subcommand("tree", "Show dependency tree");
    tree->group("Core Package Management");
    tree->callback([]() {
        pm_tree();
    });

    // upgrade
    std::string upgrade_pkg;
    auto upgrade = app.add_subcommand("upgrade", "Upgrade all dependencies or a specific dependency");
    upgrade->group("Core Package Management");
    upgrade->add_option("package", upgrade_pkg, "Package name to upgrade (optional)");
    upgrade->callback([&]() {
        pm_upgrade(upgrade_pkg);
    });

    // update
    auto update = app.add_subcommand("update", "Update all local packages");
    update->group("Core Package Management");
    update->callback([]() {
        pm_update();
    });

    // search
    std::string search_kw;
    auto search = app.add_subcommand("search", "Search available packages");
    search->group("Core Package Management");
    search->add_option("keyword", search_kw, "Keyword to search")->required();
    search->callback([&]() {
        pm_search(search_kw);
    });

    // info
    std::string info_pkg;
    auto info = app.add_subcommand("info", "Show package info");
    info->group("Core Package Management");
    info->add_option("package", info_pkg, "Package name")->required();
    info->callback([&]() {
        pm_info(info_pkg);
    });

    // clean
    auto clean = app.add_subcommand("clean", "Clean unused or broken packages");
    clean->group("Core Package Management");
    clean->callback([]() {
        pm_clean();
    });

    // install
    std::string install_pkg;
    auto install = app.add_subcommand("install", "Compile and install package to system");
    install->group("Core Package Management");
    install->add_option("package", install_pkg, "Package name to install")->required();
    install->callback([&]() {
        pm_install(install_pkg);
    });

    // install-p (parallel install)
    std::vector<std::string> install_parallel_pkgs;
    auto install_parallel = app.add_subcommand("install-p", "Parallel compile and install packages");
    install_parallel->group("Core Package Management");
    install_parallel->add_option("packages", install_parallel_pkgs, "Package names to install in parallel")->required();
    install_parallel->callback([&]() {
        pm_install_parallel(install_parallel_pkgs);
    });

    // uninstall
    std::string uninstall_pkg;
    auto uninstall = app.add_subcommand("uninstall", "Uninstall package from system");
    uninstall->group("Core Package Management");
    uninstall->add_option("package", uninstall_pkg, "Package name to uninstall")->required();
    uninstall->callback([&]() {
        pm_uninstall(uninstall_pkg);
    });

    // ============================================================================
    // 2. 依赖锁定命令 (Dependency Locking)
    // ============================================================================
    
    // 统一的lock命令
    auto lock_cmd = app.add_subcommand("lock", "Generate or update Paker.lock file");
    lock_cmd->group("Dependency Locking");
    
    // lock (默认行为 - 生成锁定文件)
    lock_cmd->callback([]() {
        pm_lock();
    });
    
    // lock install - 从锁定文件安装
    auto lock_install = lock_cmd->add_subcommand("install", "Install dependencies from lock file");
    lock_install->callback([]() {
        pm_add_lock();
    });
    
    // lock resolve - 解析依赖
    auto lock_resolve = lock_cmd->add_subcommand("resolve", "Resolve project dependencies");
    lock_resolve->callback([]() {
        pm_resolve_dependencies();
    });
    
    // lock check - 检查冲突
    auto lock_check = lock_cmd->add_subcommand("check", "Check for dependency conflicts");
    lock_check->callback([]() {
        pm_check_conflicts();
    });
    
    // lock fix - 修复冲突
    auto lock_fix = lock_cmd->add_subcommand("fix", "Resolve dependency conflicts");
    lock_fix->callback([]() {
        pm_resolve_conflicts();
    });
    
    // lock validate - 验证依赖
    auto lock_validate = lock_cmd->add_subcommand("validate", "Validate dependencies");
    lock_validate->callback([]() {
        pm_validate_dependencies();
    });

    // ============================================================================
    // 3. 缓存管理命令 (Cache Management)
    // ============================================================================
    
    // 统一的缓存管理命令
    std::string cache_pkg, cache_version;
    bool cache_smart = false, cache_force = false, cache_detailed = false;
    
    auto cache_cmd = app.add_subcommand("cache", "Add, remove, or manage cached packages");
    cache_cmd->group("Cache Management");
    
    // cache add <package> [version]
    auto cache_add = cache_cmd->add_subcommand("add", "Add package to cache");
    cache_add->add_option("package", cache_pkg, "Package name")->required();
    cache_add->add_option("version", cache_version, "Package version (optional)");
    cache_add->callback([&]() {
        Paker::pm_cache_install(cache_pkg, cache_version);
    });
    
    // cache remove <package> [version]
    auto cache_remove = cache_cmd->add_subcommand("remove", "Remove package from cache");
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
    auto cache_clean = cache_cmd->add_subcommand("clean", "Clean unused or broken packages from cache");
    cache_clean->add_flag("--smart", cache_smart, "Use smart cleanup strategy");
    cache_clean->add_flag("--force", cache_force, "Force cleanup without confirmation");
    cache_clean->callback([&]() {
        if (cache_smart) {
            Paker::pm_cache_smart_cleanup();
        } else {
            Paker::pm_cache_cleanup();
        }
    });
    
    // cache warmup
    auto cache_warmup = cache_cmd->add_subcommand("warmup", "Preload frequently used packages into cache");
    cache_warmup->callback([]() {
        Paker::pm_warmup();
    });

    // ============================================================================
    // 4. 性能监控命令 (Performance Monitoring)
    // ============================================================================
    
    // 统一的monitor命令
    std::string monitor_output;
    bool monitor_enable = true;
    
    auto monitor_cmd = app.add_subcommand("monitor", "Enable, manage, and analyze performance monitoring");
    monitor_cmd->group("Performance Monitoring");
    
    // monitor enable [--disable]
    auto monitor_enable_cmd = monitor_cmd->add_subcommand("enable", "Enable performance monitoring");
    monitor_enable_cmd->add_flag("--disable", monitor_enable, "Disable monitoring")->default_val(true);
    monitor_enable_cmd->callback([&]() {
        Paker::pm_monitor_enable(monitor_enable);
    });
    
    // monitor clear
    auto monitor_clear = monitor_cmd->add_subcommand("clear", "Clear performance monitoring data");
    monitor_clear->callback([]() {
        Paker::pm_monitor_clear();
    });
    
    // monitor perf [-o,--output]
    auto monitor_perf = monitor_cmd->add_subcommand("perf", "Generate performance report");
    monitor_perf->add_option("-o,--output", monitor_output, "Output file (optional)");
    monitor_perf->callback([&]() {
        Paker::pm_performance_report(monitor_output);
    });
    
    // monitor analyze [-o,--output]
    auto monitor_analyze = monitor_cmd->add_subcommand("analyze", "Analyze dependency structure and relationships");
    monitor_analyze->add_option("-o,--output", monitor_output, "Output file (optional)");
    monitor_analyze->callback([&]() {
        Paker::pm_analyze_dependencies(monitor_output);
    });
    
    // monitor diagnose [-o,--output]
    auto monitor_diagnose = monitor_cmd->add_subcommand("diagnose", "Run diagnostic checks for system health");
    monitor_diagnose->add_option("-o,--output", monitor_output, "Output file (optional)");
    monitor_diagnose->callback([&]() {
        Paker::pm_diagnose(monitor_output);
    });

    // ============================================================================
    // 5. 版本控制命令 (Version Control)
    // ============================================================================
    
    // 统一的version命令
    std::string version_pkg, version_target, version_timestamp;
    bool version_short = false, version_build = false, version_force = false;
    bool version_previous = false, version_timestamp_flag = false;
    bool version_list = false, version_check = false, version_stats = false;
    bool version_clean = false, version_export = false, version_import = false;
    bool version_record_list = false, version_record_files = false;
    std::string version_check_target, version_export_path, version_import_path;
    size_t version_max_entries = 50;
    
    auto version_cmd = app.add_subcommand("version", "Show version info, manage rollbacks, and view history");
    version_cmd->group("Version Control");
    
    // version (默认行为 - 显示版本信息)
    version_cmd->add_flag("--short", version_short, "Show short version");
    version_cmd->add_flag("--build", version_build, "Show build information");
    version_cmd->add_option("--check", version_check_target, "Check version compatibility");
    version_cmd->callback([&]() {
        // 只有在没有子命令时才显示版本信息
        if (version_cmd->get_subcommands().empty()) {
            if (version_short) {
                Paker::pm_version_short();
            } else if (version_build) {
                Paker::pm_version_build();
            } else if (!version_check_target.empty()) {
                Paker::pm_version_check(version_check_target);
            } else {
                Paker::pm_version();
            }
        }
    });
    
    // version rollback <package> [version] [options]
    auto version_rollback = version_cmd->add_subcommand("rollback", "Rollback package to previous or specific version");
    version_rollback->add_option("package", version_pkg, "Package name")->required();
    version_rollback->add_option("version", version_target, "Target version");
    version_rollback->add_option("timestamp", version_timestamp, "Target timestamp (YYYY-MM-DD HH:MM:SS)");
    version_rollback->add_flag("--previous", version_previous, "Rollback to previous version");
    version_rollback->add_flag("--timestamp", version_timestamp_flag, "Rollback to timestamp");
    version_rollback->add_flag("--force", version_force, "Force rollback (skip safety checks)");
    version_rollback->add_flag("--list", version_list, "List rollbackable versions");
    version_rollback->add_flag("--check", version_check, "Check rollback safety");
    version_rollback->add_flag("--stats", version_stats, "Show rollback statistics");
    version_rollback->callback([&]() {
        if (version_list) {
            Paker::pm_rollback_list(version_pkg);
        } else if (version_check && !version_target.empty()) {
            Paker::pm_rollback_check(version_pkg, version_target);
        } else if (version_stats) {
            Paker::pm_rollback_stats();
        } else if (version_previous) {
            Paker::pm_rollback_to_previous(version_pkg, version_force);
        } else if (version_timestamp_flag && !version_timestamp.empty()) {
            Paker::pm_rollback_to_timestamp(version_timestamp, version_force);
        } else if (!version_target.empty()) {
            Paker::pm_rollback_to_version(version_pkg, version_target, version_force);
        } else {
            std::cout << "Usage: version rollback <package> [version] [options]" << std::endl;
            std::cout << "       version rollback --list <package>" << std::endl;
            std::cout << "       version rollback --check <package> <version>" << std::endl;
            std::cout << "       version rollback --stats" << std::endl;
        }
    });
    
    // version history [package] [options]
    auto version_history = version_cmd->add_subcommand("history", "Show version history and manage records");
    version_history->add_option("package", version_pkg, "Package name (optional)");
    version_history->add_flag("--clean", version_clean, "Clean up old history records");
    version_history->add_flag("--export", version_export, "Export history records");
    version_history->add_flag("--import", version_import, "Import history records");
    version_history->add_option("--export-path", version_export_path, "Export file path");
    version_history->add_option("--import-path", version_import_path, "Import file path");
    version_history->add_option("--max-entries", version_max_entries, "Maximum entries to keep (default: 50)");
    version_history->callback([&]() {
        if (version_clean) {
            Paker::pm_history_cleanup(version_max_entries);
        } else if (version_export && !version_export_path.empty()) {
            Paker::pm_history_export(version_export_path);
        } else if (version_import && !version_import_path.empty()) {
            Paker::pm_history_import(version_import_path);
        } else {
            Paker::pm_history_show(version_pkg);
        }
    });
    
    // version record [package] [options]
    auto version_record = version_cmd->add_subcommand("record", "Show package installation records and files");
    version_record->add_option("package", version_pkg, "Package name (optional)");
    version_record->add_flag("--list", version_record_list, "List all packages");
    version_record->add_flag("--files", version_record_files, "Show package files");
    version_record->callback([&]() {
        Recorder::Record record(get_record_file_path());
        if (version_record_list) {
            record.showAllPackages();
        } else if (version_record_files && !version_pkg.empty()) {
            if (record.isPackageInstalled(version_pkg)) {
                std::vector<std::string> files = record.getPackageFiles(version_pkg);
                std::cout << "Files for package '" << version_pkg << "':" << std::endl;
                for (const auto& file : files) {
                    std::cout << "  " << file << std::endl;
                }
            } else {
                std::cout << "Package '" << version_pkg << "' not found in installation records." << std::endl;
            }
        } else if (!version_pkg.empty()) {
            if (record.isPackageInstalled(version_pkg)) {
                record.showPackageFiles(version_pkg);
            } else {
                std::cout << "Package '" << version_pkg << "' not found in installation records." << std::endl;
            }
        } else {
            record.showAllPackages();
        }
    });

    // ============================================================================
    // 6. 项目管理命令 (Project Management)
    // ============================================================================
    
    // init
    auto init = app.add_subcommand("init", "Initialize a new Paker project");
    init->group("Project Management");
    init->callback([]() {
        pm_init();
    });

    // remove project command
    bool force_remove = false;
    auto remove_project_cmd = app.add_subcommand("remove-project", "Remove Paker project completely");
    remove_project_cmd->group("Project Management");
    remove_project_cmd->add_flag("--force", force_remove, "Force removal without confirmation");
    remove_project_cmd->callback([&]() {
        if (force_remove) {
            Paker::pm_remove_project(true);
        } else {
            Paker::pm_remove_project_confirm();
        }
    });

    // suggestion - 智能包推荐
    std::string suggestion_category, suggestion_performance, suggestion_security;
    bool suggestion_detailed = false, suggestion_auto_install = false;
    std::string suggestion_export_path;
    
    auto suggestion = app.add_subcommand("suggestion", "Smart package recommendations based on project analysis");
    suggestion->group("Project Management");
    suggestion->add_option("--category", suggestion_category, "Filter by category (web, desktop, embedded, game)");
    suggestion->add_option("--performance", suggestion_performance, "Filter by performance level (low, medium, high)");
    suggestion->add_option("--security", suggestion_security, "Filter by security level (low, medium, high)");
    suggestion->add_flag("--detailed", suggestion_detailed, "Show detailed analysis and recommendations");
    suggestion->add_flag("--auto-install", suggestion_auto_install, "Automatically install recommended packages");
    suggestion->add_option("--export", suggestion_export_path, "Export analysis results to file");
    suggestion->callback([&]() {
        Paker::pm_smart_suggestion(suggestion_category, suggestion_performance, suggestion_security, 
                                   suggestion_detailed, suggestion_auto_install, suggestion_export_path);
    });

    // ============================================================================
    // 7. 依赖源管理命令 (Dependency Source Management)
    // ============================================================================
    
    // add-remote
    std::string remote_name, remote_url;
    auto add_remote_cmd = app.add_subcommand("source-add", "Add or update a custom dependency source");
    add_remote_cmd->group("Dependency Source Management");
    add_remote_cmd->add_option("name", remote_name, "Remote name")->required();
    add_remote_cmd->add_option("url", remote_url, "Remote url")->required();
    add_remote_cmd->callback([&]() {
        add_remote(remote_name, remote_url);
    });

    // remove-remote
    std::string remove_name;
    auto remove_remote_cmd = app.add_subcommand("source-rm", "Remove a custom dependency source");
    remove_remote_cmd->group("Dependency Source Management");
    remove_remote_cmd->add_option("name", remove_name, "Remote name")->required();
    remove_remote_cmd->callback([&]() {
        remove_remote(remove_name);
    });

    // ============================================================================
    // 8. 系统管理命令 (System Management)
    // ============================================================================
    
    // 统一的增量解析命令
    bool parse_stats = false, parse_config = false, parse_clear = false, parse_opt = false, parse_validate = false;
    
    auto parse_cmd = app.add_subcommand("parse", "Incremental dependency parsing");
    parse_cmd->group("System Management");
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
    io_cmd->group("System Management");
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

    CLI11_PARSE(app, argc, argv);
    return 0;
}
