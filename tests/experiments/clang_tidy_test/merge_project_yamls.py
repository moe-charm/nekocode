#!/usr/bin/env python3
"""プロジェクトのYAMLファイルをマージ"""
import sys
sys.path.append('../..')
from yaml_merger import ClangTidyYamlMerger

merger = ClangTidyYamlMerger()

# 実際のYAMLファイルをマージ
yaml_files = ["clang_tidy_test.yaml", "complex_deadcode.yaml"]
print(f"🔗 Merging {yaml_files}...")

merged = merger.merge_yaml_files(yaml_files, "project_merged.yaml")

# 診断タイプ別の統計
print("\n📊 Analysis by diagnostic type:")
stats = merger.analyze_by_diagnostic("project_merged.yaml")
for diag, info in sorted(stats.items()):
    print(f"  • {diag}: {info['count']} issues in {len(info['files'])} files")

# NekoCode形式に変換
print("\n📄 Converting to NekoCode format...")
nekocode_data = merger.create_nekocode_compatible_json("project_merged.yaml", "project_analysis.json")

print("\n✨ Project analysis complete!")
print("  • Merged YAML: project_merged.yaml")
print("  • NekoCode JSON: project_analysis.json")