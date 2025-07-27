#pragma once

//=============================================================================
// 🔥 PEGTL革命的解析エンジン - 軽量高性能パーサー統合版
//
// PEGTL (Parsing Expression Grammar Template Library) による：
// - ヘッダーオンリー軽量設計 ✅
// - PEG文法による高精度解析 ✅  
// - C++テンプレート最適化 ✅
// - Tree-sitterより軽量、正規表現より強力 ✅
//=============================================================================

#include "types.hpp"
#include "language_detection.hpp"
#include <tao/pegtl.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nekocode {

//=============================================================================
// 🧠 PEGTLAnalyzer - 革命的軽量解析エンジン
//=============================================================================

class PEGTLAnalyzer {
public:
    //=========================================================================
    // 🏗️ Construction & Destruction
    //=========================================================================
    
    PEGTLAnalyzer();
    ~PEGTLAnalyzer();
    
    // ムーブのみ対応（軽量設計）
    PEGTLAnalyzer(const PEGTLAnalyzer&) = delete;
    PEGTLAnalyzer& operator=(const PEGTLAnalyzer&) = delete;
    PEGTLAnalyzer(PEGTLAnalyzer&&) noexcept;
    PEGTLAnalyzer& operator=(PEGTLAnalyzer&&) noexcept;
    
    //=========================================================================
    // 🎯 革命的解析API - Tree-sitter互換
    //=========================================================================
    
    /// 🌟 マスター解析メソッド - Tree-sitterAPI互換
    Result<AnalysisResult> analyze(const std::string& content, 
                                   const std::string& filename,
                                   Language language = Language::UNKNOWN);
    
    /// 🚀 高速統計解析
    Result<AnalysisResult> analyze_statistics_only(const std::string& content,
                                                    const std::string& filename,
                                                    Language language);
    
    //=========================================================================
    // 🎭 言語別専用解析 - PEG文法ベース
    //=========================================================================
    
    /// C++解析 - クラス・関数・名前空間・include
    Result<AnalysisResult> analyze_cpp(const std::string& content, const std::string& filename);
    
    /// JavaScript解析 - クラス・関数・import/export
    Result<AnalysisResult> analyze_javascript(const std::string& content, const std::string& filename);
    
    /// TypeScript解析 - JavaScript + 型情報
    Result<AnalysisResult> analyze_typescript(const std::string& content, const std::string& filename);
    
    //=========================================================================
    // ⚙️ 設定・統計
    //=========================================================================
    
    /// パフォーマンス統計
    struct ParseMetrics {
        std::chrono::milliseconds parse_time{0};
        uint32_t nodes_parsed = 0;
        uint32_t bytes_processed = 0;
        bool has_errors = false;
        std::string parser_type = "PEGTL";
    };
    
    const ParseMetrics& get_last_parse_metrics() const;

private:
    //=========================================================================
    // 🔒 Internal Implementation
    //=========================================================================
    
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    //=========================================================================
    // 🎯 PEGTL文法・解析ロジック
    //=========================================================================
    
    /// C++要素抽出
    AnalysisResult extract_cpp_elements(const std::string& content);
    
    /// JavaScript要素抽出
    AnalysisResult extract_javascript_elements(const std::string& content);
    
    /// TypeScript要素抽出
    AnalysisResult extract_typescript_elements(const std::string& content);
    
    /// 複雑度計算（PEGTL文法ベース）
    ComplexityInfo calculate_complexity(const std::string& content, Language language);
    
    /// 言語自動検出
    Language detect_language_from_content(const std::string& content, const std::string& filename);
    
    /// JavaScript import正規表現検出（PEGTL補完用）
    void extract_js_imports_regex(const std::string& content, std::vector<ImportInfo>& imports);
    
    /// 正確な行番号計算
    uint32_t calculate_line_number(const std::string& content, size_t position);
};

//=============================================================================
// 🔥 C++ PEG文法定義
//=============================================================================

namespace cpp_grammar {
    using namespace tao::pegtl;
    
    // 基本要素
    struct identifier : seq<ranges<'a', 'z', 'A', 'Z', '_'>, star<ranges<'a', 'z', 'A', 'Z', '0', '9', '_'>>> {};
    struct whitespace : star<space> {};
    
    // コメント
    struct line_comment : seq<string<'/', '/'>, until<eolf>> {};
    struct block_comment : seq<string<'/', '*'>, until<string<'*', '/'>>> {};
    struct comment : sor<line_comment, block_comment> {};
    
    // include文
    struct include_path : seq<one<'"', '<'>, until<one<'"', '>'>>> {};
    struct include : seq<string<'#', 'i', 'n', 'c', 'l', 'u', 'd', 'e'>, whitespace, include_path> {};
    
    // 名前空間
    struct namespace_name : identifier {};
    struct namespace_decl : seq<string<'n', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e'>, whitespace, namespace_name, whitespace, one<'{'>> {};
    
    // クラス
    struct class_name : identifier {};
    struct class_keyword : sor<string<'c', 'l', 'a', 's', 's'>, string<'s', 't', 'r', 'u', 'c', 't'>> {};
    struct class_decl : seq<class_keyword, whitespace, class_name, whitespace, one<'{'>> {};
    
    // 関数
    struct function_name : identifier {};
    struct parameter_list : seq<one<'('>, until<one<')'>>> {};
    struct function_decl : seq<identifier, whitespace, function_name, whitespace, parameter_list> {};
    
    // メイン文法
    struct cpp_element : sor<include, namespace_decl, class_decl, function_decl, comment, any> {};
    struct cpp_grammar : star<cpp_element> {};
}

//=============================================================================
// 🔥 JavaScript PEG文法定義
//=============================================================================

namespace js_grammar {
    using namespace tao::pegtl;
    
    // 基本要素
    struct identifier : seq<ranges<'a', 'z', 'A', 'Z', '_', '$'>, star<ranges<'a', 'z', 'A', 'Z', '0', '9', '_', '$'>>> {};
    struct whitespace : star<space> {};
    
    // import/export
    struct import_path : seq<one<'"', '\''>, until<one<'"', '\''>>, one<'"', '\''>> {};
    struct import_stmt : seq<string<'i', 'm', 'p', 'o', 'r', 't'>, until<string<'f', 'r', 'o', 'm'>>, string<'f', 'r', 'o', 'm'>, whitespace, import_path> {};
    struct export_stmt : seq<string<'e', 'x', 'p', 'o', 'r', 't'>, whitespace> {};
    
    // 関数
    struct function_name : identifier {};
    struct function_keyword : sor<string<'f', 'u', 'n', 'c', 't', 'i', 'o', 'n'>, string<'a', 's', 'y', 'n', 'c'>> {};
    struct function_decl : seq<opt<function_keyword>, whitespace, function_name, whitespace, one<'('>, until<one<')'>>> {};
    
    // クラス
    struct class_name : identifier {};
    struct class_decl : seq<string<'c', 'l', 'a', 's', 's'>, whitespace, class_name, whitespace, one<'{'>> {};
    
    // メイン文法
    struct js_element : sor<import_stmt, export_stmt, class_decl, function_decl, any> {};
    struct js_grammar : star<js_element> {};
}

//=============================================================================
// 🎯 PEGTL統合ヘルパー
//=============================================================================

namespace pegtl_helper {
    
    /// PEG解析結果からAnalysisResultに変換
    AnalysisResult convert_to_analysis_result(const std::string& content, 
                                              const std::string& filename,
                                              Language language);
    
    /// PEGTL版情報
    struct VersionInfo {
        std::string version = "PEGTL-3.2.7";
        std::string engine = "PEG Template Library";
        bool header_only = true;
    };
    
    VersionInfo get_version_info();
}

} // namespace nekocode