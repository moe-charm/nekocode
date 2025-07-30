# 🤖 NekoCode - Claude Code クイックスタート

**NekoCodeは多言語対応のコード解析ツールです。コードの複雑度、構造、TODOコメントなどを分析できます。**

## ⚡ 3ステップで開始

### 1. ビルド
```bash
cd build && cmake .. && make -j8
```

### 2. セッション作成
```bash
# プロジェクト全体を解析
./build/nekocode_ai session-create src/

# 出力例：
# {
#   "session_id": "ai_session_20250730_123456",
#   "message": "✅ AI Session created"
# }
```

### 3. 解析実行
```bash
# セッションIDをコピペして使用
./build/nekocode_ai session-cmd ai_session_20250730_123456 stats
./build/nekocode_ai session-cmd ai_session_20250730_123456 complexity
./build/nekocode_ai session-cmd ai_session_20250730_123456 large-files
./build/nekocode_ai session-cmd ai_session_20250730_123456 duplicates
./build/nekocode_ai session-cmd ai_session_20250730_123456 todo
```

## 🎯 主要コマンド一覧

| コマンド | 説明 | 出力例 |
|---------|------|--------|
| `stats` | プロジェクト統計 | ファイル数、行数、複雑度 |
| `complexity` | 複雑度ランキング | ファイル別複雑度（高い順） |
| `large-files` | 大きいファイル一覧 | デフォルト500行以上 |
| `large-files --threshold 1000` | カスタム閾値 | 1000行以上のファイル |
| `duplicates` | 重複ファイル検出 | _backup, _old等を検出 |
| `todo` | TODOコメント検出 | TODO/FIXME/BUG等を優先度別表示 |
| `complexity-ranking` | 関数複雑度ランキング | 最も複雑な関数トップ50 |
| `find <symbol>` | シンボル検索 | 関数・変数の使用箇所 |

## 🚀 実用例

### リファクタリング対象を特定
```bash
# 1. 大きくて複雑なファイルを探す
./build/nekocode_ai session-cmd <session_id> large-files

# 2. 重複ファイルをクリーンアップ
./build/nekocode_ai session-cmd <session_id> duplicates

# 3. 緊急対応が必要なTODOを確認
./build/nekocode_ai session-cmd <session_id> todo
```

### コードレビュー準備
```bash
# プロジェクト全体の健全性チェック
./build/nekocode_ai session-cmd <session_id> stats
./build/nekocode_ai session-cmd <session_id> complexity-ranking
```

## 📚 詳細ドキュメント

- [完全使用ガイド](docs/USAGE_jp.md)
- [Claude Code専用機能](docs/claude-code/INDEX.md)
- [開発者向け情報](CLAUDE.md)

## 🆘 トラブルシューティング

### よくある問題
```bash
# セッションが見つからない場合
ls sessions/  # セッション一覧確認

# ビルドエラーの場合
rm -rf build/ && mkdir build && cd build && cmake .. && make -j8

# 大きなプロジェクトでタイムアウトする場合
./build/nekocode_ai session-create-async large_project/ --progress
```

---
**🎯 このファイルは Claude Code が最初に読むべきガイドです**
**📝 詳細な開発ルール・設定は [CLAUDE.md](CLAUDE.md) を参照**