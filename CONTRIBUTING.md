# 🤝 Contributing to NekoCode C++

We love your input! We want to make contributing to NekoCode C++ as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## 🚀 Development Process

We use GitHub to host code, to track issues and feature requests, as well as accept pull requests.

### Pull Requests

Pull requests are the best way to propose changes to the codebase. We actively welcome your pull requests:

1. **Fork the repo** and create your branch from `main`.
2. **If you've added code** that should be tested, add tests.
3. **If you've changed APIs**, update the documentation.
4. **Ensure the test suite passes**.
5. **Make sure your code follows** the existing code style.
6. **Issue that pull request**!

## 🛠️ Development Setup

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.10+**
- **Git**

### Building from Source

```bash
# Clone your fork
git clone https://github.com/your-username/nekocode.git
cd nekocode

# Create development build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests
./test_core
./test_performance
```

### Running Tests

```bash
# Basic functionality tests
./build/test_core

# Performance benchmarks
./build/test_performance

# Manual testing with examples
./build/nekocode_ai examples/basic_analysis.cpp
```

## 📝 Code Style

### C++ Guidelines

We follow modern C++ best practices:

- **C++17** standard features encouraged
- **Const-correctness** is important
- **RAII** for resource management
- **Smart pointers** over raw pointers
- **Range-based for loops** when appropriate

### Naming Conventions

```cpp
// Classes: PascalCase
class CodeAnalyzer {};

// Functions and variables: snake_case
void analyze_file(const std::string& file_path);
int line_count = 0;

// Constants: UPPER_SNAKE_CASE
const int MAX_BUFFER_SIZE = 1024;

// Private members: trailing underscore
class Example {
private:
    int member_variable_;
};
```

### Comments and Documentation

- **Public APIs**: Doxygen-style comments
- **Complex algorithms**: Explain the approach
- **TODO comments**: Include GitHub issue number when possible

```cpp
/**
 * @brief Analyzes C++ template specializations
 * @param content The source code content to analyze
 * @return Vector of template specialization information
 */
std::vector<TemplateInfo> analyze_templates(const std::string& content);

// TODO(#123): Optimize for large files
```

## 🐛 Bug Reports

We use GitHub issues to track public bugs. Report a bug by [opening a new issue](https://github.com/moe-charm/nekocode/issues).

### Great Bug Reports Include:

- **A quick summary** and/or background
- **Steps to reproduce**
  - Be specific!
  - Give sample code if you can
- **What you expected** would happen
- **What actually happens**
- **System information** (OS, compiler version, etc.)

## 🚀 Feature Requests

We use GitHub Discussions for feature requests. Before creating a new discussion:

1. **Check existing discussions** to avoid duplicates
2. **Provide clear use cases** for the feature
3. **Consider backwards compatibility**

## 📋 Issue and PR Labels

- `bug` - Something isn't working
- `enhancement` - New feature or request
- `documentation` - Improvements or additions to documentation
- `good first issue` - Good for newcomers
- `help wanted` - Extra attention is needed
- `performance` - Performance-related improvements
- `breaking change` - Changes that break backward compatibility

## 🎯 Areas for Contribution

### High Priority

- **Performance optimizations**
- **Additional language support** (Python, Rust, Go)
- **Tree-sitter integration** completion
- **Documentation improvements**

### Medium Priority

- **Platform-specific optimizations**
- **Memory usage optimizations**
- **CLI usability improvements**
- **Test coverage expansion**

### Advanced Features

- **Language Server Protocol** support
- **WebAssembly** port
- **Plugin system** architecture
- **Real-time analysis** capabilities

## 📚 Documentation

- **Code documentation**: Use Doxygen-style comments
- **User guides**: Markdown in `docs/` directory
- **Examples**: Add to `examples/` with detailed comments
- **README updates**: Keep installation and usage current

## 🧪 Testing Guidelines

### Unit Tests

- Add tests for new features
- Maintain existing test coverage
- Test edge cases and error conditions

### Performance Tests

- Benchmark critical paths
- Ensure no performance regressions
- Document performance characteristics

### Integration Tests

- Test with real-world codebases
- Verify cross-platform compatibility
- Test with various compiler versions

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.

## 🏆 Recognition

Contributors will be recognized in:

- **README.md** contributors section
- **Release notes** for significant contributions
- **GitHub contributors** page

## 💬 Community

- **GitHub Discussions**: Feature requests and general discussion
- **GitHub Issues**: Bug reports and specific problems
- **Pull Requests**: Code contributions and reviews

## 🆘 Getting Help

Stuck? Here's how to get help:

1. **Check the documentation** in `docs/` and `examples/`
2. **Search existing issues** and discussions
3. **Create a new discussion** for questions
4. **Create an issue** for bugs

---

**Thank you for contributing to NekoCode C++! 🐱**

Every contribution, no matter how small, makes this project better for everyone.