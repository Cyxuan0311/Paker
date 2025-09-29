#!/usr/bin/env python3
"""
头文件优化脚本
用于分析和优化项目中的头文件包含
"""

import os
import re
import sys
from pathlib import Path
from collections import defaultdict, Counter

class HeaderOptimizer:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.include_patterns = defaultdict(list)
        self.header_usage = Counter()
        self.duplicate_includes = []
        
    def analyze_headers(self):
        """分析头文件使用模式"""
        print("🔍 分析头文件使用模式...")
        
        for header_file in self.project_root.rglob("*.h"):
            if "third_party" in str(header_file):
                continue
                
            self._analyze_header_file(header_file)
            
        for source_file in self.project_root.rglob("*.cpp"):
            if "third_party" in str(source_file):
                continue
                
            self._analyze_source_file(source_file)
    
    def _analyze_header_file(self, file_path):
        """分析头文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            includes = re.findall(r'#include\s*[<"]([^>"]+)[>"]', content)
            
            for include in includes:
                self.header_usage[include] += 1
                self.include_patterns[include].append(str(file_path))
                
        except Exception as e:
            print(f"❌ 分析文件失败: {file_path} - {e}")
    
    def _analyze_source_file(self, file_path):
        """分析源文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            includes = re.findall(r'#include\s*[<"]([^>"]+)[>"]', content)
            
            # 检查重复包含
            include_set = set()
            for include in includes:
                if include in include_set:
                    self.duplicate_includes.append((str(file_path), include))
                include_set.add(include)
                
        except Exception as e:
            print(f"❌ 分析文件失败: {file_path} - {e}")
    
    def generate_report(self):
        """生成优化报告"""
        print("\n📊 头文件使用统计:")
        print("=" * 50)
        
        # 最常用的头文件
        print("\n🔥 最常用的头文件:")
        for header, count in self.header_usage.most_common(10):
            print(f"  {header}: {count} 次")
        
        # 重复包含
        if self.duplicate_includes:
            print(f"\n⚠️  发现 {len(self.duplicate_includes)} 个重复包含:")
            for file_path, include in self.duplicate_includes[:10]:
                print(f"  {file_path}: {include}")
        
        # 建议优化
        print("\n💡 优化建议:")
        print("1. 创建预编译头文件 (pch.h)")
        print("2. 创建通用头文件 (common.h)")
        print("3. 使用前向声明减少头文件依赖")
        print("4. 将实现细节移到 .cpp 文件中")
        
    def create_optimized_headers(self):
        """创建优化的头文件"""
        print("\n🔧 创建优化的头文件...")
        
        # 创建 common.h
        common_headers = [
            '<string>', '<vector>', '<map>', '<memory>', 
            '<chrono>', '<filesystem>', '<fstream>', '<sstream>'
        ]
        
        common_content = """#pragma once

// 常用标准库头文件
"""
        for header in common_headers:
            common_content += f"#include {header}\n"
            
        common_content += """
// 常用命名空间别名
namespace fs = std::filesystem;

// 常用类型别名
namespace Paker {
    using String = std::string;
    using StringList = std::vector<std::string>;
    using StringMap = std::map<std::string, std::string>;
}
"""
        
        common_file = self.project_root / "include" / "Paker" / "common.h"
        with open(common_file, 'w', encoding='utf-8') as f:
            f.write(common_content)
            
        print(f"✅ 创建通用头文件: {common_file}")
    
    def run(self):
        """运行优化分析"""
        print("🚀 开始头文件优化分析...")
        
        self.analyze_headers()
        self.generate_report()
        self.create_optimized_headers()
        
        print("\n✅ 头文件优化分析完成!")

def main():
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    else:
        project_root = "."
    
    optimizer = HeaderOptimizer(project_root)
    optimizer.run()

if __name__ == "__main__":
    main()
