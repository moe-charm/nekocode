# 📚 NekoCode Documentation Structure

## Quick Links

### 🤖 For Claude Code Users
- [Claude Code Documentation Index](docs/claude-code/INDEX.md)
- [Large Project Handling Guide](docs/LARGE_PROJECT_HANDLING.md)

### 📖 General Documentation
- [Usage Guide (English)](docs/USAGE.md)
- [使用ガイド (日本語)](docs/USAGE_jp.md)
- [Performance Guide](docs/PERFORMANCE_GUIDE.md)
- [Debug Guide](docs/DEBUG_GUIDE.md)

### 👨‍💻 For Developers
- [Contributing Guidelines](CONTRIBUTING.md)
- [Development Notes](docs/dev/)
  - [PEGTL Migration Plan](docs/dev/PEGTL_MIGRATION_PLAN.md)
  - [Regex Removal Notes](docs/dev/NEVER_USE_REGEX.md)
  - [Emergency Handover](docs/dev/HANDOVER_EMERGENCY.txt)
  - [JavaScript Handover](docs/dev/JAVASCRIPT_HANDOVER.txt)
  - [TypeScript Handover](docs/dev/TYPESCRIPT_HANDOVER.txt)
  - [Development Updates (English)](docs/dev/DEV_UPDATE.md)
  - [開発更新 (日本語)](docs/dev/DEV_UPDATE_JP.md)
  - [C++ Parallel Processing Update](docs/dev/CPP_PARALLEL_UPDATE.txt)
  - [Go Analysis Plan](docs/dev/go_analysis_plan.md)

### 📝 Examples
- [Examples Directory](examples/)
- [Test Suite](tests/)

### 🏗️ Project Structure
```
nekocode-cpp-github/
├── docs/               # All documentation
│   ├── claude-code/    # Claude Code specific docs
│   └── dev/            # Developer notes
├── src/                # Source code
├── include/            # Headers
├── build/              # Build output
├── sessions/           # Analysis sessions
├── test-projects/      # Test data
├── tests/              # Unit tests
└── examples/           # Usage examples
```