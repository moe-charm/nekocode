#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include "types.hpp"

namespace nekocode {

//=============================================================================
// 🌍 Multi-Language Support System
//
// 対応言語:
// - JavaScript (.js, .mjs, .jsx)
// - TypeScript (.ts, .tsx)  
// - C++ (.cpp, .cxx, .cc, .hpp, .h)
// - C (.c, .h)
// - C# (.cs, .csx)
//=============================================================================

// Language enum は types.hpp で定義済み

/// 言語情報構造体
struct LanguageInfo {
    Language type;
    std::string name;
    std::string display_name;
    std::vector<std::string> extensions;
    std::vector<std::string> keywords;
    std::vector<std::string> comment_patterns;
    
    LanguageInfo() = default;
    LanguageInfo(Language t, const std::string& n, const std::string& dn)
        : type(t), name(n), display_name(dn) {}
};

//=============================================================================
// 🎯 Language Detection Engine
//=============================================================================

class LanguageDetector {
private:
    std::unordered_map<std::string, Language> extension_map_;
    std::unordered_map<Language, LanguageInfo> language_info_;
    
public:
    LanguageDetector();
    
    /// ファイル拡張子から言語検出
    Language detect_by_extension(const std::filesystem::path& file_path) const;
    
    /// ファイル内容から言語検出（補助的）
    Language detect_by_content(const std::string& content) const;
    
    /// 総合的な言語検出
    Language detect_language(const std::filesystem::path& file_path, const std::string& content = "") const;
    
    /// 言語情報取得
    const LanguageInfo& get_language_info(Language lang) const;
    
    /// サポート言語一覧
    std::vector<Language> get_supported_languages() const;
    
    /// 拡張子フィルタ生成
    std::vector<std::string> get_extensions_for_language(Language lang) const;
    std::vector<std::string> get_all_supported_extensions() const;
    
private:
    void initialize_language_data();
    Language detect_cpp_variant(const std::string& content) const;
    Language detect_js_variant(const std::string& content) const;
};

//=============================================================================
// 🎯 Language-Specific Analysis Configuration
//=============================================================================

/// 言語別解析設定
struct LanguageAnalysisConfig {
    Language language;
    
    // 解析オプション
    bool analyze_classes = true;
    bool analyze_functions = true;
    bool analyze_namespaces = true;
    bool analyze_templates = false;      // C++のみ
    bool analyze_inheritance = true;
    bool analyze_includes = true;        // C++のみ
    bool analyze_imports = true;         // JS/TSのみ
    bool analyze_exports = true;         // JS/TSのみ
    
    // 複雑度計算
    bool calculate_cyclomatic = true;
    bool calculate_cognitive = true;
    bool calculate_nesting = true;
    
    // 詳細オプション
    bool include_private_members = true;
    bool include_static_analysis = false;
    bool include_dependency_graph = false;
    
    static LanguageAnalysisConfig for_language(Language lang);
};

//=============================================================================
// 🏗️ Multi-Language AST Structures
//=============================================================================

/// 言語共通の基底構造
struct LanguageElement {
    std::string name;
    Language source_language;
    uint32_t start_line = 0;
    uint32_t end_line = 0;
    std::string visibility;  // public, private, protected, etc.
    
    LanguageElement() = default;
    LanguageElement(Language lang) : source_language(lang) {}
};

/// C++専用構造

// C++名前空間
struct CppNamespace : public LanguageElement {
    std::vector<std::string> nested_namespaces;
    bool is_anonymous = false;
    
    CppNamespace() : LanguageElement(Language::CPP) {}
};

// C++テンプレート情報
struct CppTemplate {
    std::string name;
    std::string type;  // "class", "function"
    std::vector<std::string> parameters;
    std::vector<std::string> specializations;
    bool is_variadic = false;
};

// C++クラス（構造体含む）
struct CppClass : public LanguageElement {
    enum Type { CLASS, STRUCT, UNION };
    
    Type class_type = CLASS;
    std::vector<std::string> base_classes;
    std::vector<std::string> virtual_base_classes;
    std::vector<FunctionInfo> methods;
    std::vector<std::string> member_variables;
    std::vector<CppClass> nested_classes;
    CppTemplate template_info;
    bool is_template = false;
    bool is_abstract = false;
    
    CppClass() : LanguageElement(Language::CPP) {}
};

// C++関数
struct CppFunction : public LanguageElement {
    std::vector<std::string> parameters;
    std::string return_type;
    std::string scope;  // global, class_name::, namespace_name::
    CppTemplate template_info;
    bool is_template = false;
    bool is_virtual = false;
    bool is_pure_virtual = false;
    bool is_static = false;
    bool is_const = false;
    bool is_inline = false;
    bool is_constexpr = false;
    
    CppFunction() : LanguageElement(Language::CPP) {}
};

// C++インクルード
struct CppInclude {
    std::string path;
    bool is_system_include = false;  // <> vs ""
    uint32_t line_number = 0;
};

// C++マクロ情報
struct CppMacro {
    std::string name;
    std::string definition;
    std::vector<std::string> parameters;
    uint32_t line_number = 0;
    bool is_function_like = false;
};

// テンプレート・マクロ解析結果
struct TemplateAnalysisResult {
    std::vector<CppTemplate> templates;
    std::vector<CppMacro> macros;
    std::vector<std::string> template_specializations;
    std::vector<std::string> variadic_templates;
    uint32_t template_instantiation_count = 0;
    uint32_t macro_expansion_count = 0;
};

//=============================================================================
// 🎯 Extended Analysis Results for Multi-Language
//=============================================================================

/// C++解析結果
struct CppAnalysisResult : public AnalysisResult {
    // 基本情報は継承 (file_info, language)
    
    // C++特有構造
    std::vector<CppNamespace> namespaces;
    std::vector<CppClass> cpp_classes;        // C++特有のクラス情報
    std::vector<CppFunction> cpp_functions;   // C++特有の関数情報
    std::vector<CppInclude> includes;
    
    // テンプレート・マクロ解析（オプション）
    TemplateAnalysisResult template_analysis;
    
    // 統計
    struct CppStatistics {
        uint32_t namespace_count = 0;
        uint32_t class_count = 0;
        uint32_t struct_count = 0;
        uint32_t union_count = 0;
        uint32_t function_count = 0;
        uint32_t template_count = 0;
        uint32_t include_count = 0;
        uint32_t private_member_count = 0;
        uint32_t public_member_count = 0;
        uint32_t virtual_function_count = 0;
    } cpp_stats;
    
    // 複雑度
    ComplexityInfo complexity;
    
    // 生成時刻
    Timestamp generated_at;
    
    CppAnalysisResult() : generated_at(std::chrono::system_clock::now()) {
        language = Language::CPP;
    }
    
    void update_statistics();
};

/// 統合解析結果（全言語対応）
struct MultiLanguageAnalysisResult {
    Language detected_language;
    
    // 言語別結果（unionの代わりにoptional使用）
    std::optional<AnalysisResult> js_result;      // JavaScript/TypeScript
    std::optional<CppAnalysisResult> cpp_result;  // C++/C
    std::optional<AnalysisResult> csharp_result;  // C#
    
    // 共通メタデータ
    FileInfo file_info;
    Timestamp analyzed_at;
    
    MultiLanguageAnalysisResult() : analyzed_at(std::chrono::system_clock::now()) {}
    
    /// 有効な解析結果があるかチェック
    bool has_result() const {
        return js_result.has_value() || cpp_result.has_value() || csharp_result.has_value();
    }
    
    /// 複雑度取得（言語共通）
    ComplexityInfo get_complexity() const {
        if (js_result) return js_result->complexity;
        if (cpp_result) return cpp_result->complexity;
        if (csharp_result) return csharp_result->complexity;
        return ComplexityInfo{};
    }
};

} // namespace nekocode