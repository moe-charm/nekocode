# 🐱 NekoCode MCP Server

**多言語コード解析ツールのMCP統合版** - Claude Codeで便利に使えます！

## 🚀 特徴

- **🎮 セッション機能**: 一度解析すれば、その後の操作は超高速（3ms）！
- **高速解析**: 効率的なコード解析エンジン
- **C++特化機能**: 循環依存検出、依存グラフ、最適化提案
- **多言語対応**: JavaScript, TypeScript, C++, C, Python, C#
- **日本語対応**: 日本語でも利用可能
- **Claude Code統合**: `mcp__nekocode__*` ツールとして利用

## 📦 インストール

### 1. 前提条件
- Python 3.8+
- NekoCode バイナリ (`nekocode_ai`) がビルド済み

### 2. 依存関係確認
```bash
# 標準ライブラリのみ使用 - 特別なインストール不要！
python3 --version  # Python 3.8+ 必要
```

### 3. Claude Code設定
`claude_desktop_config.json` に追加:
```json
{
  "mcpServers": {
    "nekocode": {
      "command": "python3", 
      "args": ["/絶対パス/mcp-nekocode-server/mcp_server_real.py"],
      "env": {
        "NEKOCODE_BINARY_PATH": "/絶対パス/build/nekocode_ai"
      }
    }
  }
}
```

**詳細な設定方法は `TEST_SETUP.md` を参照してください**

## 🛠️ 利用可能なツール

### 🎮 セッション機能（推奨！）
- `mcp__nekocode__session_create` - **📍 最初にこれを使う！** 対話式セッション作成
- `mcp__nekocode__session_stats` - 📊 統計情報（超高速3ms）
- `mcp__nekocode__session_complexity` - 🧮 複雑度分析（超高速3ms）
- `mcp__nekocode__find_files` - 🔎 ファイル検索（超高速3ms）

### C++特化機能（セッション必須）
- `mcp__nekocode__include_cycles` - 🔍 循環依存検出
- `mcp__nekocode__include_graph` - 🌐 依存関係グラフ
- `mcp__nekocode__include_optimize` - ⚡ 最適化提案

### 基本機能
- `mcp__nekocode__analyze` - 🚀 高速プロジェクト解析（単発実行）
- `mcp__nekocode__list_languages` - 🌍 サポート言語一覧

## 🎯 使用例

### 📍 推奨: セッション作成から始める

**NekoCodeの最大の特徴は「セッション機能」です！** 最初にセッションを作成することで、その後の解析が超高速（3ms）で実行できます。

```python  
# 1. 最初に必ずセッション作成（これが肝心！）
session = await mcp__nekocode__session_create("/path/to/project")
# → 初回解析は時間がかかりますが、結果はメモリに保持されます

# 2. その後の操作は超高速（3ms）！
stats = await mcp__nekocode__session_stats(session["session_id"])
complexity = await mcp__nekocode__session_complexity(session["session_id"])
files = await mcp__nekocode__find_files(session["session_id"], "*.ts")

# 3. C++特化機能も高速実行
cycles = await mcp__nekocode__include_cycles(session["session_id"])
graph = await mcp__nekocode__include_graph(session["session_id"])
```

### 基本解析（単発実行）
```python
# セッションを使わない単発解析
result = await mcp__nekocode__analyze("/path/to/project")
# → 毎回フル解析するため遅い

# 高速統計のみ取得（stats_only=True）
result = await mcp__nekocode__analyze("/path/to/project", stats_only=True)
# → 複雑度解析をスキップして高速化
```

**💡 ヒント**: 複数回の操作を行う場合は、必ず最初に `session_create` を使ってください！

## ⚙️ コマンドラインオプション

NekoCode AIが内部で使用する主なオプション：

- `--stats-only` - 高速統計のみ（複雑度解析スキップ）
- `--io-threads <N>` - 並列読み込み数（推奨:16）  
- `--cpu-threads <N>` - 解析スレッド数（デフォルト:CPU数）
- `--progress` - 進捗表示
- `--debug` - 詳細ログ表示
- `--performance` - パフォーマンス統計表示
- `--no-check` - 大規模プロジェクトの事前チェックスキップ
- `--force` - 確認なしで強制実行  
- `--check-only` - サイズチェックのみ（解析しない）

## 🔧 他のツールとの使い分け

NekoCodeは**高速解析**に特化したツールです。

- **NekoCode**: プロジェクト全体の分析、統計取得、C++依存関係解析
- **他のツール**: コード編集、詳細なシンボル検索など

**併用することで最適な開発環境が構築できます**

## 🛣️ ロードマップ

### Phase 1 (完了)
- [x] 基本MCP統合
- [x] 全機能ツール化
- [x] セッション管理

### Phase 2 (予定)
- [ ] 実際のMCPプロトコル実装
- [ ] C#サポート追加
- [ ] コード編集機能

### Phase 3 (予定)  
- [ ] Python, Java, Go, Rust対応
- [ ] セキュリティ解析
- [ ] AI最適化機能

## 🐱 開発情報

- **プロジェクト**: NekoCode C++
- **作者**: NyaCore Team
- **ライセンス**: MIT
- **目標**: 使いやすく高機能な解析ツール

---
**🚀 多言語解析エンジン - 高速で便利なコード分析ツール!**