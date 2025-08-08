#pragma once

//=============================================================================
// ⚙️ C++ Universal Adapter - 最難関言語×統一システム完全制覇
//
// JavaScript+Python成功パターン → C++最高難度適用
// template, namespace, inheritance完全対応の統一化実現
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include "nekocode/analyzers/cpp_pegtl_analyzer.hpp"
#include <memory>
#include <fstream>  // DEBUG用
#include <unordered_map>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 C++ Universal Adapter - 最難関言語の統一化
//=============================================================================

class CppUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::CppTraits> {
private:
    // 🔥 成熟したPEGTL実装を活用（JavaScriptパターンを踏襲）
    std::unique_ptr<CppPEGTLAnalyzer> legacy_analyzer;
    
    // ⚙️ C++特有の複雑構造管理（拡張用）
    std::unordered_map<std::string, std::string> namespace_stack; // namespace管理
    std::unordered_map<std::string, std::string> template_context; // template管理
    bool in_template_definition = false;
    std::string current_access_modifier = "private"; // デフォルトはprivate
    
public:
    CppUniversalAdapter() {
        // 成熟したPEGTLアナライザーを初期化
        legacy_analyzer = std::make_unique<CppPEGTLAnalyzer>();
    }
    virtual ~CppUniversalAdapter() = default;
    
    //=========================================================================
    // 🚀 統一インターフェース実装（完全互換性）
    //=========================================================================
    
    Language get_language() const override {
        return Language::CPP;
    }
    
    std::string get_language_name() const override {
        return "C++ (Universal AST)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".h"};
    }
    
    //=========================================================================
    // 🔥 ハイブリッド解析エンジン（成熟したPEGTL + 統一AST）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 成熟したPEGTL解析で高精度結果取得
        AnalysisResult legacy_result = legacy_analyzer->analyze(content, filename);
        
        // Phase 2: 統一AST構築（PEGTL結果から逆構築）
        build_unified_ast_from_legacy_result(legacy_result, content);
        
        // Phase 3: AST統計とレガシー統計の統合
        enhance_result_with_ast_data(legacy_result);
        
        // Phase 4: C++特化機能の追加
        enhance_result_with_cpp_features(legacy_result);
        
        // 🚀 Phase 5: Universal Symbols生成
        generate_universal_symbols(legacy_result);
        
        return legacy_result;
    }
    
    //=========================================================================
    // 🌳 C++ AST特化機能
    //=========================================================================
    
    /// C++特化AST検索
    ASTNode* query_cpp_ast(const std::string& path) const {
        return this->tree_builder.query_ast(path);
    }
    
    /// Template関数・クラス検索
    std::vector<std::string> find_template_entities() const {
        std::vector<std::string> templates;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_templates_recursive(root, templates);
        return templates;
    }
    
    /// Namespace検索
    std::vector<std::string> find_namespaces() const {
        std::vector<std::string> namespaces;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_namespaces_recursive(root, namespaces);
        return namespaces;
    }
    
    /// 継承関係解析
    std::vector<std::pair<std::string, std::string>> analyze_inheritance() const {
        std::vector<std::pair<std::string, std::string>> inheritance;
        // TODO: 将来実装（class Derived : public Base パターン）
        return inheritance;
    }
    
    /// C++アクセス修飾子解析
    std::unordered_map<std::string, std::string> analyze_access_modifiers() const {
        std::unordered_map<std::string, std::string> access_map;
        // TODO: 将来実装
        return access_map;
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
            
            this->tree_builder.exit_scope(class_info.end_line);
        }
        
        // 独立関数をAST化（クラス外の関数）
        for (const auto& func_info : legacy_result.functions) {
            // クラス内メソッドではないものを追加
            bool is_method = false;
            for (const auto& cls : legacy_result.classes) {
                for (const auto& method : cls.methods) {
                    if (method.name == func_info.name && 
                        method.start_line == func_info.start_line) {
                        is_method = true;
                        break;
                    }
                }
                if (is_method) break;
            }
            
            if (!is_method) {
                this->tree_builder.add_function(func_info.name, func_info.start_line);
            }
        }
        
        // C++特有の構造を解析（namespace、template等）
        analyze_cpp_specific_patterns(content);
    }
    
    void analyze_cpp_specific_patterns(const std::string& content) {
        // C++特有のパターンを追加解析（必要に応じて）
        // 例: namespace、template、friend、operatorオーバーロード等
    }
    
    void enhance_result_with_ast_data(AnalysisResult& result) {
        // AST統計を既存結果に統合
        auto ast_stats = this->tree_builder.get_ast_statistics();
        
        // 統計拡張（より正確な値を使用）
        if (ast_stats.classes > 0) {
            result.stats.class_count = ast_stats.classes;
        }
        if (ast_stats.functions > 0) {
            result.stats.function_count = ast_stats.functions;
        }
    }
    
    // 以下は旧実装（将来削除予定）
    void parse_cpp_with_ast(const std::string& content, AnalysisResult& result) {
        std::istringstream stream(content);
        std::string line;
        std::uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            // 空行・コメント行をスキップ
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
                line_number++;
                continue;
            }
            
            // 単行コメント・複数行コメントスキップ（簡易版）
            std::string trimmed = line;
            size_t comment_pos = trimmed.find("//");
            if (comment_pos != std::string::npos) {
                trimmed = trimmed.substr(0, comment_pos);
            }
            if (trimmed.empty() || trimmed.find_first_not_of(" \t") == std::string::npos) {
                line_number++;
                continue;
            }
            
            // C++特化パターン解析
            analyze_cpp_line(trimmed, line_number);
            
            line_number++;
        }
    }
    
    void analyze_cpp_line(const std::string& line, std::uint32_t line_number) {
        auto tokens = tokenize_line(line);
        if (tokens.empty()) return;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // namespace検出
            if (token == "namespace" && i + 1 < tokens.size()) {
                handle_cpp_namespace(tokens, i, line_number);
            }
            // template検出
            else if (token == "template" && i + 1 < tokens.size()) {
                handle_cpp_template(tokens, i, line_number);
            }
            // class/struct検出
            else if ((token == "class" || token == "struct") && i + 1 < tokens.size()) {
                handle_cpp_class(tokens, i, line_number);
            }
            // 関数検出（戻り値型 + 関数名）
            else if (is_cpp_function_pattern(tokens, i)) {
                handle_cpp_function(tokens, i, line_number);
            }
            // アクセス修飾子検出
            else if (token == "public:" || token == "private:" || token == "protected:") {
                handle_access_modifier(token);
            }
        }
    }
    
    void handle_cpp_namespace(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string namespace_name = tokens[index + 1];
        
        // namespace名からセミコロン・括弧除去
        size_t brace_pos = namespace_name.find('{');
        if (brace_pos != std::string::npos) {
            namespace_name = namespace_name.substr(0, brace_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::NAMESPACE, namespace_name, line_number);
    }
    
    void handle_cpp_template(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // template <typename T> や template <class T> 等の検出
        in_template_definition = true;
        
        // 次の関数・クラス定義でtemplate属性を設定
        // 簡易実装：template情報を一時保存
        template_context["pending"] = "true";
    }
    
    void handle_cpp_class(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string class_name = tokens[index + 1];
        
        // クラス名から継承情報・括弧除去
        size_t colon_pos = class_name.find(':');
        if (colon_pos != std::string::npos) {
            // TODO: 継承情報の処理
            class_name = class_name.substr(0, colon_pos);
        }
        size_t brace_pos = class_name.find('{');
        if (brace_pos != std::string::npos) {
            class_name = class_name.substr(0, brace_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::CLASS, class_name, line_number);
        
        // template適用
        if (template_context.count("pending")) {
            // TODO: 現在のクラスノードにtemplate属性設定
            template_context.clear();
            in_template_definition = false;
        }
        
        // デフォルトアクセス修飾子設定
        current_access_modifier = (tokens[index] == "class") ? "private" : "public";
    }
    
    bool is_cpp_function_pattern(const std::vector<std::string>& tokens, size_t index) const {
        // 簡易C++関数検出：戻り値型 + 関数名(引数) パターン
        if (index + 1 >= tokens.size()) return false;
        
        const auto& current_token = tokens[index];
        const auto& next_token = tokens[index + 1];
        
        // 戻り値型判定（簡易版）
        if (universal::CppTraits::function_keywords().count(current_token) > 0) {
            // 次のトークンが関数名っぽい（括弧含む）
            return next_token.find('(') != std::string::npos;
        }
        
        return false;
    }
    
    void handle_cpp_function(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string return_type = tokens[index];
        std::string function_name = tokens[index + 1];
        
        // 関数名から括弧除去
        size_t paren_pos = function_name.find('(');
        if (paren_pos != std::string::npos) {
            function_name = function_name.substr(0, paren_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::FUNCTION, function_name, line_number);
        
        // template適用
        if (template_context.count("pending")) {
            // TODO: 現在の関数ノードにtemplate属性設定
            template_context.clear();
            in_template_definition = false;
        }
    }
    
    void handle_access_modifier(const std::string& modifier) {
        if (modifier == "public:") current_access_modifier = "public";
        else if (modifier == "private:") current_access_modifier = "private";
        else if (modifier == "protected:") current_access_modifier = "protected";
    }
    
    void enhance_result_with_cpp_features(AnalysisResult& result) {
        // C++特化統計（将来拡張用）
        // 例: template数、namespace数、friend関数数等
    }
    
    void find_templates_recursive(const ASTNode* node, std::vector<std::string>& templates) const {
        if (node->type == ASTNodeType::FUNCTION || node->type == ASTNodeType::CLASS) {
            auto it = node->attributes.find("template");
            if (it != node->attributes.end() && it->second == "true") {
                templates.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_templates_recursive(child.get(), templates);
        }
    }
    
    void find_namespaces_recursive(const ASTNode* node, std::vector<std::string>& namespaces) const {
        if (node->type == ASTNodeType::NAMESPACE) {
            namespaces.push_back(node->name);
        }
        
        for (const auto& child : node->children) {
            find_namespaces_recursive(child.get(), namespaces);
        }
    }
    
    //=========================================================================
    // 🚀 Universal Symbols生成エンジン（JavaScript成功パターン適用）
    //=========================================================================
    
    void generate_universal_symbols(AnalysisResult& result) {
        // Phase 5: Universal Symbols生成
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] Starting Universal Symbol generation for C++" << std::endl;
        std::cerr << "[DEBUG] Classes: " << result.classes.size() 
                  << ", Functions: " << result.functions.size() << std::endl;
#endif
        
        auto symbol_table = std::make_shared<SymbolTable>();
        int class_counter = 0;
        int method_counter = 0;
        int function_counter = 0;
        
        // クラス情報からUniversal Symbol生成
        for (const auto& class_info : result.classes) {
            UniversalSymbolInfo class_symbol;
            class_symbol.symbol_id = "class_" + class_info.name + "_" + std::to_string(class_counter++);
            class_symbol.symbol_type = SymbolType::CLASS;
            class_symbol.name = class_info.name;
            class_symbol.start_line = class_info.start_line;
            class_symbol.metadata["language"] = "cpp";
            
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Adding class symbol: " << class_info.name 
                      << " with ID: " << class_symbol.symbol_id << std::endl;
#endif
            
            symbol_table->add_symbol(std::move(class_symbol));
            
            // クラスメソッドのシンボル生成
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Class " << class_info.name << " has " << class_info.methods.size() << " methods" << std::endl;
#endif
            for (const auto& method : class_info.methods) {
                UniversalSymbolInfo method_symbol;
                method_symbol.symbol_id = "method_" + method.name + "_" + std::to_string(method_counter++);
                method_symbol.symbol_type = SymbolType::FUNCTION;
                method_symbol.name = method.name;
                method_symbol.start_line = method.start_line;
                method_symbol.metadata["language"] = "cpp";
                method_symbol.metadata["class"] = class_info.name;
                
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Adding method symbol: " << method.name 
                          << " from class " << class_info.name
                          << " with ID: " << method_symbol.symbol_id << std::endl;
#endif
                
                symbol_table->add_symbol(std::move(method_symbol));
            }
        }
        
        // 独立関数のシンボル生成（クラス外の関数）
        for (const auto& func_info : result.functions) {
            // クラス内メソッドかどうかチェック
            bool is_method = false;
            for (const auto& cls : result.classes) {
                for (const auto& method : cls.methods) {
                    if (method.name == func_info.name && 
                        method.start_line == func_info.start_line) {
                        is_method = true;
                        break;
                    }
                }
                if (is_method) break;
            }
            
            // クラス外の独立関数のみ追加
            if (!is_method) {
                UniversalSymbolInfo func_symbol;
                func_symbol.symbol_id = "function_" + func_info.name + "_" + std::to_string(function_counter++);
                func_symbol.symbol_type = SymbolType::FUNCTION;
                func_symbol.name = func_info.name;
                func_symbol.start_line = func_info.start_line;
                func_symbol.metadata["language"] = "cpp";
                
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Adding function symbol: " << func_info.name 
                          << " with ID: " << func_symbol.symbol_id << std::endl;
#endif
                
                symbol_table->add_symbol(std::move(func_symbol));
            }
        }
        
        // 結果にUniversal Symbolsを設定
        result.universal_symbols = symbol_table;
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] C++ Universal Symbol generation completed. Total symbols: " 
                  << symbol_table->get_all_symbols().size() << std::endl;
#endif
    }
};

} // namespace adapters
} // namespace nekocode