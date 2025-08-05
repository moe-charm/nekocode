#!/usr/bin/env python3
"""
Clang-Tidy プロジェクト全体解析デモ
compile_commands.jsonを使った本格的な解析
"""

import json
import subprocess
from pathlib import Path

class ProjectClangTidyAnalyzer:
    def generate_compile_commands(self, project_dir):
        """CMakeプロジェクトのcompile_commands.json生成"""
        print("📦 Generating compile_commands.json...")
        
        # CMakeプロジェクトの場合
        cmake_commands = [
            "cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .",
            "cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        ]
        
        # Bear（コンパイルコマンド記録ツール）を使う場合
        bear_commands = [
            "bear -- make",
            "bear -- g++ *.cpp"
        ]
        
        print("Option 1: CMake project")
        for cmd in cmake_commands:
            print(f"  $ {cmd}")
        
        print("\nOption 2: Using Bear tool")
        for cmd in bear_commands:
            print(f"  $ {cmd}")
        
        # サンプルcompile_commands.json作成
        sample_commands = [
            {
                "directory": str(Path(project_dir).absolute()),
                "command": "g++ -std=c++17 -Wall -c test_dead_code.cpp",
                "file": "test_dead_code.cpp"
            },
            {
                "directory": str(Path(project_dir).absolute()),
                "command": "g++ -std=c++17 -Wall -c complex_deadcode.cpp",
                "file": "complex_deadcode.cpp"
            },
            {
                "directory": str(Path(project_dir).absolute()),
                "command": "g++ -std=c++17 -Wall -c clang_tidy_test.cpp",
                "file": "clang_tidy_test.cpp"
            }
        ]
        
        with open("compile_commands.json", "w") as f:
            json.dump(sample_commands, f, indent=2)
        
        print("✅ Created sample compile_commands.json")
    
    def run_project_analysis(self, project_dir, checks=None):
        """プロジェクト全体のClang-Tidy解析"""
        print(f"\n🔍 Analyzing entire project: {project_dir}")
        
        # 方法1: run-clang-tidy（並列実行）
        print("\n📊 Method 1: Using run-clang-tidy.py (parallel)")
        cmd1 = [
            "run-clang-tidy.py",
            "-p", ".",  # compile_commands.jsonのパス
            "-checks", checks or "modernize-*,performance-*,readability-*",
            "-export-fixes", "project_fixes.yaml"
        ]
        print(f"Command: {' '.join(cmd1)}")
        
        # 方法2: 個別ファイル解析の集約
        print("\n📊 Method 2: Individual file analysis")
        yaml_files = []
        
        cpp_files = list(Path(project_dir).glob("*.cpp"))
        for i, cpp_file in enumerate(cpp_files):
            yaml_file = f"fixes_{i}.yaml"
            cmd = [
                "clang-tidy",
                str(cpp_file),
                f"--export-fixes={yaml_file}",
                f"-checks={checks or 'modernize-*,performance-*'}"
            ]
            print(f"  $ {' '.join(cmd)}")
            yaml_files.append(yaml_file)
        
        # YAML統合
        print("\n🔗 Merging YAML files...")
        self.merge_yaml_files(yaml_files, "merged_fixes.yaml")
    
    def merge_yaml_files(self, yaml_files, output_file):
        """複数のYAMLファイルを統合"""
        print(f"Merging {len(yaml_files)} YAML files into {output_file}")
        
        # 実装例（簡略版）
        merged_replacements = []
        main_source = None
        
        for yaml_file in yaml_files:
            # 各YAMLファイルから置換情報を収集
            # （実際にはyaml.safe_loadで読み込む）
            pass
        
        # 統合結果の構造
        merged_data = {
            "MainSourceFile": "project",
            "Replacements": merged_replacements
        }
        
        print(f"✅ Merged {len(merged_replacements)} replacements")
    
    def show_usage_examples(self):
        """実際の使用例を表示"""
        print("\n📚 Real-world usage examples:\n")
        
        print("1️⃣ Basic project analysis:")
        print("   $ clang-tidy -p . $(find . -name '*.cpp') --export-fixes=all_fixes.yaml")
        
        print("\n2️⃣ With specific checks:")
        print("   $ clang-tidy -p . src/*.cpp -checks='-*,modernize-*,misc-unused-*' \\")
        print("     --export-fixes=modernize_fixes.yaml")
        
        print("\n3️⃣ Using clang-tidy with CMake:")
        print("   $ cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
        print("   $ clang-tidy -p build/ src/*.cpp --export-fixes=cmake_fixes.yaml")
        
        print("\n4️⃣ Parallel execution (fastest):")
        print("   $ run-clang-tidy.py -p build/ -j 8 -export-fixes=parallel_fixes.yaml")
        
        print("\n5️⃣ Apply fixes automatically:")
        print("   $ clang-apply-replacements . # YAMLファイルから修正を適用")


# デモ実行
if __name__ == "__main__":
    analyzer = ProjectClangTidyAnalyzer()
    
    print("🚀 Clang-Tidy Project Analysis Demo")
    print("=" * 60)
    
    # compile_commands.json生成方法
    analyzer.generate_compile_commands(".")
    
    # プロジェクト解析方法
    analyzer.run_project_analysis(".")
    
    # 実際の使用例
    analyzer.show_usage_examples()
    
    print("\n💡 Key points for project-wide analysis:")
    print("  • compile_commands.json is essential for accurate analysis")
    print("  • run-clang-tidy.py enables parallel processing")
    print("  • YAML files can be merged for unified view")
    print("  • NekoCode can parse these YAMLs for visualization!")