#pragma once

//=============================================================================
// 🔥 JavaScript Universal Adapter - AST革命×統一システム融合
//
// 既存のJavaScript PEGTL成功実装 → 新統一アーキテクチャへの完璧な橋渡し
// 99%共通処理 + JavaScript特化1% の理想的実現
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include "../analyzers/javascript/javascript_pegtl_analyzer.hpp"
#include "../analyzers/javascript/javascript_minimal_grammar.hpp"
#include <memory>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 JavaScript Universal Adapter - 既存成功×新統一の完璧融合
//=============================================================================

class JavaScriptUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::JavaScriptTraits> {
private:
    // 🔥 既存の成功実装を活用
    std::unique_ptr<JavaScriptPEGTLAnalyzer> legacy_analyzer;
    
public:
    JavaScriptUniversalAdapter() {
        legacy_analyzer = std::make_unique<JavaScriptPEGTLAnalyzer>();
    }
    
    virtual ~JavaScriptUniversalAdapter() = default;
    
    //=========================================================================
    // 🚀 統一インターフェース実装（完全互換性）
    //=========================================================================
    
    Language get_language() const override {
        return Language::JAVASCRIPT;
    }
    
    std::string get_language_name() const override {
        return "JavaScript (Universal AST)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".js", ".mjs", ".jsx", ".cjs"};
    }
    
    //=========================================================================
    // 🔥 ハイブリッド解析エンジン（既存PEGTL + 新AST統一）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 既存PEGTL解析で高精度結果取得
        AnalysisResult legacy_result = legacy_analyzer->analyze(content, filename);
        
        // Phase 2: 統一AST構築（既存結果からASTを逆構築）
        build_unified_ast_from_legacy_result(legacy_result, content);
        
        // Phase 3: AST統計とレガシー統計の統合
        enhance_result_with_ast_data(legacy_result);
        
        return legacy_result;
    }
    
    //=========================================================================
    // 🌳 AST革命機能（JavaScript特化拡張）
    //=========================================================================
    
    /// JavaScript特化AST検索
    ASTNode* query_javascript_ast(const std::string& path) const {
        // 既存のJavaScript成功パターンを統一システムで拡張
        return query_ast(path);
    }
    
    /// スコープチェーン解析（JavaScript特有）
    std::vector<std::string> analyze_scope_chain(std::uint32_t line_number) const {
        std::vector<std::string> scope_chain;
        ASTNode* scope = analyze_scope_at_line(line_number);
        
        // JavaScript特有のスコープチェーン構築
        while (scope && scope->type != ASTNodeType::FILE_ROOT) {
            if (!scope->name.empty()) {
                scope_chain.insert(scope_chain.begin(), scope->name);
            }
            // TODO: 親ノード辿り実装後に完全版
            break;
        }
        
        return scope_chain;
    }
    
    /// async/await解析
    std::vector<std::string> find_async_functions() const {
        std::vector<std::string> async_functions;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_async_functions_recursive(root, async_functions);
        return async_functions;
    }
    
protected:
    //=========================================================================
    // 🔄 レガシー → 統一AST変換エンジン
    //=========================================================================
    
    void build_unified_ast_from_legacy_result(const AnalysisResult& legacy_result, const std::string& content) {
        // レガシー結果から統一AST構築
        
        // クラス情報をAST化
        for (const auto& class_info : legacy_result.classes) {
            this->tree_builder.enter_scope(ASTNodeType::CLASS, class_info.name, class_info.start_line);
            
            // クラス内のメソッドを追加
            for (const auto& method : class_info.methods) {
                this->tree_builder.add_function(method.name, method.start_line);
            }
            
            this->tree_builder.exit_scope();
        }
        
        // 独立関数をAST化
        for (const auto& func_info : legacy_result.functions) {
            this->tree_builder.add_function(func_info.name, func_info.start_line);
        }
        
        // JavaScript特有の構造を解析
        analyze_javascript_specific_patterns(content);
    }
    
    void analyze_javascript_specific_patterns(const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        std::uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            // Arrow functions検出
            if (line.find("=>") != std::string::npos) {
                detect_arrow_function(line, line_number);
            }
            
            // async/await検出
            if (line.find("async") != std::string::npos || line.find("await") != std::string::npos) {
                detect_async_pattern(line, line_number);
            }
            
            // Promise chains検出
            if (line.find(".then(") != std::string::npos || line.find(".catch(") != std::string::npos) {
                this->tree_builder.add_control_structure(ASTNodeType::TRY_BLOCK, line_number);
            }
            
            line_number++;
        }
    }
    
    void detect_arrow_function(const std::string& line, std::uint32_t line_number) {
        // 簡易Arrow function検出
        size_t arrow_pos = line.find("=>");
        if (arrow_pos != std::string::npos) {
            // 関数名抽出（簡易版）
            std::string func_name = "arrow_func_" + std::to_string(line_number);
            
            // const name = ... の場合
            size_t const_pos = line.find("const ");
            if (const_pos != std::string::npos) {
                size_t name_start = const_pos + 6;
                size_t name_end = line.find(" ", name_start);
                if (name_end != std::string::npos) {
                    func_name = line.substr(name_start, name_end - name_start);
                }
            }
            
            this->tree_builder.add_function(func_name, line_number);
        }
    }
    
    void detect_async_pattern(const std::string& line, std::uint32_t line_number) {
        if (line.find("async function") != std::string::npos || 
            line.find("async ") != std::string::npos) {
            // async関数検出（既存解析でカバーされるが、メタデータ追加）   
            // TODO: メタデータ拡張実装
        }
    }
    
    void enhance_result_with_ast_data(AnalysisResult& result) {
        // AST統計を既存結果に統合
        auto ast_stats = this->tree_builder.get_ast_statistics(); 
        
        // 統計拡張（既存に追加情報）
        result.stats.class_count = std::max(result.stats.class_count, ast_stats.classes);
        result.stats.function_count = std::max(result.stats.function_count, ast_stats.functions);
        
        // JavaScript特化統計（将来拡張用）
        // result.stats.control_structure_count = ast_stats.control_structures;
        // TODO: async_function_count等の拡張統計
    }
    
    void find_async_functions_recursive(const ASTNode* node, std::vector<std::string>& async_functions) const {
        if (node->type == ASTNodeType::FUNCTION) {
            // メタデータからasync判定
            auto it = node->attributes.find("async");
            if (it != node->attributes.end() && it->second == "true") {
                async_functions.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_async_functions_recursive(child.get(), async_functions);
        }
    }
};

} // namespace adapters
} // namespace nekocode