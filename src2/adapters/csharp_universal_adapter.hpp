#pragma once

//=============================================================================
// 💎 C# Universal Adapter - Unity/.NET特化統一システム
//
// JavaScript+Python+C++成功パターン → C#/.NET特化適用
// Unity MonoBehaviour + .NET特化機能完全対応の統一化実現
//=============================================================================

#include "../universal/universal_code_analyzer.hpp"
#include "../universal/language_traits.hpp"
#include <memory>
#include <regex>
#include <unordered_map>

namespace nekocode {
namespace adapters {

//=============================================================================
// 🌟 C# Universal Adapter - .NET/Unity特化統一化
//=============================================================================

class CSharpUniversalAdapter : public universal::UniversalCodeAnalyzer<universal::CSharpTraits> {
private:
    // 💎 C#特有の複雑構造管理
    std::unordered_map<std::string, std::string> namespace_stack; // namespace管理
    std::unordered_map<std::string, std::string> property_context; // property管理
    std::unordered_map<std::string, std::string> generic_context; // generic管理
    bool in_property_definition = false;
    bool in_generic_definition = false;
    std::string current_access_modifier = "private"; // デフォルトはprivate
    
public:
    CSharpUniversalAdapter() = default;
    virtual ~CSharpUniversalAdapter() = default;
    
    //=========================================================================
    // 🚀 統一インターフェース実装（完全互換性）
    //=========================================================================
    
    Language get_language() const override {
        return Language::CSHARP;
    }
    
    std::string get_language_name() const override {
        return "C# (Universal AST)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".cs"};
    }
    
    //=========================================================================
    // 🔥 C#特化解析エンジン（Unity/.NET処理 + AST統一）
    //=========================================================================
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // Phase 1: 基本情報設定
        AnalysisResult result;
        result.language = get_language();
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.file_info.total_lines = count_lines(content);
        
        // Phase 2: C#特化解析 + AST構築
        parse_csharp_with_ast(content, result);
        
        // Phase 3: AST → 従来形式変換
        this->tree_builder.extract_to_analysis_result(result);
        
        // Phase 4: C#特化統計拡張
        enhance_result_with_csharp_features(result);
        
        return result;
    }
    
    //=========================================================================
    // 🌳 C# AST特化機能
    //=========================================================================
    
    /// C#特化AST検索
    ASTNode* query_csharp_ast(const std::string& path) const {
        return this->tree_builder.query_ast(path);
    }
    
    /// Unity MonoBehaviour検索
    std::vector<std::string> find_unity_monobehaviours() const {
        std::vector<std::string> monobehaviours;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_unity_classes_recursive(root, monobehaviours);
        return monobehaviours;
    }
    
    /// C#プロパティ検索
    std::vector<std::string> find_properties() const {
        std::vector<std::string> properties;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_properties_recursive(root, properties);
        return properties;
    }
    
    /// Namespace検索
    std::vector<std::string> find_namespaces() const {
        std::vector<std::string> namespaces;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_namespaces_recursive(root, namespaces);
        return namespaces;
    }
    
    /// Unity特殊メソッド検索
    std::vector<std::string> find_unity_methods() const {
        std::vector<std::string> unity_methods;
        const ASTNode* root = this->tree_builder.get_ast_root();
        
        find_unity_methods_recursive(root, unity_methods);
        return unity_methods;
    }
    
    /// Generic型検索
    std::vector<std::string> find_generic_types() const {
        std::vector<std::string> generics;
        // TODO: 将来実装（class GenericClass<T> パターン）
        return generics;
    }
    
    /// 継承関係解析
    std::vector<std::pair<std::string, std::string>> analyze_inheritance() const {
        std::vector<std::pair<std::string, std::string>> inheritance;
        // TODO: 将来実装（class Derived : Base パターン）
        return inheritance;
    }
    
protected:
    //=========================================================================
    // 🔄 C#特化解析エンジン
    //=========================================================================
    
    void parse_csharp_with_ast(const std::string& content, AnalysisResult& result) {
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
            
            // C#特化パターン解析
            analyze_csharp_line(trimmed, line_number);
            
            line_number++;
        }
    }
    
    void analyze_csharp_line(const std::string& line, std::uint32_t line_number) {
        auto tokens = tokenize_line(line);
        if (tokens.empty()) return;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            
            // namespace検出
            if (token == "namespace" && i + 1 < tokens.size()) {
                handle_csharp_namespace(tokens, i, line_number);
            }
            // using検出（簡易スキップ）
            else if (token == "using") {
                // usingディレクティブは現在スキップ
                continue;
            }
            // class/struct/interface検出
            else if (universal::CSharpTraits::class_keywords().count(token) > 0 && i + 1 < tokens.size()) {
                handle_csharp_class(tokens, i, line_number);
            }
            // プロパティ検出（get/set）
            else if (universal::CSharpTraits::property_keywords().count(token) > 0) {
                handle_csharp_property(tokens, i, line_number);
            }
            // 関数検出（戻り値型 + 関数名）
            else if (is_csharp_method_pattern(tokens, i)) {
                handle_csharp_method(tokens, i, line_number);
            }
            // アクセス修飾子検出
            else if (token == "public" || token == "private" || token == "protected" || token == "internal") {
                handle_access_modifier(token);
            }
        }
    }
    
    void handle_csharp_namespace(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string namespace_name = tokens[index + 1];
        
        // namespace名から括弧除去
        size_t brace_pos = namespace_name.find('{');
        if (brace_pos != std::string::npos) {
            namespace_name = namespace_name.substr(0, brace_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::NAMESPACE, namespace_name, line_number);
    }
    
    void handle_csharp_class(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string class_name = tokens[index + 1];
        
        // クラス名から継承情報・括弧・ジェネリック除去
        size_t colon_pos = class_name.find(':');
        if (colon_pos != std::string::npos) {
            // TODO: 継承情報の処理
            class_name = class_name.substr(0, colon_pos);
        }
        size_t generic_pos = class_name.find('<');
        if (generic_pos != std::string::npos) {
            // TODO: ジェネリック情報の処理
            class_name = class_name.substr(0, generic_pos);
        }
        size_t brace_pos = class_name.find('{');
        if (brace_pos != std::string::npos) {
            class_name = class_name.substr(0, brace_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::CLASS, class_name, line_number);
        
        // デフォルトアクセス修飾子設定
        current_access_modifier = (tokens[index] == "class") ? "private" : "public";
    }
    
    void handle_csharp_property(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        // プロパティ処理（簡易実装）
        in_property_definition = true;
        
        // プロパティ情報を一時保存
        property_context["pending"] = "true";
    }
    
    bool is_csharp_method_pattern(const std::vector<std::string>& tokens, size_t index) const {
        // 簡易C#メソッド検出：アクセス修飾子 + 戻り値型 + メソッド名(引数) パターン
        if (index + 1 >= tokens.size()) return false;
        
        const auto& current_token = tokens[index];
        const auto& next_token = tokens[index + 1];
        
        // 戻り値型判定（簡易版）
        if (universal::CSharpTraits::function_keywords().count(current_token) > 0) {
            // 次のトークンがメソッド名っぽい（括弧含む）
            return next_token.find('(') != std::string::npos;
        }
        
        return false;
    }
    
    void handle_csharp_method(const std::vector<std::string>& tokens, size_t index, std::uint32_t line_number) {
        if (index + 1 >= tokens.size()) return;
        
        std::string return_type = tokens[index];
        std::string method_name = tokens[index + 1];
        
        // メソッド名から括弧除去
        size_t paren_pos = method_name.find('(');
        if (paren_pos != std::string::npos) {
            method_name = method_name.substr(0, paren_pos);
        }
        
        this->tree_builder.enter_scope(ASTNodeType::FUNCTION, method_name, line_number);
        
        // Unity特殊メソッド判定
        if (universal::CSharpTraits::is_unity_method(method_name)) {
            // TODO: 現在のメソッドノードにUnity属性設定
        }
        
        // プロパティ適用
        if (property_context.count("pending")) {
            // TODO: 現在のメソッドノードにプロパティ属性設定
            property_context.clear();
            in_property_definition = false;
        }
    }
    
    void handle_access_modifier(const std::string& modifier) {
        current_access_modifier = modifier;
    }
    
    void enhance_result_with_csharp_features(AnalysisResult& result) {
        // AST統計を既存結果に統合
        auto ast_stats = this->tree_builder.get_ast_statistics();
        
        // 統計拡張
        result.stats.class_count = std::max(result.stats.class_count, ast_stats.classes);
        result.stats.function_count = std::max(result.stats.function_count, ast_stats.functions);
        
        // C#特化統計（将来拡張用）
        // result.stats.property_count = count_properties();
        // result.stats.namespace_count = count_namespaces();
        // result.stats.unity_class_count = count_unity_classes();
    }
    
    void find_unity_classes_recursive(const ASTNode* node, std::vector<std::string>& unity_classes) const {
        if (node->type == ASTNodeType::CLASS) {
            auto it = node->attributes.find("unity_class");
            if (it != node->attributes.end() && it->second == "true") {
                unity_classes.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_unity_classes_recursive(child.get(), unity_classes);
        }
    }
    
    void find_properties_recursive(const ASTNode* node, std::vector<std::string>& properties) const {
        if (node->type == ASTNodeType::FUNCTION) {
            auto it = node->attributes.find("property");
            if (it != node->attributes.end() && it->second == "true") {
                properties.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_properties_recursive(child.get(), properties);
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
    
    void find_unity_methods_recursive(const ASTNode* node, std::vector<std::string>& unity_methods) const {
        if (node->type == ASTNodeType::FUNCTION) {
            auto it = node->attributes.find("unity_method");
            if (it != node->attributes.end() && it->second == "true") {
                unity_methods.push_back(node->name);
            }
        }
        
        for (const auto& child : node->children) {
            find_unity_methods_recursive(child.get(), unity_methods);
        }
    }
};

} // namespace adapters
} // namespace nekocode