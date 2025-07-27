#pragma once

#include "language_detection.hpp"
#include "utf8_utils.hpp"
#include "types.hpp"
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace nekocode {

//=============================================================================
// 🔥 C++ Code Analyzer - 地獄のnyamesh_v22対応版
//
// 特徴:
// - 大規模C++プロジェクト対応
// - テンプレート・名前空間・継承解析
// - 1000行超えファイル対応
// - マルチファイル依存関係
//=============================================================================

class CppAnalyzer {
public:
    CppAnalyzer();
    ~CppAnalyzer();
    
    //=========================================================================
    // 🎯 Main Analysis Interface
    //=========================================================================
    
    /// C++ファイル完全解析
    CppAnalysisResult analyze_cpp_file(const std::string& content, const std::string& filename = "");
    
    /// 高速統計のみ解析
    CppAnalysisResult analyze_cpp_stats_only(const std::string& content, const std::string& filename = "");
    
    //=========================================================================
    // 🏗️ Structure Analysis
    //=========================================================================
    
    /// 名前空間解析
    std::vector<CppNamespace> analyze_namespaces(const std::string& content);
    
    /// クラス・構造体解析  
    std::vector<CppClass> analyze_classes(const std::string& content);
    
    /// 関数解析
    std::vector<CppFunction> analyze_functions(const std::string& content);
    
    /// インクルード解析
    std::vector<CppInclude> analyze_includes(const std::string& content);
    
    //=========================================================================
    // 🎯 Advanced C++ Features
    //=========================================================================
    
    /// テンプレート解析
    std::vector<CppTemplate> analyze_templates(const std::string& content);
    
    /// テンプレート・マクロ統合解析（新機能）
    TemplateAnalysisResult analyze_templates_and_macros(const std::string& content);
    
    /// 継承関係解析
    std::unordered_map<std::string, std::vector<std::string>> analyze_inheritance(const std::vector<CppClass>& classes);
    
    /// マクロ定義解析
    std::vector<std::pair<std::string, std::string>> analyze_macros(const std::string& content);
    
    /// enum解析
    std::vector<std::pair<std::string, std::vector<std::string>>> analyze_enums(const std::string& content);
    
    //=========================================================================
    // 🧮 C++ Complexity Analysis
    //=========================================================================
    
    /// C++特有複雑度計算
    ComplexityInfo calculate_cpp_complexity(const std::string& content);
    
    /// テンプレート複雑度
    uint32_t calculate_template_complexity(const std::string& content);
    
    /// 継承階層複雑度
    uint32_t calculate_inheritance_complexity(const std::vector<CppClass>& classes);
    
    //=========================================================================
    // ⚙️ Configuration
    //=========================================================================
    
    /// 解析設定更新
    void set_analysis_config(const LanguageAnalysisConfig& config);
    
    /// C++標準指定 (C++11, C++14, C++17, C++20, C++23)
    void set_cpp_standard(const std::string& standard);
    
    /// コメントスタイル指定
    enum class CommentStyle { ALL, DOXYGEN_ONLY, STANDARD_ONLY };
    void set_comment_style(CommentStyle style);

private:
    //=========================================================================
    // 🔧 Internal Implementation
    //=========================================================================
    
    LanguageAnalysisConfig config_;
    std::string cpp_standard_;
    CommentStyle comment_style_;
    
    // 正規表現パターン（UTF-8対応）
    std::regex namespace_regex_;
    std::regex class_regex_;
    std::regex struct_regex_;
    std::regex union_regex_;
    std::regex function_regex_;
    std::regex method_regex_;
    std::regex include_regex_;
    std::regex template_regex_;
    std::regex macro_regex_;
    std::regex enum_regex_;
    
    // C++キーワード・演算子
    std::unordered_set<std::string> cpp_keywords_;
    std::unordered_set<std::string> cpp_operators_;
    std::unordered_set<std::string> access_specifiers_;
    std::unordered_set<std::string> storage_specifiers_;
    
    /// 正規表現初期化
    void initialize_patterns();
    
    /// C++キーワード初期化
    void initialize_cpp_keywords();
    
    //=========================================================================
    // 🎯 Parsing Helpers
    //=========================================================================
    
    /// プリプロセッサ除去（条件コンパイル考慮）
    std::string remove_preprocessor_conditionals(const std::string& content);
    
    /// C++コメント除去（Doxygen保持オプション）
    std::string remove_cpp_comments(const std::string& content, bool preserve_doxygen = false);
    
    /// 文字列・文字リテラル除去（Raw文字列対応）
    std::string remove_cpp_literals(const std::string& content);
    
    /// ブロック境界解析（{}の対応関係）
    std::vector<std::pair<size_t, size_t>> find_block_boundaries(const std::string& content);
    
    /// テンプレート引数抽出
    std::vector<std::string> extract_template_parameters(const std::string& template_decl);
    
    /// 関数引数パース
    std::vector<std::string> parse_function_parameters(const std::string& params_str);
    
    /// 継承リスト解析
    std::vector<std::string> parse_base_classes(const std::string& inheritance_str);
    
    /// スコープ解決
    std::string resolve_scope(const std::string& name, const std::vector<CppNamespace>& namespaces);
    
    //=========================================================================
    // 🔍 Advanced Pattern Matching
    //=========================================================================
    
    /// クラスメンバ解析
    struct MemberInfo {
        std::string name;
        std::string type;
        std::string visibility;
        bool is_static = false;
        bool is_const = false;
    };
    std::vector<MemberInfo> analyze_class_members(const std::string& class_body);
    
    /// 仮想関数検出
    bool is_virtual_function(const std::string& function_decl);
    
    /// テンプレート特殊化検出
    bool is_template_specialization(const std::string& decl);
    
    /// 演算子オーバーロード検出
    bool is_operator_overload(const std::string& function_name);
    
    //=========================================================================
    // 📊 Statistics Calculation
    //=========================================================================
    
    /// C++統計計算
    void calculate_cpp_statistics(CppAnalysisResult& result);
    
    /// 複雑度統計
    void calculate_complexity_statistics(CppAnalysisResult& result, const std::string& content);
    
    /// 依存関係統計
    void calculate_dependency_statistics(CppAnalysisResult& result);
};

//=============================================================================
// 🎯 C++ Language Features Detection
//=============================================================================

class CppFeatureDetector {
public:
    /// C++標準機能検出
    struct CppFeatures {
        bool has_auto_keyword = false;
        bool has_range_based_for = false;
        bool has_lambda = false;
        bool has_smart_pointers = false;
        bool has_constexpr = false;
        bool has_nullptr = false;
        bool has_move_semantics = false;
        bool has_variadic_templates = false;
        bool has_concepts = false;          // C++20
        bool has_modules = false;           // C++20
        bool has_coroutines = false;        // C++20
        std::string estimated_standard;     // "C++11", "C++14", etc.
    };
    
    /// コード内容からC++機能を検出
    static CppFeatures detect_features(const std::string& content);
    
    /// 推定C++標準を判定
    static std::string estimate_cpp_standard(const CppFeatures& features);
};

//=============================================================================
// 🚨 C++ Specific Error Detection
//=============================================================================

class CppErrorDetector {
public:
    /// よくあるC++エラーパターン
    enum class ErrorType {
        MISSING_SEMICOLON,
        UNMATCHED_BRACES,
        MISSING_INCLUDE_GUARD,
        MEMORY_LEAK_RISK,
        UNDEFINED_BEHAVIOR,
        PERFORMANCE_ISSUE
    };
    
    struct CppError {
        ErrorType type;
        std::string message;
        uint32_t line_number;
        std::string suggestion;
    };
    
    /// C++コード品質チェック
    static std::vector<CppError> detect_common_errors(const std::string& content);
    
    /// メモリ安全性チェック
    static std::vector<CppError> check_memory_safety(const std::string& content);
    
    /// パフォーマンス問題検出
    static std::vector<CppError> detect_performance_issues(const std::string& content);
};

} // namespace nekocode