# 🤖 Claude Code 関連ドキュメント インデックス

Claude Codeユーザー向けドキュメントのクイックアクセスガイド

## 📌 重要なお知らせ
**NekoCodeはClaude Code専用に設計されています** - [設計哲学](DESIGN_PHILOSOPHY.md)

## 📚 主要ドキュメント

### 1. 🏗️ **大規模プロジェクト対応ガイド** ⭐️ NEW!
**📍 Path:** `docs/LARGE_PROJECT_HANDLING.md`

Claude Code最適化の最新機能！高速処理でブロックを回避
- 🚀 --stats-only オプション（高速統計）
- 📊 --io-threads 16 オプション（並列処理）
- 💡 Claude Code Quick Startセクション

### 2. 🎯 **基本使用方法（日本語）**
**📍 Path:** `docs/USAGE_jp.md`

NekoCode AIの基本的な使い方
- 対話式セッション作成
- find機能の使い方
- --debugオプション

### 3. 🐛 **デバッグガイド**
**📍 Path:** `docs/DEBUG_GUIDE.md`

Claude Code使用時のトラブルシューティング
- JSON出力の問題解決
- デバッグ情報の制御
- エラー対処法

### 4. ⚡ **パフォーマンスガイド**
**📍 Path:** `docs/PERFORMANCE_GUIDE.md`

大規模プロジェクトの高速化テクニック
- ストレージ別最適化（--ssd/--hdd）
- 並列処理の調整
- メモリ使用の最適化

## 🚀 クイックスタート

### 通常の使用（小〜中規模プロジェクト）
```bash
# セッション作成
nekocode_ai session-create project/

# 検索
nekocode_ai session-cmd [session_id] "find interface --limit 20"
```

### 大規模プロジェクト（1,000ファイル以上）
```bash
# 高速統計解析（Claude Codeブロック回避）
nekocode_ai analyze large_project/ --stats-only --io-threads 16

# 詳細解析が必要な場合はセッション作成
nekocode_ai session-create large_project/
nekocode_ai session-cmd [session_id] stats
```

## 💡 よくある質問

**Q: Claude Codeが長時間フリーズする**
→ A: `--stats-only --io-threads 16` オプションで高速化

**Q: JSON出力にエラーが混ざる**
→ A: `docs/DEBUG_GUIDE.md` のトラブルシューティング参照

**Q: 解析が遅い**
→ A: `docs/PERFORMANCE_GUIDE.md` の最適化手法を確認

## 📌 その他の関連ドキュメント

- `README.md` - プロジェクト全体の概要
- `README_jp.md` - 日本語版README
- `examples/ai_excitement_demo.md` - AI向けデモ
- `CHANGELOG_PEGTL.md` - 技術的な変更履歴

---
**最終更新:** 2025-07-30
**対応バージョン:** NekoCode AI v2.0 (Claude Code専用設計)