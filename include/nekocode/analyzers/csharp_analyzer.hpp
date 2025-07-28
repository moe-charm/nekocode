#pragma once

//=============================================================================
// 🎯 C# Language Analyzer - C#専用解析エンジン
//
// C#コードの構造解析・複雑度計算
// .NET/.NET Core/Unity対応、LINQ・async/await・属性解析
//=============================================================================

#include "base_analyzer.hpp"
#include <regex>
#include <vector>
#include <unordered_set>

namespace nekocode {

//=============================================================================
// 🎯 C#固有データ構造
//=============================================================================

/// C#属性情報
struct CSharpAttribute {
    std::string name;              // 属性名 (例: ApiController)
    std::string full_expression;  // 完全な式 (例: [HttpPost("api/users")])
    uint32_t line_number = 0;
};

/// C#プロパティ情報
struct CSharpProperty {
    std::string name;
    std::string type;
    bool has_getter = false;
    bool has_setter = false;
    bool is_auto_property = false;
    std::vector<CSharpAttribute> attributes;
    uint32_t line_number = 0;
};

/// C#メソッド情報（関数情報の拡張）
struct CSharpMethod {
    std::string name;
    std::string return_type;
    std::vector<std::string> parameters;
    std::vector<CSharpAttribute> attributes;
    bool is_async = false;
    bool is_static = false;
    bool is_virtual = false;
    bool is_override = false;
    bool is_abstract = false;
    std::string access_modifier; // public, private, protected, internal
    uint32_t start_line = 0;
    uint32_t end_line = 0;
};

/// C#クラス情報（クラス情報の拡張）
struct CSharpClass {
    std::string name;
    std::string namespace_name;
    std::vector<std::string> base_classes;
    std::vector<std::string> interfaces;
    std::vector<CSharpAttribute> attributes;
    std::vector<CSharpMethod> methods;
    std::vector<CSharpProperty> properties;
    std::vector<std::string> fields;
    bool is_static = false;
    bool is_abstract = false;
    bool is_sealed = false;
    bool is_partial = false;
    std::string class_type; // class, interface, struct, enum, record
    std::string access_modifier; // public, private, protected, internal
    uint32_t start_line = 0;
    uint32_t end_line = 0;
};

/// C#using文情報
struct CSharpUsing {
    std::string namespace_or_type;
    bool is_static = false;       // using static
    bool is_alias = false;        // using Alias = Type
    std::string alias_name;       // エイリアス名
    uint32_t line_number = 0;
};

//=============================================================================
// 🎯 CSharpAnalyzer - C#専用解析クラス
//=============================================================================

class CSharpAnalyzer : public BaseAnalyzer {
public:
    CSharpAnalyzer();
    ~CSharpAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装
    //=========================================================================
    
    Language get_language() const override { return Language::CSHARP; }
    std::string get_language_name() const override { return "C#"; }
    std::vector<std::string> get_supported_extensions() const override {
        return {".cs", ".csx"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    
    //=========================================================================
    // 🎯 C#固有解析メソッド
    //=========================================================================
    
    /// C#固有の詳細解析結果を取得
    struct CSharpAnalysisResult {
        std::vector<CSharpClass> classes;
        std::vector<std::string> namespaces;
        std::vector<CSharpUsing> using_statements;
        AnalysisResult base_result; // 基本解析結果
    };
    
    CSharpAnalysisResult analyze_csharp_detailed(const std::string& content, const std::string& filename);
    
private:
    //=========================================================================
    // 🔧 内部実装
    //=========================================================================
    
    /// 正規表現パターン初期化
    void initialize_patterns();
    
    /// namespace抽出
    void extract_namespaces(const std::string& content, std::vector<std::string>& namespaces);
    
    /// using文抽出
    void extract_using_statements(const std::string& content, std::vector<CSharpUsing>& usings);
    
    /// クラス・構造体・インターフェース抽出
    void extract_classes(const std::string& content, std::vector<CSharpClass>& classes);
    
    /// メソッド抽出
    void extract_methods(const std::string& class_content, CSharpClass& class_info, uint32_t base_line);
    
    /// プロパティ抽出
    void extract_properties(const std::string& class_content, CSharpClass& class_info, uint32_t base_line);
    
    /// 属性抽出
    std::vector<CSharpAttribute> extract_attributes(const std::string& content, size_t start_pos);
    
    /// C#固有複雑度計算
    ComplexityInfo calculate_csharp_complexity(const std::string& content);
    
    /// LINQ式複雑度計算
    uint32_t calculate_linq_complexity(const std::string& content);
    
    /// async/await複雑度計算
    uint32_t calculate_async_complexity(const std::string& content);
    
    /// ジェネリクス検出
    bool has_generics(const std::string& declaration);
    
    /// null許容型検出
    bool has_nullable_types(const std::string& declaration);
    
    /// クラス終端検出（ブレース対応）
    size_t find_class_end(const std::string& content, size_t class_start);
    
    /// アクセス修飾子抽出
    std::string extract_access_modifier(const std::string& declaration);
    
    //=========================================================================
    // 📊 正規表現パターン
    //=========================================================================
    
    // 基本構造
    std::regex namespace_pattern_;
    std::regex class_pattern_;
    std::regex interface_pattern_;
    std::regex struct_pattern_;
    std::regex enum_pattern_;
    std::regex record_pattern_;
    
    // using文
    std::vector<std::regex> using_patterns_;
    
    // メソッド・プロパティ
    std::regex method_pattern_;
    std::regex property_pattern_;
    std::regex auto_property_pattern_;
    
    // 属性
    std::regex attribute_pattern_;
    std::regex attribute_multiline_pattern_;
    
    // LINQ・async
    std::regex linq_pattern_;
    std::regex async_pattern_;
    std::regex await_pattern_;
    
    // 複雑度キーワード
    std::unordered_set<std::string> complexity_keywords_;
    std::unordered_set<std::string> linq_keywords_;
};

} // namespace nekocode