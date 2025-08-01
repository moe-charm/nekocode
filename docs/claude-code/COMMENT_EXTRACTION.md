# 💬 Comment Extraction & Analysis - Claude Code完全ガイド

## 📌 概要

NekoCode v2.1で追加された**コメント抽出・解析機能**は、コメントアウトされたコードを自動検出し、AIでコードらしさを判定する革新的機能です。

## 🚀 クイックスタート

```bash
# 単一ファイル解析
./nekocode_ai analyze legacy_code.py --io-threads 8

# フォルダ全体の高速統計
./nekocode_ai analyze project/ --stats-only --io-threads 16
```

## 📊 JSON出力フォーマット

### 単一ファイル解析時
```json
{
  "analysis_type": "single_file",
  "commented_lines": [
    {
      "line_start": 42,
      "line_end": 42,
      "type": "single_line",
      "content": "# old_function(data)",
      "looks_like_code": true
    },
    {
      "line_start": 50,
      "line_end": 55,
      "type": "multi_line",
      "content": "/* class LegacyProcessor:\n     def process(self):\n         return self.data */",
      "looks_like_code": true
    }
  ],
  "statistics": {
    "commented_lines_count": 120
  }
}
```

### フォルダ解析時（--stats-only）
```json
{
  "summary": {
    "total_commented_lines": 3456,
    "total_files": 234,
    "total_lines": 45678
  }
}
```

## 🤖 コード判定アルゴリズム

### 判定基準（スコアリング方式）

各言語でキーワード・構文パターンをスコアリング：

#### JavaScript/TypeScript
- **+2点**: `const`, `let`, `var`, `function`, `class`, `if`, `for`, `while`, `return` など
- **+1点**: `()`, `[]`, `{}`, `=`, `.`, `=>`, `===` などの演算子・記号
- **-1点**: `TODO`, `FIXME`, `NOTE`, `BUG` などのメタコメント
- **判定**: 3点以上でコードと判定

#### Python
- **+2点**: `def`, `class`, `import`, `from`, `if`, `for`, `while`, `return` など
- **+3点**: `import`, `from` ステートメント
- **+1点**: `()`, `[]`, `=`, `.`, `:` などのPython構文
- **判定**: 3点以上でコードと判定

#### C/C++
- **+2点**: `class`, `struct`, `if`, `for`, `while`, `return`, `void`, `int` など
- **+1点**: `()`, `[]`, `{}`, `=`, `->`, `::`, `;` などのC++構文
- **判定**: 3点以上でコードと判定

## 🎯 Claude Code活用パターン

### 1. レガシーコード発見
```bash
# プロジェクト全体をスキャン
./nekocode_ai analyze src/ --stats-only --io-threads 16

# 結果分析（Claude Codeで）
You: "commented_lines_countが多いファイルを特定して"
Claude: "sessions.py に120個のコメント行があり、90個がコードと判定されています。
これは過去の実装や代替案が多く残っている可能性を示しています。"
```

### 2. リファクタリング支援
```bash
# 特定ファイルの詳細解析
./nekocode_ai analyze src/core/engine.cpp --io-threads 8

# Claude Codeでの分析
You: "looks_like_codeがtrueのコメントを分析して、削除可能か判断して"
Claude: "42-45行目の旧アルゴリズムは新実装で完全に置き換えられているため削除可能です。
ただし、78行目のコメントは性能比較のために残すことを推奨します。"
```

### 3. コード品質評価
```bash
# 統計情報から品質指標を算出
total_lines: 10000
commented_lines_count: 1500
code_like_comments: 900

# Claude Codeでの評価
You: "このプロジェクトのコメント品質を評価して"
Claude: "総行数の15%がコメントで、その60%がコードと判定されています。
これは技術的負債が蓄積している可能性を示唆しています。"
```

### 4. 開発履歴の理解
```bash
# 時系列でコメントを分析
You: "コメントアウトされた関数定義から開発の変遷を推測して"
Claude: "コメントから以下の進化が読み取れます：
1. 初期: シンプルな同期処理（process_sync）
2. 中期: 非同期対応（process_async）
3. 現在: ストリーム処理（process_stream）
パフォーマンス改善を重視した開発が行われています。"
```

## 📈 実績データ

実際のプロジェクトでのテスト結果：

| プロジェクト | ファイル数 | 総コメント数 | コード判定率 | 処理時間 |
|---|---|---|---|---|
| Python requests | 40+ | 120/file | 75% | <1秒/file |
| TypeScript compiler | 100+ | 6,639/file | 64% | 11.6秒/file |
| C++ nlohmann/json | 1 | 3,923 | 27% | 16.7秒 |

## 🔧 技術仕様

### パフォーマンス最適化
- 既存の前処理ステップに統合（追加オーバーヘッドなし）
- 言語別最適化されたパーサー使用
- メモリ効率的な処理（大規模ファイル対応）

### 制限事項
- ネストしたコメント非対応（言語仕様に準拠）
- 文字列内のコメント記号は正しく除外
- エスケープシーケンスは考慮

## 💡 ベストプラクティス

1. **大規模プロジェクト**: `--stats-only --io-threads 16` で高速統計
2. **詳細分析**: 個別ファイルを `--io-threads 8` で解析
3. **定期監視**: CI/CDに組み込んでコメント増加を監視
4. **リファクタリング**: 四半期ごとにコメント整理

## 🆘 トラブルシューティング

### Q: コメントが検出されない
A: ファイルエンコーディングがUTF-8か確認。NekoCodeはUTF-8完全対応。

### Q: looks_like_codeの判定が不正確
A: 言語指定オプション `--lang python` で精度向上。

### Q: 処理が遅い
A: `--io-threads` を増やす（SSD環境では16-32推奨）。

---
**Last Updated**: 2025-08-01  
**Version**: NekoCode AI v2.1