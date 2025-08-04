#pragma once

//=============================================================================
// 🔌 Language Traits Pattern - 言語固有特性の分離
//
// 1%の言語固有部分を完全分離し、99%の共通処理を実現
// Strategy Pattern + Template Traits の融合による美しい設計
//=============================================================================

#include "nekocode/types.hpp"
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
    /// 言語識別
    static Language get_language_enum() {
        return Language::CPP;
    }
    
    static std::string get_language_name() {
        return "C++";
    }
    
    static std::vector<std::string> get_supported_extensions() {
        return {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".h"};
    }
    
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
    /// 言語識別
    static Language get_language_enum() {
        return Language::CSHARP;
    }
    
    static std::string get_language_name() {
        return "C#";
    }
    
    static std::vector<std::string> get_supported_extensions() {
        return {".cs"};
    }
    
    /// C#メソッドキーワード（拡張版）
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "void", "int", "string", "bool", "float", "double", "decimal", "object",
            "public", "private", "protected", "internal", "static", "virtual", 
            "override", "abstract", "async", "extern", "unsafe"
        };
        return keywords;
    }
    
    /// C#クラスキーワード
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "class", "struct", "interface", "enum", "record"
        };
        return keywords;
    }
    
    /// C#制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "for", "foreach", "while", "do", "switch", "case", 
            "try", "catch", "finally", "return", "yield", "break", "continue"
        };
        return keywords;
    }
    
    /// C#プロパティキーワード
    static const std::unordered_set<std::string>& property_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "get", "set", "init", "value"
        };
        return keywords;
    }
    
    /// Unity特殊クラス検出
    static bool is_unity_class(const std::string& name) {
        static const std::unordered_set<std::string> unity_bases = {
            "MonoBehaviour", "ScriptableObject", "Component", "Behaviour", 
            "MonoBehaviourInterface", "StateMachineBehaviour"
        };
        return unity_bases.count(name) > 0;
    }
    
    /// Unity特殊メソッド検出
    static bool is_unity_method(const std::string& name) {
        static const std::unordered_set<std::string> unity_methods = {
            "Awake", "Start", "Update", "FixedUpdate", "LateUpdate", 
            "OnEnable", "OnDisable", "OnDestroy", "OnTriggerEnter",
            "OnCollisionEnter", "OnGUI"
        };
        return unity_methods.count(name) > 0;
    }
    
    /// ノード作成（C#特殊処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // Unity特有の処理
        if (type == ASTNodeType::CLASS && is_unity_class(name)) {
            node->attributes["unity_class"] = "true";
        }
        
        if (type == ASTNodeType::FUNCTION && is_unity_method(name)) {
            node->attributes["unity_method"] = "true";
        }
        
        // プロパティ検出
        if (name.find("get_") == 0 || name.find("set_") == 0) {
            node->attributes["property"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// 🟢 Go Traits - シンプル言語の統一化
//=============================================================================

class GoTraits : public BaseLanguageTraits<GoTraits> {
public:
    /// 言語識別
    static Language get_language_enum() {
        return Language::GO;
    }
    
    static std::string get_language_name() {
        return "Go";
    }
    
    static std::vector<std::string> get_supported_extensions() {
        return {".go"};
    }
    
    /// Go関数キーワード
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "func"
        };
        return keywords;
    }
    
    /// Goクラスキーワード（struct/interfaceをクラスとして扱う）
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "type", "struct", "interface"
        };
        return keywords;
    }
    
    /// Go型定義キーワード
    static const std::unordered_set<std::string>& type_keywords() {
        return class_keywords(); // class_keywordsと同じ
    }
    
    /// Go制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "for", "range", "switch", "case", "default", 
            "return", "break", "continue", "goto", "defer", "go", "select"
        };
        return keywords;
    }
    
    /// Go変数宣言キーワード
    static const std::unordered_set<std::string>& variable_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "var", "const"
        };
        return keywords;
    }
    
    /// パッケージキーワード
    static const std::unordered_set<std::string>& package_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "package", "import"
        };
        return keywords;
    }
    
    /// Go並行処理キーワード
    static const std::unordered_set<std::string>& concurrency_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "go", "chan", "select"
        };
        return keywords;
    }
    
    /// レシーバー付きメソッド検出
    static bool is_method_with_receiver(const std::string& line) {
        // (receiver Type) methodName パターンの簡易検出
        return line.find("func (") != std::string::npos;
    }
    
    /// ノード作成（Go特殊処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // Go特有の処理
        if (type == ASTNodeType::FUNCTION && name.find("Test") == 0) {
            node->attributes["test_function"] = "true";
        }
        
        if (type == ASTNodeType::FUNCTION && name.find("Benchmark") == 0) {
            node->attributes["benchmark_function"] = "true";
        }
        
        // goroutine検出
        if (name.find("go ") == 0) {
            node->attributes["goroutine"] = "true";
        }
        
        return node;
    }
};

//=============================================================================
// 🦀 Rust Traits - 最新言語の統一化
//=============================================================================

class RustTraits : public BaseLanguageTraits<RustTraits> {
public:
    /// 言語識別
    static Language get_language_enum() {
        return Language::RUST;
    }
    
    static std::string get_language_name() {
        return "Rust";
    }
    
    static std::vector<std::string> get_supported_extensions() {
        return {".rs"};
    }
    
    /// Rust関数キーワード
    static const std::unordered_set<std::string>& function_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "fn", "async", "const", "unsafe", "extern"
        };
        return keywords;
    }
    
    /// Rust型定義キーワード
    static const std::unordered_set<std::string>& class_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "struct", "enum", "trait", "impl", "type"
        };
        return keywords;
    }
    
    /// Rust制御構造キーワード
    static const std::unordered_set<std::string>& control_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "if", "else", "match", "loop", "while", "for", "return", 
            "break", "continue", "await", "yield"
        };
        return keywords;
    }
    
    /// Rust変数宣言キーワード
    static const std::unordered_set<std::string>& variable_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "let", "const", "static", "mut"
        };
        return keywords;
    }
    
    /// Rustモジュールキーワード
    static const std::unordered_set<std::string>& module_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "mod", "use", "pub", "crate", "super", "self"
        };
        return keywords;
    }
    
    /// Rust所有権キーワード
    static const std::unordered_set<std::string>& ownership_keywords() {
        static const std::unordered_set<std::string> keywords = {
            "move", "mut", "ref", "&", "&mut"
        };
        return keywords;
    }
    
    /// マクロ検出
    static bool is_macro(const std::string& name) {
        return name.find('!') != std::string::npos;
    }
    
    /// derive属性検出
    static bool is_derive_attribute(const std::string& line) {
        return line.find("#[derive(") != std::string::npos;
    }
    
    /// test属性検出
    static bool is_test_attribute(const std::string& line) {
        return line.find("#[test]") != std::string::npos || 
               line.find("#[cfg(test)]") != std::string::npos;
    }
    
    /// ノード作成（Rust特殊処理）
    static std::unique_ptr<ASTNode> create_node(ASTNodeType type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(type, name);
        
        // Rust特有の処理
        if (type == ASTNodeType::FUNCTION && name.find("test_") == 0) {
            node->attributes["test_function"] = "true";
        }
        
        if (type == ASTNodeType::FUNCTION && name.find("bench_") == 0) {
            node->attributes["benchmark_function"] = "true";
        }
        
        // async関数検出
        if (type == ASTNodeType::FUNCTION && name.find("async") != std::string::npos) {
            node->attributes["async_function"] = "true";
        }
        
        // マクロ検出
        if (is_macro(name)) {
            node->attributes["macro"] = "true";
        }
        
        return node;
    }
};

} // namespace universal
} // namespace nekocode