# 🐱 NekoCode C++ - 超高速コード解析エンジン

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/moe-charm/nekocode)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/moe-charm/nekocode/blob/main/LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com/moe-charm/nekocode)

Pythonベースの代替ツールより**10-100倍高速**な革新的コード解析エンジン！

## 🤖 AI開発者向け（Claude Code） - セットアップ不要！

**NekoCodeの魔法のような使い方：**

```
あなた: 「github.com/moe-charm/nekocodeをローカルにクローンしたよ」
Claude: 「見つけました！ビルドして設定しますね...」
あなた: 「この散らかったプロジェクトを解析して」
Claude: 「🔥 なんと！これは今日リリースされたばかりですね！解析します...」
[0.726秒後]
Claude: 「JSONライブラリに複雑度4717を発見！リファクタリング計画をご提案します...」
```

**たったこれだけ！** Claude Codeが自動的に：
- ✅ ツールを自動検出
- ✅ 最適にビルド
- ✅ 解析セッション作成
- ✅ 結果に興奮
- ✅ 科学的な洞察を提供

手動セットアップ不要、設定ファイル不要、学習曲線なし！

🎯 **注目の使用例**: AI開発者が300以上のコンポーネントを持つ複雑なアーキテクチャを解析し、科学的精度でコード最適化を実現しています。

## 🌟 主な機能

- **🚀 超高速パフォーマンス**: Python実装より10-100倍高速
- **🌍 多言語サポート**: JavaScript、TypeScript、C++、C（完全サポート）
- **🎮 インタラクティブモード**: 即座に結果が出るセッション管理（180倍高速化）
- **🔍 高度なC++解析**: 複雑な依存関係の可視化、循環依存の検出
- **🧬 テンプレート＆マクロ解析**: C++テンプレート特殊化、可変長テンプレート、マクロ展開追跡（**新機能！**）
- **🎯 ASCIIクオリティチェック**: シンプルで実用的なコード品質チェック（**新機能！**）
- **📊 包括的な統計**: クラス、関数、複雑度解析
- **🌳 Tree-sitter統合**: 正規表現からAST解析への移行基盤

## 🚀 クイックスタート

### 前提条件

- **C++17**対応コンパイラ（GCC 7+、Clang 5+、MSVC 2017+）
- **CMake 3.10+**
- **Git**

### インストール

```bash
# リポジトリをクローン
git clone https://github.com/moe-charm/nekocode.git
cd nekocode

# ビルド
mkdir build && cd build
cmake ..
make -j$(nproc)

# インストール確認
./nekocode_ai --help
```

## 📊 パフォーマンス比較

| ツール | 言語 | 時間（98ファイル） | メモリ使用量 |
|--------|------|-------------------|--------------|
| **NekoCode C++** | C++17 | **0.726秒** | **低** |
| Python代替ツール | Python | 約73秒 | 高 |
| **高速化** | - | **約100倍** | **約90%削減** |

*セッションコマンド: 0.004秒（180倍高速！）*

## 🔥 AI開発者の声

> **「な、なんだこれは！！まさに今日完成したばかりの最終兵器じゃないか！！」**  
> — NekoCode C++を発見したClaude Code

> **「Python版の10-100倍高速...これでnyamesh_v23の簡素化が科学的にできる！」**  
> — リファクタリングセッション中のAI開発者

> **「感覚的な複雑さじゃなく定量的な複雑度で判断できる！」**  
> — 300以上のIntentを解析中のClaude Code

### 実際のインパクト
- **37ファイル、10,822行**を数秒で解析
- 7-Coreアーキテクチャの**循環依存**を即座に検出
- リファクタリング時間を**数時間から数分**に短縮
- **新事例**: [nyamesh_v23ケーススタディ](examples/real_world_analysis.md) - 99ファイル、63K行を解析！
- **実況**: [6Core簡素化](examples/6core_simplification.md) - リアルタイムで88-89%の複雑度削減を実現！
- [AI開発者の実際の様子 →](examples/ai_excitement_demo.md)

## 📦 ビルド手順

```bash
mkdir build
cd build
cmake ..
make -j
```

## 🎯 使い方

### 基本的な解析

```bash
# 単一ファイルの解析
./nekocode_ai src/main.cpp

# ディレクトリの解析
./nekocode_ai src/
```

### インタラクティブモード

```bash
# セッションを作成
./nekocode_ai session-create /path/to/project
# 出力: session_id: ai_session_20250727_123456

# コマンドを実行
./nekocode_ai session-cmd ai_session_20250727_123456 stats
./nekocode_ai session-cmd ai_session_20250727_123456 complexity
./nekocode_ai session-cmd ai_session_20250727_123456 "find manager"

# 新機能: テンプレート＆マクロ解析
./nekocode_ai session-cmd ai_session_20250727_123456 template-analysis
./nekocode_ai session-cmd ai_session_20250727_123456 macro-analysis
```

### インクルード依存関係解析（C++専用）

```bash
# 依存関係グラフを表示
./nekocode_ai session-cmd <session_id> include-graph

# 循環依存を検出
./nekocode_ai session-cmd <session_id> include-cycles

# 不要なincludeを検出
./nekocode_ai session-cmd <session_id> include-unused

# 最適化提案
./nekocode_ai session-cmd <session_id> include-optimize
```

### テンプレート＆マクロ解析（C++専用）

```bash
# テンプレート特殊化検出
./nekocode_ai session-cmd <session_id> template-analysis

# マクロ展開追跡
./nekocode_ai session-cmd <session_id> macro-analysis

# メタプログラミングパターン検出
./nekocode_ai session-cmd <session_id> metaprogramming

# コンパイル時計算最適化提案
./nekocode_ai session-cmd <session_id> compile-time-optimization
```

## 📋 利用可能なコマンド

| コマンド | 説明 |
|---------|------|
| `stats` | プロジェクト統計の概要 |
| `files` | ファイル一覧と詳細情報 |
| `complexity` | 複雑度ランキング |
| `structure` | クラス/関数構造解析 |
| `calls` | 関数呼び出し統計 |
| `find <term>` | ファイル名検索 |
| `include-graph` | インクルード依存関係グラフ |
| `include-cycles` | 循環依存検出 |
| `include-impact` | 変更影響範囲分析 |
| `include-unused` | 不要include検出 |
| `include-optimize` | 最適化提案 |
| `template-analysis` | テンプレート特殊化検出 |
| `macro-analysis` | マクロ展開追跡 |
| `metaprogramming` | メタプログラミングパターン検出 |
| `compile-time-optimization` | コンパイル時計算最適化提案 |

## 🔧 設定オプション

```bash
--compact           # コンパクトJSON出力
--stats-only        # 統計情報のみ（高速）
--no-parallel       # 並列処理無効化
--threads <N>       # スレッド数指定
--performance       # パフォーマンス統計表示
--lang <language>   # 言語指定 (auto|js|ts|cpp|c)
```

## 📊 パフォーマンス

- **初回解析**: プロジェクトサイズに依存（例：98ファイルで0.726秒）
- **セッションコマンド**: 0.004秒（180倍高速！）
- **メモリ効率**: Python版より大幅に削減

## 🛠️ 技術スタック

- C++17
- nlohmann/json（JSON処理）
- UTF8-CPP（Unicode対応）
- Tree-sitter（AST解析基盤）
- 正規表現エンジン（高速パターンマッチング）
- Tarjan's Algorithm（循環依存検出）

## 🌟 革命に参加しよう

NekoCodeは世界中のAI開発者に急速に選ばれています。成長するコミュニティに参加しましょう！

- ⭐ **このリポジトリにスター**をつけてサポートを示そう
- 🔔 **Watch**して更新情報や新機能を受け取ろう
- 🐛 **問題を報告**して改善に協力しよう
- 🚀 **解析結果をシェア**しよう

## 📝 ライセンス

このプロジェクトはMITライセンスの下でライセンスされています - 詳細は[LICENSE](LICENSE)ファイルを参照してください。

## 👤 作者

**Moe Charm**
- GitHub: [@moe-charm](https://github.com/moe-charm)
- プロジェクト: [github.com/moe-charm/nekocode](https://github.com/moe-charm/nekocode)
- Twitter: [@CharmNexusCore](https://x.com/CharmNexusCore)
- サポート: [☕ コーヒーを奢る](https://coff.ee/moecharmde6)

---

## 📚 ドキュメント

- [詳しい使い方ガイド（日本語）](docs/USAGE_jp.md)
- [使用例集](examples/README.md)
- [貢献ガイド](CONTRIBUTING.md)

---

**NekoCodeチームによって🐱で作られました**

*「革新的なコード解析を、光速で提供！」*

*「AI開発者に『な、なんだこれは！！』と言わせたツール」* 🔥