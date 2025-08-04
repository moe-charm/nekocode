# 🐱 NekoCode Project - Claude Context Information

## 📋 **プロジェクト概要**

**NekoCode** は10-100倍高速な多言語コード解析ツールです。現在、**Universal AST Revolution**という大規模統一化プロジェクトを実行中。

### **基本情報**
- **言語**: C++17, PEGTL, CMake
- **対応言語**: JavaScript, TypeScript, C++, C, Python, C#, Go, Rust
- **特徴**: Claude Code最適化、MCP統合、セッション機能

## 🚀 **現在実行中: Universal AST Revolution**

### **背景・問題**
- JavaScript/TypeScriptのみAST機能実装済み
- 他言語は重複コードだらけ（extract_functions系が11ファイルに散在）
- 99%共通化可能なのに分離されている

### **解決策・目標**
**統一アーキテクチャ**により全言語でAST Revolution実現：
```cpp
template<typename LanguageTraits>
class UniversalCodeAnalyzer {
    // 99%共通処理 + 1%言語固有適応
};
```

## 🛡️ **実行中の作戦: 安全第一3段階アプローチ**

### **Phase 1: 実験環境構築** [🔥 現在実行中]
```bash
cp -r src src2           # 安全なコピー環境
mkdir src2/universal     # 抽象化レイヤー
```
- **リスク**: ゼロ（元srcは無傷）
- **期間**: 1-2時間
- **状況**: 実行中

### **Phase 2: 抽象化レイヤー設計** [次の段階]
- UniversalTreeBuilder<LanguageTraits>
- UniversalCodeAnalyzer<Grammar, Adapter>
- Language Adapter Pattern

### **Phase 3: 段階的移行** [最終段階]
1. JavaScript (既存AST活用)
2. TypeScript (継承済み)  
3. Python (文法シンプル)
4. C++ (最難関)
5. 他言語

## 📊 **既存コード再利用マップ**

### 🟢 **完全再利用 (33% - 10ファイル)**
- `src/core/` - Session管理、統計処理 → **変更なし**
- `src/utils/` - ファイル処理、UTF8処理 → **変更なし**  
- `src/formatters/` - 出力フォーマット → **変更なし**

### 🟡 **部分再利用 (50% - 15ファイル)**
- 各言語analyzer - パターンマッチング部分抽出
- base_analyzer.hpp - インターフェース活用

### 🔴 **新規設計 (17% - 5ファイル)**
- src2/universal/ - 統一アーキテクチャ

## 🎯 **重要なファイル**

### **現在のAST実装** (参考用)
- `src/analyzers/javascript/javascript_pegtl_analyzer.hpp` - 既存AST実装
- `include/nekocode/types.hpp` - ASTNode定義済み

### **進捗管理**
- `current_task.md` - 現在のタスク詳細
- このファイル (CLAUDE.md) - プロジェクト全体把握

### **実験環境**
- `src2/` - 安全な実験環境（コピー）
- `src2/universal/` - 新しい統一システム

## 💡 **技術的ポイント**

### **AST Revolution の核心**
```cpp
// 既存: 言語別に重複実装
JavaScriptAnalyzer::extract_functions_from_line()
PythonAnalyzer::extract_functions()
CppAnalyzer::extract_functions()

// 新設計: 99%共通化
template<typename Lang>
UniversalAnalyzer<Lang>::analyze() {
    // 共通処理 + 言語固有アダプター
}
```

### **現在利用可能なAST機能** (JavaScript/TS)
```bash
# 既に動作中のAST機能（JS/TS専用）
./nekocode_ai session-command <id> ast-stats
./nekocode_ai session-command <id> ast-query <path>
./nekocode_ai session-command <id> scope-analysis <line>
./nekocode_ai session-command <id> ast-dump [format]
```

## 🔄 **進捗状況**

### **完了済み**
- [x] AST Revolution機能実装（JavaScript/TypeScript）
- [x] MCP統合完了
- [x] ドキュメント更新完了
- [x] 大規模リファクタリング作戦立案完了
- [x] 既存コード評価完了

### **実行中** 
- [ ] **現在**: src2実験環境構築
- [ ] CLAUDE.md情報記録（このファイル）

### **次の予定**
- [ ] universal/抽象化レイヤー設計
- [ ] JavaScript統一アダプター実装
- [ ] 段階的全言語展開

---

## 📝 **Claude向けのメモ**

### **重要なコマンド**
```bash
# プロジェクトのビルド
cd build && make -j$(nproc)

# テスト実行  
./bin/nekocode_ai session-create test.js
./bin/nekocode_ai session-command <id> ast-stats

# 進捗確認
cat current_task.md
```

### **注意点**
- 元の`src/`は絶対に変更しない（安全性確保）
- `src2/`で全ての実験を実行
- 各段階で必ずcurrent_task.md更新
- ビルドエラーが出たら即座に報告

---
**最終更新**: 2025-01-05 00:55:00  
**作成者**: Claude + User collaborative design  
**状況**: 🔥 Universal AST Revolution 実行中