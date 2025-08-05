# 🎯 Phase優先順位の見直し - デッドコード検出ファースト

## 🚫 Clang-Tidyを後回しにする理由

### 1. **複雑性の問題**
- YAML形式が複雑（Diagnostics → DiagnosticMessage → Replacements）
- セッションデータの大幅な拡張が必要
- インターフェースの大きな変更が必要

### 2. **本質的でない**
```
デッドコード検出 = 構造に関する問題（NekoCodeの本来の領域）
コード品質改善 = スタイルの問題（別の領域）
```

### 3. **維持コストが高い**
- Clang-Tidyのバージョンアップで頻繁に変更
- 言語ごとに異なるツール・フォーマット
- 設定オプションが多すぎる

## ✅ 正しい優先順位

### **Phase 1: デッドコード検出の完全制覇** 🎯
```
目標: 全言語でデッドコード検出率90%以上
```

#### C++ ✅ (完了)
- **ツール**: LTO (Link Time Optimization)
- **精度**: 100%
- **検出**: 未使用関数、未使用グローバル変数

#### Python 🔄 (次)
- **ツール**: Vulture
- **検出**: 未使用関数、未使用クラス、未使用インポート
```bash
vulture --make-whitelist src/ > whitelist.py
vulture src/ whitelist.py
```

#### Go 🔄 (次)
- **ツール**: staticcheck の unused サブセット
- **検出**: 未使用関数、未使用変数、未使用パッケージ
```bash
staticcheck -checks=U1000,U1001 ./...
```

#### Rust 🔄 (次)
- **ツール**: cargo の unused 警告
- **検出**: 未使用関数、未使用struct、未使用mod
```bash
cargo check --message-format=json | jq '.message.code.code' | grep unused
```

#### JavaScript/TypeScript 🔄 (次)
- **ツール**: ts-prune, dead-code-elimination
- **検出**: 未使用エクスポート
```bash
npx ts-prune
```

### **Phase 2: セッションデータ統一** 🔧
```json
{
  "mode": "complete", 
  "analysis": { /* NekoCode構造解析 */ },
  "dead_code": {
    "language": "cpp",
    "tool": "LTO",
    "unused_functions": [...],
    "unused_variables": [...],
    "unused_imports": [...],  // 言語による
    "detection_rate": "100%"
  }
}
```

### **Phase 3: コード品質改善（将来）** 🚀
- Clang-Tidy (C++)
- Pylint (Python)  
- golangci-lint (Go)
- Clippy (Rust)
- ESLint (JavaScript)

## 🎯 実装戦略

### 統一インターフェース
```bash
# 共通フォーマット
nekocode_ai analyze <path> --complete

# 内部で言語判定 → 適切なツール選択
C++    : NekoCode + LTO
Python : NekoCode + Vulture  
Go     : NekoCode + staticcheck(unused)
Rust   : NekoCode + cargo(unused)
JS/TS  : NekoCode + ts-prune
```

### 統一出力フォーマット
```json
{
  "dead_code": {
    "functions": ["unused_func1", "unused_func2"],
    "variables": ["unused_var1"],
    "imports": ["unused_import1"],  // 言語による
    "classes": ["UnusedClass"],     // 言語による
    "detection_method": "tool_name",
    "confidence": "high|medium|low"
  }
}
```

## 💡 メリット

### 1. **シンプル**
- セッションデータの変更最小限
- インターフェース一貫性
- デッドコード = 明確な目標

### 2. **実用性**
- デッドコード削除 = 即座にコードサイズ削減
- 全言語対応 = 実際のプロジェクトで使える
- CI/CD統合しやすい

### 3. **拡張性**
- Phase 1完了後にPhase 2を検討
- 各言語のベストプラクティスツール追加可能

## 🚀 次のアクション

1. **Python Vulture統合** (最優先)
2. **Go staticcheck統合**
3. **統一結果フォーマット設計**
4. **Rust cargo unused統合**
5. **JavaScript ts-prune統合**

これで「完全解析 = 完全デッドコード検出」として明確な価値提供ができるにゃ！