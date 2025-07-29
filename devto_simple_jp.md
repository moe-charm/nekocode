---
title: NekoCode - Claude Codeが自動で使える爆速コード解析ツール
published: false
tags: cpp, ai, claudecode, 日本語
---

## 🐱 NekoCodeとは？

C++で書かれた超高速コード解析ツールです。Python製の代替ツールより**10-100倍高速**！

そして最高なのは、Claude Code（AnthropicのAIコーディングアシスタント）が**自動で使える**ことです。

## 🚀 Claude Codeユーザーは設定不要

```bash
# クローンするだけ
git clone https://github.com/moe-charm/nekocode.git

# Claude Codeに伝えるだけ
あなた: 「NekoCodeをローカルにクローンしたよ」
Claude: 「見つけました！ビルドして解析します...」
```

以上！Claude Codeが勝手に：
- NekoCodeを検出
- 自動ビルド
- 爆速解析実行
- 複雑度や依存関係を分析

## 📊 v2.0の新機能

### 1. **巨大プロジェクト対応**（30,000ファイル以上）
```bash
# 非同期でブロックしない
./nekocode_ai session-create-async 巨大プロジェクト/
```

### 2. **std::regex完全排除**
- Before: 4関数検出、頻繁にクラッシュ
- After: 2,362関数検出、クラッシュゼロ
- 結果: **590倍の改善！**

### 3. **リアルタイム進捗表示**
```bash
# 何が起きてるか見える
tail -f sessions/*/progress.txt
```

### 4. **実行時デバッグモード**
```bash
# リコンパイル不要！
./nekocode_ai find "パターン" --debug
```

## 🎯 実プロジェクトでの実績

| プロジェクト | ファイル数 | 検出関数数 | 処理時間 |
|------------|----------|-----------|---------|
| TypeScriptコンパイラ | 20,700 | 2,362 | 5秒以下 |
| lodash.js | 544KB | 489 | 1秒以下 |
| Unityゲーム | 30,000+ | 数千 | 非同期処理 |

## 💡 こんな人におすすめ

- Claude Code使ってる
- 大規模コードベースを扱ってる
- Pythonの解析ツールが遅くてイライラ
- 即座にコードの洞察が欲しい

---

⭐ [GitHub: moe-charm/nekocode](https://github.com/moe-charm/nekocode)  
🐦 [@CharmNexusCore](https://x.com/CharmNexusCore)