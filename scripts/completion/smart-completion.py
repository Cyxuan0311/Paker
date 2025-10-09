#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Paker 智能补全逻辑实现
支持上下文感知、智能建议、错误处理
"""

import os
import json
import subprocess
import sys
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import argparse

class PakerSmartCompletion:
    """Paker 智能补全核心类"""
    
    def __init__(self):
        self.project_root = self._find_project_root()
        self.cache_file = os.path.expanduser("~/.paker/completion_cache.json")
        self.cache_data = self._load_cache()
        
    def _find_project_root(self) -> Optional[str]:
        """查找Paker项目根目录"""
        current = Path.cwd()
        for parent in [current] + list(current.parents):
            if (parent / "Paker.json").exists():
                return str(parent)
        return None
    
    def _load_cache(self) -> Dict:
        """加载补全缓存"""
        if os.path.exists(self.cache_file):
            try:
                with open(self.cache_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except:
                pass
        return {
            "packages": [],
            "remotes": [],
            "installed_packages": [],
            "cached_packages": [],
            "last_updated": 0
        }
    
    def _save_cache(self):
        """保存补全缓存"""
        os.makedirs(os.path.dirname(self.cache_file), exist_ok=True)
        with open(self.cache_file, 'w', encoding='utf-8') as f:
            json.dump(self.cache_data, f, ensure_ascii=False, indent=2)
    
    def _run_paker_command(self, command: List[str]) -> Optional[str]:
        """执行Paker命令并返回结果"""
        try:
            result = subprocess.run(
                ["Paker"] + command,
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                return result.stdout.strip()
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        return None
    
    def get_installed_packages(self) -> List[str]:
        """获取已安装的包列表"""
        if not self.project_root:
            return []
        
        # 从缓存获取
        if self.cache_data.get("installed_packages"):
            return self.cache_data["installed_packages"]
        
        # 从Paker.json获取
        paker_json = Path(self.project_root) / "Paker.json"
        if paker_json.exists():
            try:
                with open(paker_json, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    packages = list(data.get("dependencies", {}).keys())
                    self.cache_data["installed_packages"] = packages
                    self._save_cache()
                    return packages
            except:
                pass
        
        # 从Paker命令获取
        result = self._run_paker_command(["list", "--names-only"])
        if result:
            packages = result.split('\n')
            self.cache_data["installed_packages"] = packages
            self._save_cache()
            return packages
        
        return []
    
    def get_cached_packages(self) -> List[str]:
        """获取缓存中的包列表"""
        if self.cache_data.get("cached_packages"):
            return self.cache_data["cached_packages"]
        
        result = self._run_paker_command(["cache", "list", "--names-only"])
        if result:
            packages = result.split('\n')
            self.cache_data["cached_packages"] = packages
            self._save_cache()
            return packages
        
        return []
    
    def get_configured_remotes(self) -> List[str]:
        """获取已配置的远程源"""
        if self.cache_data.get("remotes"):
            return self.cache_data["remotes"]
        
        result = self._run_paker_command(["remote-list", "--names-only"])
        if result:
            remotes = result.split('\n')
            self.cache_data["remotes"] = remotes
            self._save_cache()
            return remotes
        
        return []
    
    def get_common_packages(self) -> List[Tuple[str, str]]:
        """获取常用包列表"""
        return [
            ("fmt", "现代C++格式化库"),
            ("spdlog", "快速C++日志库"),
            ("nlohmann-json", "现代C++ JSON库"),
            ("boost", "Boost C++库集合"),
            ("catch2", "现代C++测试框架"),
            ("gtest", "Google测试框架"),
            ("benchmark", "Google基准测试框架"),
            ("eigen", "线性代数库"),
            ("opencv", "计算机视觉库"),
            ("qt", "跨平台GUI框架")
        ]
    
    def get_common_remotes(self) -> List[Tuple[str, str]]:
        """获取常用远程源"""
        return [
            ("github", "GitHub远程源"),
            ("gitlab", "GitLab远程源"),
            ("bitbucket", "Bitbucket远程源"),
            ("custom", "自定义远程源")
        ]
    
    def get_git_services(self) -> List[Tuple[str, str]]:
        """获取Git服务列表"""
        return [
            ("github.com", "GitHub"),
            ("gitlab.com", "GitLab"),
            ("bitbucket.org", "Bitbucket"),
            ("gitee.com", "Gitee"),
            ("dev.azure.com", "Azure DevOps")
        ]
    
    def analyze_context(self, words: List[str]) -> Dict:
        """分析命令上下文"""
        context = {
            "command": None,
            "subcommand": None,
            "flags": [],
            "arguments": [],
            "is_in_project": self.project_root is not None,
            "suggestions": []
        }
        
        if len(words) > 1:
            context["command"] = words[1]
        
        if len(words) > 2:
            context["subcommand"] = words[2]
        
        # 分析标志
        for word in words:
            if word.startswith('--'):
                context["flags"].append(word)
            elif not word.startswith('-') and word != "Paker":
                context["arguments"].append(word)
        
        return context
    
    def generate_suggestions(self, context: Dict) -> List[str]:
        """生成智能建议"""
        suggestions = []
        
        if not context["command"]:
            # 主命令建议
            suggestions = [
                "init", "add", "remove", "list", "tree", "search", "info",
                "update", "upgrade", "lock", "cache", "monitor", "version",
                "parse", "io", "source-add", "source-rm", "remove-project",
                "suggestion"
            ]
        elif context["command"] == "add":
            # 添加包建议
            if context["is_in_project"]:
                suggestions = self.get_installed_packages()
            else:
                suggestions = [pkg[0] for pkg in self.get_common_packages()]
        elif context["command"] == "remove":
            # 移除包建议
            suggestions = self.get_installed_packages()
        elif context["command"] == "lock":
            # 锁定管理建议
            suggestions = ["install", "resolve", "check", "fix", "validate"]
        elif context["command"] == "cache":
            # 缓存管理建议
            if context["subcommand"] in ["add", "remove"]:
                suggestions = self.get_cached_packages()
            else:
                suggestions = ["add", "remove", "status", "clean", "warmup"]
        elif context["command"] == "monitor":
            # 监控管理建议
            suggestions = ["enable", "clear", "perf", "analyze", "diagnose"]
        elif context["command"] == "version":
            # 版本管理建议
            if context["subcommand"] == "rollback":
                if "--list" in context["flags"] or "--check" in context["flags"]:
                    suggestions = self.get_installed_packages()
                else:
                    suggestions = ["--previous", "--timestamp", "--force", "--list", "--check", "--stats"]
            elif context["subcommand"] == "history":
                suggestions = ["--clean", "--export", "--import", "--max-entries"]
            elif context["subcommand"] == "record":
                suggestions = ["--list", "--files"]
            else:
                suggestions = ["rollback", "history", "record", "--short", "--build", "--check"]
        elif context["command"] == "parse":
            # 解析管理建议
            suggestions = ["--stats", "--config", "--clear", "--opt", "--validate"]
        elif context["command"] == "io":
            # I/O管理建议
            suggestions = ["--stats", "--config", "--test", "--bench", "--opt"]
        elif context["command"] == "source-add":
            # 源添加建议
            suggestions = [remote[0] for remote in self.get_common_remotes()]
        elif context["command"] == "source-rm":
            # 源移除建议
            suggestions = self.get_configured_remotes()
        
        return suggestions
    
    def get_smart_tips(self, context: Dict) -> List[str]:
        """获取智能提示"""
        tips = []
        
        if context["command"] == "add":
            tips.append("💡 提示: 使用 'Paker add <package>' 添加依赖包")
            tips.append("   示例: Paker add fmt spdlog nlohmann-json")
        elif context["command"] == "lock":
            tips.append("💡 提示: 使用 'Paker lock' 生成锁定文件")
            tips.append("   使用 'Paker lock install' 从锁定文件安装")
        elif context["command"] == "cache":
            tips.append("💡 提示: 使用 'Paker cache status' 查看缓存状态")
            tips.append("   使用 'Paker cache clean --smart' 智能清理缓存")
        elif context["command"] == "monitor":
            tips.append("💡 提示: 使用 'Paker monitor enable' 启用监控")
            tips.append("   使用 'Paker monitor perf' 生成性能报告")
        elif context["command"] == "version":
            if context["subcommand"] == "rollback":
                tips.append("💡 提示: 使用 'Paker version rollback --list <package>' 查看可回滚版本")
                tips.append("   使用 'Paker version rollback <package> <version>' 回滚到指定版本")
            else:
                tips.append("💡 提示: 使用 'Paker version' 显示版本信息")
                tips.append("   使用 'Paker version --short' 显示简短版本")
        elif not context["is_in_project"]:
            tips.append("💡 提示: 当前目录不是Paker项目")
            tips.append("   使用 'Paker init' 初始化项目")
        
        return tips
    
    def handle_error(self, error_type: str, command: str) -> List[str]:
        """处理错误并提供建议"""
        suggestions = []
        
        if error_type == "command_not_found":
            suggestions.append(f"❌ 命令未找到: {command}")
            suggestions.append("💡 建议: 使用 'Paker --help' 查看可用命令")
        elif error_type == "package_not_found":
            suggestions.append(f"❌ 包未找到: {command}")
            suggestions.append("💡 建议: 使用 'Paker search <package>' 搜索包")
        elif error_type == "not_in_project":
            suggestions.append("❌ 当前目录不是Paker项目")
            suggestions.append("💡 建议: 使用 'Paker init' 初始化项目")
        elif error_type == "permission_denied":
            suggestions.append("❌ 权限不足")
            suggestions.append("💡 建议: 检查文件权限或使用sudo")
        
        return suggestions
    
    def update_cache(self, silent=False):
        """更新补全缓存"""
        if not silent:
            print("🔄 更新补全缓存...")
        
        # 更新已安装包列表
        if self.project_root:
            self.cache_data["installed_packages"] = self.get_installed_packages()
        
        # 更新缓存包列表
        self.cache_data["cached_packages"] = self.get_cached_packages()
        
        # 更新远程源列表
        self.cache_data["remotes"] = self.get_configured_remotes()
        
        # 更新时间戳
        import time
        self.cache_data["last_updated"] = int(time.time())
        
        # 保存缓存
        self._save_cache()
        if not silent:
            print("✅ 补全缓存已更新")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="Paker 智能补全")
    parser.add_argument("--suggest", nargs="*", help="获取建议")
    parser.add_argument("--tips", nargs="*", help="获取提示")
    parser.add_argument("--update-cache", action="store_true", help="更新缓存")
    parser.add_argument("--silent", action="store_true", help="静默模式")
    parser.add_argument("--error", help="处理错误")
    
    args = parser.parse_args()
    
    completion = PakerSmartCompletion()
    
    if args.update_cache:
        completion.update_cache(silent=args.silent)
    elif args.suggest:
        context = completion.analyze_context(args.suggest)
        suggestions = completion.generate_suggestions(context)
        for suggestion in suggestions:
            print(suggestion)
    elif args.tips:
        context = completion.analyze_context(args.tips)
        tips = completion.get_smart_tips(context)
        for tip in tips:
            print(tip)
    elif args.error:
        suggestions = completion.handle_error("command_not_found", args.error)
        for suggestion in suggestions:
            print(suggestion)

if __name__ == "__main__":
    main()
