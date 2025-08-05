#!/usr/bin/env python3
"""
Universal Dead Code Analyzer
全言語対応・外部ツール戦略
"""

import subprocess
import json
import sys
import shutil
from pathlib import Path

class UniversalDeadCodeAnalyzer:
    def __init__(self):
        self.nekocode_bin = "./bin/nekocode_ai"
        
    def analyze(self, filepath, complete=False):
        """統一インターフェース"""
        result = {
            "analysis_mode": "complete" if complete else "normal",
            "target": filepath
        }
        
        # Step 1: NekoCode構造解析（必須）
        print("📊 Structure Analysis...")
        result["structure"] = self._run_nekocode(filepath)
        
        # Step 2: 完全解析（オプション）
        if complete:
            language = self._detect_language(filepath)
            print(f"🔍 Complete Analysis ({language})...")
            result["dead_code"] = self._analyze_deadcode(filepath, language)
        
        return result
    
    def _run_nekocode(self, filepath):
        """NekoCode実行（外部コマンド）"""
        try:
            result = subprocess.run(
                [self.nekocode_bin, "analyze", filepath, "--io-threads", "8"],
                capture_output=True, text=True
            )
            return json.loads(result.stdout)
        except Exception as e:
            return {"error": f"NekoCode failed: {e}"}
    
    def _detect_language(self, filepath):
        """言語判定"""
        suffix = Path(filepath).suffix.lower()
        lang_map = {
            '.cpp': 'cpp', '.cc': 'cpp', '.cxx': 'cpp', '.hpp': 'cpp',
            '.py': 'python',
            '.go': 'go',
            '.cs': 'csharp',
            '.rs': 'rust',
            '.js': 'javascript', '.ts': 'typescript'
        }
        return lang_map.get(suffix, 'unknown')
    
    def _analyze_deadcode(self, filepath, language):
        """言語別デッドコード解析"""
        analyzers = {
            'cpp': self._analyze_cpp_deadcode,
            'python': self._analyze_python_deadcode,
            'go': self._analyze_go_deadcode,
            'csharp': self._analyze_csharp_deadcode,
            'rust': self._analyze_rust_deadcode,
            'javascript': self._analyze_js_deadcode,
            'typescript': self._analyze_js_deadcode
        }
        
        if language in analyzers:
            return analyzers[language](filepath)
        else:
            return {
                "status": "unsupported",
                "message": f"Dead code analysis not available for {language}"
            }
    
    def _analyze_cpp_deadcode(self, filepath):
        """C++ デッドコード解析 - 単体ファイルとフォルダで戦略を切り替え"""
        path = Path(filepath)
        
        # フォルダかファイルかで判別
        if path.is_dir():
            # フォルダの場合はLTO解析
            return self._analyze_cpp_lto(path)
        else:
            # 単体ファイルの場合はclang-tidy
            return self._analyze_cpp_clang_tidy(filepath)
    
    def _analyze_cpp_clang_tidy(self, filepath):
        """C++ 単体ファイル解析 - clang-tidyとコンパイラ警告の併用"""
        # まずclang-tidyを試す
        if shutil.which("clang-tidy"):
            print("  🔧 Running clang-tidy analysis...")
            try:
                cmd = [
                    "clang-tidy",
                    "-checks=-*,misc-unused-*,readability-redundant-*,bugprone-*",
                    filepath,
                    "--",
                    "-std=c++17"
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True)
                unused_items = []
                
                # 結果をパース
                for line in result.stdout.split('\n'):
                    if "warning:" in line and ("unused" in line or "redundant" in line):
                        parts = line.split(":", 4)
                        if len(parts) >= 5:
                            location = f"{parts[0]}:{parts[1]}"
                            message = parts[4].strip()
                            unused_items.append(f"{message} [{location}]")
                
                if unused_items:
                    return {
                        "status": "success",
                        "tool": "clang-tidy",
                        "unused_items": unused_items,
                        "total_found": len(unused_items)
                    }
            except:
                pass
        
        # clang-tidyで結果が得られない場合、コンパイラ警告を使用
        if shutil.which("clang++"):
            print("  🔧 Using compiler warnings for analysis...")
            try:
                # staticな関数の未使用を検出
                cmd = [
                    "clang++",
                    "-std=c++17",
                    "-Wunused-function",
                    "-Wunused-variable",
                    "-Wunused-member-function",
                    "-c",
                    filepath,
                    "-o", "/dev/null"
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True, cwd=Path(filepath).parent)
                unused_items = []
                
                # 警告をパース
                for line in result.stderr.split('\n'):
                    if "warning:" in line and "unused" in line:
                        # test_dead_code.cpp:12:6: warning: unused function 'unused_function'
                        import re
                        match = re.search(r'([^:]+):(\d+):\d+: warning: (.+)', line)
                        if match:
                            filename = Path(match.group(1)).name
                            line_num = match.group(2)
                            message = match.group(3)
                            unused_items.append(f"{message} [{filename}:{line_num}]")
                
                return {
                    "status": "success",
                    "tool": "clang++ compiler",
                    "unused_items": unused_items,
                    "total_found": len(unused_items)
                }
            except Exception as e:
                return {
                    "status": "error",
                    "message": f"Compiler analysis failed: {e}",
                    "tool": "clang++"
                }
        
        # どちらのツールも利用できない場合
        return {
            "status": "tool_missing",
            "message": "⚠️ C++解析ツールがインストールされていません。\n" +
                     "以下のいずれかをインストールしてください:\n" +
                     "- clang-tidy: sudo apt install clang-tidy\n" +
                     "- clang++: sudo apt install clang",
            "tool": "clang-tidy/clang++"
        }
    
    def _analyze_cpp_lto(self, project_dir):
        """C++ プロジェクト全体解析 - LTO使用"""
        if not shutil.which("g++"):
            return {
                "status": "tool_missing",
                "message": "⚠️ g++がインストールされていません。\n" +
                         "インストール方法: sudo apt install gcc g++\n" +
                         "LTO解析にはGCC/G++が必要です",
                "tool": "LTO (g++)"
            }
        
        print("  🔧 Running LTO analysis for project...")
        # TODO: プロジェクト全体のLTO解析実装
        # 現在は単純化のため基本的な結果を返す
        return {
            "status": "not_implemented",
            "message": "Project-wide LTO analysis is under development",
            "tool": "LTO"
        }
    
    def _analyze_python_deadcode(self, filepath):
        """Python Vulture解析"""
        # ツール確認
        if not shutil.which("vulture"):
            return {
                "status": "tool_missing",
                "message": "⚠️ vultureがインストールされていません。\nインストール方法: pip install vulture",
                "tool": "Vulture"
            }
        
        print("  🐍 Running Vulture analysis...")
        try:
            result = subprocess.run(
                ["vulture", filepath, "--min-confidence", "60"],
                capture_output=True, text=True
            )
            
            # Vulture出力解析（確信度をクリーンに表示、位置情報保持）
            unused_items = []
            for line in result.stdout.split('\n'):  # Vultureはstdoutに出力
                if line.strip() and ':' in line:
                    # "test.py:6: unused import 'os' (90% confidence)"
                    # → "unused import 'os' (90%) [test.py:6]"
                    parts = line.split(': ', 1)
                    if len(parts) >= 2:
                        location = parts[0]  # "test.py:6"
                        message = parts[1].replace("% confidence)", "%)")  # "unused import 'os' (90%)"
                        # 位置情報を追加
                        clean_item = f"{message} [{location}]"
                        unused_items.append(clean_item)
            
            return {
                "status": "success",
                "tool": "Vulture",
                "unused_items": unused_items,
                "total_found": len(unused_items)
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f"Vulture analysis failed: {e}",
                "tool": "Vulture"
            }
    
    def _analyze_go_deadcode(self, filepath):
        """Go staticcheck解析"""
        # ツール確認（フルパスも試す）
        staticcheck_cmd = None
        if shutil.which("staticcheck"):
            staticcheck_cmd = "staticcheck"
        elif Path("~/go/bin/staticcheck").expanduser().exists():
            staticcheck_cmd = str(Path("~/go/bin/staticcheck").expanduser())
        else:
            return {
                "status": "tool_missing", 
                "message": "⚠️ staticcheckがインストールされていません。\n" +
                         "インストール方法: go install honnef.co/go/tools/cmd/staticcheck@latest",
                "tool": "staticcheck"
            }
        
        print("  🐹 Running staticcheck...")
        try:
            # GOPATHを設定してstaticcheck実行
            import os
            env = os.environ.copy()
            env["PATH"] = env.get("PATH", "") + ":/usr/local/go/bin"
            
            result = subprocess.run(
                [staticcheck_cmd, "-checks=U1000,U1001", filepath],
                capture_output=True, text=True, env=env
            )
            
            # staticcheckの出力解析（stdoutに出力される）  
            unused_items = []
            for line in result.stdout.split('\n'):
                if line.strip() and ':' in line and 'is unused' in line:
                    # "test.go:15:6: func unusedFunction is unused (U1000)"
                    # → "func unusedFunction is unused (100%) [test.go:15]"
                    parts = line.split(': ', 1)
                    if len(parts) >= 2:
                        location_part = parts[0]  # "test.go:15:6"
                        message_part = parts[1]   # "func unusedFunction is unused (U1000)"
                        
                        # 位置情報から列番号を除去
                        location_parts = location_part.split(':')
                        if len(location_parts) >= 2:
                            file_line = f"{location_parts[0]}:{location_parts[1]}"
                            
                            # メッセージから診断コードを除去（確信度は書かない）
                            message_clean = message_part.split(' (U')[0]  # "(U1000)"を除去
                            clean_item = f"{message_clean} [{file_line}]"
                            unused_items.append(clean_item)
            
            return {
                "status": "success",
                "tool": "staticcheck",
                "unused_items": unused_items,
                "total_found": len(unused_items)
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f"staticcheck failed: {e}",
                "tool": "staticcheck"
            }
    
    def _analyze_rust_deadcode(self, filepath):
        """Rust cargo解析"""
        if not shutil.which("cargo"):
            return {
                "status": "tool_missing",
                "message": "⚠️ Rust/Cargoがインストールされていません。\n" +
                         "インストール方法: https://rustup.rs/",
                "tool": "cargo"
            }
        
        print("  🦀 Running cargo check...")
        # Rustプロジェクトのルートを探す
        current_dir = Path(filepath).parent
        while current_dir != current_dir.parent:
            if (current_dir / "Cargo.toml").exists():
                break
            current_dir = current_dir.parent
        else:
            return {
                "status": "error",
                "message": "Cargo.toml not found. Run in Rust project directory.",
                "tool": "cargo"
            }
        
        try:
            result = subprocess.run(
                ["cargo", "check", "--message-format=json"],
                cwd=current_dir, capture_output=True, text=True
            )
            
            unused_items = []
            for line in result.stdout.split('\n'):
                if line.strip():
                    try:
                        msg = json.loads(line)
                        if 'message' in msg and 'code' in msg['message']:
                            code = msg['message']['code'].get('code', '')
                            if 'unused' in code or 'dead_code' in code:
                                message_text = msg['message']['message']
                                # 位置情報を取得
                                if 'spans' in msg['message'] and msg['message']['spans']:
                                    span = msg['message']['spans'][0]
                                    file_name = Path(span['file_name']).name
                                    line_num = span['line_start']
                                    unused_items.append(f"{message_text} [{file_name}:{line_num}]")
                                else:
                                    unused_items.append(message_text)
                    except:
                        continue
            
            return {
                "status": "success",
                "tool": "cargo",
                "unused_items": unused_items,
                "total_found": len(unused_items)
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f"cargo check failed: {e}",
                "tool": "cargo"
            }
    
    def _analyze_csharp_deadcode(self, filepath):
        """C# .NET SDK アナライザー解析"""
        # ツール確認
        if not shutil.which("dotnet"):
            return {
                "status": "tool_missing",
                "message": "⚠️ .NET SDKがインストールされていません。\n" +
                         "インストール方法: https://dotnet.microsoft.com/download",
                "tool": ".NET SDK"
            }
        
        print("  🔷 Running .NET analyzers...")
        
        # C#プロジェクトディレクトリを探す
        current_dir = Path(filepath).parent
        project_file = None
        
        # 同一ディレクトリ内の.csprojファイルを探す
        for csproj in current_dir.glob("*.csproj"):
            project_file = csproj
            break
        
        if not project_file:
            return {
                "status": "error", 
                "message": "No .csproj file found. Run in C# project directory.",
                "tool": ".NET SDK"
            }
        
        try:
            # クリーンビルドで警告を確実に表示
            subprocess.run(["dotnet", "clean"], cwd=current_dir, capture_output=True)
            result = subprocess.run(
                ["dotnet", "build", "--verbosity", "normal"],
                cwd=current_dir, capture_output=True, text=True
            )
            
            # 警告を解析
            import re
            unused_items = []
            seen_warnings = set()  # 重複除去用
            
            for line in result.stdout.split('\n'):
                if 'warning CS' in line and '.cs(' in line:
                    # "/path/file.cs(line,col): warning CSxxxx: message [project]"
                    match = re.search(r'([^/]+\.cs)\((\d+),(\d+)\): warning (CS\d+): (.+?) \[', line)
                    if match:
                        file_name, line_num, col_num, code, message = match.groups()
                        location = f"{file_name}:{line_num}"
                        
                        # CS0414: フィールドが未使用
                        # CS0169: 変数が未使用  
                        # CS8019: using が未使用
                        if code in ['CS0414', 'CS0169', 'CS8019']:
                            clean_item = f"{message} [{location}]"
                            # 重複チェック
                            warning_key = f"{code}:{location}"
                            if warning_key not in seen_warnings:
                                unused_items.append(clean_item)
                                seen_warnings.add(warning_key)
            
            return {
                "status": "success",
                "tool": ".NET SDK",
                "unused_items": unused_items,
                "total_found": len(unused_items)
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f".NET build failed: {e}",
                "tool": ".NET SDK"
            }
    
    def _analyze_js_deadcode(self, filepath):
        """JavaScript/TypeScript ts-prune解析"""
        # Node.jsのnpxコマンド確認
        if not shutil.which("npx"):
            return {
                "status": "tool_missing",
                "message": "⚠️ Node.jsがインストールされていません。\n" +
                         "インストール方法: https://nodejs.org/\n" +
                         "ts-pruneインストール: npm install -g ts-prune",
                "tool": "ts-prune"
            }
        
        print("  📜 Running ts-prune...")
        
        # TypeScript/JavaScriptプロジェクトのルートを探す
        path = Path(filepath)
        current_dir = path if path.is_dir() else path.parent
        
        # tsconfig.jsonまたはpackage.jsonを探す
        project_root = None
        while current_dir != current_dir.parent:
            if (current_dir / "tsconfig.json").exists() or (current_dir / "package.json").exists():
                project_root = current_dir
                break
            current_dir = current_dir.parent
        
        if not project_root:
            return {
                "status": "error",
                "message": "tsconfig.json or package.json not found. Run in TypeScript/JavaScript project directory.",
                "tool": "ts-prune"
            }
        
        try:
            # ts-pruneを実行
            result = subprocess.run(
                ["npx", "ts-prune"],
                cwd=project_root,
                capture_output=True, 
                text=True
            )
            
            # ts-pruneの出力を解析
            unused_items = []
            for line in result.stdout.strip().split('\n'):
                if line.strip() and not line.startswith('ts-prune'):
                    # ts-prune出力例: "src/utils.ts:10 - myFunction"
                    # → "myFunction [src/utils.ts:10]"
                    if ' - ' in line:
                        location, item = line.split(' - ', 1)
                        unused_items.append(f"{item.strip()} [{location.strip()}]")
                    else:
                        unused_items.append(line.strip())
            
            return {
                "status": "success",
                "tool": "ts-prune",
                "unused_items": unused_items,
                "total_found": len(unused_items)
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f"ts-prune failed: {e}",
                "tool": "ts-prune"
            }


def main():
    """統一インターフェース"""
    if len(sys.argv) < 2:
        print("Usage: python3 universal_deadcode_analyzer.py <file> [--complete]")
        sys.exit(1)
    
    filepath = sys.argv[1]
    complete = "--complete" in sys.argv
    
    analyzer = UniversalDeadCodeAnalyzer()
    result = analyzer.analyze(filepath, complete)
    
    # 結果表示
    print("\n" + "="*60)
    print("📋 Analysis Results")
    print("="*60)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()