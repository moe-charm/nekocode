# 🐱 NekoCode Memory - 編集履歴管理

このディレクトリには、NekoCodeの編集履歴が保存されます。

## 📁 ディレクトリ構造

```
memory/
├── edit_previews/      # プレビュー詳細（実行前）
├── edit_history/       # 実行済み履歴  
└── index.json         # 統合インデックス
```

## 🔍 検索方法

```bash
# 特定のパターンを検索
grep -r "old_function" .

# 特定ファイルの編集履歴
grep -r "src/analyzer.cpp" edit_history/
```

## ⚙️ 設定

- 最大保存件数: 100件（自動ローテーション）
- 圧縮: なし（検索性優先）
- 形式: JSON + プレーンテキスト