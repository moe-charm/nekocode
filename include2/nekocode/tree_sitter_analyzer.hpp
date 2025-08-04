#pragma once

//=============================================================================
// 🌳 Tree-sitter革命的解析エンジン - 正規表現地獄からの完全脱出
//
// Tree-sitter様の力で：
// - 正確無比なAST解析 ✅
// - エラー耐性完璧 ✅  
// - インクリメンタル解析 ✅
// - 多言語統一API ✅
// - 保守性革命的向上 ✅
//=============================================================================

#include "types.hpp"
#include "language_detection.hpp"
#include <tree-sitter/api.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace nekocode {

//=============================================================================
// 🌍 Tree-sitter言語パーサー
//=============================================================================

// 🌳 本物Tree-sitter言語パーサー関数宣言
extern "C" {
    // Tree-sitter language parsers - 本物のパーサー！
    const TSLanguage *tree_sitter_javascript();
    const TSLanguage *tree_sitter_typescript();
    const TSLanguage *tree_sitter_cpp();
}

//=============================================================================
// 🧠 TreeSitterAnalyzer - 革命的解析エンジン
//=============================================================================

class TreeSitterAnalyzer {
public:
    //=========================================================================
    // 🏗️ Construction & Destruction
    //=========================================================================
    
    TreeSitterAnalyzer();
    ~TreeSitterAnalyzer();
    
    // コピー・ムーブ（RAII設計）
    TreeSitterAnalyzer(const TreeSitterAnalyzer&) = delete;
    TreeSitterAnalyzer& operator=(const TreeSitterAnalyzer&) = delete;
    TreeSitterAnalyzer(TreeSitterAnalyzer&&) noexcept;
    TreeSitterAnalyzer& operator=(TreeSitterAnalyzer&&) noexcept;
    
    //=========================================================================
    // 🎯 革命的解析API
    //=========================================================================
    
    /// 🌟 マスター解析メソッド - あらゆる言語を統一API で解析
    Result<AnalysisResult> analyze(const std::string& content, 
                                   const std::string& filename,
                                   Language language = Language::UNKNOWN);
    
    /// 🚀 高速統計解析 - AST統計のみ高速取得
    Result<AnalysisResult> analyze_statistics_only(const std::string& content,
                                                    const std::string& filename,
                                                    Language language);
    
    /// 🔍 言語自動検出解析
    Result<AnalysisResult> analyze_auto_detect(const std::string& content,
                                                const std::string& filename);
    
    //=========================================================================
    // 🎭 言語別専用解析
    //=========================================================================
    
    /// JavaScript解析
    Result<AnalysisResult> analyze_javascript(const std::string& content, const std::string& filename);
    
    /// TypeScript解析  
    Result<AnalysisResult> analyze_typescript(const std::string& content, const std::string& filename);
    
    /// C++解析
    Result<AnalysisResult> analyze_cpp(const std::string& content, const std::string& filename);
    
    //=========================================================================
    // 🌳 AST操作・検査
    //=========================================================================
    
    /// AST構造をJSON形式で出力（デバッグ用）
    std::string dump_ast_json(const std::string& content, Language language);
    
    /// AST構造を美しいツリー形式で出力（Human用）
    std::string dump_ast_tree(const std::string& content, Language language);
    
    /// ノード統計取得
    struct ASTStats {
        uint32_t total_nodes = 0;
        uint32_t max_depth = 0;
        uint32_t error_nodes = 0;
        std::unordered_map<std::string, uint32_t> node_type_counts;
    };
    
    ASTStats get_ast_statistics(const std::string& content, Language language);
    
    //=========================================================================
    // ⚙️ 設定・最適化
    //=========================================================================
    
    /// エラー回復モード設定
    void set_error_recovery_enabled(bool enabled);
    
    /// インクリメンタル解析有効化
    void enable_incremental_parsing(bool enabled);
    
    /// パフォーマンス統計
    struct ParseMetrics {
        std::chrono::milliseconds parse_time{0};
        uint32_t nodes_parsed = 0;
        uint32_t bytes_processed = 0;
        bool has_errors = false;
    };
    
    const ParseMetrics& get_last_parse_metrics() const;

private:
    //=========================================================================
    // 🔒 Internal Implementation
    //=========================================================================
    
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    //=========================================================================
    // 🎯 AST走査・要素抽出
    //=========================================================================
    
    /// JavaScript AST走査
    AnalysisResult extract_javascript_elements(TSNode root, const std::string& content);
    
    /// TypeScript AST走査
    AnalysisResult extract_typescript_elements(TSNode root, const std::string& content);
    
    /// C++ AST走査
    AnalysisResult extract_cpp_elements(TSNode root, const std::string& content);
    
    /// 共通要素抽出
    void extract_functions(TSNode node, const std::string& content, 
                          std::vector<FunctionInfo>& functions);
    void extract_classes(TSNode node, const std::string& content,
                         std::vector<ClassInfo>& classes);
    void extract_imports_exports(TSNode node, const std::string& content,
                                std::vector<ImportInfo>& imports,
                                std::vector<ExportInfo>& exports);
    
    // クラスメソッド抽出ヘルパー
    void extract_class_methods(TSNode class_body, const std::string& content,
                              std::vector<FunctionInfo>& methods);
    void extract_cpp_class_methods(TSNode field_list, const std::string& content,
                                   std::vector<FunctionInfo>& methods);
    
    /// 複雑度計算（AST基盤）
    ComplexityInfo calculate_ast_complexity(TSNode root);
    
    /// 複雑度計算（コンテンツ基盤）
    ComplexityInfo calculate_content_complexity(const std::string& content);
    
    /// ユーティリティ
    std::string get_node_text(TSNode node, const std::string& content);
    uint32_t get_node_line_number(TSNode node);
    Language detect_language_from_ast(TSNode root);
};

//=============================================================================
// 🎯 Tree-sitter統合ヘルパー
//=============================================================================

namespace tree_sitter {
    
    /// 言語からTSLanguage取得
    const TSLanguage* get_language(Language lang);
    
    /// サポート言語一覧
    std::vector<Language> get_supported_languages();
    
    /// 言語名取得
    std::string get_language_name(Language lang);
    
    /// Tree-sitterバージョン情報
    struct VersionInfo {
        uint32_t major;
        uint32_t minor; 
        uint32_t patch;
        std::string version_string;
    };
    
    VersionInfo get_version_info();
    
    /// パーサー統計
    struct ParserStats {
        uint32_t total_parsers_loaded = 0;
        std::vector<std::string> available_languages;
        size_t memory_usage_bytes = 0;
    };
    
    ParserStats get_parser_statistics();
}

} // namespace nekocode