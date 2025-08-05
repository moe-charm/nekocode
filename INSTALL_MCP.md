# 🐱 NekoCode MCP Server インストールガイド

## 🚀 3ステップでClaude Codeと連携！

### ステップ1: NekoCodeをビルド
```bash
# プロジェクトをclone
git clone https://github.com/your-org/nekocode-cpp-github.git
cd nekocode-cpp-github

# ビルド (C++17以上が必要)
mkdir -p build && cd build
cmake .. && make -j8

# 動作確認
./nekocode_ai --help
```

### ステップ2: MCPサーバーの準備
```bash
# Python3が必要 (標準ライブラリのみ使用)
cd .. # プロジェクトルートに戻る
chmod +x mcp-nekocode-server/mcp_server_real.py

# 動作テスト
echo '{"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}}' | python3 mcp-nekocode-server/mcp_server_real.py
```

### ステップ3: Claude Code設定

#### 方法A: claude mcp addコマンド（推奨）
**重要**: あなたが解析したいプロジェクトのルートで実行してください！

```bash
# あなたのプロジェクトに移動
cd ~/your-awesome-project  # ← あなたが開発中のプロジェクト

# そこでNekoCodeを追加
claude mcp add nekocode \
  -e NEKOCODE_BINARY_PATH=/絶対パス/nekocode-cpp-github/bin/nekocode_ai \
  -- python3 /絶対パス/nekocode-cpp-github/mcp-nekocode-server/mcp_server_real.py
```

#### 方法B: 手動設定
**Linux**: `~/.config/claude-desktop/config.json`  
**Mac**: `~/Library/Application Support/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "nekocode": {
      "command": "python3",
      "args": ["/絶対パス/nekocode-cpp-github/mcp-nekocode-server/mcp_server_real.py"],
      "env": {
        "NEKOCODE_BINARY_PATH": "/絶対パス/nekocode-cpp-github/bin/nekocode_ai"
      }
    }
  }
}
```

**⚠️ 注意**: パスは絶対パスで指定してください！

## 🎯 完了確認

Claude Codeを再起動して、新しいチャットで：
```
利用可能なツールを教えて
```

以下が表示されれば成功：
- mcp__nekocode__analyze
- mcp__nekocode__session_create
- mcp__nekocode__list_languages
- など6つのツール

## 🆚 Serenaとの違い

| 項目 | Serena | NekoCode |
|------|--------|----------|
| **インストール** | `uv tool install serena` | 3ステップ手動設定 |
| **依存関係** | fastmcp, uvx | Python3標準ライブラリのみ |
| **設定** | 自動？ | 手動パス設定必要 |
| **カスタマイズ** | 困難 | 完全制御可能 |
| **起動速度** | ライブラリ読み込み時間 | 瞬間起動 |

## 🔧 トラブルシューティング

### Q: ツールが見つからない
A: Claude Codeを完全再起動してください

### Q: バイナリが見つからない
```bash
# パスを確認
ls -la /絶対パス/nekocode-cpp-github/bin/nekocode_ai
# 実行権限を確認
chmod +x /絶対パス/nekocode-cpp-github/bin/nekocode_ai
```

### Q: 設定ファイルが見つからない
```bash
# ディレクトリを作成
mkdir -p ~/.config/claude-desktop/
# 設定ファイルを作成
touch ~/.config/claude-desktop/config.json
```

## 💡 上級者向け

### シェルラッパー作成 (Serena風)
```bash
#!/bin/bash
# bin/nekocode-mcp-server
export NEKOCODE_BINARY_PATH="$(dirname "$0")/nekocode_ai"
exec python3 "$(dirname "$0")/../mcp-nekocode-server/mcp_server_real.py" "$@"
```

設定が簡潔に：
```json
{
  "mcpServers": {
    "nekocode": {
      "command": "/絶対パス/nekocode-cpp-github/bin/nekocode-mcp-server"
    }
  }
}
```

---
🐱 NekoCodeで高速解析を体験しよう！