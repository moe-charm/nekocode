# 🔴 C++ 解析戦略 - 最終ボス戦

## 🎯 言語特性と課題

### **C++は全言語中最凶の敵**
```cpp
namespace std {
    template<typename T, typename Alloc = std::allocator<T>>
    class vector {
    public:
        template<typename... Args>
        constexpr decltype(auto) emplace_back(Args&&... args) 
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            -> reference;
    };
}
```

### **C++特有の地獄**
| 問題 | 例 | 難易度 |
|------|----|----|
| **テンプレート** | `template<class T, int N = 5>` | 😱😱😱 |
| **名前空間** | `namespace nested::deep::hell {}` | 😱😱 |
| **プリプロセッサ** | `#define MACRO(x) ...` | 😱😱😱 |
| **多重継承** | `class D : public A, private B` | 😱😱 |
| **const修飾** | `const int& func() const noexcept` | 😱😱 |
| **ヘッダー分離** | 宣言と定義が別ファイル | 😱😱 |

## 🔧 採用戦略: 段階的降伏作戦

### **Phase 1: 極小文法で最低限検出**
```cpp
// 目標: 完璧解析は諦め、存在検出のみ
struct simple_class : seq<
    star<space>,  // インデント許可
    class_keyword,
    required_ws,
    identifier,
    star<space>,
    block  // 内容は読み飛ばす
> {};
```

**検出目標:**
- ✅ `class ClassName {` - 基本クラス
- ✅ `struct StructName {` - 構造体
- ✅ `namespace ns {` - 名前空間
- ❓ `int function()` - 戻り値型付き関数

### **Phase 2: テンプレート無視戦略**
```cpp
// テンプレートは「ノイズ」として完全スキップ
struct template_noise : seq<
    TAO_PEGTL_STRING("template"),
    one<'<'>,
    until<one<'>'>>  // template<...> 全部無視
> {};

struct class_with_template : seq<
    star<space>,
    opt<template_noise>,  // ← テンプレート無視
    star<space>,
    class_keyword,
    // ... 以下同じ
> {};
```

### **Phase 3: プリプロセッサ前処理**
```cpp
// #include, #define等を事前除去
std::string preprocess(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.starts_with("#")) {  // プリプロセッサ行を除去
            lines.push_back(line);
        }
    }
    return join(lines, "\n");
}
```

## 🚨 現在の深刻な問題

**2025-07-28時点:**
```cpp
// この最小限のC++すら検出できない状況
class TestClass {
};
// → total_classes: 0
```

**考えられる原因:**
1. **PEGTL文法の根本的な欠陥**
2. **アクション関数の未実行**
3. **メインルールの構造問題**

## 🔧 緊急デバッグ計画

### **Step 1: 最小限検証**
```cpp
// これだけでも検出できるか？
class A{};
```

### **Step 2: アクション動作確認**
```cpp
template<>
struct cpp_action<cpp::minimal_grammar::simple_class> {
    static void apply(...) {
        std::cerr << "CLASS DETECTED!" << std::endl;  // ← この出力が出るか？
    }
};
```

### **Step 3: フォールバック実装**
```cpp
// PEGTL完全失敗時の緊急対応
if (pegtl_classes.empty() && pegtl_functions.empty()) {
    // 正規表現による強制検出
    std::regex class_regex("^\\s*class\\s+(\\w+)");
    // ... 泥臭い実装で確実に検出
}
```

## 📊 目標設定

| ファイル | 複雑度 | 現状 | 最低目標 | 理想目標 |
|----------|--------|------|----------|----------|
| **test_cpp_simple.cpp** | 低 | 0検出 | 3クラス+2関数 | 完全検出 |
| **nlohmann/json.hpp** | 超高 | 0検出 | 10+要素 | 100+要素 |

## 💭 設計哲学

### **「完璧主義を捨てる」**
```cpp
// ❌ 目指さない: 完璧なC++パーサー
// ✅ 目指す: 実用的な要素検出器

// 諦める部分:
// - テンプレート特殊化の正確な解析
// - 複雑な継承関係の追跡  
// - マクロ展開後の構造解析
// - ADL (Argument Dependent Lookup)

// 重視する部分:
// - クラス名と関数名の存在確認
// - 大まかな構造の把握
// - 統計情報の妥当性
```

## 💡 実装メモ

**現在の状況:**
- ✅ C++ PEGTLアナライザーは呼ばれている
- ✅ 行数カウントは正常動作
- ❌ クラス・関数検出が完全に失敗
- 🚨 根本的なPEGTL文法の見直しが必要

**次のアクション:**
1. 最小限のC++でデバッグ出力確認
2. アクション関数の動作検証
3. 必要に応じてフォールバック実装

**教訓:**
「C++の複雑さを舐めてた。でも諦めない。」