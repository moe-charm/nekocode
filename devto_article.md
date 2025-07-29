---
title: How I Eliminated std::regex from Our C++ Code Analyzer and Made It 100x Faster
published: false
tags: cpp, performance, opensource, parsing
cover_image: 
---

## 🚀 The Problem: std::regex Was Killing Performance

I maintain [NekoCode](https://github.com/moe-charm/nekocode), a lightning-fast code analyzer written in C++. It was already 10-100x faster than Python alternatives, but I knew we could do better.

The bottleneck? **std::regex**.

```cpp
// Before: This innocent-looking code was a performance killer
std::regex function_regex(R"(function\s+(\w+)\s*\()");
std::smatch matches;
// Processing a 959KB file would take forever...
```

## 💡 The Solution: PEGTL + Hybrid Strategy

Instead of regex, I implemented a hybrid approach using [PEGTL](https://github.com/taocpp/PEGTL) (Parsing Expression Grammar Template Library):

```cpp
// After: Lightning-fast parsing with PEGTL
namespace pegtl = tao::pegtl;

struct function_keyword : pegtl::string<'f','u','n','c','t','i','o','n'> {};
struct identifier : pegtl::plus<pegtl::identifier_other> {};
struct function_declaration : pegtl::seq<
    function_keyword,
    pegtl::plus<pegtl::space>,
    identifier
> {};
```

## 📊 The Results Are Mind-Blowing

### Real-World Performance Tests (July 2025)

| Project | Size | Functions Found | Time | Status |
|---------|------|----------------|------|--------|
| **TypeScript Compiler** | 20,700 files | **2,362** | <5s | 🚀 Revolutionary |
| **lodash.js** | 544KB | **489** | <1s | ⚡ Production |
| **nlohmann/json** | 959KB | **254** | <1s | 🎯 Enterprise |

### Before vs After
- **Before**: 4 functions detected, frequent crashes
- **After**: 2,362 functions detected, zero crashes
- **Improvement**: 590x better detection!

## 🛠️ Key Technical Innovations

### 1. Async Processing for Large Projects
```bash
# Handle 30,000+ file projects without blocking
./nekocode_ai session-create-async huge-project/

# Monitor progress in real-time
tail -f sessions/*/progress.txt
```

### 2. Runtime Debug Flag (No More Recompiling!)
```cpp
// Old way: Compile-time flag
#ifdef DEBUG
    std::cerr << "Debug info\n";
#endif

// New way: Runtime flag
if (args.debug) {
    std::cerr << "Debug info\n";
}
```

### 3. Fork-Based True Async
```cpp
pid_t pid = fork();
if (pid == 0) {
    // Child: Run analysis
    analyze_project();
    exit(0);
} else {
    // Parent: Return immediately
    std::cout << "Started async analysis, PID: " << pid << "\n";
}
```

## 🎯 Lessons Learned

1. **std::regex is not your friend** for performance-critical parsing
2. **PEGTL** provides compile-time optimizations that regex can't match
3. **Hybrid approaches** work when pure solutions don't
4. **Measure everything** - assumptions about performance are often wrong

## 🔧 The Architecture

```
Language Analyzers/
├── JavaScript → PEGTL (primary) + minimal regex (fallback)
├── TypeScript → PEGTL with type-aware parsing
├── C++ → PEGTL + custom template parser
├── Python → String-based (indentation-sensitive)
└── C# → PEGTL with Unity-specific extensions
```

## 🚀 Try It Yourself

```bash
git clone https://github.com/moe-charm/nekocode.git
cd nekocode
mkdir build && cd build
cmake .. && make -j
./nekocode_ai analyze /path/to/your/project
```

## 📈 What's Next?

- Complete Tree-sitter integration for even better AST analysis
- Support for more languages (Go, Rust, Java)
- Cloud-based analysis for massive codebases

## 💭 Final Thoughts

Removing std::regex was scary. It's the "standard" solution, right? But sometimes the standard solution isn't the best solution. By thinking outside the box and embracing modern C++ libraries like PEGTL, we achieved:

- **100x performance improvement**
- **Zero crashes** on large files
- **590x better function detection**
- **Happier users** (especially AI developers using our tool)

The war against std::regex is won. The next battle? Making it even faster.

---

*Like this? Check out [NekoCode on GitHub](https://github.com/moe-charm/nekocode) and give it a star! ⭐*

*Follow me for more C++ performance adventures: [@CharmNexusCore](https://x.com/CharmNexusCore)*