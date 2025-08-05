#!/usr/bin/env python3
"""
🐱 NekoCode MCP セットアップ - 超シンプル版
"""
import os

# 現在のディレクトリ（bin/）を取得
current_dir = os.path.dirname(os.path.abspath(__file__))
nekocode_path = os.path.join(current_dir, "nekocode_ai")
mcp_server_path = os.path.join(current_dir, "..", "mcp-nekocode-server", "mcp_server_real.py")

# 絶対パスに変換
nekocode_abs = os.path.abspath(nekocode_path)
mcp_server_abs = os.path.abspath(mcp_server_path)

print(f"""
🐱 NekoCode MCP セットアップ
==============================

⚠️ 重要: 以下のコマンドは【プロジェクトのルートディレクトリ】で実行してください！
        （Claude Codeがプロジェクトを認識するため）

1. まずプロジェクトルートに移動:
   cd {os.path.dirname(current_dir)}

2. そこで以下のコマンドを実行:

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
""")