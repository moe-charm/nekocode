#pragma once

//=============================================================================
// 🦀 Rust Language Analyzer
//
// Rust言語の構造解析器
// - 関数定義 (fn, async fn, unsafe fn)
// - 構造体・列挙型 (struct, enum)
// - トレイト・impl (trait, impl)
// - マクロ (macro_rules!)
// - モジュール (mod, use)
//=============================================================================

#include "base_analyzer.hpp"
// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"
#include <regex>
#include <string>
#include <vector>
#include <unordered_map>

namespace nekocode {

//=============================================================================
// 🦀 Rust特有の構造定義
//=============================================================================

struct RustFunctionInfo {
    std::string name;
    LineNumber line_number = 0;
    LineNumber end_line = 0;                // 🎯 end_line追加
    bool is_async = false;
    bool is_unsafe = false;
    bool is_pub = false;
    bool is_const = false;
    std::vector<std::string> generics;      // <T, U>
    std::vector<std::string> lifetimes;     // 'a, 'b
    std::string return_type;                // -> i32
    ComplexityInfo complexity;              // 🔧 関数別複雑度追加
};

struct TraitInfo {
    std::string name;
    LineNumber line_number = 0;
    bool is_pub = false;
    std::vector<std::string> generics;
    std::vector<std::string> methods;       // トレイトメソッド
};

struct ImplInfo {
    std::string struct_name;                // impl対象
    std::string trait_name;                 // impl Trait for Struct
    LineNumber line_number = 0;
    std::vector<std::string> methods;       // 実装メソッド
};

struct MacroInfo {
    std::string name;
    LineNumber line_number = 0;
    bool is_declarative = true;             // macro_rules! vs proc macro
};

struct StructInfo {
    std::string name;
    LineNumber line_number = 0;
    bool is_pub = false;
    std::vector<std::string> fields;
    std::vector<std::string> generics;
};

struct EnumInfo {
    std::string name;
    LineNumber line_number = 0;
    bool is_pub = false;
    std::vector<std::string> variants;
    std::vector<std::string> generics;
};

//=============================================================================
// 🦀 Rust Analyzer 本体
//=============================================================================

class RustAnalyzer : public BaseAnalyzer {
public:
    RustAnalyzer() = default;
    ~RustAnalyzer() = default;
    
    Language get_language() const override {
        return Language::RUST;
    }
    
    std::string get_language_name() const override {
        return "Rust";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".rs"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;

private:
    // Rust特有の解析結果
    std::vector<RustFunctionInfo> rust_functions_;
    std::vector<StructInfo> structs_;
    std::vector<EnumInfo> enums_;
    std::vector<TraitInfo> traits_;
    std::vector<ImplInfo> impls_;
    std::vector<MacroInfo> macros_;
    
    // 🚀 Phase 5: Universal Symbol直接生成
    std::shared_ptr<SymbolTable> symbol_table_;         // Universal Symbolテーブル  
    std::unordered_map<std::string, int> id_counters_;  // ID生成用カウンター
    
    // 解析メソッド
    void analyze_functions(const std::string& content);
    void analyze_structs(const std::string& content);
    void analyze_enums(const std::string& content);
    void analyze_traits(const std::string& content);
    void analyze_impls(const std::string& content);
    void analyze_macros(const std::string& content);
    void analyze_modules(const std::string& content, AnalysisResult& result);
    void analyze_use_statements(const std::string& content, AnalysisResult& result);
    
    // 🎯 メンバ変数検出（新機能）
    void detect_member_variables(AnalysisResult& result, const std::string& content);
    
    // 複雑度計算
    ComplexityInfo calculate_rust_complexity(const std::string& content);
    ComplexityInfo calculate_function_complexity(const std::string& function_body);  // 🔧 個別関数用
    
    // ヘルパー関数
    std::string extract_generics(const std::string& line, size_t start_pos);
    std::string extract_return_type(const std::string& line, size_t fn_pos);
    std::vector<std::string> extract_lifetimes(const std::string& generics);
    LineNumber find_function_end_line(const std::vector<std::string>& lines, size_t start_line);  // 🎯 end_line計算
    std::string extract_function_body(const std::string& content, size_t fn_start_line);  // 🔧 関数ボディ抽出
    
    // 🆕 Phase 1: impl分類修正機能
    void fix_impl_method_classification(AnalysisResult& result);
    ClassInfo* find_struct_in_classes(std::vector<ClassInfo>& classes, const std::string& struct_name);
    
    // 🚀 Phase 5: Universal Symbol生成メソッド
    void initialize_symbol_table();
    std::string generate_unique_id(const std::string& base);
    void add_test_struct_symbol(const std::string& struct_name, std::uint32_t start_line);
    void add_test_enum_symbol(const std::string& enum_name, std::uint32_t start_line);
    void add_test_function_symbol(const std::string& function_name, std::uint32_t start_line);
};

} // namespace nekocode