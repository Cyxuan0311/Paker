#!/usr/bin/env python3
"""
头文件依赖分析工具
分析项目中的头文件依赖关系，识别循环依赖和优化机会
"""

import os
import re
import sys
from pathlib import Path
from collections import defaultdict, deque

class DependencyAnalyzer:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.dependencies = defaultdict(set)
        self.reverse_dependencies = defaultdict(set)
        self.include_paths = set()
        
    def analyze_dependencies(self):
        """分析头文件依赖关系"""
        print("🔍 分析头文件依赖关系...")
        
        # 收集所有头文件
        header_files = []
        for header_file in self.project_root.rglob("*.h"):
            if "third_party" not in str(header_file):
                header_files.append(header_file)
        
        # 分析每个头文件的依赖
        for header_file in header_files:
            self._analyze_file_dependencies(header_file)
    
    def _analyze_file_dependencies(self, file_path):
        """分析单个文件的依赖关系"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 提取 #include 语句
            includes = re.findall(r'#include\s*[<"]([^>"]+)[>"]', content)
            
            file_str = str(file_path)
            for include in includes:
                # 标准化包含路径
                if include.startswith('Paker/'):
                    self.dependencies[file_str].add(include)
                    self.include_paths.add(include)
                elif include.startswith('<') or include.startswith('"'):
                    # 系统头文件或第三方库
                    continue
                    
        except Exception as e:
            print(f"❌ 分析文件失败: {file_path} - {e}")
    
    def find_circular_dependencies(self):
        """查找循环依赖"""
        print("\n🔄 查找循环依赖...")
        
        circular_deps = []
        visited = set()
        rec_stack = set()
        
        def dfs(node, path):
            if node in rec_stack:
                # 找到循环依赖
                cycle_start = path.index(node)
                cycle = path[cycle_start:] + [node]
                circular_deps.append(cycle)
                return
            
            if node in visited:
                return
                
            visited.add(node)
            rec_stack.add(node)
            
            for dep in self.dependencies[node]:
                if dep in self.dependencies:
                    dfs(dep, path + [node])
            
            rec_stack.remove(node)
        
        for file_path in self.dependencies:
            if file_path not in visited:
                dfs(file_path, [])
        
        return circular_deps
    
    def find_heavy_dependencies(self):
        """查找重度依赖的文件"""
        print("\n📊 分析依赖复杂度...")
        
        dependency_counts = {}
        for file_path, deps in self.dependencies.items():
            dependency_counts[file_path] = len(deps)
        
        # 按依赖数量排序
        sorted_deps = sorted(dependency_counts.items(), key=lambda x: x[1], reverse=True)
        
        return sorted_deps[:10]  # 返回前10个最复杂的文件
    
    def generate_optimization_suggestions(self):
        """生成优化建议"""
        print("\n💡 生成优化建议...")
        
        suggestions = []
        
        # 分析循环依赖
        circular_deps = self.find_circular_dependencies()
        if circular_deps:
            suggestions.append(f"发现 {len(circular_deps)} 个循环依赖，建议使用前向声明")
        
        # 分析重度依赖
        heavy_deps = self.find_heavy_dependencies()
        if heavy_deps:
            suggestions.append("以下文件依赖过多，建议重构:")
            for file_path, count in heavy_deps:
                suggestions.append(f"  {file_path}: {count} 个依赖")
        
        # 通用优化建议
        suggestions.extend([
            "使用前向声明减少头文件依赖",
            "将实现细节移到 .cpp 文件中",
            "创建通用头文件减少重复包含",
            "使用预编译头文件提升编译速度"
        ])
        
        return suggestions
    
    def generate_report(self):
        """生成分析报告"""
        print("\n📋 头文件依赖分析报告")
        print("=" * 50)
        
        # 基本统计
        total_files = len(self.dependencies)
        total_includes = sum(len(deps) for deps in self.dependencies.values())
        
        print(f"📁 总文件数: {total_files}")
        print(f"🔗 总依赖数: {total_includes}")
        print(f"📈 平均依赖数: {total_includes / total_files:.1f}")
        
        # 循环依赖
        circular_deps = self.find_circular_dependencies()
        if circular_deps:
            print(f"\n⚠️  发现 {len(circular_deps)} 个循环依赖")
            for i, cycle in enumerate(circular_deps[:3]):  # 只显示前3个
                print(f"  循环 {i+1}: {' -> '.join(cycle)}")
        else:
            print("\n✅ 未发现循环依赖")
        
        # 重度依赖
        heavy_deps = self.find_heavy_dependencies()
        if heavy_deps:
            print(f"\n🔥 依赖最多的文件:")
            for file_path, count in heavy_deps:
                print(f"  {file_path}: {count} 个依赖")
        
        # 优化建议
        suggestions = self.generate_optimization_suggestions()
        print(f"\n💡 优化建议:")
        for i, suggestion in enumerate(suggestions, 1):
            print(f"  {i}. {suggestion}")
    
    def run(self):
        """运行依赖分析"""
        print("🚀 开始头文件依赖分析...")
        
        self.analyze_dependencies()
        self.generate_report()
        
        print("\n✅ 依赖分析完成!")

def main():
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    else:
        project_root = "."
    
    analyzer = DependencyAnalyzer(project_root)
    analyzer.run()

if __name__ == "__main__":
    main()
