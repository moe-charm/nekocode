#pragma once

//=============================================================================
// 🐍 Python Universal Adapter - インデント言語×統一システム融合
//
// JavaScript成功パターン → Python特化適用
// インデントベース言語の挑戦：99%共通処理 + 1%Python特化
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include <memory>
#include <stack>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 Python Universal Adapter - インデント言語の統一化
//=============================================================================

class PythonUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::PythonTraits> {
private:
    // 🐍 インデント管理スタック（Python特有）
    std::stack<std::uint32_t> indent_stack;
    std::uint32_t current_indent = 0;
    
public:
    PythonUniversalAdapter() {
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
    // 🔥 Python特化解析エンジン（インデント処理 + AST統一）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 基本情報設定
        AnalysisResult result;
        result.language = get_language();
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.file_info.total_lines = count_lines(content);
        
        // Phase 2: Python特化解析 + AST構築
        parse_python_with_ast(content, result);
        
        // Phase 3: AST → 従来形式変換
        this->tree_builder.extract_to_analysis_result(result);
        
        // Phase 4: Python特化統計拡張
        enhance_result_with_python_features(result);
        
        return result;
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
    // 🔄 Python特化解析エンジン
    //=========================================================================
    
    void parse_python_with_ast(const std::string& content, AnalysisResult& result) {
        std::istringstream stream(content);
        std::string line;
        std::uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            // 空行・コメント行をスキップ
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
                line_number++;
                continue;
            }
            if (line.find_first_not_of(" \t") != std::string::npos && 
                line[line.find_first_not_of(" \t")] == '#') {
                line_number++;
                continue;
            }
            
            // インデント検出・スコープ管理
            std::uint32_t line_indent = detect_indentation(line);
            manage_python_scope(line_indent);
            
            // Python特化パターン解析
            analyze_python_line(line, line_number, line_indent);
            
            line_number++;
        }
        
        // 残っているスコープを全て閉じる
        while (indent_stack.size() > 1) {
            this->tree_builder.exit_scope();
            indent_stack.pop();
        }
    }
    
    void manage_python_scope(std::uint32_t line_indent) {
        if (line_indent > current_indent) {
            // インデント増加：新しいスコープ開始
            indent_stack.push(line_indent);
            current_indent = line_indent;
        } else if (line_indent < current_indent) {
            // インデント減少：スコープ終了処理
            while (!indent_stack.empty() && indent_stack.top() > line_indent) {
                this->tree_builder.exit_scope();
                indent_stack.pop();
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
        // AST統計を既存結果に統合
        auto ast_stats = this->tree_builder.get_ast_statistics();
        
        // 統計拡張
        result.stats.class_count = std::max(result.stats.class_count, ast_stats.classes);
        result.stats.function_count = std::max(result.stats.function_count, ast_stats.functions);
        
        // Python特化統計（将来拡張用）
        // result.stats.special_method_count = count_special_methods();
        // result.stats.instance_variable_count = count_instance_variables();
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
};

} // namespace adapters
} // namespace nekocode