# 🐱 NekoCode使い方ガイド

## 📖 目次

1. [はじめに](#はじめに)
2. [インストール](#インストール)
3. [基本的な使い方](#基本的な使い方)
4. [高度な機能](#高度な機能)
5. [AI開発者向けガイド](#ai開発者向けガイド)
6. [トラブルシューティング](#トラブルシューティング)

## はじめに

NekoCode C++は、超高速なコード解析ツールです。特にAI開発者（Claude Code、GitHub Copilot等）との相性が抜群です！

## インストール

### 必要なもの
- C++17対応コンパイラ（GCC 7+、Clang 5+、MSVC 2017+）
- CMake 3.10以上
- Git

### ビルド手順

```bash
# 1. リポジトリをクローン
git clone https://github.com/moe-charm/nekocode.git
cd nekocode

# 2. ビルドディレクトリを作成
mkdir build && cd build

# 3. CMakeでビルド設定
cmake ..

# 4. ビルド実行（並列ビルド推奨）
make -j$(nproc)

# 5. 動作確認
./nekocode_ai --help
```

## 基本的な使い方

### 単一ファイル解析

```bash
# C++ファイルを解析
./nekocode_ai main.cpp

# JavaScriptファイルを解析
./nekocode_ai app.js

# 詳細な統計情報付き
./nekocode_ai --performance main.cpp
```

### ディレクトリ全体の解析

```bash
# srcディレクトリ全体を解析
./nekocode_ai src/

# 特定の言語のみ解析
./nekocode_ai --lang cpp src/

# コンパクトなJSON出力
./nekocode_ai --compact src/
```

## 高度な機能

### ⚡ パフォーマンス最適化（新機能！）

NekoCodeは超高速なストレージ最適化機能を搭載！

```bash
# 🔥 SSDモード - 並列処理で最高速
./nekocode_ai analyze large-project/ --ssd --performance
# CPUコア数フル活用、NVMe/SSDで威力発揮

# 🛡️ HDDモード - 安全なシーケンシャル処理
./nekocode_ai analyze large-project/ --hdd --performance  
# 1スレッドでHDDに優しい処理

# 📊 プログレス表示 - 大規模プロジェクト監視
./nekocode_ai session-create large-project/ --progress
# リアルタイム進捗: "🚀 Starting analysis: 38,021 files"
# プログレスファイル: sessions/SESSION_ID_progress.txt
```

**Claude Code攻略法**: 30,000ファイル以上のプロジェクトでは必ず `--progress` で進捗監視！

### インタラクティブセッション

最も強力な機能の1つです！

```bash
# 1. プログレス監視付きセッション作成
./nekocode_ai session-create /path/to/your/project --progress
# 出力例: Session created! Session ID: ai_session_20250727_180532

# 2. セッションIDを使って様々な解析を実行
SESSION_ID=ai_session_20250727_180532

# プロジェクト統計
./nekocode_ai session-cmd $SESSION_ID stats

# 複雑度ランキング（最重要！）
./nekocode_ai session-cmd $SESSION_ID complexity

# ファイル検索
./nekocode_ai session-cmd $SESSION_ID "find manager"

# 関数構造解析
./nekocode_ai session-cmd $SESSION_ID structure
```

### C++専用の高度な解析

#### インクルード依存関係

```bash
# 依存関係グラフを生成
./nekocode_ai session-cmd $SESSION_ID include-graph

# 循環依存を検出（重要！）
./nekocode_ai session-cmd $SESSION_ID include-cycles

# 不要なincludeを検出
./nekocode_ai session-cmd $SESSION_ID include-unused
```

#### テンプレート・マクロ解析

```bash
# テンプレート特殊化を検出
./nekocode_ai session-cmd $SESSION_ID template-analysis

# マクロ展開を追跡
./nekocode_ai session-cmd $SESSION_ID macro-analysis

# メタプログラミングパターンを検出
./nekocode_ai session-cmd $SESSION_ID metaprogramming
```

## AI開発者向けガイド

### Claude Codeでの使い方

1. **プロジェクトにNekoCodeを配置**
   ```bash
   cd your-project
   git clone https://github.com/moe-charm/nekocode.git tools/nekocode
   ```

2. **Claudeに伝える魔法の言葉**
   ```
   「tools/nekocodeにコード解析ツールがあるから使って」
   「このプロジェクトの複雑度を測定して」
   「循環依存をチェックして」
   ```

3. **Claudeが自動的に実行**
   - ビルド
   - セッション作成
   - 解析実行
   - 結果の解釈

### 実践例：リファクタリング

```bash
# 1. 現在の複雑度を測定
./nekocode_ai session-cmd $SESSION_ID complexity

# 出力例:
# FileA.cpp: Complexity 156 (Very Complex)
# FileB.cpp: Complexity 89 (Complex)

# 2. リファクタリング実施

# 3. 改善を確認
./nekocode_ai session-cmd $SESSION_ID complexity
# FileA.cpp: Complexity 23 (Simple)  ← 85%削減！
```

## トラブルシューティング

### ビルドエラー

**Q: CMakeがC++17をサポートしていないと言われる**
```bash
# GCCのバージョンを確認
g++ --version

# 古い場合は新しいコンパイラを指定
cmake -DCMAKE_CXX_COMPILER=g++-9 ..
```

**Q: Tree-sitterが見つからない**
```bash
# プレースホルダーモードでビルド（Tree-sitter無しで動作）
cmake -DUSE_TREE_SITTER=OFF ..
```

### 使用時の問題

**Q: セッションが見つからない**
```bash
# セッション一覧を確認
ls sessions/

# 新しいセッションを作成
./nekocode_ai session-create .
```

**Q: メモリ不足**
```bash
# スレッド数を制限
./nekocode_ai --threads 2 large-project/

# ファイル数を制限
./nekocode_ai --stats-only large-project/
```

## 💡 Pro Tips

1. **複雑度優先**: まず`complexity`コマンドで問題のあるファイルを特定
2. **セッション活用**: 何度も解析する場合は必ずセッションを使用（180倍高速！）
3. **並列ビルド**: `make -j$(nproc)`で全コアを使用してビルド
4. **JSON出力**: 他のツールと連携する場合は`--compact`オプション

---

詳しい情報は[公式ドキュメント](https://github.com/moe-charm/nekocode)をご覧ください！

*Happy Analyzing! 🐱*