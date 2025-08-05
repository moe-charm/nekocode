#!/usr/bin/env python3
"""
Perfect Analyzer - NekoCode + Compiler Integration
完全解析システムのプロトタイプ
"""

import subprocess
import json
import re
import sys
from pathlib import Path

class PerfectAnalyzer:
    def __init__(self):
        self.nekocode_bin = "./bin/nekocode_ai"
        
    def run_nekocode(self, filepath):
        """NekoCodeで構造解析"""
        try:
            result = subprocess.run(
                [self.nekocode_bin, "analyze", filepath, "--io-threads", "8"],
                capture_output=True,
                text=True
            )
            return json.loads(result.stdout)
        except Exception as e:
            print(f"NekoCode error: {e}")
            return None
    
    def run_lto_analysis(self, filepath):
        """LTOで未使用コード検出"""
        # コンパイルと未使用セクション検出
        cmd = [
            "g++", "-flto", "-ffunction-sections", "-fdata-sections",
            "-Wall", "-Wextra", "-Wunused-function", "-Wunused-variable",
            filepath, "-o", "temp_analysis",
            "-Wl,--gc-sections", "-Wl,--print-gc-sections"
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            # 削除されたセクションを解析
            removed_sections = []
            for line in result.stderr.split('\n'):
                if "removing unused section" in line:
                    # セクション名を抽出
                    match = re.search(r"'\.text\.([^']+)'", line)
                    if match:
                        mangled = match.group(1)
                        # デマングル
                        demangled = self.demangle_symbol(mangled)
                        removed_sections.append({
                            'mangled': mangled,
                            'demangled': demangled,
                            'type': 'function'
                        })
                    
                    # データセクション（変数）
                    match = re.search(r"'\.data\.([^']+)'", line)
                    if match:
                        removed_sections.append({
                            'name': match.group(1),
                            'type': 'variable'
                        })
            
            return removed_sections
            
        except Exception as e:
            print(f"LTO analysis error: {e}")
            return []
        finally:
            # 一時ファイル削除
            Path("temp_analysis").unlink(missing_ok=True)
    
    def demangle_symbol(self, mangled):
        """C++シンボルのデマングル"""
        try:
            result = subprocess.run(
                ["c++filt", mangled],
                capture_output=True,
                text=True
            )
            return result.stdout.strip()
        except:
            return mangled
    
    def analyze(self, filepath):
        """完全解析実行"""
        print(f"🔍 Perfect Analysis for: {filepath}")
        print("=" * 60)
        
        # Step 1: NekoCode構造解析
        print("📊 Step 1: NekoCode Structure Analysis")
        nekocode_data = self.run_nekocode(filepath)
        
        if nekocode_data:
            print(f"  ✅ Found {nekocode_data['statistics']['total_functions']} functions")
            print(f"  ✅ Found {nekocode_data['statistics']['total_classes']} classes")
            
            # 全関数リスト
            all_functions = {f['name'] for f in nekocode_data.get('functions', [])}
            print(f"\n  Defined functions: {all_functions}")
        
        # Step 2: LTO未使用コード検出
        print(f"\n🔬 Step 2: LTO Dead Code Detection")
        unused_items = self.run_lto_analysis(filepath)
        
        unused_functions = []
        unused_variables = []
        
        for item in unused_items:
            if item['type'] == 'function':
                name = item.get('demangled', item.get('mangled', ''))
                # 関数名のみ抽出 (括弧を除去)
                func_name = name.split('(')[0] if '(' in name else name
                unused_functions.append(func_name)
            elif item['type'] == 'variable':
                unused_variables.append(item['name'])
        
        print(f"  ❌ Unused functions: {unused_functions}")
        print(f"  ❌ Unused variables: {unused_variables}")
        
        # Step 3: 統合結果
        print(f"\n📋 Step 3: Perfect Analysis Result")
        print("=" * 60)
        
        if nekocode_data and all_functions:
            used_functions = all_functions - set(unused_functions)
            print(f"✅ Used functions: {used_functions}")
            print(f"❌ Unused functions: {unused_functions}")
            print(f"❌ Unused variables: {unused_variables}")
            
            # 使用率計算
            usage_rate = len(used_functions) / len(all_functions) * 100 if all_functions else 0
            print(f"\n📊 Code Usage Rate: {usage_rate:.1f}%")
            
            # JSON結果出力
            result = {
                'file': filepath,
                'analysis': {
                    'defined_functions': list(all_functions),
                    'used_functions': list(used_functions),
                    'unused_functions': unused_functions,
                    'unused_variables': unused_variables,
                    'usage_rate': usage_rate
                }
            }
            
            print(f"\n🎯 Perfect Analysis Complete!")
            return result
        
        return None


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 perfect_analyzer.py <source_file>")
        sys.exit(1)
    
    analyzer = PerfectAnalyzer()
    result = analyzer.analyze(sys.argv[1])
    
    # JSON出力（他のツールとの統合用）
    if result:
        print(f"\n📄 JSON Output:")
        print(json.dumps(result, indent=2))