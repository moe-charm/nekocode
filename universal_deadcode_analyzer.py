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
        """C++ LTO解析"""
        # ツール確認
        if not shutil.which("g++"):
            return {
                "status": "tool_missing",
                "message": "Install g++ for C++ dead code analysis: sudo apt install gcc g++",
                "tool": "LTO (g++)"
            }
        
        print("  🔧 Running LTO analysis...")
        # 実際のLTO処理（既存のnekocode_lto_analyzer.pyから）
        cmd = [
            "g++", "-flto", "-ffunction-sections", "-fdata-sections",
            filepath, "-o", "temp_analysis", "-Wl,--gc-sections", "-Wl,--print-gc-sections"
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            # LTO結果解析
            unused_functions = []
            for line in result.stderr.split('\n'):
                if "removing unused section" in line and ".text." in line:
                    # 関数名抽出
                    import re
                    match = re.search(r"'\\.text\\.([^']+)'", line)
                    if match:
                        unused_functions.append(match.group(1))
            
            return {
                "status": "success", 
                "tool": "LTO",
                "unused_items": [f"unused function '{func}'" for func in unused_functions],
                "total_found": len(unused_functions)
            }
        except Exception as e:
            return {
                "status": "error", 
                "message": f"LTO analysis failed: {e}",
                "tool": "LTO (g++)"
            }
        finally:
            Path("temp_analysis").unlink(missing_ok=True)
    
    def _analyze_python_deadcode(self, filepath):
        """Python Vulture解析"""
        # ツール確認
        if not shutil.which("vulture"):
            return {
                "status": "tool_missing",
                "message": "Install vulture for Python dead code analysis: pip install vulture",
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
                "message": "Install staticcheck: go install honnef.co/go/tools/cmd/staticcheck@latest",
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
                "message": "Install Rust: https://rustup.rs/",
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
                            code = msg['message']['code']['code']
                            if 'unused' in code or 'dead_code' in code:
                                unused_items.append(msg['message']['message'])
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
                "message": "Install .NET SDK: https://dotnet.microsoft.com/download",
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
        if not shutil.which("npx"):
            return {
                "status": "tool_missing",
                "message": "Install Node.js and ts-prune: npm install -g ts-prune",
                "tool": "ts-prune"
            }
        
        print("  📜 Running ts-prune...")
        try:
            result = subprocess.run(
                ["npx", "ts-prune", "--project", "."],
                capture_output=True, text=True
            )
            
            unused_exports = result.stdout.strip().split('\n') if result.stdout.strip() else []
            
            return {
                "status": "success",
                "tool": "ts-prune",
                "unused_exports": unused_exports,
                "total_found": len(unused_exports)
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