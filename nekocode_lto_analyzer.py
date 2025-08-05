#!/usr/bin/env python3
"""
NekoCode + LTO Dead Code Analyzer
シンプルで強力なデッドコード検出
"""

import subprocess
import json
import re
import sys
from pathlib import Path

class NekoCodeLTOAnalyzer:
    def __init__(self):
        self.nekocode_bin = "./bin/nekocode_ai"
        
    def analyze_cpp_file(self, filepath):
        """C++ファイルのデッドコード検出"""
        print(f"🔍 Analyzing: {filepath}")
        print("=" * 60)
        
        # Step 1: NekoCodeで構造解析
        print("📊 Step 1: NekoCode Structure Analysis")
        nekocode_result = self._run_nekocode(filepath)
        
        if not nekocode_result:
            return None
            
        defined_functions = {f['name'] for f in nekocode_result.get('functions', [])}
        defined_classes = {c['name'] for c in nekocode_result.get('classes', [])}
        
        print(f"  ✅ Found {len(defined_functions)} functions")
        print(f"  ✅ Found {len(defined_classes)} classes")
        
        # Step 2: LTOで使用状況解析
        print("\n🔬 Step 2: LTO Usage Analysis")
        unused_items = self._run_lto_analysis(filepath)
        
        unused_functions = set()
        unused_variables = []
        
        for item in unused_items:
            if item['type'] == 'function':
                func_name = item['name'].split('(')[0] if '(' in item['name'] else item['name']
                unused_functions.add(func_name)
            elif item['type'] == 'variable':
                unused_variables.append(item['name'])
        
        # Step 3: 結果統合
        print("\n📋 Step 3: Dead Code Detection Results")
        print("=" * 60)
        
        # 使用されている関数
        used_functions = defined_functions - unused_functions
        
        # 未使用関数の詳細
        dead_functions = []
        for func in nekocode_result.get('functions', []):
            if func['name'] in unused_functions:
                dead_functions.append({
                    'name': func['name'],
                    'line': func.get('start_line', 0),
                    'type': 'function'
                })
        
        # 使用率計算
        usage_rate = (len(used_functions) / len(defined_functions) * 100) if defined_functions else 100
        
        # 結果表示
        print(f"✅ Used functions ({len(used_functions)}):")
        for func in sorted(used_functions):
            print(f"   - {func}")
            
        print(f"\n❌ Dead code detected ({len(dead_functions)} functions, {len(unused_variables)} variables):")
        for func in dead_functions:
            print(f"   - Function '{func['name']}' at line {func['line']}")
        for var in unused_variables:
            print(f"   - Variable '{var}'")
            
        print(f"\n📊 Code usage rate: {usage_rate:.1f}%")
        
        # JSON結果
        result = {
            'file': filepath,
            'analysis': {
                'defined': {
                    'functions': list(defined_functions),
                    'classes': list(defined_classes)
                },
                'used': {
                    'functions': list(used_functions)
                },
                'dead_code': {
                    'functions': dead_functions,
                    'variables': unused_variables
                },
                'statistics': {
                    'total_functions': len(defined_functions),
                    'used_functions': len(used_functions),
                    'dead_functions': len(dead_functions),
                    'dead_variables': len(unused_variables),
                    'usage_rate': usage_rate
                }
            }
        }
        
        return result
    
    def _run_nekocode(self, filepath):
        """NekoCode実行"""
        try:
            result = subprocess.run(
                [self.nekocode_bin, "analyze", filepath, "--io-threads", "8"],
                capture_output=True,
                text=True
            )
            return json.loads(result.stdout)
        except Exception as e:
            print(f"  ❌ NekoCode error: {e}")
            return None
    
    def _run_lto_analysis(self, filepath):
        """LTO解析実行"""
        cmd = [
            "g++", "-flto", "-ffunction-sections", "-fdata-sections",
            "-Wall", "-Wextra", filepath, "-o", "temp_lto_analysis",
            "-Wl,--gc-sections", "-Wl,--print-gc-sections"
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            unused_items = []
            for line in result.stderr.split('\n'):
                if "removing unused section" in line:
                    # 関数セクション
                    match = re.search(r"'\.text\.([^']+)'", line)
                    if match:
                        mangled = match.group(1)
                        demangled = self._demangle_symbol(mangled)
                        unused_items.append({
                            'name': demangled,
                            'type': 'function'
                        })
                    
                    # データセクション（変数）
                    match = re.search(r"'\.data\.([^']+)'", line)
                    if match:
                        unused_items.append({
                            'name': match.group(1),
                            'type': 'variable'
                        })
            
            print(f"  ✅ LTO detected {len(unused_items)} unused items")
            return unused_items
            
        except Exception as e:
            print(f"  ❌ LTO error: {e}")
            return []
        finally:
            Path("temp_lto_analysis").unlink(missing_ok=True)
    
    def _demangle_symbol(self, mangled):
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
    
    def analyze_directory(self, directory):
        """ディレクトリ内の全C++ファイルを解析"""
        cpp_files = list(Path(directory).rglob("*.cpp"))
        cpp_files.extend(Path(directory).rglob("*.cc"))
        cpp_files.extend(Path(directory).rglob("*.cxx"))
        
        print(f"🔍 Found {len(cpp_files)} C++ files in {directory}")
        
        all_results = []
        total_dead_functions = 0
        total_dead_variables = 0
        
        for cpp_file in cpp_files:
            result = self.analyze_cpp_file(str(cpp_file))
            if result:
                all_results.append(result)
                stats = result['analysis']['statistics']
                total_dead_functions += stats['dead_functions']
                total_dead_variables += stats['dead_variables']
        
        print(f"\n📊 Summary: {total_dead_functions} dead functions, {total_dead_variables} dead variables")
        return all_results


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 nekocode_lto_analyzer.py <file_or_directory>")
        sys.exit(1)
    
    target = sys.argv[1]
    analyzer = NekoCodeLTOAnalyzer()
    
    if Path(target).is_file():
        result = analyzer.analyze_cpp_file(target)
        if result:
            print(f"\n📄 JSON Output:")
            print(json.dumps(result, indent=2))
    elif Path(target).is_dir():
        results = analyzer.analyze_directory(target)
        # 結果をファイルに保存
        output_file = "dead_code_report.json"
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"\n📄 Full report saved to: {output_file}")
    else:
        print(f"Error: {target} not found")
        sys.exit(1)


if __name__ == "__main__":
    main()