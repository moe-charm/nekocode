# 🐱 NekoCode Usage Guide

## 📖 Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Basic Usage](#basic-usage)
4. [Advanced Features](#advanced-features)
5. [AI Developer Guide](#ai-developer-guide)
6. [Troubleshooting](#troubleshooting)

## Introduction

NekoCode C++ is a lightning-fast code analysis tool designed for modern development workflows, especially for AI-assisted development with tools like Claude Code and GitHub Copilot.

## Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- Git

### Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/moe-charm/nekocode.git
cd nekocode

# 2. Create build directory
mkdir build && cd build

# 3. Configure with CMake
cmake ..

# 4. Build (parallel build recommended)
make -j$(nproc)

# 5. Verify installation
./nekocode_ai --help
```

## Basic Usage

### Single File Analysis

```bash
# Analyze a C++ file
./nekocode_ai main.cpp

# Analyze a JavaScript file
./nekocode_ai app.js

# With performance statistics
./nekocode_ai --performance main.cpp
```

### Directory Analysis

```bash
# Analyze entire src directory
./nekocode_ai src/

# Analyze specific language only
./nekocode_ai --lang cpp src/

# Compact JSON output
./nekocode_ai --compact src/
```

## Advanced Features

### ⚡ Performance Optimization (NEW!)

NekoCode now includes intelligent storage optimization for lightning-fast analysis:

```bash
# 🔥 SSD Mode - Maximum parallel performance
./nekocode_ai analyze large-project/ --ssd --performance
# Uses all CPU cores, perfect for NVMe/SSD storage

# 🛡️ HDD Mode - Safe sequential processing  
./nekocode_ai analyze large-project/ --hdd --performance
# Single thread, prevents HDD thrashing

# 📊 Progress Display - Monitor large projects
./nekocode_ai session-create large-project/ --progress
# Real-time progress: "🚀 Starting analysis: 38,021 files"
# Creates progress file: sessions/SESSION_ID_progress.txt
```

**Claude Code Pro Tip**: For projects with 30,000+ files, always use `--progress` to monitor completion!

### Interactive Sessions

The most powerful feature of NekoCode!

```bash
# 1. Create a session with progress monitoring
./nekocode_ai session-create /path/to/your/project --progress
# Output: Session created! Session ID: ai_session_20250727_180532

# 2. Run various analyses using session ID
SESSION_ID=ai_session_20250727_180532

# Project statistics
./nekocode_ai session-command $SESSION_ID stats

# Complexity ranking (most important!)
./nekocode_ai session-command $SESSION_ID complexity

# File search
./nekocode_ai session-command $SESSION_ID "find manager"

# Function structure analysis
./nekocode_ai session-command $SESSION_ID structure
```

### C++ Specific Analysis

#### Include Dependencies

```bash
# Generate dependency graph
./nekocode_ai session-command $SESSION_ID include-graph

# Detect circular dependencies (critical!)
./nekocode_ai session-command $SESSION_ID include-cycles

# Find unused includes
./nekocode_ai session-command $SESSION_ID include-unused
```

#### Template & Macro Analysis

```bash
# Detect template specializations
./nekocode_ai session-command $SESSION_ID template-analysis

# Track macro expansions
./nekocode_ai session-command $SESSION_ID macro-analysis

# Detect metaprogramming patterns
./nekocode_ai session-command $SESSION_ID metaprogramming
```

## AI Developer Guide

### Using with Claude Code

1. **Place NekoCode in your project**
   ```bash
   cd your-project
   git clone https://github.com/moe-charm/nekocode.git tools/nekocode
   ```

2. **Magic words to tell Claude**
   ```
   "There's a code analysis tool in tools/nekocode"
   "Measure the complexity of this project"
   "Check for circular dependencies"
   ```

3. **Claude automatically**
   - Builds the tool
   - Creates a session
   - Runs analysis
   - Interprets results

### Practical Example: Refactoring

```bash
# 1. Measure current complexity
./nekocode_ai session-command $SESSION_ID complexity

# Output example:
# FileA.cpp: Complexity 156 (Very Complex)
# FileB.cpp: Complexity 89 (Complex)

# 2. Perform refactoring

# 3. Verify improvements
./nekocode_ai session-command $SESSION_ID complexity
# FileA.cpp: Complexity 23 (Simple)  ← 85% reduction!
```

## Troubleshooting

### Build Issues

**Q: CMake says C++17 is not supported**
```bash
# Check GCC version
g++ --version

# If old, specify newer compiler
cmake -DCMAKE_CXX_COMPILER=g++-9 ..
```

**Q: Tree-sitter not found**
```bash
# Build in placeholder mode (works without Tree-sitter)
cmake -DUSE_TREE_SITTER=OFF ..
```

### Runtime Issues

**Q: Session not found**
```bash
# List available sessions
ls sessions/

# Create new session
./nekocode_ai session-create .
```

**Q: Out of memory**
```bash
# Use HDD mode (single thread)
./nekocode_ai analyze large-project/ --hdd

# Manual thread limit
./nekocode_ai --threads 2 large-project/

# Stats only mode
./nekocode_ai --stats-only large-project/
```

**Q: Analysis taking too long**
```bash
# For SSD/NVMe storage - use parallel mode
./nekocode_ai analyze project/ --ssd --progress

# Monitor progress in real-time
tail -f sessions/SESSION_ID_progress.txt

# Check what's happening
./nekocode_ai analyze project/ --performance
```

**Q: HDD getting hammered**
```bash
# Safe HDD mode (single thread, sequential)
./nekocode_ai analyze project/ --hdd --progress

# This prevents disk thrashing on mechanical drives
```

## 💡 Pro Tips

1. **Storage-Aware Analysis**: 
   - Use `--ssd` for SSD/NVMe drives (4-16x faster!)
   - Use `--hdd` for mechanical drives (safe & stable)
   - Always add `--progress` for projects with 1000+ files

2. **Complexity First**: Always start with `complexity` command to identify problem files

3. **Use Sessions**: For repeated analysis, always use sessions (180x faster!)

4. **Large Project Strategy**:
   ```bash
   # Perfect workflow for 30K+ files
   ./nekocode_ai session-create huge-project/ --ssd --progress
   tail -f sessions/ai_session_*/progress.txt  # Monitor progress
   ```

5. **Parallel Build**: Use `make -j$(nproc)` to utilize all cores

6. **JSON Output**: Use `--compact` option for integration with other tools

---

For more information, visit the [official documentation](https://github.com/moe-charm/nekocode)!

*Happy Analyzing! 🐱*