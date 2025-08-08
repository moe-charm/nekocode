#pragma once

//=============================================================================
// 🐍 Python Universal Adapter - インデント言語×統一システム融合
//
// JavaScript成功パターン → Python特化適用
// インデントベース言語の挑戦：99%共通処理 + 1%Python特化
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include "nekocode/analyzers/python_pegtl_analyzer.hpp"
#include <memory>
#include <stack>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 Python Universal Adapter - インデント言語の統一化
//=============================================================================

class PythonUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::PythonTraits> {
private:
    // 🔥 成熟したPEGTL実装を活用（JavaScriptパターンを踏襲）
    std::unique_ptr<PythonPEGTLAnalyzer> legacy_analyzer;
    
    // 🐍 インデント管理スタック（Python特有の拡張用）
    std::stack<std::uint32_t> indent_stack;
    std::uint32_t current_indent = 0;
    
    // スコープ終了行追跡用
    std::stack<std::uint32_t> scope_start_lines;
    std::uint32_t last_non_empty_line = 0;
    
public:
    PythonUniversalAdapter() {
        // 成熟したPEGTLアナライザーを初期化
        legacy_analyzer = std::make_unique<PythonPEGTLAnalyzer>();
        // インデントスタック初期化（トップレベル=0）
        indent_stack.push(0);
    }
    
    virtual ~PythonUniversalAdapter() = default;
    
    //=========================================================================
    // 🚀 統一インターフェース実装（完全互換性）
    //=========================================================================
    
    Language get_language() const override {
        return Language::PYTHON;
    }
    
    std::string get_language_name() const override {
        return "Python (Universal AST)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".py", ".pyx", ".pyi"};
    }
    
    //=========================================================================
    // 🔥 ハイブリッド解析エンジン（成熟したPEGTL + 統一AST）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 成熟したPEGTL解析で高精度結果取得（5/5関数検出成功済み！）
        AnalysisResult legacy_result = legacy_analyzer->analyze(content, filename);
        
        // Phase 2: 統一AST構築（PEGTL結果から逆構築）
        build_unified_ast_from_legacy_result(legacy_result, content);
        
        // Phase 3: AST統計とレガシー統計の統合
        enhance_result_with_ast_data(legacy_result);
        
        // Phase 4: Python特化機能の追加
        enhance_result_with_python_features(legacy_result);
        
        // 🚀 Phase 5: Universal Symbols生成（BaseAnalyzer共通関数を使用）
        this->generate_universal_symbols(legacy_result, "python");
        
        return legacy_result;
    }
    
    //=========================================================================
    // 🌳 Python AST特化機能
    //=========================================================================
    
    /// Python特化AST検索
    ASTNode* query_python_ast(const std::string& path) const {
        return this->tree_builder.query_ast(path);
    }
    
    /// 特殊メソッド検索（__init__, __str__等）
    std::vector<std::string> find_special_methods() const {
        std::vector<std::string> special_methods;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_special_methods_recursive(root, special_methods);
        return special_methods;
    }
    
    /// インスタンス変数検索（self.variable）
    std::vector<std::string> find_instance_variables() const {
        std::vector<std::string> instance_vars;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_instance_variables_recursive(root, instance_vars);
        return instance_vars;
    }
    
    /// Pythonクラス継承チェーン解析
    std::vector<std::string> analyze_inheritance_chain(const std::string& class_name) const {
        // TODO: 将来実装
        return {};
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
            
            // TODO: メンバ変数対応（将来実装）
            // member_variables フィールドの正しい構造を確認後に実装
            
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
        
        // Python特有の構造を解析（特殊メソッド等）
        analyze_python_specific_patterns(content);
    }
    
    void analyze_python_specific_patterns(const std::string& content) {
        // Python特有のパターンを追加解析（必要に応じて）
        // 例: デコレータ、ジェネレータ、ラムダ等
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
    void parse_python_with_ast(const std::string& content, AnalysisResult& result) {
        std::istringstream stream(content);
        std::string line;
        std::uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            // 空行・コメント行をスキップ（ただし最終行は記録）
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
                line_number++;
                continue;
            }
            if (line.find_first_not_of(" \t") != std::string::npos && 
                line[line.find_first_not_of(" \t")] == '#') {
                line_number++;
                continue;
            }
            
            // 非空行を記録
            last_non_empty_line = line_number;
            
            // インデント検出・スコープ管理
            std::uint32_t line_indent = detect_indentation(line);
            manage_python_scope(line_indent, line_number);
            
            // Python特化パターン解析
            analyze_python_line(line, line_number, line_indent);
            
            line_number++;
        }
        
        // 残っているスコープを全て閉じる（end_lineを設定）
        while (indent_stack.size() > 1) {
            this->tree_builder.exit_scope(last_non_empty_line);
            indent_stack.pop();
            if (!scope_start_lines.empty()) {
                scope_start_lines.pop();
            }
        }
    }
    
    void manage_python_scope(std::uint32_t line_indent, std::uint32_t line_number) {
        if (line_indent > current_indent) {
            // インデント増加：新しいスコープ開始
            indent_stack.push(line_indent);
            current_indent = line_indent;
        } else if (line_indent < current_indent) {
            // インデント減少：スコープ終了処理（end_lineを設定）
            while (!indent_stack.empty() && indent_stack.top() > line_indent) {
                // スコープ終了時、前の行番号をend_lineとして設定
                this->tree_builder.exit_scope(line_number - 1);
                indent_stack.pop();
                if (!scope_start_lines.empty()) {
                    scope_start_lines.pop();
                }
            }
            current_indent = line_indent;
        }
        // line_indent == current_indent の場合は何もしない（同じスコープ継続）
    }
    
    void analyze_python_line(const std::string& line, std::uint32_t line_number, std::uint32_t indent) {
        auto tokens = tokenize_line(line);
        if (tokens.empty()) return;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // 関数定義検出
            if (token == "def" && i + 1 < tokens.size()) {
                handle_python_function(tokens, i, line_number);
            }
            // クラス定義検出
            else if (token == "class" && i + 1 < tokens.size()) {
                handle_python_class(tokens, i, line_number);
            }
            // self.変数検出
            else if (token.find("self.") == 0) {
                handle_instance_variable(token, line_number);
            }
        }
    }
    
    void handle_python_function(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string func_name = tokens[index + 1];
        
        // 関数名から括弧を除去
        size_t paren_pos = func_name.find('(');
        if (paren_pos != std::string::npos) {
            func_name = func_name.substr(0, paren_pos);
        }
        
        // 特殊メソッド判定
        bool is_special = universal::PythonTraits::is_special_method(func_name);
        
        this->tree_builder.enter_scope(ASTNodeType::FUNCTION, func_name, line_number);
        scope_start_lines.push(line_number);
        
        // メタデータ設定
        if (is_special) {
            // 特殊メソッドの場合、attributesで標識
            // （実際のノード取得が必要だが、簡易実装）
        }
    }
    
    void handle_python_class(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string class_name = tokens[index + 1];
        
        // クラス名から括弧・コロンを除去
        size_t colon_pos = class_name.find(':');
        if (colon_pos != std::string::npos) {
            class_name = class_name.substr(0, colon_pos);
        }
        size_t paren_pos = class_name.find('(');
        if (paren_pos != std::string::npos) {
            class_name = class_name.substr(0, paren_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::CLASS, class_name, line_number);
        scope_start_lines.push(line_number);
    }
    
    void handle_instance_variable(const std::string& token, std::uint32_t line_number) {
        // self.variable_name の variable_name 部分を抽出
        if (token.length() > 5) { // "self."より長い
            std::string var_name = token.substr(5); // "self."をスキップ
            
            // 代入演算子等を除去
            size_t assign_pos = var_name.find('=');
            if (assign_pos != std::string::npos) {
                var_name = var_name.substr(0, assign_pos);
            }
            
            this->tree_builder.add_variable(var_name, line_number);
        }
    }
    
    void enhance_result_with_python_features(AnalysisResult& result) {
        // Python特化統計（将来拡張用）
        // 例: 特殊メソッド数、デコレータ数、ジェネレータ数等
    }
    
    void find_special_methods_recursive(const ASTNode* node, std::vector<std::string>& special_methods) const {
        if (node->type == ASTNodeType::FUNCTION) {
            if (universal::PythonTraits::is_special_method(node->name)) {
                special_methods.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_special_methods_recursive(child.get(), special_methods);
        }
    }
    
    void find_instance_variables_recursive(const ASTNode* node, std::vector<std::string>& instance_vars) const {
        if (node->type == ASTNodeType::VARIABLE) {
            // インスタンス変数判定（簡易版）
            auto it = node->attributes.find("instance_method");
            if (it != node->attributes.end() && it->second == "true") {
                instance_vars.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_instance_variables_recursive(child.get(), instance_vars);
        }
    }
    
    //=========================================================================
    // 🚀 Universal Symbols生成エンジン（JavaScript成功パターン適用）
    //=========================================================================
    
    void generate_universal_symbols(AnalysisResult& result) {
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] Starting Universal Symbol generation for Python" << std::endl;
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
            class_symbol.metadata["language"] = "python";
            
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
                method_symbol.metadata["language"] = "python";
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
                func_symbol.metadata["language"] = "python";
                
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
        std::cerr << "[DEBUG] Python Universal Symbol generation completed. Total symbols: " 
                  << symbol_table->get_all_symbols().size() << std::endl;
#endif
    }
};

} // namespace adapters
} // namespace nekocode