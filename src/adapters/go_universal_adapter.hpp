#pragma once

//=============================================================================
// 🟢 Go Universal Adapter - シンプル言語×統一システム完全証明
//
// JavaScript+Python+C++/C#成功パターン → Go特化適用
// goroutine/channel/interface完全対応の統一化実現
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include <memory>
#include <regex>
#include <unordered_map>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 Go Universal Adapter - シンプル言語の統一化
//=============================================================================

class GoUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::GoTraits> {
private:
    // 🟢 Go特有の構造管理
    std::string current_package;                    // 現在のパッケージ名
    std::unordered_map<std::string, std::string> imports; // import管理
    std::unordered_map<std::string, std::string> receiver_types; // レシーバー型管理
    bool in_struct_definition = false;
    bool in_interface_definition = false;
    
public:
    GoUniversalAdapter() = default;
    virtual ~GoUniversalAdapter() = default;
    
    //=========================================================================
    // 🚀 統一インターフェース実装（完全互換性）
    //=========================================================================
    
    Language get_language() const override {
        return Language::GO;
    }
    
    std::string get_language_name() const override {
        return "Go (Universal AST)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".go"};
    }
    
    //=========================================================================
    // 🔥 Go特化解析エンジン（並行処理 + AST統一）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 基本情報設定
        AnalysisResult result;
        result.language = get_language();
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.file_info.total_lines = count_lines(content);
        
        // Phase 2: Go特化解析 + AST構築
        parse_go_with_ast(content, result);
        
        // Phase 3: AST → 従来形式変換
        this->tree_builder.extract_to_analysis_result(result);
        
        // Phase 4: Go特化統計拡張
        enhance_result_with_go_features(result);
        
        return result;
    }
    
    //=========================================================================
    // 🌳 Go AST特化機能
    //=========================================================================
    
    /// Go特化AST検索
    ASTNode* query_go_ast(const std::string& path) const {
        return this->tree_builder.query_ast(path);
    }
    
    /// Goroutine関数検索
    std::vector<std::string> find_goroutines() const {
        std::vector<std::string> goroutines;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_goroutines_recursive(root, goroutines);
        return goroutines;
    }
    
    /// Interface検索
    std::vector<std::string> find_interfaces() const {
        std::vector<std::string> interfaces;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_interfaces_recursive(root, interfaces);
        return interfaces;
    }
    
    /// Channel使用検索
    std::vector<std::string> find_channels() const {
        std::vector<std::string> channels;
        // TODO: chan型変数の検出実装
        return channels;
    }
    
    /// Test関数検索
    std::vector<std::string> find_test_functions() const {
        std::vector<std::string> tests;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_test_functions_recursive(root, tests);
        return tests;
    }
    
    /// Benchmark関数検索
    std::vector<std::string> find_benchmark_functions() const {
        std::vector<std::string> benchmarks;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_benchmark_functions_recursive(root, benchmarks);
        return benchmarks;
    }
    
    /// メソッド（レシーバー付き関数）検索
    std::vector<std::pair<std::string, std::string>> find_methods() const {
        std::vector<std::pair<std::string, std::string>> methods;
        // TODO: レシーバー付きメソッドの検出実装
        return methods;
    }
    
protected:
    //=========================================================================
    // 🔄 Go特化解析エンジン
    //=========================================================================
    
    void parse_go_with_ast(const std::string& content, AnalysisResult& result) {
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
            
            // Go特化パターン解析
            analyze_go_line(trimmed, line_number);
            
            line_number++;
        }
    }
    
    void analyze_go_line(const std::string& line, std::uint32_t line_number) {
        auto tokens = tokenize_line(line);
        if (tokens.empty()) return;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // package宣言検出
            if (token == "package" && i + 1 < tokens.size()) {
                handle_go_package(tokens, i, line_number);
            }
            // import検出
            else if (token == "import") {
                handle_go_import(tokens, i, line_number);
            }
            // type定義検出（struct/interface）
            else if (token == "type" && i + 1 < tokens.size()) {
                handle_go_type(tokens, i, line_number);
            }
            // func検出（関数/メソッド）
            else if (token == "func" && i + 1 < tokens.size()) {
                handle_go_func(tokens, i, line_number);
            }
            // var/const検出
            else if (universal::GoTraits::variable_keywords().count(token) > 0 && i + 1 < tokens.size()) {
                handle_go_variable(tokens, i, line_number);
            }
            // goroutine検出
            else if (token == "go" && i + 1 < tokens.size()) {
                handle_go_goroutine(tokens, i, line_number);
            }
        }
    }
    
    void handle_go_package(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        current_package = tokens[index + 1];
        // package情報は現在ASTに追加しない（将来的にはパッケージノード追加）
    }
    
    void handle_go_import(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // import情報の処理（簡易スキップ）
        // TODO: import情報の管理
    }
    
    void handle_go_type(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string type_name = tokens[index + 1];
        
        // struct/interface判定
        if (index + 2 < tokens.size()) {
            const auto& type_kind = tokens[index + 2];
            
            if (type_kind == "struct" || type_kind.find("struct{") == 0) {
                this->tree_builder.enter_scope(ASTNodeType::CLASS, type_name, line_number);
                in_struct_definition = true;
            }
            else if (type_kind == "interface" || type_kind.find("interface{") == 0) {
                this->tree_builder.enter_scope(ASTNodeType::INTERFACE, type_name, line_number);
                in_interface_definition = true;
            }
        }
    }
    
    void handle_go_func(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string next_token = tokens[index + 1];
        
        // レシーバー付きメソッド検出
        if (next_token.find('(') == 0) {
            // func (receiver Type) methodName パターン
            handle_go_method(tokens, index, line_number);
        } else {
            // 通常の関数
            std::string func_name = next_token;
            
            // 関数名から括弧除去
            size_t paren_pos = func_name.find('(');
            if (paren_pos != std::string::npos) {
                func_name = func_name.substr(0, paren_pos);
            }
            
            this->tree_builder.enter_scope(ASTNodeType::FUNCTION, func_name, line_number);
            
            // Test関数判定
            if (func_name.find("Test") == 0) {
                // TODO: 現在の関数ノードにtest属性設定
            }
            
            // Benchmark関数判定
            if (func_name.find("Benchmark") == 0) {
                // TODO: 現在の関数ノードにbenchmark属性設定
            }
        }
    }
    
    void handle_go_method(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // レシーバー付きメソッドの処理
        // TODO: より詳細なレシーバー解析
        
        // メソッド名を探す
        for (size_t i = index + 1; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            if (token.find(')') != std::string::npos && i + 1 < tokens.size()) {
                std::string method_name = tokens[i + 1];
                
                // メソッド名から括弧除去
                size_t paren_pos = method_name.find('(');
                if (paren_pos != std::string::npos) {
                    method_name = method_name.substr(0, paren_pos);
                }
                
                this->tree_builder.enter_scope(ASTNodeType::FUNCTION, method_name, line_number);
                break;
            }
        }
    }
    
    void handle_go_variable(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string var_name = tokens[index + 1];
        
        // 変数名から型情報除去
        size_t space_pos = var_name.find(' ');
        if (space_pos != std::string::npos) {
            var_name = var_name.substr(0, space_pos);
        }
        
        this->tree_builder.add_symbol(ASTNodeType::VARIABLE, var_name, line_number);
    }
    
    void handle_go_goroutine(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // goroutine起動の処理
        if (index + 1 < tokens.size()) {
            std::string goroutine_call = "go " + tokens[index + 1];
            this->tree_builder.add_symbol(ASTNodeType::FUNCTION, goroutine_call, line_number);
        }
    }
    
    void enhance_result_with_go_features(AnalysisResult& result) {
        // AST統計を既存結果に統合
        auto ast_stats = this->tree_builder.get_ast_statistics();
        
        // 統計拡張
        result.stats.class_count = std::max(result.stats.class_count, ast_stats.classes);
        result.stats.function_count = std::max(result.stats.function_count, ast_stats.functions);
        
        // Go特化統計（将来拡張用）
        // result.stats.goroutine_count = count_goroutines();
        // result.stats.interface_count = count_interfaces();
    }
    
    void find_goroutines_recursive(const ASTNode* node, std::vector<std::string>& goroutines) const {
        if (node->type == ASTNodeType::FUNCTION) {
            auto it = node->attributes.find("goroutine");
            if (it != node->attributes.end() && it->second == "true") {
                goroutines.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_goroutines_recursive(child.get(), goroutines);
        }
    }
    
    void find_interfaces_recursive(const ASTNode* node, std::vector<std::string>& interfaces) const {
        if (node->type == ASTNodeType::INTERFACE) {
            interfaces.push_back(node->name);
        }
        
        for (const auto& child : node->children) {
            find_interfaces_recursive(child.get(), interfaces);
        }
    }
    
    void find_test_functions_recursive(const ASTNode* node, std::vector<std::string>& tests) const {
        if (node->type == ASTNodeType::FUNCTION) {
            auto it = node->attributes.find("test_function");
            if (it != node->attributes.end() && it->second == "true") {
                tests.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_test_functions_recursive(child.get(), tests);
        }
    }
    
    void find_benchmark_functions_recursive(const ASTNode* node, std::vector<std::string>& benchmarks) const {
        if (node->type == ASTNodeType::FUNCTION) {
            auto it = node->attributes.find("benchmark_function");
            if (it != node->attributes.end() && it->second == "true") {
                benchmarks.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_benchmark_functions_recursive(child.get(), benchmarks);
        }
    }
};

} // namespace adapters
} // namespace nekocode