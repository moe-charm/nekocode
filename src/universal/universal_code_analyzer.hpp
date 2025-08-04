#pragma once

//=============================================================================
// 🚀 Universal Code Analyzer - 全言語統一解析システム
//
// 99%共通処理 + 1%言語固有適応の完全実現
// 既存の重複排除と美しいアーキテクチャの融合
//=============================================================================

#include "universal_tree_builder.hpp"
#include "language_traits.hpp"
#include "../analyzers/base_analyzer.hpp"
#include <sstream>
#include <iostream>

namespace nekocode {
namespace universal {

//=============================================================================
// 🎯 Universal Code Analyzer - テンプレート統一システム
//=============================================================================

template<typename LanguageTraits>
class UniversalCodeAnalyzer : public BaseAnalyzer {
protected:
    UniversalTreeBuilder<LanguageTraits> tree_builder;
    
public:
    UniversalCodeAnalyzer() = default;
    virtual ~UniversalCodeAnalyzer() = default;
    
    //=========================================================================
    // 🔧 BaseAnalyzer実装（統一インターフェース）
    //=========================================================================
    
    Language get_language() const override {
        return LanguageTraits::get_language_enum();
    }
    
    std::string get_language_name() const override {
        return LanguageTraits::get_language_name();
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return LanguageTraits::get_supported_extensions();
    }
    
    //=========================================================================
    // 🚀 統一解析エンジン（99%共通処理）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        AnalysisResult result;
        result.language = get_language();
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.file_info.total_lines = count_lines(content);
        
        // Phase 1: 共通前処理
        preprocess_content(content, result);
        
        // Phase 2: 言語固有解析 + AST構築
        parse_and_build_ast(content, result);
        
        // Phase 3: AST → 従来形式変換（既存システム互換性）
        tree_builder.extract_to_analysis_result(result);
        
        // Phase 4: 共通後処理
        postprocess_result(result);
        
        return result;
    }
    
    //=========================================================================
    // 🌳 AST革命機能（JavaScript成功パターン統一化）
    //=========================================================================
    
    /// AST統計取得
    const ASTStatistics& get_ast_statistics() const {
        return tree_builder.get_ast_statistics();
    }
    
    /// AST検索
    ASTNode* query_ast(const std::string& path) const {
        return tree_builder.query_ast(path);
    }
    
    /// スコープ解析
    ASTNode* analyze_scope_at_line(std::uint32_t line_number) const {
        return tree_builder.get_scope_at_line(line_number);
    }
    
    /// AST構造ダンプ
    std::string dump_ast(const std::string& format = "tree") const {
        return format_ast_output(tree_builder.get_ast_root(), format);
    }
    
protected:
    //=========================================================================
    // 🔧 共通処理フェーズ（99%共通）
    //=========================================================================
    
    /// Phase 1: 共通前処理
    virtual void preprocess_content(const std::string& content, AnalysisResult& result) {
        // コメント抽出（全言語共通パターン）
        extract_comments(content, result);
        
        // 基本複雑度計算
        result.complexity = calculate_complexity(content);
    }
    
    /// Phase 2: 解析 + AST構築（言語固有 + 共通パターン）
    virtual void parse_and_build_ast(const std::string& content, AnalysisResult& result) {
        std::istringstream stream(content);
        std::string line;
        std::uint32_t line_number = 1;
        std::uint32_t current_indent = 0;
        
        while (std::getline(stream, line)) {
            // インデント検出（Python等で重要）
            current_indent = detect_indentation(line);
            
            // 言語固有パターンマッチング
            analyze_line(line, line_number, current_indent);
            
            line_number++;
        }
    }
    
    /// Phase 4: 共通後処理
    virtual void postprocess_result(AnalysisResult& result) {
        // 統計更新
        result.update_statistics();
        
        // 複雑度再計算（AST情報考慮）
        enhance_complexity_with_ast(result);
    }
    
    //=========================================================================
    // 🎯 言語固有解析（Template Method Pattern）
    //=========================================================================
    
    /// 行解析（言語固有実装）
    virtual void analyze_line(const std::string& line, std::uint32_t line_number, std::uint32_t indent) {
        // トークン分割
        auto tokens = tokenize_line(line);
        if (tokens.empty()) return;
        
        // パターンマッチング（言語固有）
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // 関数検出
            if (LanguageTraits::is_function_keyword(token)) {
                handle_function_pattern(tokens, i, line_number);
            }
            // クラス検出
            else if (LanguageTraits::is_class_keyword(token)) {
                handle_class_pattern(tokens, i, line_number);
            }
            // 制御構造検出
            else if (LanguageTraits::is_control_keyword(token)) {
                handle_control_pattern(tokens, i, line_number);
            }
        }
    }
    
    //=========================================================================
    // 🛠️ パターンハンドラー（共通ロジック + 言語固有適応）
    //=========================================================================
    
    virtual void handle_function_pattern(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // 共通関数検出ロジック
        std::string function_name = extract_function_name(tokens, index);
        if (!function_name.empty()) {
            tree_builder.add_function(function_name, line_number);
        }
    }
    
    virtual void handle_class_pattern(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // 共通クラス検出ロジック
        std::string class_name = extract_class_name(tokens, index);
        if (!class_name.empty()) {
            tree_builder.enter_scope(ASTNodeType::CLASS, class_name, line_number);
        }
    }
    
    virtual void handle_control_pattern(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // 共通制御構造検出
        const auto& keyword = tokens[index];
        ASTNodeType control_type = map_control_keyword_to_type(keyword);
        tree_builder.add_control_structure(control_type, line_number);
    }
    
    //=========================================================================
    // 🧰 ユーティリティ（共通処理）
    //=========================================================================
    
    std::uint32_t count_lines(const std::string& content) const {
        return std::count(content.begin(), content.end(), '\n') + 1;
    }
    
    std::uint32_t detect_indentation(const std::string& line) const {
        std::uint32_t indent = 0;
        for (char c : line) {
            if (c == ' ') indent++;
            else if (c == '\t') indent += 4; // タブ=4スペース換算
            else break;
        }
        return indent;
    }
    
    std::vector<std::string> tokenize_line(const std::string& line) const {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    std::string extract_function_name(const std::vector<std::string>& tokens, size_t index) const {
        // 次のトークンが関数名の可能性が高い
        if (index + 1 < tokens.size()) {
            std::string candidate = tokens[index + 1];
            // 基本的なバリデーション
            if (!candidate.empty() && std::isalpha(candidate[0])) {
                return candidate;
            }
        }
        return "";
    }
    
    std::string extract_class_name(const std::vector<std::string>& tokens, size_t index) const {
        // 次のトークンがクラス名
        if (index + 1 < tokens.size()) {
            return tokens[index + 1];
        }
        return "";
    }
    
    ASTNodeType map_control_keyword_to_type(const std::string& keyword) const {
        if (keyword == "if") return ASTNodeType::IF_STATEMENT;
        if (keyword == "for" || keyword == "foreach") return ASTNodeType::FOR_LOOP;
        if (keyword == "while") return ASTNodeType::WHILE_LOOP;
        if (keyword == "switch") return ASTNodeType::SWITCH_STATEMENT;
        if (keyword == "try") return ASTNodeType::TRY_BLOCK;
        if (keyword == "catch" || keyword == "except") return ASTNodeType::CATCH_BLOCK;
        return ASTNodeType::UNKNOWN;
    }
    
    void extract_comments(const std::string& content, AnalysisResult& result) {
        // TODO: 共通コメント抽出（既存実装から統一化）
    }
    
    void enhance_complexity_with_ast(AnalysisResult& result) {
        // TODO: AST情報を使った高度な複雑度計算
    }
    
    std::string format_ast_output(const ASTNode* root, const std::string& format) const {
        // TODO: AST可視化（JavaScript実装から統一化）
        return "AST output format: " + format;
    }
};

//=============================================================================
// 🎯 Language-Specific Extensions（必要に応じて追加）
//=============================================================================

} // namespace universal
} // namespace nekocode