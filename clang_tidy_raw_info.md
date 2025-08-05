# 🔍 Clang-Tidy YAML生情報から得られるもの

## 📊 基本情報

### 1. **診断情報（Diagnostics）**
- `DiagnosticName`: どの検査ルールで発見されたか
  - `modernize-use-nullptr`: NULL → nullptr
  - `modernize-use-auto`: 型推論を使うべき
  - `readability-container-size-empty`: size()==0 → empty()
  - `misc-unused-*`: 未使用コード検出

### 2. **位置情報**
- `FilePath`: 問題のあるファイル（絶対パス）
- `FileOffset`: ファイル先頭からのバイト位置
- `Offset`: 置換開始位置（バイト単位）
- `Length`: 置換する文字数

### 3. **修正内容**
- `ReplacementText`: 置換後のテキスト
- `Message`: 人間が読める説明
- `Level`: 重要度（Warning, Error, Note）

## 🎯 得られる価値

### コード品質指標
```yaml
DiagnosticName: modernize-use-nullptr
# → C++11以降のモダンな書き方を使っていない箇所数
# → レガシーコードの割合が分かる

DiagnosticName: performance-unnecessary-copy-initialization  
# → パフォーマンス問題の箇所数
# → 最適化の余地が分かる

DiagnosticName: readability-*
# → 可読性の問題箇所数
# → コードの読みやすさ指標
```

### 技術的債務の可視化
1. **モダン化度**: modernize-*ルールの検出数
2. **パフォーマンス**: performance-*ルールの検出数
3. **バグリスク**: bugprone-*ルールの検出数
4. **未使用コード**: misc-unused-*の検出数

### 自動修正可能性
- `Replacements`が存在 = 自動修正可能
- 空の`Replacements` = 手動修正が必要
- 複数の`Replacements` = 複雑な修正

## 📈 統計的分析

### ファイル別統計
```python
# YAMLから抽出できる情報
{
  "file1.cpp": {
    "total_issues": 15,
    "auto_fixable": 12,
    "categories": {
      "modernization": 8,
      "performance": 3,
      "readability": 4
    }
  }
}
```

### プロジェクト全体の健全性
- 総問題数
- 自動修正可能率
- カテゴリ別分布
- ホットスポット（問題が集中しているファイル）

## 🔧 活用方法

### 1. 優先順位付け
```yaml
# 重要度の高い順
1. bugprone-* (バグリスク)
2. performance-* (パフォーマンス)
3. readability-* (可読性)
4. modernize-* (モダン化)
```

### 2. 段階的改善
- Phase 1: 自動修正可能なものを一括修正
- Phase 2: パフォーマンス関連を手動修正
- Phase 3: モダン化を計画的に実施

### 3. CI/CD統合
```bash
# プルリクエストごとに
clang-tidy --export-fixes=pr_fixes.yaml
# 新規問題の検出 = マージ拒否
```

## 💡 NekoCodeとの統合価値

### NekoCodeが提供するもの
- 構造情報（関数、クラス、依存関係）
- 複雑度メトリクス
- インクルード解析

### Clang-Tidyが追加するもの
- 実行時の問題検出
- コーディング規約違反
- パフォーマンス改善提案
- 未使用コード検出（静的解析）

### 統合による相乗効果
```
NekoCode構造 + Clang-Tidy品質 = 完全なコード理解
```

これでコードの「構造」と「品質」の両面から完全な分析が可能にゃ！