#!/usr/bin/env python3
"""
🐱 NekoCode MCP Server - 実際のMCP実装版

実際のMCPプロトコル（stdio + JSON-RPC）で実装
"""

import asyncio
import json
import sys
import subprocess
import os
from typing import Dict, List, Any, Optional
import logging

# ログ設定 (stderrに出力、stdioと混同しないように)
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    stream=sys.stderr
)
logger = logging.getLogger(__name__)


class NekoCodeMCPServer:
    """実際のMCPプロトコル実装"""
    
    def __init__(self):
        self.nekocode_path = self._find_nekocode_binary()
        self.sessions = {}
        self.tools = self._define_tools()
    
    def _find_nekocode_binary(self) -> str:
        """nekocode_ai バイナリの場所を特定"""
        # 環境変数から取得を優先
        env_path = os.environ.get('NEKOCODE_BINARY_PATH')
        if env_path and os.path.exists(env_path):
            return os.path.abspath(env_path)
        
        possible_paths = [
            "./bin/nekocode_ai",
            "../bin/nekocode_ai",
            "./build/nekocode_ai",
            "../build/nekocode_ai", 
            "/usr/local/bin/nekocode_ai",
            "nekocode_ai"
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                return os.path.abspath(path)
        
        # PATHから検索
        import shutil
        binary = shutil.which("nekocode_ai")
        if binary:
            return binary
        
        # デフォルト（エラーは実行時に出す）
        return "./bin/nekocode_ai"
    
    def _define_tools(self) -> List[Dict]:
        """利用可能なツール定義"""
        return [
            {
                "name": "analyze",
                "description": "🚀 高速プロジェクト解析",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string", "description": "解析対象パス"},
                        "language": {"type": "string", "description": "言語指定", "default": "auto"},
                        "stats_only": {"type": "boolean", "description": "統計のみ", "default": False}
                    },
                    "required": ["path"]
                }
            },
            {
                "name": "session_create",
                "description": "🎮 対話式セッション作成",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string", "description": "プロジェクトパス"}
                    },
                    "required": ["path"]
                }
            },
            {
                "name": "session_stats",
                "description": "📊 セッション統計情報",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "session_id": {"type": "string", "description": "セッションID"}
                    },
                    "required": ["session_id"]
                }
            },
            {
                "name": "include_cycles",
                "description": "🔍 C++循環依存検出",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "session_id": {"type": "string", "description": "セッションID"}
                    },
                    "required": ["session_id"]
                }
            },
            {
                "name": "include_graph",
                "description": "🌐 C++依存関係グラフ",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "session_id": {"type": "string", "description": "セッションID"}
                    },
                    "required": ["session_id"]
                }
            },
            {
                "name": "list_languages",
                "description": "🌍 サポート言語一覧",
                "inputSchema": {"type": "object", "properties": {}}
            },
            {
                "name": "replace_preview",
                "description": "📝 置換プレビュー（セッション不要・直接実行）",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "file_path": {"type": "string", "description": "ファイルパス"},
                        "pattern": {"type": "string", "description": "検索パターン"},
                        "replacement": {"type": "string", "description": "置換文字列"}
                    },
                    "required": ["file_path", "pattern", "replacement"]
                }
            },
            {
                "name": "replace_confirm",
                "description": "✅ 置換実行（セッション不要・プレビューID指定）",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "preview_id": {"type": "string", "description": "プレビューID"}
                    },
                    "required": ["preview_id"]
                }
            },
            {
                "name": "insert_preview",
                "description": "📝 挿入プレビュー（セッション不要・start/end/行番号）",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "file_path": {"type": "string", "description": "ファイルパス"},
                        "position": {"type": "string", "description": "挿入位置（start/end/行番号）"},
                        "content": {"type": "string", "description": "挿入内容"}
                    },
                    "required": ["file_path", "position", "content"]
                }
            },
            {
                "name": "insert_confirm",
                "description": "✅ 挿入実行（セッション不要・プレビューID指定）",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "preview_id": {"type": "string", "description": "プレビューID"}
                    },
                    "required": ["preview_id"]
                }
            },
            {
                "name": "edit_history",
                "description": "📋 編集履歴表示（セッション不要・最新20件）",
                "inputSchema": {
                    "type": "object",
                    "properties": {}
                }
            },
            {
                "name": "edit_show",
                "description": "🔍 編集詳細表示（ID指定）",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "session_id": {"type": "string", "description": "セッションID"},
                        "edit_id": {"type": "string", "description": "編集ID"}
                    },
                    "required": ["session_id", "edit_id"]
                }
            }
        ]
    
    async def _run_nekocode(self, args: List[str]) -> Dict:
        """NekoCode実行"""
        try:
            cmd = [self.nekocode_path] + args
            logger.info(f"Executing: {' '.join(cmd)}")
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            
            # --helpなどは0以外のreturn codeでも正常
            if result.returncode != 0 and "--help" not in args:
                return {"error": f"NekoCode実行エラー: {result.stderr}"}
            
            # stderrに出力される場合もある（helpなど）
            output = result.stdout if result.stdout.strip() else result.stderr
            
            try:
                return json.loads(output)
            except json.JSONDecodeError:
                return {"output": output, "raw": True}
                
        except subprocess.TimeoutExpired:
            return {"error": "実行がタイムアウトしました"}
        except FileNotFoundError:
            return {"error": f"NekoCodeバイナリが見つかりません: {self.nekocode_path}"}
        except Exception as e:
            return {"error": f"予期しないエラー: {str(e)}"}
    
    # ========================================
    # MCPプロトコル実装
    # ========================================
    
    async def handle_initialize(self, params: Dict) -> Dict:
        """初期化ハンドラ"""
        logger.info("MCP Server initializing...")
        return {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {"listChanged": False},
                "resources": {"subscribe": False, "listChanged": False}
            },
            "serverInfo": {
                "name": "nekocode",
                "version": "1.0.0"
            }
        }
    
    async def handle_tools_list(self, params: Dict) -> Dict:
        """ツール一覧ハンドラ"""
        return {"tools": self.tools}
    
    async def handle_resources_list(self, params: Dict) -> Dict:
        """リソース一覧ハンドラ"""
        readme_path = os.path.join(os.path.dirname(__file__), "README.md")
        
        resources = []
        if os.path.exists(readme_path):
            resources.append({
                "uri": "nekocode://readme",
                "name": "NekoCode MCP Server README",
                "description": "🐱 NekoCodeの使い方ガイド - セッション機能を活用した高速解析",
                "mimeType": "text/markdown"
            })
        
        return {"resources": resources}
    
    async def handle_resources_read(self, params: Dict) -> Dict:
        """リソース読み取りハンドラ"""
        uri = params.get("uri", "")
        
        if uri == "nekocode://readme":
            readme_path = os.path.join(os.path.dirname(__file__), "README.md")
            if os.path.exists(readme_path):
                with open(readme_path, "r", encoding="utf-8") as f:
                    contents = f.read()
                
                return {
                    "contents": [{
                        "uri": uri,
                        "mimeType": "text/markdown",
                        "text": contents
                    }]
                }
        
        return {"error": f"Resource not found: {uri}"}
    
    async def handle_tools_call(self, params: Dict) -> Dict:
        """ツール実行ハンドラ"""
        tool_name = params.get("name")
        arguments = params.get("arguments", {})
        
        logger.info(f"Tool call: {tool_name} with args: {arguments}")
        
        try:
            if tool_name == "analyze":
                return await self._tool_analyze(arguments)
            elif tool_name == "session_create":
                return await self._tool_session_create(arguments)
            elif tool_name == "session_stats":
                return await self._tool_session_stats(arguments)
            elif tool_name == "include_cycles":
                return await self._tool_include_cycles(arguments)
            elif tool_name == "include_graph":
                return await self._tool_include_graph(arguments)
            elif tool_name == "list_languages":
                return await self._tool_list_languages(arguments)
            elif tool_name == "replace_preview":
                return await self._tool_replace_preview(arguments)
            elif tool_name == "replace_confirm":
                return await self._tool_replace_confirm(arguments)
            elif tool_name == "insert_preview":
                return await self._tool_insert_preview(arguments)
            elif tool_name == "insert_confirm":
                return await self._tool_insert_confirm(arguments)
            elif tool_name == "edit_history":
                return await self._tool_edit_history(arguments)
            elif tool_name == "edit_show":
                return await self._tool_edit_show(arguments)
            else:
                return {
                    "content": [{"type": "text", "text": f"Unknown tool: {tool_name}"}],
                    "isError": True
                }
        except Exception as e:
            logger.error(f"Tool execution error: {e}")
            return {
                "content": [{"type": "text", "text": f"エラー: {str(e)}"}],
                "isError": True
            }
    
    # ========================================
    # ツール実装
    # ========================================
    
    async def _tool_analyze(self, args: Dict) -> Dict:
        """プロジェクト解析"""
        path = args["path"]
        language = args.get("language", "auto")
        stats_only = args.get("stats_only", False)
        
        cmd_args = ["analyze", path]
        if language != "auto":
            cmd_args.extend(["--lang", language])
        if stats_only:
            cmd_args.append("--stats-only")
        cmd_args.extend(["--io-threads", "8"])
        
        result = await self._run_nekocode(cmd_args)
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_session_create(self, args: Dict) -> Dict:
        """セッション作成"""
        path = args["path"]
        result = await self._run_nekocode(["session-create", path])
        
        if "session_id" in result:
            self.sessions[result["session_id"]] = {"path": path}
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_session_stats(self, args: Dict) -> Dict:
        """セッション統計"""
        session_id = args["session_id"]
        
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"セッション {session_id} が見つかりません"}],
                "isError": True
            }
        
        result = await self._run_nekocode(["session-command", session_id, "stats"])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_include_cycles(self, args: Dict) -> Dict:
        """循環依存検出"""
        session_id = args["session_id"]
        
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"セッション {session_id} が見つかりません"}],
                "isError": True
            }
        
        result = await self._run_nekocode(["session-command", session_id, "include-cycles"])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_include_graph(self, args: Dict) -> Dict:
        """依存関係グラフ"""
        session_id = args["session_id"]
        
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"セッション {session_id} が見つかりません"}],
                "isError": True
            }
        
        result = await self._run_nekocode(["session-command", session_id, "include-graph"])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_list_languages(self, args: Dict) -> Dict:
        """言語一覧"""
        # 最新版ではhelpから言語情報を取得
        result = await self._run_nekocode(["--help"])
        
        if "output" in result:
            # LANGUAGES行を抽出
            lines = result["output"].split('\n')
            lang_line = next((line for line in lines if 'LANGUAGES:' in line), "")
            languages = lang_line.replace('LANGUAGES:', '').strip() if lang_line else "JS/TS/C++/C/Python/C#"
            return {"content": [{"type": "text", "text": f"対応言語: {languages}"}]}
        else:
            return {"content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]}
    
    async def _tool_replace_preview(self, args: Dict) -> Dict:
        """置換プレビュー"""
        session_id = args["session_id"]
        file_path = args["file_path"]
        pattern = args["pattern"]
        replacement = args["replacement"]
        
        # セッション存在チェック
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"Session not found: {session_id}"}],
                "isError": True
            }
        
        # コマンド実行（引数を個別に渡す）
        result = await self._run_nekocode(["session-command", session_id, "replace-preview", file_path, pattern, replacement])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result.get("output", result), indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_replace_confirm(self, args: Dict) -> Dict:
        """置換実行"""
        session_id = args["session_id"]
        preview_id = args["preview_id"]
        
        # セッション存在チェック
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"Session not found: {session_id}"}],
                "isError": True
            }
        
        # コマンド実行（引数を個別に渡す）
        result = await self._run_nekocode(["session-command", session_id, "replace-confirm", preview_id])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result.get("output", result), indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_insert_preview(self, args: Dict) -> Dict:
        """挿入プレビュー"""
        session_id = args["session_id"]
        file_path = args["file_path"]
        position = args["position"]
        content = args["content"]
        
        # セッション存在チェック
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"Session not found: {session_id}"}],
                "isError": True
            }
        
        # コマンド実行（引数を個別に渡す）
        result = await self._run_nekocode(["session-command", session_id, "insert-preview", file_path, position, content])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result.get("output", result), indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_insert_confirm(self, args: Dict) -> Dict:
        """挿入実行（直接実行）"""
        preview_id = args["preview_id"]
        
        # 直接コマンド実行（セッション不要）
        result = await self._run_nekocode(["insert-confirm", preview_id])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result.get("output", result), indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_edit_history(self, args: Dict) -> Dict:
        """編集履歴表示（セッション不要）"""
        # セッション不要でedit-historyディレクトリから直接読み込み
        try:
            import os
            import glob
            
            history_dir = "memory/edit_history"
            if not os.path.exists(history_dir):
                return {
                    "content": [{"type": "text", "text": json.dumps({"history": [], "total_count": 0, "summary": "編集履歴なし"}, indent=2, ensure_ascii=False)}]
                }
            
            # JSONファイルを取得して最新順でソート
            history_files = glob.glob(f"{history_dir}/*.json")
            history_files.sort(key=os.path.getmtime, reverse=True)
            
            history_list = []
            for file_path in history_files[:20]:  # 最新20件
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        history_data = json.load(f)
                        history_list.append(history_data)
                except Exception as e:
                    logger.warning(f"Failed to load history file {file_path}: {e}")
            
            result = {
                "command": "edit-history",
                "total_count": len(history_files),
                "history": history_list,
                "summary": "最新20件の編集履歴"
            }
            
        except Exception as e:
            logger.error(f"Edit history error: {e}")
            result = {"error": f"編集履歴の取得に失敗: {str(e)}"}
        
        return {
            "content": [{"type": "text", "text": json.dumps(result, indent=2, ensure_ascii=False)}]
        }
    
    async def _tool_edit_show(self, args: Dict) -> Dict:
        """編集詳細表示"""
        session_id = args["session_id"]
        edit_id = args["edit_id"]
        
        # セッション存在チェック
        if session_id not in self.sessions:
            return {
                "content": [{"type": "text", "text": f"Session not found: {session_id}"}],
                "isError": True
            }
        
        # コマンド実行（引数を個別に渡す）
        result = await self._run_nekocode(["session-command", session_id, "edit-show", edit_id])
        
        return {
            "content": [{"type": "text", "text": json.dumps(result.get("output", result), indent=2, ensure_ascii=False)}]
        }
    
    # ========================================
    # MCPプロトコル通信
    # ========================================
    
    async def send_message(self, message: Dict):
        """メッセージ送信 (stdout)"""
        json.dump(message, sys.stdout, ensure_ascii=False)
        sys.stdout.write('\n')
        sys.stdout.flush()
    
    async def receive_message(self) -> Optional[Dict]:
        """メッセージ受信 (stdin)"""
        try:
            line = sys.stdin.readline()
            if not line:
                return None
            return json.loads(line.strip())
        except json.JSONDecodeError as e:
            logger.error(f"JSON decode error: {e}")
            return None
        except Exception as e:
            logger.error(f"Message receive error: {e}")
            return None
    
    async def run(self):
        """MCPサーバー実行"""
        logger.info("🐱 NekoCode MCP Server starting...")
        logger.info(f"📂 NekoCode binary: {self.nekocode_path}")
        
        while True:
            try:
                # メッセージ受信
                message = await self.receive_message()
                if message is None:
                    break
                
                method = message.get("method")
                params = message.get("params", {})
                message_id = message.get("id")
                
                logger.info(f"Received: {method}")
                
                # ハンドラ呼び出し
                if method == "initialize":
                    result = await self.handle_initialize(params)
                elif method == "tools/list":
                    result = await self.handle_tools_list(params)
                elif method == "tools/call":
                    result = await self.handle_tools_call(params)
                elif method == "resources/list":
                    result = await self.handle_resources_list(params)
                elif method == "resources/read":
                    result = await self.handle_resources_read(params)
                else:
                    result = {"error": f"Unknown method: {method}"}
                
                # レスポンス送信
                if message_id is not None:
                    response = {
                        "jsonrpc": "2.0",
                        "id": message_id,
                        "result": result
                    }
                    await self.send_message(response)
                
            except KeyboardInterrupt:
                break
            except Exception as e:
                logger.error(f"Main loop error: {e}")
                break
        
        logger.info("🐱 NekoCode MCP Server stopped")


if __name__ == "__main__":
    server = NekoCodeMCPServer()
    asyncio.run(server.run())