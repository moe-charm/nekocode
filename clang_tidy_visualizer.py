#!/usr/bin/env python3
"""
Clang-Tidy YAML Visualizer for NekoCode
修正提案を可視化してNekoCodeセッションと統合
"""

import yaml
import json
import subprocess
from pathlib import Path
from collections import defaultdict

class ClangTidyVisualizer:
    def __init__(self):
        self.nekocode_bin = "./bin/nekocode_ai"
        
    def parse_tidy_yaml(self, yaml_file):
        """Clang-TidyのYAMLファイルを解析（新フォーマット対応）"""
        try:
            with open(yaml_file, 'r') as f:
                data = yaml.safe_load(f)
            
            # 診断名ごとにグループ化
            issues_by_type = defaultdict(list)
            
            # 新フォーマット: Diagnosticsセクション
            if 'Diagnostics' in data:
                for diag in data['Diagnostics']:
                    diagnostic_name = diag.get('DiagnosticName', 'unknown')
                    msg_info = diag.get('DiagnosticMessage', {})
                    
                    for replacement in msg_info.get('Replacements', []):
                        issues_by_type[diagnostic_name].append({
                            'file': replacement.get('FilePath'),
                            'offset': replacement.get('Offset'),
                            'length': replacement.get('Length'),
                            'fix': replacement.get('ReplacementText'),
                            'message': msg_info.get('Message', ''),
                            'level': diag.get('Level', 'Warning')
                        })
            
            # 旧フォーマット: Replacementsセクション（互換性維持）
            elif 'Replacements' in data:
                for replacement in data['Replacements']:
                    diagnostic = replacement.get('DiagnosticName', 'unknown')
                    issues_by_type[diagnostic].append({
                        'file': replacement.get('FilePath'),
                        'offset': replacement.get('Offset'),
                        'length': replacement.get('Length'),
                        'fix': replacement.get('ReplacementText'),
                        'message': replacement.get('DiagnosticMessage', '')
                    })
            
            return issues_by_type
            
        except Exception as e:
            print(f"Error parsing YAML: {e}")
            return {}
    
    def run_clang_tidy(self, filepath, checks=None):
        """Clang-Tidyを実行してYAML出力を生成"""
        yaml_file = "tidy_fixes.yaml"
        
        cmd = ["clang-tidy", filepath, f"--export-fixes={yaml_file}"]
        
        if checks:
            cmd.append(f"-checks={checks}")
        else:
            # デフォルトチェック
            cmd.append("-checks=modernize-*,performance-*,readability-*,misc-unused-*")
        
        try:
            print(f"🔧 Running Clang-Tidy on {filepath}...")
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if Path(yaml_file).exists():
                return self.parse_tidy_yaml(yaml_file)
            else:
                print("No fixes generated")
                return {}
                
        except FileNotFoundError:
            print("⚠️  Clang-Tidy not found. Using example YAML...")
            # 例のYAMLを使用
            if Path("example_tidy_fixes.yaml").exists():
                return self.parse_tidy_yaml("example_tidy_fixes.yaml")
            return {}
        except Exception as e:
            print(f"Error running Clang-Tidy: {e}")
            return {}
    
    def visualize_issues(self, issues_by_type):
        """問題を視覚的に表示"""
        print("\n📊 Clang-Tidy Analysis Results")
        print("=" * 60)
        
        total_issues = sum(len(issues) for issues in issues_by_type.values())
        print(f"Total issues found: {total_issues}")
        print()
        
        # カテゴリ別に表示
        categories = {
            'modernize': '🔄 Modernization',
            'performance': '⚡ Performance',
            'readability': '📖 Readability',
            'misc-unused': '❌ Unused Code',
            'bugprone': '🐛 Bug Risk'
        }
        
        for category, label in categories.items():
            category_issues = {k: v for k, v in issues_by_type.items() if k.startswith(category)}
            
            if category_issues:
                print(f"\n{label}:")
                for diagnostic, issues in category_issues.items():
                    print(f"  • {diagnostic}: {len(issues)} issues")
                    # 最初の3つの例を表示
                    for issue in issues[:3]:
                        print(f"    - {issue['message']}")
                    if len(issues) > 3:
                        print(f"    ... and {len(issues) - 3} more")
    
    def integrate_with_nekocode(self, filepath, tidy_issues):
        """NekoCodeの解析結果と統合"""
        print("\n🔗 Integrating with NekoCode Analysis")
        print("=" * 60)
        
        # NekoCode解析実行
        try:
            result = subprocess.run(
                [self.nekocode_bin, "analyze", filepath, "--io-threads", "8"],
                capture_output=True,
                text=True
            )
            nekocode_data = json.loads(result.stdout)
            
            # 統合レポート作成
            report = {
                'file': filepath,
                'nekocode': {
                    'functions': nekocode_data.get('statistics', {}).get('total_functions', 0),
                    'classes': nekocode_data.get('statistics', {}).get('total_classes', 0),
                    'complexity': nekocode_data.get('complexity', {}).get('cyclomatic_complexity', 0)
                },
                'clang_tidy': {
                    'total_issues': sum(len(issues) for issues in tidy_issues.values()),
                    'categories': {
                        'modernization': len([i for k, v in tidy_issues.items() if k.startswith('modernize') for i in v]),
                        'performance': len([i for k, v in tidy_issues.items() if k.startswith('performance') for i in v]),
                        'readability': len([i for k, v in tidy_issues.items() if k.startswith('readability') for i in v]),
                        'unused_code': len([i for k, v in tidy_issues.items() if 'unused' in k for i in v]),
                        'bug_risk': len([i for k, v in tidy_issues.items() if k.startswith('bugprone') for i in v])
                    }
                }
            }
            
            print(f"📊 NekoCode: {report['nekocode']['functions']} functions, "
                  f"{report['nekocode']['classes']} classes, "
                  f"complexity {report['nekocode']['complexity']}")
            
            print(f"\n🔧 Clang-Tidy: {report['clang_tidy']['total_issues']} total issues")
            for category, count in report['clang_tidy']['categories'].items():
                if count > 0:
                    print(f"  • {category}: {count}")
            
            return report
            
        except Exception as e:
            print(f"Error integrating with NekoCode: {e}")
            return None
    
    def analyze_project(self, directory):
        """プロジェクト全体を解析"""
        print(f"🔍 Analyzing project: {directory}")
        
        cpp_files = list(Path(directory).rglob("*.cpp"))
        cpp_files.extend(Path(directory).rglob("*.hpp"))
        
        all_issues = defaultdict(list)
        
        for file in cpp_files:
            issues = self.run_clang_tidy(str(file))
            for diagnostic, file_issues in issues.items():
                all_issues[diagnostic].extend(file_issues)
        
        self.visualize_issues(all_issues)
        
        # サマリー
        print(f"\n📋 Project Summary:")
        print(f"  • Files analyzed: {len(cpp_files)}")
        print(f"  • Total issues: {sum(len(v) for v in all_issues.values())}")
        
        return all_issues


# 使用例
if __name__ == "__main__":
    import sys
    
    visualizer = ClangTidyVisualizer()
    
    if len(sys.argv) > 1:
        target = sys.argv[1]
        
        # YAMLファイルの解析
        if target.endswith('.yaml') or target.endswith('.yml'):
            print(f"📄 Parsing YAML file: {target}")
            issues = visualizer.parse_tidy_yaml(target)
            visualizer.visualize_issues(issues)
            
        elif Path(target).is_file():
            # C++ファイルの解析
            issues = visualizer.run_clang_tidy(target)
            visualizer.visualize_issues(issues)
            visualizer.integrate_with_nekocode(target, issues)
            
        elif Path(target).is_dir():
            # プロジェクト解析
            visualizer.analyze_project(target)
    else:
        # デモ: example YAMLを使用
        print("📚 Demo mode: Using example YAML")
        issues = visualizer.parse_tidy_yaml("example_tidy_fixes.yaml")
        visualizer.visualize_issues(issues)