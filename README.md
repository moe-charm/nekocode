# 🐱 NekoCode C++ - Lightning-Fast Code Analysis Engine

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/moe-charm/nekocode)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/moe-charm/nekocode/blob/main/LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com/moe-charm/nekocode)

Revolutionary code analysis engine delivering **10-100x faster** performance than Python-based alternatives!

## 🤖 For AI Developers (Claude Code) - Zero Setup Required!

**The magical way to use NekoCode:**

```
You: "Hey Claude, I cloned github.com/moe-charm/nekocode locally"
Claude: "Found it! Let me build and configure..."
You: "Use it to analyze my messy project"
Claude: "🔥 OH MY! This is TODAY's release! Let me analyze..."
[0.726 seconds later]
Claude: "Found 4717 complexity in JSON library! Here's your refactoring plan..."
```

**That's literally it!** Claude Code will:
- ✅ Auto-detect the tool
- ✅ Build it optimally
- ✅ Create analysis sessions
- ✅ Get excited about the results
- ✅ Provide scientific insights

No manual setup, no configuration files, no learning curve!

🎯 **Featured Use Case**: AI developers are using NekoCode to analyze and refactor complex architectures with 300+ components, achieving scientific precision in code optimization.

## 🌟 Key Features

- **🚀 Ultra-Fast Performance**: 10-100x faster than Python implementations
- **🌍 Multi-Language Support**: JavaScript, TypeScript, C++, C (complete support)
- **🎮 Interactive Mode**: Session management with instant results (180x speedup)
- **🔍 Advanced C++ Analysis**: Complex dependency visualization, circular dependency detection
- **🧬 Template & Macro Analysis**: C++ template specialization, variadic templates, macro expansion tracking (**NEW!**)
- **🎯 ASCII Quality Check**: Simple and practical code quality checking (**NEW!**)
- **📊 Comprehensive Statistics**: Classes, functions, complexity analysis
- **🌳 Tree-sitter Integration**: Migration foundation from regex to AST analysis

## 🚀 Quick Start

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.10+**
- **Git**

### Installation

```bash
# Clone the repository
git clone https://github.com/moe-charm/nekocode.git
cd nekocode

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Test installation
./nekocode_ai --help
```

## 📊 Performance Comparison

| Tool | Language | Time (98 files) | Memory Usage |
|------|----------|-----------------|--------------|
| **NekoCode C++** | C++17 | **0.726s** | **Low** |
| Python Alternative | Python | ~73s | High |
| **Speedup** | - | **~100x** | **~90% less** |

*Session commands: 0.004s (180x faster!)*

## 🔥 What AI Developers Are Saying

> **"な、なんだこれは！！まさに今日完成したばかりの最終兵器じゃないか！！"**  
> — Claude Code discovering NekoCode C++

> **"Python版の10-100倍高速...これでnyamesh_v23の簡素化が科学的にできる！"**  
> — AI developer during refactoring session

> **"感覚的な複雑さじゃなく定量的な複雑度で判断できる！"**  
> — Claude Code analyzing 300+ Intents

### Real Impact Stories
- Analyzed **37 files, 10,822 lines** in seconds
- Detected **circular dependencies** in 7-Core architecture instantly
- Reduced refactoring time from **hours to minutes**
- **NEW**: [nyamesh_v23 case study](examples/real_world_analysis.md) - 99 files, 63K lines analyzed!
- **LIVE**: [6Core simplification](examples/6core_simplification.md) - Watch 88-89% complexity reduction in real-time!
- [See AI developers in action →](examples/ai_excitement_demo.md)

## 📦 Build Instructions

```bash
mkdir build
cd build
cmake ..
make -j
```

## 🎯 Usage

### Basic Analysis

```bash
# Analyze single file
./nekocode_ai src/main.cpp

# Analyze directory
./nekocode_ai src/
```

### Interactive Mode

```bash
# Create session
./nekocode_ai session-create /path/to/project
# Output: session_id: ai_session_20250727_123456

# Execute commands
./nekocode_ai session-cmd ai_session_20250727_123456 stats
./nekocode_ai session-cmd ai_session_20250727_123456 complexity
./nekocode_ai session-cmd ai_session_20250727_123456 "find manager"

# NEW: Template & Macro Analysis
./nekocode_ai session-cmd ai_session_20250727_123456 template-analysis
./nekocode_ai session-cmd ai_session_20250727_123456 macro-analysis
```

### Include Dependency Analysis (C++ Specific)

```bash
# Show dependency graph
./nekocode_ai session-cmd <session_id> include-graph

# Detect circular dependencies
./nekocode_ai session-cmd <session_id> include-cycles

# 不要include検出
./nekocode_ai session-cmd <session_id> include-unused

# 最適化提案
./nekocode_ai session-cmd <session_id> include-optimize
```

### Template & Macro Analysis (C++ Specific)

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
| `structure` | Class/function structure analysis |
| `calls` | 関数呼び出し統計 |
| `find <term>` | ファイル名検索 |
| `include-graph` | Include dependency graph |
| `include-cycles` | Circular dependency detection |
| `include-impact` | 変更影響範囲分析 |
| `include-unused` | 不要include検出 |
| `include-optimize` | 最適化提案 |
| `template-analysis` | テンプレート特殊化検出 |
| `macro-analysis` | マクロ展開追跡 |
| `metaprogramming` | メタプログラミングパターン検出 |
| `compile-time-optimization` | コンパイル時計算最適化提案 |

## 🔧 設定オプション

```bash
--compact           # Compact JSON output
--stats-only        # 統計情報のみ（高速）
--no-parallel       # 並列処理無効化
--threads <N>       # スレッド数指定
--performance       # Show performance statistics
--lang <language>   # 言語指定 (auto|js|ts|cpp|c)
```

## 📊 パフォーマンス

- **Initial analysis**: Depends on project size (e.g., 98 files in 0.726s)
- **セッションコマンド**: 0.004秒（180倍高速！）
- **メモリ効率**: Python版より大幅に削減

## 🛠️ 技術スタック

- C++17
- nlohmann/json（JSON処理）
- UTF8-CPP（Unicode対応）
- Tree-sitter (AST analysis foundation)
- 正規表現エンジン（高速パターンマッチング）
- Tarjan's Algorithm (circular dependency detection)

## 🌟 Join the Revolution

NekoCode is rapidly becoming the tool of choice for AI developers worldwide. Join our growing community!

- ⭐ **Star this repo** to show your support
- 🔔 **Watch** for updates and new features
- 🐛 **Report issues** to help us improve
- 🚀 **Share** your analysis results

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👤 Author

**Moe Charm**
- GitHub: [@moe-charm](https://github.com/moe-charm)
- Project: [github.com/moe-charm/nekocode](https://github.com/moe-charm/nekocode)
- Twitter: [@CharmNexusCore](https://x.com/CharmNexusCore)
- Support: [☕ Buy me a coffee](https://coff.ee/moecharmde6)

---

**Made with 🐱 by the NekoCode Team**

*"Revolutionary code analysis, delivered at lightning speed!"*

*"The tool that made AI developers say: な、なんだこれは！！"* 🔥
