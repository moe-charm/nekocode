# 🔗 LTO-Based C++ Project Analysis Design

## 🎯 **目標**
プロジェクト全体のC++コードに対してLTO（Link Time Optimization）を活用し、クロスファイル間の未使用関数・変数・クラスを正確に検出する。

## 🏗️ **設計概要**

### **Phase 1: プロジェクト発見・分析**
1. **CMakeLists.txt検出** - CMakeプロジェクトの場合
2. **Makefileベース** - 既存ビルドシステム活用
3. **自動検出モード** - `.cpp/.hpp`ファイル自動収集

### **Phase 2: LTOビルド戦略**
```bash
# LTOコンパイル戦略
g++ -flto -O2 -ffunction-sections -fdata-sections -c *.cpp
g++ -flto -O2 -Wl,--gc-sections -Wl,--print-gc-sections *.o -o project
```

### **Phase 3: 未使用コード検出手法**

#### **手法A: リンカー削除ログ解析**
```bash
# --print-gc-sections でリンカーが削除したセクションをログ出力
g++ -flto -O2 -Wl,--gc-sections -Wl,--print-gc-sections
# 出力例:
# removing unused section '.text._Z13unused_funcv' in file 'main.o'
```

#### **手法B: シンボル比較解析**
1. **コンパイル時**: `nm`でオブジェクトファイルの全シンボル収集
2. **リンク後**: `nm`で最終バイナリのシンボル収集  
3. **差分計算**: コンパイル時 - リンク後 = 削除されたシンボル

#### **手法C: LTO中間表現解析**
```bash
# GIMPLE中間表現ダンプでより詳細な解析
g++ -flto -O2 -fdump-ipa-all -fdump-tree-all
# 生成される.ipa/.treeファイルを解析
```

## 🛠️ **実装アーキテクチャ**

### **Class: LTOProjectAnalyzer**
```python
class LTOProjectAnalyzer:
    def __init__(self, project_path):
        self.project_path = Path(project_path)
        self.build_system = self._detect_build_system()
        
    def analyze_deadcode(self):
        # 1. プロジェクト構造解析
        cpp_files = self._discover_cpp_files()
        
        # 2. LTOビルド実行
        build_result = self._build_with_lto()
        
        # 3. 未使用コード検出
        unused_symbols = self._extract_unused_symbols()
        
        # 4. NekoCode構造解析との統合
        return self._merge_with_nekocode_analysis()
```

### **ビルドシステム対応**
- **CMake**: `cmake -DCMAKE_CXX_FLAGS="-flto -O2"`
- **Makefile**: Makefile書き換えでLTOフラグ注入
- **自動モード**: 全.cppファイル収集→LTOビルド

## 🚨 **エラーハンドリング**

### **想定される課題**
1. **ビルドエラー** - テンプレート・リンクエラー対応
2. **メモリ不足** - 大規模プロジェクトでのLTO
3. **時間制限** - タイムアウト機能
4. **依存関係** - 外部ライブラリとの競合

### **対処方針**
```python
def safe_lto_build(self):
    try:
        # フルLTO試行
        return self._build_with_full_lto()
    except LTOMemoryError:
        # メモリ不足時はThinLTO
        return self._build_with_thin_lto()
    except BuildError:
        # ビルドエラー時は段階的フォールバック
        return self._fallback_analysis()
```

## 📊 **出力フォーマット**

### **統合結果JSON**
```json
{
  "analysis_type": "lto_project_wide",
  "build_system": "cmake",
  "total_files": 25,
  "lto_results": {
    "status": "success",
    "unused_functions": [
      {"name": "unused_helper", "file": "utils.cpp", "line": 42},
      {"name": "debug_function", "file": "debug.cpp", "line": 15}
    ],
    "unused_variables": [
      {"name": "global_counter", "file": "globals.cpp", "line": 8}
    ],
    "unused_classes": [
      {"name": "UnusedClass", "file": "legacy.cpp", "line": 100}
    ],
    "confidence": "high",
    "method": "lto_linker_gc_sections"
  }
}
```

## 🎯 **NekoCode統合ポイント**

1. **universal_deadcode_analyzer.py** の `_analyze_cpp_lto()` を実装
2. **SessionData** の `dead_code_info` に LTO結果を統合
3. **MCP経由** でClaude Codeから利用可能

---
**目標**: static関数制限を打破し、プロジェクト全体の本格的デッドコード検出を実現！🚀