#!/usr/bin/env python3
"""
Clang-Tidy YAML Merger
複数のYAMLファイルを統合してプロジェクト全体の修正提案を管理
"""

import yaml
import json
from pathlib import Path
from collections import defaultdict

class ClangTidyYamlMerger:
    def merge_yaml_files(self, yaml_files, output_file="merged_fixes.yaml"):
        """複数のClang-Tidy YAMLファイルを統合（新フォーマット対応）"""
        print(f"🔗 Merging {len(yaml_files)} YAML files...")
        
        all_diagnostics = []
        all_replacements = []  # 旧フォーマット用
        files_processed = set()
        
        for yaml_file in yaml_files:
            if not Path(yaml_file).exists():
                print(f"  ⚠️  Skipping {yaml_file} (not found)")
                continue
                
            try:
                with open(yaml_file, 'r') as f:
                    data = yaml.safe_load(f)
                
                # 新フォーマット: Diagnosticsセクション
                if 'Diagnostics' in data:
                    diagnostics = data['Diagnostics']
                    all_diagnostics.extend(diagnostics)
                    
                    # 統計情報収集
                    replacement_count = 0
                    for diag in diagnostics:
                        if 'DiagnosticMessage' in diag:
                            for repl in diag['DiagnosticMessage'].get('Replacements', []):
                                if 'FilePath' in repl:
                                    files_processed.add(repl['FilePath'])
                                replacement_count += 1
                    
                    print(f"  ✅ {yaml_file}: {len(diagnostics)} diagnostics, {replacement_count} replacements")
                
                # 旧フォーマット: Replacementsセクション
                elif 'Replacements' in data:
                    replacements = data['Replacements']
                    all_replacements.extend(replacements)
                    
                    for repl in replacements:
                        if 'FilePath' in repl:
                            files_processed.add(repl['FilePath'])
                    
                    print(f"  ✅ {yaml_file}: {len(replacements)} replacements")
                    
            except Exception as e:
                print(f"  ❌ Error reading {yaml_file}: {e}")
        
        # 統合結果作成
        if all_diagnostics:  # 新フォーマット
            # Replacementsをカウント
            total_replacements = sum(
                len(diag.get('DiagnosticMessage', {}).get('Replacements', [])) 
                for diag in all_diagnostics
            )
            
            merged_data = {
                'MainSourceFile': '',  # プロジェクト全体
                'Diagnostics': all_diagnostics,
                'Metadata': {
                    'MergedFrom': [str(f) for f in yaml_files],
                    'TotalDiagnostics': len(all_diagnostics),
                    'TotalReplacements': total_replacements,
                    'FilesAffected': len(files_processed),
                    'AffectedFiles': sorted(list(files_processed))
                }
            }
        else:  # 旧フォーマット
            merged_data = {
                'MainSourceFile': '',
                'Replacements': all_replacements,
                'Metadata': {
                    'MergedFrom': [str(f) for f in yaml_files],
                    'TotalReplacements': len(all_replacements),
                    'FilesAffected': len(files_processed),
                    'AffectedFiles': sorted(list(files_processed))
                }
            }
        
        # 保存
        with open(output_file, 'w') as f:
            yaml.dump(merged_data, f, default_flow_style=False)
        
        print(f"\n📊 Merge Summary:")
        if all_diagnostics:
            total_replacements = sum(
                len(diag.get('DiagnosticMessage', {}).get('Replacements', [])) 
                for diag in all_diagnostics
            )
            print(f"  • Total diagnostics: {len(all_diagnostics)}")
            print(f"  • Total replacements: {total_replacements}")
        else:
            print(f"  • Total replacements: {len(all_replacements)}")
        print(f"  • Files affected: {len(files_processed)}")
        print(f"  • Output: {output_file}")
        
        return merged_data
    
    def analyze_by_diagnostic(self, yaml_file):
        """診断タイプ別に統計を取る"""
        with open(yaml_file, 'r') as f:
            data = yaml.safe_load(f)
        
        stats = defaultdict(lambda: {'count': 0, 'files': set()})
        
        # 新フォーマット: Diagnosticsセクション
        if 'Diagnostics' in data:
            for diag in data['Diagnostics']:
                diag_name = diag.get('DiagnosticName', 'unknown')
                replacements = diag.get('DiagnosticMessage', {}).get('Replacements', [])
                stats[diag_name]['count'] += len(replacements)
                for repl in replacements:
                    if 'FilePath' in repl:
                        stats[diag_name]['files'].add(repl['FilePath'])
        
        # 旧フォーマット: Replacementsセクション
        elif 'Replacements' in data:
            for repl in data['Replacements']:
                diag = repl.get('DiagnosticName', 'unknown')
                stats[diag]['count'] += 1
                if 'FilePath' in repl:
                    stats[diag]['files'].add(repl['FilePath'])
        
        # set をリストに変換
        for diag in stats:
            stats[diag]['files'] = sorted(list(stats[diag]['files']))
        
        return dict(stats)
    
    def create_nekocode_compatible_json(self, yaml_file, output_file="clang_tidy_report.json"):
        """NekoCodeセッション形式に変換（新フォーマット対応）"""
        with open(yaml_file, 'r') as f:
            data = yaml.safe_load(f)
        
        # ファイル別に整理
        files_data = defaultdict(lambda: {
            'issues': [],
            'stats': defaultdict(int)
        })
        
        # 新フォーマット: Diagnosticsセクション
        if 'Diagnostics' in data:
            for diag in data['Diagnostics']:
                diag_name = diag.get('DiagnosticName', 'unknown')
                diag_msg = diag.get('DiagnosticMessage', {})
                
                for repl in diag_msg.get('Replacements', []):
                    filepath = repl.get('FilePath', 'unknown')
                    
                    issue = {
                        'type': 'clang-tidy',
                        'diagnostic': diag_name,
                        'message': diag_msg.get('Message', ''),
                        'offset': repl.get('Offset', 0),
                        'length': repl.get('Length', 0),
                        'replacement': repl.get('ReplacementText', ''),
                        'category': self._categorize_diagnostic(diag_name),
                        'level': diag.get('Level', 'Warning')
                    }
                    
                    files_data[filepath]['issues'].append(issue)
                    files_data[filepath]['stats'][issue['category']] += 1
        
        # 旧フォーマット: Replacementsセクション
        elif 'Replacements' in data:
            for repl in data.get('Replacements', []):
                filepath = repl.get('FilePath', 'unknown')
                diag_name = repl.get('DiagnosticName', 'unknown')
                
                issue = {
                    'type': 'clang-tidy',
                    'diagnostic': diag_name,
                    'message': repl.get('DiagnosticMessage', ''),
                    'offset': repl.get('Offset', 0),
                    'length': repl.get('Length', 0),
                    'replacement': repl.get('ReplacementText', ''),
                    'category': self._categorize_diagnostic(diag_name)
                }
                
                files_data[filepath]['issues'].append(issue)
                files_data[filepath]['stats'][issue['category']] += 1
        
        # NekoCode互換形式
        nekocode_format = {
            'analysis_type': 'clang-tidy-quality',
            'project_stats': {
                'total_files': len(files_data),
                'total_issues': sum(len(f['issues']) for f in files_data.values()),
                'categories': self._aggregate_categories(files_data)
            },
            'files': dict(files_data)
        }
        
        with open(output_file, 'w') as f:
            json.dump(nekocode_format, f, indent=2)
        
        print(f"✅ Created NekoCode-compatible JSON: {output_file}")
        return nekocode_format
    
    def _categorize_diagnostic(self, diag_name):
        """診断名をカテゴリに分類"""
        if diag_name.startswith('modernize-'):
            return 'modernization'
        elif diag_name.startswith('performance-'):
            return 'performance'
        elif diag_name.startswith('readability-'):
            return 'readability'
        elif 'unused' in diag_name:
            return 'unused_code'
        elif diag_name.startswith('bugprone-'):
            return 'bug_risk'
        else:
            return 'other'
    
    def _aggregate_categories(self, files_data):
        """カテゴリ別の集計"""
        totals = defaultdict(int)
        for file_data in files_data.values():
            for category, count in file_data['stats'].items():
                totals[category] += count
        return dict(totals)


# 使用例
if __name__ == "__main__":
    merger = ClangTidyYamlMerger()
    
    # デモ: プロジェクト解析の流れ
    print("🚀 Clang-Tidy Project Analysis Workflow")
    print("=" * 60)
    
    # Step 1: 個別ファイルのYAML生成（実際のコマンド例）
    print("\n📋 Step 1: Generate individual YAML files")
    print("$ clang-tidy test_dead_code.cpp --export-fixes=fixes_1.yaml")
    print("$ clang-tidy complex_deadcode.cpp --export-fixes=fixes_2.yaml")
    print("$ clang-tidy clang_tidy_test.cpp --export-fixes=fixes_3.yaml")
    
    # Step 2: YAML統合
    print("\n📋 Step 2: Merge YAML files")
    yaml_files = ["example_tidy_fixes.yaml"]  # デモ用
    merged = merger.merge_yaml_files(yaml_files, "project_merged.yaml")
    
    # Step 3: 統計分析
    print("\n📋 Step 3: Analyze by diagnostic type")
    stats = merger.analyze_by_diagnostic("project_merged.yaml")
    for diag, info in sorted(stats.items()):
        print(f"  • {diag}: {info['count']} issues in {len(info['files'])} files")
    
    # Step 4: NekoCode形式に変換
    print("\n📋 Step 4: Convert to NekoCode format")
    nekocode_data = merger.create_nekocode_compatible_json("project_merged.yaml")
    
    print("\n✨ Project analysis complete!")
    print("  Use clang_tidy_visualizer.py to visualize the results")