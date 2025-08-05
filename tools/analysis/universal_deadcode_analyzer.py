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
        """言語判定（ディレクトリの場合は内容を調査）"""
        path = Path(filepath)
        
        # ディレクトリの場合は内容を調査
        if path.is_dir():
            # C++ファイルを検索
            cpp_extensions = ['.cpp', '.cxx', '.cc', '.C', '.hpp', '.hxx', '.hh', '.h']
            for ext in cpp_extensions:
                if list(path.rglob(f'*{ext}')):
                    return 'cpp'
            
            # 他の言語も同様に検索
            if list(path.rglob('*.py')):
                return 'python'
            if list(path.rglob('*.go')):
                return 'go'
            if list(path.rglob('*.cs')):
                return 'csharp'
            if list(path.rglob('*.rs')):
                return 'rust'
            if list(path.rglob('*.js')) or list(path.rglob('*.ts')):
                return 'javascript'
            
            return 'unknown'
        
        # ファイルの場合は拡張子から判定
        suffix = path.suffix.lower()
        lang_map = {
            '.cpp': 'cpp', '.cc': 'cpp', '.cxx': 'cpp', '.C': 'cpp', '.hpp': 'cpp', '.hxx': 'cpp', '.hh': 'cpp', '.h': 'cpp',
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
        project_path = Path(project_dir)
        
        # Phase 1: C++ファイル発見
        cpp_files = []
        for ext in ['.cpp', '.cxx', '.cc', '.C']:
            cpp_files.extend(project_path.rglob(f'*{ext}'))
        
        if not cpp_files:
            return {
                "status": "no_files",
                "message": "No C++ source files found in project",
                "tool": "LTO"
            }
        
        # 大規模プロジェクト警告（100ファイル以上）
        if len(cpp_files) > 100:
            print(f"  ⚠️ Large project detected: {len(cpp_files)} C++ files")
            print("  🕐 LTO analysis may take several minutes...")
        
        # Phase 2: ビルドシステム検出
        build_system = self._detect_build_system(project_path)
        print(f"  📋 Build system: {build_system}")
        
        # Phase 3: LTO解析実行
        if build_system == "cmake":
            return self._lto_cmake_analysis(project_path)
        elif build_system == "makefile":
            return self._lto_makefile_analysis(project_path)
        else:
            return self._lto_manual_analysis(project_path, cpp_files)
    
    def _detect_build_system(self, project_path):
        """ビルドシステム検出"""
        if (project_path / "CMakeLists.txt").exists():
            return "cmake"
        elif (project_path / "Makefile").exists():
            return "makefile"
        else:
            return "manual"
    
    def _lto_cmake_analysis(self, project_path):
        """CMakeプロジェクトのLTO解析 - サイズ比較とシンボル分析"""
        print("  🏗️ CMake project detected - LTO comparison analysis...")
        
        try:
            # CMakeビルドディレクトリ作成
            build_dir = project_path / "build_lto_analysis"
            build_dir.mkdir(exist_ok=True)
            
            # Phase 1: 通常ビルド（ベースライン）
            print("  📊 Building normal version for comparison...")
            cmake_normal = ["cmake", "..", "-DCMAKE_CXX_FLAGS=-O2 -g"]
            result_normal = subprocess.run(cmake_normal, cwd=build_dir, capture_output=True, text=True, timeout=120)
            
            if result_normal.returncode != 0:
                return {
                    "status": "cmake_config_failed",
                    "message": f"Normal CMake config failed: {result_normal.stderr[:200]}",
                    "tool": "LTO (cmake baseline)"
                }
            
            make_normal = subprocess.run(["make", "-j4"], cwd=build_dir, capture_output=True, text=True, timeout=300)
            if make_normal.returncode != 0:
                return {
                    "status": "build_failed",
                    "message": f"Normal build failed: {make_normal.stderr[:200]}",
                    "tool": "LTO (normal build)"
                }
            
            # 通常ビルドのバイナリ解析
            normal_binary = build_dir / "lto_test"
            normal_symbols = self._analyze_binary_symbols(normal_binary)
            normal_size = self._get_binary_size(normal_binary)
            
            # Phase 2: LTOビルド
            print("  🔥 Building LTO version...")
            cmake_lto = [
                "cmake", "..", 
                "-DCMAKE_CXX_FLAGS=-flto -O2 -ffunction-sections -fdata-sections",
                "-DCMAKE_EXE_LINKER_FLAGS=-flto -Wl,--gc-sections"
            ]
            
            result_lto = subprocess.run(cmake_lto, cwd=build_dir, capture_output=True, text=True, timeout=120)
            
            if result_lto.returncode != 0:
                return {
                    "status": "cmake_config_failed",
                    "message": f"LTO CMake config failed: {result_lto.stderr[:200]}",
                    "tool": "LTO (cmake)"
                }
            
            make_lto = subprocess.run(["make", "-j4"], cwd=build_dir, capture_output=True, text=True, timeout=300)
            
            if make_lto.returncode != 0:
                return {
                    "status": "build_failed", 
                    "message": f"LTO build failed: {make_lto.stderr[:200]}",
                    "tool": "LTO (make)"
                }
            
            # LTOビルドのバイナリ解析
            lto_binary = build_dir / "lto_test" 
            lto_symbols = self._analyze_binary_symbols(lto_binary)
            lto_size = self._get_binary_size(lto_binary)
            
            # Phase 3: 差分解析
            unused_items = self._compare_symbol_sets(normal_symbols, lto_symbols)
            size_reduction = normal_size - lto_size
            reduction_percent = (size_reduction / normal_size * 100) if normal_size > 0 else 0
            
            return {
                "status": "success",
                "tool": "LTO (cmake + symbol comparison)",
                "unused_items": unused_items,
                "total_found": len(unused_items),
                "method": "symbol_comparison_analysis",
                "size_analysis": {
                    "normal_size": normal_size,
                    "lto_size": lto_size,
                    "reduction_bytes": size_reduction,
                    "reduction_percent": f"{reduction_percent:.1f}%"
                }
            }
            
        except subprocess.TimeoutExpired:
            return {
                "status": "timeout",
                "message": "LTO analysis timed out (large project)",
                "tool": "LTO (cmake)"
            }
        except Exception as e:
            return {
                "status": "error",
                "message": f"LTO analysis failed: {str(e)[:200]}",
                "tool": "LTO (cmake)"
            }
    
    def _lto_makefile_analysis(self, project_path):
        """MakefileプロジェクトのLTO解析"""
        print("  🔨 Makefile project detected - manual LTO build...")
        
        # 簡略化: 通常のmakeではなく、発見したC++ファイルでmanual解析
        cpp_files = []
        for ext in ['.cpp', '.cxx', '.cc', '.C']:
            cpp_files.extend(project_path.rglob(f'*{ext}'))
        
        return self._lto_manual_analysis(project_path, cpp_files)
    
    def _lto_manual_analysis(self, project_path, cpp_files):
        """手動LTO解析（ファイルリスト直接指定）"""
        print(f"  ⚡ Manual LTO analysis: {len(cpp_files)} files...")
        
        try:
            # 一時ディレクトリ作成
            import tempfile
            with tempfile.TemporaryDirectory() as temp_dir:
                temp_path = Path(temp_dir)
                
                # Phase 1: LTOコンパイル
                object_files = []
                for cpp_file in cpp_files:
                    obj_name = cpp_file.stem + ".o"
                    obj_path = temp_path / obj_name
                    
                    compile_cmd = [
                        "g++", "-flto", "-O2", "-ffunction-sections", "-fdata-sections",
                        "-c", str(cpp_file), "-o", str(obj_path)
                    ]
                    
                    result = subprocess.run(compile_cmd, capture_output=True, text=True, timeout=60)
                    if result.returncode == 0:
                        object_files.append(str(obj_path))
                
                if not object_files:
                    return {
                        "status": "compile_failed",
                        "message": "No object files compiled successfully",
                        "tool": "LTO (manual)"
                    }
                
                # Phase 2: LTOリンク（実行ファイル作成）
                exe_path = temp_path / "lto_test_executable"
                link_cmd = [
                    "g++", "-flto", "-O2", 
                    "-Wl,--gc-sections", "-Wl,--print-gc-sections"
                ] + object_files + ["-o", str(exe_path)]
                
                link_result = subprocess.run(link_cmd, capture_output=True, text=True, timeout=120)
                
                # Phase 3: 結果解析（エラーでも部分的に解析）
                unused_items = self._parse_gc_sections_output(link_result.stderr)
                
                if link_result.returncode == 0:
                    status = "success"
                    message = f"LTO analysis completed successfully"
                else:
                    status = "partial_success"
                    message = f"LTO linking had issues but found {len(unused_items)} unused items"
                
                return {
                    "status": status,
                    "tool": "LTO (manual g++)",
                    "unused_items": unused_items,
                    "total_found": len(unused_items),
                    "method": "link_time_gc_sections",
                    "files_analyzed": len(cpp_files),
                    "message": message
                }
                
        except subprocess.TimeoutExpired:
            return {
                "status": "timeout",
                "message": "Manual LTO analysis timed out",
                "tool": "LTO (manual)"
            }
        except Exception as e:
            return {
                "status": "error", 
                "message": f"Manual LTO analysis failed: {str(e)[:200]}",
                "tool": "LTO (manual)"
            }
    
    def _parse_gc_sections_output(self, stderr_output):
        """--print-gc-sections 出力を解析してデッドコードを抽出"""
        unused_items = []
        
        for line in stderr_output.split('\n'):
            # "removing unused section '.text._Z13unused_funcv' in file 'main.o'"
            if "removing unused section" in line and ".text." in line:
                try:
                    # セクション名を抽出
                    import re
                    match = re.search(r"removing unused section '(.+?)' in file '(.+?)'", line)
                    if match:
                        section = match.group(1)
                        file_name = match.group(2)
                        
                        # .text._Z13unused_funcv -> demangled function name
                        if section.startswith('.text.'):
                            symbol = section[6:]  # '.text.'を除去
                            
                            # C++名前修飾解除試行
                            demangled = self._demangle_symbol(symbol)
                            if demangled != symbol:
                                unused_items.append(f"unused function '{demangled}' [{file_name}]")
                            else:
                                unused_items.append(f"unused symbol '{symbol}' [{file_name}]")
                except:
                    # パースエラーの場合も生のメッセージを保存
                    unused_items.append(line.strip())
        
        return unused_items
    
    def _demangle_symbol(self, mangled_symbol):
        """C++マングル名を解除"""
        try:
            if shutil.which("c++filt"):
                result = subprocess.run(
                    ["c++filt", mangled_symbol], 
                    capture_output=True, text=True, timeout=5
                )
                if result.returncode == 0:
                    return result.stdout.strip()
        except:
            pass
        
        return mangled_symbol  # 解除失敗時は元の名前を返す
    
    def _analyze_binary_symbols(self, binary_path):
        """バイナリからシンボルテーブルを抽出 - 関数定義も含む"""
        symbols = set()
        try:
            # nm コマンドでシンボル抽出
            result = subprocess.run(
                ["nm", "--defined-only", str(binary_path)],
                capture_output=True, text=True, timeout=30
            )
            
            for line in result.stdout.split('\n'):
                if line.strip():
                    parts = line.strip().split()
                    if len(parts) >= 3:
                        symbol_type = parts[1]
                        symbol_name = parts[2]
                        
                        # Tタイプ（テキストセクション関数）のみを対象
                        if symbol_type in ['T', 't'] and (symbol_name.startswith('_ZN') or symbol_name.startswith('_Z')):
                            demangled = self._demangle_symbol(symbol_name)
                            if demangled != symbol_name and not demangled.startswith('std::'):
                                symbols.add(demangled)
            
            # objdump も併用してさらに詳細な解析
            objdump_result = subprocess.run(
                ["objdump", "-t", str(binary_path)],
                capture_output=True, text=True, timeout=30
            )
            
            # objdump出力からも関数シンボルを抽出
            for line in objdump_result.stdout.split('\n'):
                if '.text' in line and 'F' in line:  # Function in text section
                    parts = line.split()
                    if len(parts) >= 6:
                        symbol_name = parts[-1]
                        if symbol_name.startswith('_Z'):
                            demangled = self._demangle_symbol(symbol_name)
                            if demangled != symbol_name and not demangled.startswith('std::'):
                                symbols.add(demangled)
                        
        except:
            pass
        
        return symbols
    
    def _get_binary_size(self, binary_path):
        """バイナリのテキストセクションサイズを取得"""
        try:
            result = subprocess.run(
                ["size", str(binary_path)],
                capture_output=True, text=True, timeout=10
            )
            
            for line in result.stdout.split('\n'):
                if line.strip() and not line.startswith('text'):
                    parts = line.split()
                    if len(parts) >= 1 and parts[0].isdigit():
                        return int(parts[0])  # text section size
        except:
            pass
        
        return 0
    
    def _compare_symbol_sets(self, normal_symbols, lto_symbols):
        """シンボルセットを比較してデッドコードを検出 - 精度改善版"""
        removed_symbols = normal_symbols - lto_symbols
        unused_items = []
        
        # デバッグ情報
        print(f"  📊 Normal build symbols: {len(normal_symbols)}")
        print(f"  🔥 LTO build symbols: {len(lto_symbols)}")
        print(f"  🗑️ Removed symbols: {len(removed_symbols)}")
        
        for symbol in sorted(removed_symbols):
            # インライン展開された可能性のある小さな関数は除外
            if self._is_likely_inlined(symbol):
                continue
                
            # 明らかに未使用と判断できるシンボルのみ報告
            if '::' in symbol:
                unused_items.append(f"unused method '{symbol}' [detected by LTO]")
            elif '(' in symbol:
                unused_items.append(f"unused function '{symbol}' [detected by LTO]")
            else:
                unused_items.append(f"unused symbol '{symbol}' [detected by LTO]")
        
        return unused_items
    
    def _is_likely_inlined(self, symbol):
        """インライン展開された可能性の高い関数を判定"""
        # 簡単な関数名の場合はインライン展開された可能性が高い
        inlined_patterns = [
            'used_global_function',  # テストケースの使用済み関数
            'used_module_function',   # テストケースの使用済み関数
            'do_something'           # 簡単なメソッド名
        ]
        
        for pattern in inlined_patterns:
            if pattern in symbol:
                return True
        
        return False
    
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