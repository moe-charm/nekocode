#!/usr/bin/env python3
"""
Clang-Tidy YAML生データ解析デモ
何が取れるか具体的に見せる
"""

import yaml
import json
from collections import defaultdict
from pathlib import Path

def analyze_raw_yaml(yaml_file):
    """YAMLから取れる生情報を全部見せる"""
    
    with open(yaml_file, 'r') as f:
        data = yaml.safe_load(f)
    
    print("🔍 Clang-Tidy YAML Raw Data Analysis")
    print("=" * 60)
    
    # 1. トップレベル情報
    print("\n📋 Top-level Info:")
    print(f"  • MainSourceFile: {data.get('MainSourceFile', 'N/A')}")
    print(f"  • Has Diagnostics: {'Diagnostics' in data}")
    print(f"  • Has Replacements: {'Replacements' in data}")
    
    # 2. 診断統計
    if 'Diagnostics' in data:
        diagnostics = data['Diagnostics']
        print(f"\n📊 Diagnostics Summary:")
        print(f"  • Total diagnostics: {len(diagnostics)}")
        
        # 診断名の分布
        diag_counts = defaultdict(int)
        for diag in diagnostics:
            diag_counts[diag.get('DiagnosticName', 'unknown')] += 1
        
        print("\n  📈 Distribution by diagnostic type:")
        for name, count in sorted(diag_counts.items()):
            print(f"    - {name}: {count}")
        
        # レベル別統計
        level_counts = defaultdict(int)
        for diag in diagnostics:
            level_counts[diag.get('Level', 'unknown')] += 1
        
        print("\n  ⚠️  Distribution by severity:")
        for level, count in sorted(level_counts.items()):
            print(f"    - {level}: {count}")
        
        # 詳細な情報抽出
        print("\n📝 Detailed Information per Diagnostic:")
        for i, diag in enumerate(diagnostics[:3]):  # 最初の3つ
            print(f"\n  Diagnostic #{i+1}:")
            print(f"    • Name: {diag.get('DiagnosticName')}")
            print(f"    • Level: {diag.get('Level')}")
            
            msg = diag.get('DiagnosticMessage', {})
            print(f"    • Message: {msg.get('Message', 'N/A')}")
            print(f"    • FilePath: {Path(msg.get('FilePath', '')).name}")
            print(f"    • FileOffset: {msg.get('FileOffset', 'N/A')}")
            
            replacements = msg.get('Replacements', [])
            print(f"    • Replacements: {len(replacements)}")
            
            for j, repl in enumerate(replacements):
                print(f"      Replacement #{j+1}:")
                print(f"        - Offset: {repl.get('Offset')}")
                print(f"        - Length: {repl.get('Length')}")
                print(f"        - New text: '{repl.get('ReplacementText')}'")
                
            # ビルドディレクトリ情報
            if 'BuildDirectory' in diag:
                print(f"    • BuildDirectory: {Path(diag['BuildDirectory']).name}")
                
            # ノート（追加情報）
            if 'Notes' in diag:
                print(f"    • Has notes: {len(diag['Notes'])} notes")
        
        # 3. ファイル別統計
        print("\n📁 File-level Statistics:")
        file_issues = defaultdict(lambda: {
            'count': 0,
            'diagnostics': set(),
            'auto_fixable': 0,
            'manual_fix': 0
        })
        
        for diag in diagnostics:
            msg = diag.get('DiagnosticMessage', {})
            filepath = msg.get('FilePath', 'unknown')
            diag_name = diag.get('DiagnosticName', 'unknown')
            replacements = msg.get('Replacements', [])
            
            file_issues[filepath]['count'] += 1
            file_issues[filepath]['diagnostics'].add(diag_name)
            
            if replacements:
                file_issues[filepath]['auto_fixable'] += 1
            else:
                file_issues[filepath]['manual_fix'] += 1
        
        for filepath, stats in file_issues.items():
            print(f"\n  📄 {Path(filepath).name}:")
            print(f"    • Total issues: {stats['count']}")
            print(f"    • Unique diagnostic types: {len(stats['diagnostics'])}")
            print(f"    • Auto-fixable: {stats['auto_fixable']}")
            print(f"    • Manual fix needed: {stats['manual_fix']}")
            print(f"    • Fix rate: {stats['auto_fixable']/stats['count']*100:.1f}%")
        
        # 4. カテゴリ別分析
        print("\n🏷️  Category Analysis:")
        categories = defaultdict(int)
        for diag in diagnostics:
            name = diag.get('DiagnosticName', '')
            if name.startswith('modernize-'):
                categories['Modernization'] += 1
            elif name.startswith('performance-'):
                categories['Performance'] += 1
            elif name.startswith('readability-'):
                categories['Readability'] += 1
            elif 'unused' in name:
                categories['Unused Code'] += 1
            elif name.startswith('bugprone-'):
                categories['Bug Risk'] += 1
            else:
                categories['Other'] += 1
        
        total = sum(categories.values())
        for cat, count in sorted(categories.items(), key=lambda x: x[1], reverse=True):
            percentage = count / total * 100
            print(f"  • {cat}: {count} ({percentage:.1f}%)")
        
        # 5. 修正の複雑さ分析
        print("\n🔧 Fix Complexity Analysis:")
        fix_complexity = {
            'simple': 0,    # 1箇所の置換
            'moderate': 0,  # 2-3箇所の置換
            'complex': 0    # 4箇所以上の置換
        }
        
        for diag in diagnostics:
            replacements = diag.get('DiagnosticMessage', {}).get('Replacements', [])
            if len(replacements) == 1:
                fix_complexity['simple'] += 1
            elif 2 <= len(replacements) <= 3:
                fix_complexity['moderate'] += 1
            elif len(replacements) >= 4:
                fix_complexity['complex'] += 1
        
        print(f"  • Simple fixes (1 replacement): {fix_complexity['simple']}")
        print(f"  • Moderate fixes (2-3 replacements): {fix_complexity['moderate']}")
        print(f"  • Complex fixes (4+ replacements): {fix_complexity['complex']}")
        
        # 6. プロジェクト健全性スコア
        print("\n🏆 Project Health Score:")
        
        # スコア計算（例）
        base_score = 100
        score_deductions = {
            'bugprone': 5,      # バグリスクは重い
            'performance': 3,   # パフォーマンスは中程度
            'readability': 2,   # 可読性は軽い
            'modernize': 1      # モダン化は最も軽い
        }
        
        current_score = base_score
        for diag in diagnostics:
            name = diag.get('DiagnosticName', '')
            for key, deduction in score_deductions.items():
                if name.startswith(key + '-'):
                    current_score -= deduction / 10  # 調整
                    break
        
        print(f"  • Base score: {base_score}")
        print(f"  • Current score: {max(0, current_score):.1f}")
        print(f"  • Grade: {get_grade(current_score)}")
        
        # 改善提案
        print("\n💡 Improvement Recommendations:")
        if categories['Bug Risk'] > 0:
            print("  1. 🚨 Fix bug-prone issues first (highest priority)")
        if categories['Performance'] > 5:
            print("  2. ⚡ Address performance issues for better efficiency")
        if fix_complexity['simple'] > 10:
            print("  3. 🤖 Use clang-apply-replacements for bulk auto-fixes")
        if categories['Modernization'] > 20:
            print("  4. 🔄 Plan a modernization sprint")

def get_grade(score):
    """スコアからグレードを決定"""
    if score >= 90: return "A (Excellent)"
    elif score >= 80: return "B (Good)"
    elif score >= 70: return "C (Fair)"
    elif score >= 60: return "D (Poor)"
    else: return "F (Needs major improvement)"

# 使用例
if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1:
        yaml_file = sys.argv[1]
        if Path(yaml_file).exists():
            analyze_raw_yaml(yaml_file)
        else:
            print(f"Error: {yaml_file} not found")
    else:
        print("Usage: python3 analyze_clang_tidy_yaml.py <yaml_file>")
        print("\nThis tool shows all information extractable from Clang-Tidy YAML")