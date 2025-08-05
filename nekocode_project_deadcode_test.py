#!/usr/bin/env python3
"""
NekoCodeプロジェクト全体のデッドコード検出テスト
複数ファイルプロジェクトでのLTO真価確認
"""

import subprocess
import glob
import json
from pathlib import Path

def main():
    print("🚀 NekoCode Project Dead Code Analysis")
    print("=" * 50)
    
    # C++ファイル一覧取得
    cpp_files = []
    for pattern in ["src/**/*.cpp", "src/**/*.hpp"]:
        cpp_files.extend(glob.glob(pattern, recursive=True))
    
    print(f"📁 Found {len(cpp_files)} C++ files")
    
    # プロジェクト全体で代表的ファイル5個をテスト
    test_files = [
        "src/core/core.cpp",
        "src/core/session_commands.cpp", 
        "src/analyzers/cpp/cpp_analyzer.cpp",
        "src/main/main_ai.cpp",
        "src/adapters/cpp_universal_adapter.cpp"
    ]
    
    total_dead_items = 0
    
    for file_path in test_files:
        if Path(file_path).exists():
            print(f"\n🔍 Analyzing: {file_path}")
            
            # 完全解析実行
            result = subprocess.run([
                "python3", "universal_deadcode_analyzer.py", 
                file_path, "--complete"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                try:
                    # JSON結果解析
                    output_lines = result.stdout.strip().split('\n')
                    json_start = -1
                    for i, line in enumerate(output_lines):
                        if line.strip() == '{':
                            json_start = i
                            break
                    
                    if json_start >= 0:
                        json_text = '\n'.join(output_lines[json_start:])
                        data = json.loads(json_text)
                        
                        if 'dead_code' in data:
                            dead_info = data['dead_code']
                            found = dead_info.get('total_found', 0)
                            total_dead_items += found
                            
                            print(f"  ✅ Tool: {dead_info.get('tool', 'Unknown')}")
                            print(f"  📊 Dead items found: {found}")
                            
                            if found > 0 and 'unused_items' in dead_info:
                                for item in dead_info['unused_items'][:3]:  # 最初の3個まで
                                    print(f"    🔍 {item}")
                                if len(dead_info['unused_items']) > 3:
                                    print(f"    ... and {len(dead_info['unused_items']) - 3} more")
                        else:
                            print("  ⚠️ No dead code analysis in result")
                            
                except json.JSONDecodeError as e:
                    print(f"  ❌ JSON parse error: {e}")
            else:
                print(f"  ❌ Analysis failed: {result.stderr}")
    
    print(f"\n🎯 PROJECT SUMMARY")
    print(f"📊 Total dead code items found: {total_dead_items}")
    print(f"📁 Files analyzed: {len([f for f in test_files if Path(f).exists()])}")
    
    if total_dead_items == 0:
        print("✅ NekoCode project is very clean! No dead code detected.")
        print("🏆 This indicates excellent code quality and maintenance!")
    else:
        print(f"🧹 Found {total_dead_items} items that could be cleaned up.")

if __name__ == "__main__":
    main()