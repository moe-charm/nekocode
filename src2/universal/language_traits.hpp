#pragma once

//=============================================================================
// 🔌 Language Traits Pattern - 言語固有特性の分離
//
// 1%の言語固有部分を完全分離し、99%の共通処理を実現
// Strategy Pattern + Template Traits の融合による美しい設計
//=============================================================================

#include "../../include2/nekocode/types.hpp"
#include <memory>
#include <string>
#include <unordered_set>

namespace nekocode {
namespace universal {

//=============================================================================
// 🎯 Base Language Traits - 共通インターフェース
//=============================================================================

template<typename Derived>
class BaseLanguageTraits {
public:
    /// ノード作成（言語固有の特殊処理対応）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        return std::make_unique<ASTNode>(type, name);
    }
    
    /// 関数キーワード判定
    static bool is_function_keyword(const std::string& token) {
        return Derived::function_keywords().count(token) > 0;
    }
    
    /// クラスキーワード判定
    static bool is_class_keyword(const std::string& token) {
        return Derived::class_keywords().count(token) > 0;
    }
    
    /// 制御構造キーワード判定
    static bool is_control_keyword(const std::string& token) {
        return Derived::control_keywords().count(token) > 0;
    }
    
    /// 言語enum取得（デフォルト実装）
    static Language get_language_enum() {
        return Language::UNKNOWN;
    }
    
    /// 言語名取得（デフォルト実装）
    static std::string get_language_name() {
        return "Unknown";
    }
    
    /// サポート拡張子（デフォルト実装）
    static std::vector<std::string> get_supported_extensions() {
        return {};
    }
};

//=============================================================================
// 🟨 JavaScript Traits - 既存実装の特性抽出
//=============================================================================

class JavaScriptTraits : public BaseLanguageTraits<JavaScriptTraits> {
public:
    /// 言語識別
    static Language get_language_enum() {
        return Language::JAVASCRIPT;
    }
    
    static std::string get_language_name() {
        return "JavaScript";
    }
    
    static std::vector<std::string> get_supported_extensions() {
        return {".js", ".mjs", ".jsx", ".cjs"};
    }
    
    /// JavaScript関数キーワード
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "function", "async", "=>", "get", "set"
        };
        return keywords;
    }
    
    /// JavaScriptクラスキーワード  
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "class"
        };
        return keywords;
    }
    
    /// JavaScript制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "for", "while", "switch", "case", "try", "catch", "return"
        };
        return keywords;
    }
    
    /// JavaScript変数宣言キーワード
    static const std::unordered_set<std::string>& variable_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "var", "let", "const"
        };
        return keywords;
    }
    
    /// ノード作成（JavaScript特殊処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // JavaScript特有の処理
        if (type == ASTNodeType::FUNCTION && name.find("async") != std::string::npos) {
            node->attributes["async"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// 🟦 TypeScript Traits - JavaScript継承パターン
//=============================================================================

class TypeScriptTraits : public JavaScriptTraits {
public:
    /// TypeScript追加キーワード
    static const std::unordered_set<std::string>& typescript_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "interface", "type", "enum", "namespace", "declare", "abstract"
        };
        return keywords;
    }
    
    /// ノード作成（TypeScript型情報付加）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = JavaScriptTraits::create_node(type, name);
        
        // TypeScript特有の処理
        if (type == ASTNodeType::FUNCTION && name.find(":") != std::string::npos) {
            // 型注釈検出
            node->attributes["typed"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// 🐍 Python Traits - インデントベース言語対応
//=============================================================================

class PythonTraits : public BaseLanguageTraits<PythonTraits> {
public:
    /// Python関数キーワード
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "def", "async def", "lambda"
        };
        return keywords;
    }
    
    /// Pythonクラスキーワード
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "class"
        };
        return keywords;
    }
    
    /// Python制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "elif", "else", "for", "while", "try", "except", "finally", "return", "yield"
        };
        return keywords;
    }
    
    /// Python特殊メソッド検出
    static bool is_special_method(const std::string& name) {
        return name.size() >= 4 && name.substr(0, 2) == "__" && name.substr(name.size()-2) == "__";
    }
    
    /// ノード作成（Python特殊処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // Python特有の処理
        if (type == ASTNodeType::FUNCTION && is_special_method(name)) {
            node->attributes["special_method"] = "true";
        }
        
        if (name.find("self.") == 0) {
            node->attributes["instance_method"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// ⚙️ C++ Traits - 複雑言語対応
//=============================================================================

class CppTraits : public BaseLanguageTraits<CppTraits> {
public:
    /// C++関数・メソッドパターン（複雑）
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "void", "int", "string", "auto", "template", "inline", "static", "virtual"
        };
        return keywords;
    }
    
    /// C++クラス関連キーワード
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "class", "struct", "union", "enum", "namespace"
        };
        return keywords;
    }
    
    /// C++制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "for", "while", "switch", "case", "try", "catch", "return"
        };
        return keywords;
    }
    
    /// C++アクセス修飾子
    static const std::unordered_set<std::string>& access_modifiers() {
        static const std::unordered_set<std::string> modifiers = {
            "public", "private", "protected"
        };
        return modifiers;
    }
    
    /// ノード作成（C++複雑処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // C++特有の処理
        if (type == ASTNodeType::FUNCTION && name.find("template") != std::string::npos) {
            node->attributes["template"] = "true";
        }
        
        if (type == ASTNodeType::CLASS && name.find("::") != std::string::npos) {
            node->attributes["namespaced"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// 💎 C# Traits - Unity対応含む
//=============================================================================

class CSharpTraits : public BaseLanguageTraits<CSharpTraits> {
public:
    /// C#メソッドキーワード
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "void", "int", "string", "public", "private", "static", "async", "override"
        };
        return keywords;
    }
    
    /// C#クラスキーワード
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "class", "struct", "interface", "enum"
        };
        return keywords;
    }
    
    /// C#制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "for", "foreach", "while", "switch", "case", "try", "catch", "return"
        };
        return keywords;
    }
    
    /// Unity特殊クラス検出
    static bool is_unity_class(const std::string& name) {
        static const std::unordered_set<std::string> unity_bases = {
            "MonoBehaviour", "ScriptableObject", "Component"
        };
        return unity_bases.count(name) > 0;
    }
};

} // namespace universal
} // namespace nekocode