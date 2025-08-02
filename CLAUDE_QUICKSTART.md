# 🚀 Claude Code Quick Start Guide for NekoCode

## Overview
This guide helps Claude Code users quickly understand and work with the NekoCode project.

## Project Structure at a Glance
```
nekocode/
├── src/lib/        # Core analysis library
├── src/apps/       # CLI applications  
├── include/        # Public headers
├── tests/          # Unit tests
└── docs/           # Documentation
```

## Common Tasks

### 1. Building the Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. Running Analysis
```bash
# Analyze a single file
./bin/nekocode_ai analyze path/to/file.cpp

# Analyze a directory
./bin/nekocode_ai analyze src/ --stats-only

# Get detailed output
./bin/nekocode_ai analyze src/ --format json
```

### 3. Adding a New Language Analyzer
1. Create analyzer in `src/lib/analyzers/newlang/`
2. Inherit from `BaseAnalyzer`
3. Register in `analyzer_factory.cpp`
4. Update CMakeLists.txt

### 4. Running Tests
```bash
cd build
ctest
# Or run specific test
./bin/test_core
```

## Key Files to Know

### Core Components
- `src/lib/analyzers/base_analyzer.hpp` - Base class for all analyzers
- `src/lib/core/core.cpp` - Main analysis engine
- `src/apps/cli/main_ai.cpp` - CLI entry point

### Configuration
- `CMakeLists.txt` - Build configuration
- `include/nekocode/types.hpp` - Core type definitions

## Debugging Tips

### 1. Enable Debug Output
```bash
# Set debug environment variable
export NEKOCODE_DEBUG=1
./bin/nekocode_ai analyze src/
```

### 2. Common Issues
- **Build fails**: Check C++17 support
- **Slow analysis**: Use `--io-threads` flag
- **Memory issues**: Use `--stats-only` for large codebases

## Code Style Guidelines
- Use C++17 features
- Follow existing naming conventions
- Avoid std::regex (use PEGTL instead)
- Keep analyzers language-specific

## Performance Optimization
1. Use parallel STL algorithms where possible
2. Minimize string copies
3. Prefer const references
4. Use move semantics

## Contributing Workflow
1. Create feature branch
2. Make changes
3. Run tests
4. Update documentation
5. Submit PR

## Useful Commands for Development
```bash
# Clean rebuild
rm -rf build && mkdir build && cd build && cmake .. && make

# Run with profiling
time ./bin/nekocode_ai analyze large-project/

# Check binary size
ls -lh bin/

# Run specific analyzer
./bin/nekocode_ai analyze file.ts --analyzer typescript
```

## Contact & Support
- GitHub Issues: https://github.com/moe-charm/nekocode/issues
- Documentation: See docs/ directory

## Quick Reference
- **Fast**: 10-100x faster than Python alternatives
- **Memory Efficient**: Handles large codebases
- **AI-Optimized**: JSON output for LLMs
- **Multi-Language**: C++, TS, JS, Python, C#, Go, Rust