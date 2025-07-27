# 📚 NekoCode C++ Examples

This directory contains practical examples demonstrating various features of NekoCode C++.

## 🎯 Available Examples

### 1. Basic Analysis (`basic_analysis.cpp`)
- **Purpose**: Demonstrates single file analysis
- **Features**: Classes, functions, templates, macros
- **Usage**: Perfect for understanding basic NekoCode capabilities

```bash
# Analyze the example file
./nekocode_ai examples/basic_analysis.cpp

# Interactive analysis
./nekocode_ai session-create examples/
./nekocode_ai session-cmd <session_id> complexity
```

### 2. Project Analysis (`project_analysis/`)
- **Purpose**: Multi-file project analysis
- **Features**: Inter-file dependencies, include analysis
- **Usage**: Understanding large codebase analysis

### 3. Template Analysis (`template_examples.cpp`)
- **Purpose**: Advanced C++ template analysis
- **Features**: Template specialization, variadic templates, SFINAE
- **Usage**: Testing template-specific features

### 4. Real-World Analysis (`real_world_analysis.md`)
- **Purpose**: Actual nyamesh_v23 refactoring case study
- **Features**: 99 files, 63K lines analyzed scientifically
- **Usage**: See how AI developers use NekoCode in production

### 5. Interactive Session (`interactive_demo.sh`)
- **Purpose**: Demonstrates session-based workflow
- **Features**: All available commands in sequence
- **Usage**: Quick start guide for new users

### 6. 6Core Simplification (`6core_simplification.md`)
- **Purpose**: Real-time AI developer workflow example
- **Features**: Live refactoring session with 88-89% complexity reduction
- **Usage**: See how to achieve 10x productivity with data-driven refactoring

## 🚀 Quick Start

1. **Build NekoCode**:
   ```bash
   mkdir build && cd build
   cmake .. && make -j$(nproc)
   cd ..
   ```

2. **Run Basic Example**:
   ```bash
   ./build/nekocode_ai examples/basic_analysis.cpp
   ```

3. **Try Interactive Mode**:
   ```bash
   ./build/nekocode_ai session-create examples/
   # Use the returned session_id for commands
   ./build/nekocode_ai session-cmd <session_id> stats
   ```

## 📊 Expected Output

Each example includes comments describing expected analysis results:

- **Lines of Code**: Actual count
- **Functions/Classes**: Detected entities
- **Complexity Metrics**: Cyclomatic complexity scores
- **Template Analysis**: Template specializations found
- **Macro Analysis**: Macro definitions and usage

## 🎮 Interactive Commands to Try

```bash
# Basic information
stats              # Project overview
files              # File-by-file breakdown
complexity         # Complexity analysis

# C++ specific
include-graph      # Dependency visualization
template-analysis  # Template specialization detection
macro-analysis     # Macro expansion tracking

# Search and exploration
find <term>        # Search for files/functions
structure          # Code structure overview
calls              # Function call analysis
```

## 🛠️ Customization

Feel free to modify these examples to test:

- Different coding patterns
- Various complexity levels
- Template metaprogramming techniques
- Macro usage patterns
- Include dependency structures

## 📈 Performance Testing

For performance benchmarking:

```bash
# Time the analysis
time ./build/nekocode_ai examples/

# Compare with session mode
./build/nekocode_ai session-create examples/
time ./build/nekocode_ai session-cmd <session_id> stats
```

The session mode should be significantly faster for repeated operations!

---

**Happy analyzing with NekoCode! 🐱**