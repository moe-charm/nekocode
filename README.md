# 🦀 NekoCode - Ultra-Fast Rust Code Analyzer | 16x Faster than C++ | Tree-sitter Powered

> 🚀 **NEW! Rust Edition with Tree-sitter: 16x faster, better accuracy, zero build hell!**
> 🤖 **Claude Code Users: [PROJECT_OVERVIEW.txt](PROJECT_OVERVIEW.txt) ← START HERE!**  
> 📚 **Quick Guide: [CLAUDE_QUICKSTART.md](CLAUDE_QUICKSTART.md) ← 3-step setup**  
> 📋 **CLI & MCP Reference: [docs/CLI_MCP_REFERENCE.md](docs/CLI_MCP_REFERENCE.md) ← Complete command guide**  
> 📖 **Full Docs: [docs/claude-code/](docs/claude-code/) ← All Claude Code documentation**

[![Rust](https://img.shields.io/badge/Rust-000000?logo=rust&logoColor=white)](https://www.rust-lang.org/)
[![Tree-sitter](https://img.shields.io/badge/Tree--sitter-green.svg)](https://tree-sitter.github.io/tree-sitter/)
[![AI Compatible](https://img.shields.io/badge/AI-Compatible-purple.svg)](https://github.com/moe-charm/nekocode)
[![Multi Language](https://img.shields.io/badge/Multi--Language-orange.svg)](https://github.com/moe-charm/nekocode)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/moe-charm/nekocode)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/moe-charm/nekocode/blob/main/LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com/moe-charm/nekocode)

[🇯🇵 日本語版](README_jp.md) | English

**Revolutionary Rust-powered code analyzer** delivering **16x faster performance** than traditional parsers! 
Featuring **Tree-sitter integration** for lightning-fast, accurate analysis of **8 languages: JavaScript, TypeScript, C++, C, Python, C#, Go, Rust** with **Claude Code optimization**.

## 🚀 Why Rust Edition?

### ⚡ **Blazing Fast Performance**
```bash
# TypeScript Compiler (68 files) Performance Comparison:
┌──────────────────┬────────────┬─────────────┐
│ Parser           │ Time       │ Speed       │
├──────────────────┼────────────┼─────────────┤
│ Rust Tree-sitter │    1.2s    │ 🚀 16.38x   │
│ C++ (PEGTL)      │   19.5s    │ 1.00x       │
│ Rust (PEST)      │   60.7s    │ 0.32x       │
└──────────────────┴────────────┴─────────────┘
```

### 🎯 **Superior Detection Accuracy**
```bash
# Detection Comparison (Medium JS File):
┌──────────────────┬───────────┬──────────┬────────┐
│ Parser           │ Functions │ Classes  │ Total  │
├──────────────────┼───────────┼──────────┼────────┤
│ Rust Tree-sitter │    20     │    2     │   22   │
│ Rust (PEST)      │    13     │    1     │   14   │
│ C++ (PEGTL)      │     4     │    2     │    6   │
└──────────────────┴───────────┴──────────┴────────┘
```

### 🛠️ **Zero Build Hell**
```bash
# Rust Edition (Heaven ✨)
cargo build --release  # Done in 3 seconds!

# vs C++ Edition (Hell 💀)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j16  # Template errors, dependency hell, 5+ hours debugging...
```

## 🔧 Installation

### Prerequisites
- [Rust](https://rustup.rs/) (Latest stable)

### Build (Simple!)
```bash
cd nekocode-rust/
cargo build --release

# Binary location: ./target/release/nekocode-rust
```

## 🚀 Quick Start

### Basic Analysis
```bash
# Analyze a JavaScript/TypeScript project
./target/release/nekocode-rust analyze src/ --parser tree-sitter

# Compare parsers (PEST vs Tree-sitter)
./target/release/nekocode-rust analyze src/ --benchmark

# Analyze specific languages
./target/release/nekocode-rust analyze myfile.py --parser tree-sitter
./target/release/nekocode-rust analyze myfile.cpp --parser tree-sitter
```

### Advanced Features
```bash
# Session-based analysis
./target/release/nekocode-rust session-create src/
# Session ID: 12345678

# AST analysis
./target/release/nekocode-rust ast-stats 12345678
./target/release/nekocode-rust ast-query 12345678 "MyClass::myMethod"

# Code editing (MCP integration)
./target/release/nekocode-rust replace-preview file.js "oldCode" "newCode"
./target/release/nekocode-rust moveclass-preview 12345678 MyClass target.js
```

## 🌟 Key Features

### 🚀 **Ultra-High Performance**
- **Tree-sitter Integration**: GitHub's cutting-edge parser technology
- **Parallel Processing**: Safe Rust concurrency for maximum speed
- **Incremental Parsing**: Only re-analyze changed parts
- **Memory Efficient**: Rust's zero-cost abstractions

### 🎯 **Multi-Language Support**
```
🟨 JavaScript (.js, .mjs, .jsx, .cjs)
🔷 TypeScript (.ts, .tsx)  
🔵 C++ (.cpp, .cxx, .cc, .hpp, .hxx, .hh)
🔵 C (.c, .h)
🐍 Python (.py, .pyw, .pyi)
🟦 C# (.cs)
🐹 Go (.go)
🦀 Rust (.rs)
```

### 🧠 **AI-Optimized Analysis**
- **Function Detection**: Including arrow functions, async functions
- **Class Analysis**: Inheritance, methods, properties
- **Dependency Mapping**: Imports, exports, module relationships
- **Complexity Metrics**: Cyclomatic complexity, nesting depth
- **AST Operations**: Query, scope analysis, structure dump

### 🔧 **Developer-Friendly**
- **Session Management**: Persistent analysis sessions
- **Code Editing**: Replace, insert, move operations with preview
- **Memory System**: Save/load analysis results
- **MCP Integration**: Claude Code Server support
- **Configuration**: Flexible settings management

## 📊 Benchmarks

### Real-World Performance
```bash
# TypeScript Compiler (Microsoft)
# 68 files, ~200KB total
Rust Tree-sitter: 1.189s ⚡
C++ PEGTL:       19.477s
Rust PEST:       60.733s

# Detection Accuracy: 
# Functions detected: 1,000+ (Tree-sitter) vs 200+ (PEGTL)
```

## 🏗️ Architecture

### Tree-sitter Integration
```rust
// Lightning-fast parsing with Tree-sitter
let mut parser = Parser::new();
parser.set_language(&tree_sitter_javascript::LANGUAGE.into())?;
let tree = parser.parse(content, None)?;

// Parallel processing with Rust safety
let results = tokio::task::spawn_blocking(move || {
    analyze_with_tree_sitter(tree, content)
}).await?;
```

### Parser Comparison
```bash
# Switch between parsers easily
./target/release/nekocode-rust analyze file.js --parser pest        # PEST parser
./target/release/nekocode-rust analyze file.js --parser tree-sitter # Tree-sitter (recommended)
./target/release/nekocode-rust analyze file.js --benchmark          # Compare both
```

## 🤖 Claude Code Integration

NekoCode Rust Edition is optimized for AI-assisted development:

```bash
# MCP Server integration
./target/release/nekocode-rust session-create large-project/
# Use with Claude Code for intelligent code analysis

# Direct editing operations  
./target/release/nekocode-rust replace-preview src/main.js "oldPattern" "newPattern"
./target/release/nekocode-rust moveclass-preview session123 UserClass src/models/user.js
```

## 📚 Commands Reference

### Analysis Commands
```bash
analyze <path>              # Analyze files/directories
languages                   # List supported languages  
```

### Session Management
```bash
session-create <path>       # Create analysis session
session-command <id> <cmd>  # Execute session command
```

### Code Editing (MCP)
```bash
replace-preview <file> <pattern> <replacement>  # Preview replacement
replace-confirm <preview_id>                    # Confirm replacement
insert-preview <file> <line> <content>          # Preview insertion
moveclass-preview <session> <class> <target>    # Preview class move
```

### AST Operations
```bash
ast-stats <session>         # AST statistics
ast-query <session> <path>  # Query AST nodes
scope-analysis <session> <line>  # Analyze scope at line
ast-dump <session> [format] # Dump AST structure
```

## 🏆 Why Choose Rust Edition?

### ✅ **Performance Champion**
- 16x faster than C++ implementation
- Superior detection accuracy
- Tree-sitter's cutting-edge technology
- Parallel processing safety

### ✅ **Developer Experience**
- One-command build: `cargo build --release`
- No dependency hell, no template errors
- Cross-platform compilation
- Modern tooling and packaging

### ✅ **Future-Proof**
- Tree-sitter: Used by GitHub, Neovim, Atom
- Rust: Growing ecosystem, memory safety
- Active development and modern features
- AI-first design philosophy

## 🗂️ Repository Structure

```
nekocode-rust/              # 🦀 Main Rust implementation (RECOMMENDED)
├── src/
│   ├── analyzers/          # Language-specific analyzers
│   │   ├── javascript/     # JS/TS with Tree-sitter + PEST
│   │   ├── python/         # Python analyzer
│   │   ├── cpp/           # C++ analyzer  
│   │   └── ...            # Other languages
│   ├── core/              # Core functionality
│   │   ├── session.rs     # Session management
│   │   ├── memory.rs      # Memory system
│   │   └── ast.rs         # AST operations
│   └── main.rs            # CLI interface

# Legacy C++ implementation (reference only)
src/                        # C++ source (legacy)
build/                      # C++ build directory  
docs/                       # Documentation
```

## 🤝 Contributing

We welcome contributions! The Rust edition is now the primary development target.

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

**🔥 Ready to experience 16x faster code analysis?**

```bash
git clone https://github.com/your-org/nekocode.git
cd nekocode/nekocode-rust/
cargo build --release
./target/release/nekocode-rust analyze your-project/ --parser tree-sitter
```

**No more build hell. No more waiting. Just blazing fast analysis.** 🚀🦀