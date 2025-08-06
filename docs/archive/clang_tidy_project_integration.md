# 🔧 Clang-Tidy プロジェクト解析 & NekoCode統合

## 📊 プロジェクト単位の解析方法

### 1️⃣ **compile_commands.json を使った方法**（推奨）

```bash
# CMakeプロジェクトの場合
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build/ src/*.cpp --export-fixes=project_fixes.yaml

# Makeプロジェクトの場合（Bearツール使用）
bear -- make
clang-tidy -p . *.cpp --export-fixes=project_fixes.yaml
```

### 2️⃣ **直接ファイル指定**

```bash
# プロジェクト内の全C++ファイル
clang-tidy $(find . -name "*.cpp" -o -name "*.hpp") \
  --export-fixes=all_fixes.yaml \
  -checks="-*,modernize-*,misc-unused-*,performance-*"
```

### 3️⃣ **並列実行**（大規模プロジェクト向け）

```bash
# run-clang-tidy.py を使用（Clangに付属）
run-clang-tidy.py -p build/ -j 8 -export-fixes=parallel_fixes.yaml
```

## 🎯 NekoCode + Clang-Tidy 完全統合案

```python
# 統合ワークフロー
class NekoCodeClangTidyIntegration:
    def analyze_project(self, project_dir):
        # 1. NekoCode構造解析（高速）
        nekocode_session = run_nekocode_session(project_dir)
        
        # 2. Clang-Tidy品質解析（詳細）
        clang_tidy_yaml = run_clang_tidy_project(project_dir)
        
        # 3. 統合レポート生成
        return {
            "structure": nekocode_session,      # 構造情報
            "quality": clang_tidy_yaml,         # 品質改善提案
            "dead_code": lto_analysis,          # デッドコード（LTO）
            "visualization": create_graph()      # 視覚化
        }
```

## 📋 YAML出力の特徴

### 単一ファイル
```yaml
MainSourceFile: 'main.cpp'
Replacements:
  - FilePath: 'main.cpp'
    Offset: 123
    ...
```

### プロジェクト全体
```yaml
MainSourceFile: ''  # 空になる
Replacements:
  - FilePath: 'src/main.cpp'
    Offset: 123
    ...
  - FilePath: 'src/utils.cpp'
    Offset: 456
    ...
  - FilePath: 'include/config.hpp'
    Offset: 789
    ...
```

## 🚀 利点

1. **スケーラブル**: 小規模から大規模プロジェクトまで対応
2. **並列処理**: run-clang-tidy.pyで高速化
3. **統一フォーマット**: YAMLで全ての修正提案を管理
4. **NekoCode統合**: 構造解析と品質改善の両立

## 💡 ChatGPTさんの提案通り

```bash
# 1. Clang-Tidyで解析
clang-tidy main.cpp --export-fixes=tidy.yaml

# 2. NekoCodeでYAML読み込み＆可視化
python3 clang_tidy_visualizer.py tidy.yaml
```

これがプロジェクト全体でも動作するにゃ！