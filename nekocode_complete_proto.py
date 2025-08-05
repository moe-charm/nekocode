#!/usr/bin/env python3
"""
NekoCode 完全解析モード プロトタイプ
--complete フラグで動作確認
"""

import subprocess
import json
import sys
import time
from pathlib import Path

class NekoCodeCompleteAnalyzer:
    def __init__(self):
        self.nekocode_bin = "./bin/nekocode_ai"
        
    def run_analysis(self, target, complete=False):
        """解析実行（通常 or 完全）"""
        start_time = time.time()
        
        print(f"🔍 NekoCode Analysis")
        print(f"  Target: {target}")
        print(f"  Mode: {'Complete' if complete else 'Normal'}")
        print("=" * 60)
        
        # Step 1: 通常のNekoCode解析（必須）
        print("\n📊 Step 1: Structure Analysis")
        structure_result = self._run_nekocode(target)
        
        result = {
            "mode": "complete" if complete else "normal",
            "target": target,
            "analysis": structure_result
        }
        
        # Step 2: 完全解析（オプション）
        if complete:
            print("\n🔬 Step 2: Complete Analysis")
            
            # 言語判定
            if target.endswith(('.cpp', '.hpp', '.cc', '.cxx')):
                print("  Language: C++")
                
                # LTO解析
                print("\n  📌 Running LTO analysis...")
                result["dead_code"] = self._run_lto_analysis(target)
                
                # Clang-Tidy解析
                print("\n  📌 Running Clang-Tidy analysis...")
                result["quality"] = self._run_clang_tidy(target)
                
            elif target.endswith('.py'):
                print("  Language: Python")
                print("  📌 Running Vulture analysis...")
                result["dead_code"] = self._run_vulture(target)
                
            elif target.endswith('.go'):
                print("  Language: Go")
                print("  📌 Running staticcheck...")
                result["quality"] = self._run_staticcheck(target)
            
            else:
                print("  ⚠️  Complete analysis not available for this language")
        
        # 実行時間
        elapsed = time.time() - start_time
        result["execution_time"] = f"{elapsed:.2f}s"
        
        # サマリー表示
        self._show_summary(result)
        
        return result
    
    def _run_nekocode(self, target):
        """通常のNekoCode解析"""
        try:
            result = subprocess.run(
                [self.nekocode_bin, "analyze", target, "--io-threads", "8"],
                capture_output=True,
                text=True
            )
            data = json.loads(result.stdout)
            
            # 基本統計
            stats = data.get('statistics', {})
            print(f"  ✅ Functions: {stats.get('total_functions', 0)}")
            print(f"  ✅ Classes: {stats.get('total_classes', 0)}")
            print(f"  ✅ Complexity: {data.get('complexity', {}).get('cyclomatic_complexity', 0)}")
            
            return data
            
        except Exception as e:
            print(f"  ❌ Error: {e}")
            return {}
    
    def _run_lto_analysis(self, target):
        """LTOによるデッドコード検出"""
        # 実際の実装では nekocode_lto_analyzer.py を呼ぶ
        print("    - Compiling with LTO...")
        print("    - Analyzing unused sections...")
        
        # デモ結果
        return {
            "tool": "LTO (Link Time Optimization)",
            "unused_functions": ["unused_function", "dead_loop_a", "dead_loop_b"],
            "unused_variables": ["unused_global"],
            "accuracy": "100%"
        }
    
    def _run_clang_tidy(self, target):
        """Clang-Tidyによる品質解析"""
        print("    - Running clang-tidy checks...")
        
        # デモ結果
        return {
            "tool": "clang-tidy",
            "modernization": 5,
            "performance": 2,
            "readability": 3,
            "total_suggestions": 10
        }
    
    def _run_vulture(self, target):
        """Vultureによる未使用コード検出（Python）"""
        print("    - Analyzing with Vulture...")
        
        return {
            "tool": "vulture",
            "unused_functions": ["deprecated_func"],
            "unused_variables": ["old_config"],
            "unused_imports": ["unused_module"]
        }
    
    def _run_staticcheck(self, target):
        """staticcheckによる静的解析（Go）"""
        print("    - Running staticcheck...")
        
        return {
            "tool": "staticcheck",
            "issues": 3,
            "categories": ["simplification", "performance"]
        }
    
    def _show_summary(self, result):
        """結果サマリー表示"""
        print("\n" + "=" * 60)
        print("📋 Analysis Summary")
        print("=" * 60)
        
        # 基本情報
        print(f"Mode: {result['mode'].upper()}")
        print(f"Execution time: {result['execution_time']}")
        
        # 構造解析結果
        if 'analysis' in result and 'statistics' in result['analysis']:
            stats = result['analysis']['statistics']
            print(f"\nStructure:")
            print(f"  • Functions: {stats.get('total_functions', 0)}")
            print(f"  • Classes: {stats.get('total_classes', 0)}")
        
        # 完全解析結果
        if result['mode'] == 'complete':
            if 'dead_code' in result:
                dc = result['dead_code']
                print(f"\nDead Code ({dc.get('tool', 'Unknown')}):")
                if 'unused_functions' in dc:
                    print(f"  • Unused functions: {len(dc['unused_functions'])}")
                if 'unused_variables' in dc:
                    print(f"  • Unused variables: {len(dc['unused_variables'])}")
            
            if 'quality' in result:
                q = result['quality']
                print(f"\nCode Quality ({q.get('tool', 'Unknown')}):")
                if 'total_suggestions' in q:
                    print(f"  • Total suggestions: {q['total_suggestions']}")
                elif 'issues' in q:
                    print(f"  • Issues found: {q['issues']}")


def main():
    """メイン処理（コマンドライン風）"""
    analyzer = NekoCodeCompleteAnalyzer()
    
    # デモ: 引数パース
    if len(sys.argv) < 2:
        print("Usage: nekocode_ai analyze <target> [--complete]")
        sys.exit(1)
    
    target = sys.argv[1]
    complete = "--complete" in sys.argv
    
    # 解析実行
    result = analyzer.run_analysis(target, complete)
    
    # JSON出力（AIモード用）
    print(f"\n💾 JSON output saved to: analysis_result.json")
    with open("analysis_result.json", "w") as f:
        json.dump(result, f, indent=2)


if __name__ == "__main__":
    # デモ実行
    print("🚀 NekoCode Complete Analysis Prototype")
    print()
    
    # 通常モード
    print("Example 1: Normal mode")
    print("$ nekocode_ai analyze test.cpp")
    analyzer = NekoCodeCompleteAnalyzer()
    analyzer.run_analysis("test.cpp", complete=False)
    
    print("\n" + "="*80 + "\n")
    
    # 完全解析モード
    print("Example 2: Complete mode")
    print("$ nekocode_ai analyze test.cpp --complete")
    analyzer.run_analysis("test.cpp", complete=True)