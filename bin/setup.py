#!/usr/bin/env python3
"""
🐱 NekoCode MCP セットアップ - 超シンプル版
"""
import os

# 現在のディレクトリ（bin/）を取得
current_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(current_dir)
nekocode_path = os.path.join(current_dir, "nekocode_ai")
mcp_server_path = os.path.join(current_dir, "..", "mcp-nekocode-server", "mcp_server_real.py")

# 絶対パスに変換
nekocode_abs = os.path.abspath(nekocode_path)
mcp_server_abs = os.path.abspath(mcp_server_path)

print(f"""
🐱 NekoCode MCP セットアップ
==============================

⚠️ 重要: 以下のコマンドは【あなたが解析したいプロジェクトのルート】で実行してください！
        （Claude CodeがそのプロジェクトをNekoCodeで解析するため）

1. あなたのプロジェクトに移動:
   cd ~/your-awesome-project   # ← あなたが開発中のプロジェクト
   
   例: cd ~/my-react-app
       cd ~/rust-project  
       cd ~/python-app
   
   ※NekoCodeのディレクトリではありません！

2. そのプロジェクトのルートで以下を実行:

claude mcp add nekocode \\
  -e NEKOCODE_BINARY_PATH={nekocode_abs} \\
  -- python3 {mcp_server_abs}

または、手動で設定ファイルに追加：
~/.config/claude-desktop/config.json (Linux)
~/Library/Application Support/Claude/claude_desktop_config.json (Mac)

{{
  "mcpServers": {{
    "nekocode": {{
      "command": "python3",
      "args": ["{mcp_server_abs}"],
      "env": {{
        "NEKOCODE_BINARY_PATH": "{nekocode_abs}"
      }}
    }}
  }}
}}

設定後、Claude Codeを再起動してください！

========================================
📝 まとめ:
1. NekoCodeをインストール済み ✓
2. あなたのプロジェクトフォルダに移動
3. そこでclaude mcp addを実行
4. Claude Code再起動
5. そのプロジェクトでNekoCodeが使える！
========================================
""")