//=============================================================================
// 🌍 Multi-Language Detection Engine Implementation
//
// JavaScript/TypeScript/C++/C の自動検出・分類システム
//=============================================================================

#include "nekocode/language_detection.hpp"
#include <regex>
#include <algorithm>

namespace nekocode {

//=============================================================================
// 🎯 LanguageDetector Implementation
//=============================================================================

LanguageDetector::LanguageDetector() {
    initialize_language_data();
}

void LanguageDetector::initialize_language_data() {
    // 拡張子マッピング
    extension_map_ = {
        // JavaScript
        {".js", Language::JAVASCRIPT},
        {".mjs", Language::JAVASCRIPT},
        {".jsx", Language::JAVASCRIPT},
        
        // TypeScript
        {".ts", Language::TYPESCRIPT},
        {".tsx", Language::TYPESCRIPT},
        
        // C++
        {".cpp", Language::CPP},
        {".cxx", Language::CPP},
        {".cc", Language::CPP},
        {".C", Language::CPP},
        {".hpp", Language::CPP},
        {".hxx", Language::CPP},
        {".hh", Language::CPP},
        {".H", Language::CPP},
        
        // C (注意: .h は曖昧)
        {".c", Language::C},
        
        // Python
        {".py", Language::PYTHON},
        {".pyw", Language::PYTHON},
        {".pyi", Language::PYTHON},
        
        // C#
        {".cs", Language::CSHARP},
        {".csx", Language::CSHARP},
        
        // Go
        {".go", Language::GO},
        
        // Rust
        {".rs", Language::RUST}
    };
    
    // 言語情報初期化
    language_info_[Language::JAVASCRIPT] = LanguageInfo{
        Language::JAVASCRIPT, 
        "javascript", 
        "JavaScript"
    };
    language_info_[Language::JAVASCRIPT].extensions = {".js", ".mjs", ".jsx"};
    language_info_[Language::JAVASCRIPT].keywords = {
        "function", "const", "let", "var", "class", "import", "export", 
        "async", "await", "=>", "typeof", "instanceof"
    };
    language_info_[Language::JAVASCRIPT].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::TYPESCRIPT] = LanguageInfo{
        Language::TYPESCRIPT, 
        "typescript", 
        "TypeScript"
    };
    language_info_[Language::TYPESCRIPT].extensions = {".ts", ".tsx"};
    language_info_[Language::TYPESCRIPT].keywords = {
        "interface", "type", "enum", "namespace", "declare", "readonly",
        "public", "private", "protected", "abstract", "implements"
    };
    language_info_[Language::TYPESCRIPT].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::CPP] = LanguageInfo{
        Language::CPP, 
        "cpp", 
        "C++"
    };
    language_info_[Language::CPP].extensions = {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".hh"};
    language_info_[Language::CPP].keywords = {
        "class", "namespace", "template", "typename", "using", "constexpr",
        "override", "final", "virtual", "explicit", "operator", "friend",
        "public", "private", "protected", "static", "const", "mutable"
    };
    language_info_[Language::CPP].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::C] = LanguageInfo{
        Language::C, 
        "c", 
        "C"
    };
    language_info_[Language::C].extensions = {".c", ".h"};
    language_info_[Language::C].keywords = {
        "struct", "union", "typedef", "static", "extern", "register",
        "volatile", "const", "sizeof", "enum"
    };
    language_info_[Language::C].comment_patterns = {"/*", "*/"};
    
    language_info_[Language::PYTHON] = LanguageInfo{
        Language::PYTHON,
        "python",
        "Python"
    };
    language_info_[Language::PYTHON].extensions = {".py", ".pyw", ".pyi"};
    language_info_[Language::PYTHON].keywords = {
        "def", "class", "import", "from", "async", "await", "lambda",
        "if", "elif", "else", "for", "while", "with", "as", "return",
        "yield", "try", "except", "finally", "raise", "assert"
    };
    language_info_[Language::PYTHON].comment_patterns = {"#", "\"\"\"", "'''"};
    
    language_info_[Language::CSHARP] = LanguageInfo{
        Language::CSHARP,
        "csharp",
        "C#"
    };
    language_info_[Language::CSHARP].extensions = {".cs", ".csx"};
    language_info_[Language::CSHARP].keywords = {
        "using", "namespace", "class", "interface", "struct", "enum", "record",
        "public", "private", "protected", "internal", "static", "virtual", "override",
        "abstract", "async", "await", "var", "const", "readonly", "new", "this",
        "base", "if", "else", "for", "foreach", "while", "do", "switch", "case",
        "try", "catch", "finally", "throw", "return", "yield", "lock", "using"
    };
    language_info_[Language::CSHARP].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::GO] = LanguageInfo{
        Language::GO,
        "go",
        "Go"
    };
    language_info_[Language::GO].extensions = {".go"};
    language_info_[Language::GO].keywords = {
        "package", "import", "func", "type", "struct", "interface", "go", "chan",
        "select", "defer", "make", "new", "var", "const", "if", "else", "for",
        "range", "switch", "case", "default", "return", "break", "continue",
        "fallthrough", "goto", "panic", "recover"
    };
    language_info_[Language::GO].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::RUST] = LanguageInfo{
        Language::RUST,
        "rust",
        "Rust"
    };
    language_info_[Language::RUST].extensions = {".rs"};
    language_info_[Language::RUST].keywords = {
        "fn", "let", "mut", "const", "static", "struct", "enum", "trait", "impl",
        "pub", "mod", "use", "extern", "crate", "async", "await", "move",
        "if", "else", "match", "for", "while", "loop", "break", "continue",
        "return", "yield", "where", "unsafe", "macro_rules", "dyn", "ref",
        "as", "in", "self", "Self", "super", "type"
    };
    language_info_[Language::RUST].comment_patterns = {"//", "/*", "*/"};
    
    language_info_[Language::UNKNOWN] = LanguageInfo{
        Language::UNKNOWN, 
        "unknown", 
        "Unknown"
    };
}

Language LanguageDetector::detect_by_extension(const std::filesystem::path& file_path) const {
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = extension_map_.find(ext);
    if (it != extension_map_.end()) {
        // .h ファイルの特別処理（C vs C++ 判定）
        if (ext == ".h") {
            // 内容を見て判定する必要がある
            return Language::UNKNOWN; // 内容チェックが必要
        }
        return it->second;
    }
    
    return Language::UNKNOWN;
}

Language LanguageDetector::detect_by_content(const std::string& content) const {
    if (content.empty()) return Language::UNKNOWN;
    
    // C++ 特有キーワード検出
    std::vector<std::string> cpp_indicators = {
        "class ", "namespace ", "template", "typename", "using namespace",
        "::", "std::", "constexpr", "override", "virtual", "explicit",
        "#include <iostream>", "#include <string>", "#include <vector>"
    };
    
    // TypeScript 特有キーワード検出
    std::vector<std::string> ts_indicators = {
        "interface ", "type ", ": string", ": number", ": boolean",
        "declare ", "namespace ", "enum ", "readonly ", "implements "
    };
    
    // JavaScript 特有キーワード検出
    std::vector<std::string> js_indicators = {
        "function ", "const ", "let ", "var ", "=>", 
        "import ", "export ", "require(", "module.exports"
    };
    
    // C 特有パターン検出
    std::vector<std::string> c_indicators = {
        "#include <stdio.h>", "#include <stdlib.h>", "int main(int argc, char",
        "malloc(", "free(", "printf("
    };
    
    // Python 特有パターン検出
    std::vector<std::string> python_indicators = {
        "def ", "class ", "import ", "from ", "if __name__", "__init__",
        "self.", "print(", "range(", "len(", "lambda ", "yield ", "async def"
    };
    
    // C# 特有パターン検出
    std::vector<std::string> csharp_indicators = {
        "using System", "namespace ", "public class ", "private class ",
        "public static ", "async Task", "await ", "var ", "public string",
        "public int", "[HttpPost]", "[ApiController]", "=> ", "{ get; set; }"
    };
    
    // Go 特有パターン検出
    std::vector<std::string> go_indicators = {
        "package ", "func ", "go ", "chan ", "defer ", "select ",
        "type struct", "interface{", "make(", "goroutine", "fmt."
    };
    
    // Rust 特有パターン検出
    std::vector<std::string> rust_indicators = {
        "fn ", "let mut ", "impl ", "trait ", "pub fn", "match ",
        "Some(", "None", "Ok(", "Err(", "unwrap()", "expect(",
        "use std::", "mod ", "struct ", "enum ", "macro_rules!",
        "&mut ", "&self", "self.", "::", "->", ".clone()"
    };
    
    int cpp_score = 0, ts_score = 0, js_score = 0, c_score = 0, python_score = 0, csharp_score = 0, go_score = 0, rust_score = 0;
    
    // スコア計算
    for (const auto& indicator : cpp_indicators) {
        if (content.find(indicator) != std::string::npos) {
            cpp_score++;
        }
    }
    
    for (const auto& indicator : ts_indicators) {
        if (content.find(indicator) != std::string::npos) {
            ts_score++;
        }
    }
    
    for (const auto& indicator : js_indicators) {
        if (content.find(indicator) != std::string::npos) {
            js_score++;
        }
    }
    
    for (const auto& indicator : c_indicators) {
        if (content.find(indicator) != std::string::npos) {
            c_score++;
        }
    }
    
    for (const auto& indicator : python_indicators) {
        if (content.find(indicator) != std::string::npos) {
            python_score++;
        }
    }
    
    for (const auto& indicator : csharp_indicators) {
        if (content.find(indicator) != std::string::npos) {
            csharp_score++;
        }
    }
    
    for (const auto& indicator : go_indicators) {
        if (content.find(indicator) != std::string::npos) {
            go_score++;
        }
    }
    
    for (const auto& indicator : rust_indicators) {
        if (content.find(indicator) != std::string::npos) {
            rust_score++;
        }
    }
    
    // 最高スコアの言語を返す
    int max_score = std::max({cpp_score, ts_score, js_score, c_score, python_score, csharp_score, go_score, rust_score});
    if (max_score == 0) return Language::UNKNOWN;
    
    if (rust_score == max_score) return Language::RUST;
    if (go_score == max_score) return Language::GO;
    if (csharp_score == max_score) return Language::CSHARP;
    if (python_score == max_score) return Language::PYTHON;
    if (cpp_score == max_score) return Language::CPP;
    if (ts_score == max_score) return Language::TYPESCRIPT;
    if (js_score == max_score) return Language::JAVASCRIPT;
    if (c_score == max_score) return Language::C;
    
    return Language::UNKNOWN;
}

Language LanguageDetector::detect_language(const std::filesystem::path& file_path, const std::string& content) const {
    // まず拡張子から判定
    Language ext_result = detect_by_extension(file_path);
    
    if (ext_result != Language::UNKNOWN) {
        // .h ファイルの場合は内容も確認
        if (file_path.extension() == ".h" && !content.empty()) {
            Language content_result = detect_by_content(content);
            if (content_result == Language::CPP) {
                return Language::CPP;
            }
            // C++ でなければ C と仮定
            return Language::C;
        }
        return ext_result;
    }
    
    // 拡張子で判定できない場合は内容から判定
    if (!content.empty()) {
        return detect_by_content(content);
    }
    
    return Language::UNKNOWN;
}

const LanguageInfo& LanguageDetector::get_language_info(Language lang) const {
    auto it = language_info_.find(lang);
    if (it != language_info_.end()) {
        return it->second;
    }
    return language_info_.at(Language::UNKNOWN);
}

std::vector<Language> LanguageDetector::get_supported_languages() const {
    return {Language::JAVASCRIPT, Language::TYPESCRIPT, Language::CPP, Language::C, Language::PYTHON, Language::CSHARP, Language::GO, Language::RUST};
}

std::vector<std::string> LanguageDetector::get_extensions_for_language(Language lang) const {
    auto it = language_info_.find(lang);
    if (it != language_info_.end()) {
        return it->second.extensions;
    }
    return {};
}

std::vector<std::string> LanguageDetector::get_all_supported_extensions() const {
    std::vector<std::string> all_extensions;
    for (const auto& pair : extension_map_) {
        all_extensions.push_back(pair.first);
    }
    return all_extensions;
}

Language LanguageDetector::detect_cpp_variant(const std::string& content) const {
    // C++ 特有の構文をより詳細にチェック
    std::regex cpp_class_regex(R"(class\s+\w+\s*(?::\s*(?:public|private|protected)\s+\w+)?\s*\{)");
    std::regex cpp_namespace_regex(R"(namespace\s+\w+\s*\{)");
    std::regex cpp_template_regex(R"(template\s*<[^>]*>\s*)");
    std::regex cpp_using_regex(R"(using\s+namespace\s+\w+;)");
    
    if (std::regex_search(content, cpp_class_regex) ||
        std::regex_search(content, cpp_namespace_regex) ||
        std::regex_search(content, cpp_template_regex) ||
        std::regex_search(content, cpp_using_regex)) {
        return Language::CPP;
    }
    
    return Language::C;
}

Language LanguageDetector::detect_js_variant(const std::string& content) const {
    // TypeScript 特有の型注釈検出
    std::regex ts_type_regex(R"(:\s*(?:string|number|boolean|object)\s*[;,=\)])");
    std::regex ts_interface_regex(R"(interface\s+\w+\s*\{)");
    std::regex ts_type_alias_regex(R"(type\s+\w+\s*=)");
    std::regex ts_generic_regex(R"(<[A-Z]\w*(?:\s*,\s*[A-Z]\w*)*>\s*\()");
    
    if (std::regex_search(content, ts_type_regex) ||
        std::regex_search(content, ts_interface_regex) ||
        std::regex_search(content, ts_type_alias_regex) ||
        std::regex_search(content, ts_generic_regex)) {
        return Language::TYPESCRIPT;
    }
    
    return Language::JAVASCRIPT;
}

//=============================================================================
// 🎯 Language-Specific Analysis Configuration
//=============================================================================

LanguageAnalysisConfig LanguageAnalysisConfig::for_language(Language lang) {
    LanguageAnalysisConfig config;
    config.language = lang;
    
    switch (lang) {
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
            config.analyze_classes = true;
            config.analyze_functions = true;
            config.analyze_namespaces = false;
            config.analyze_templates = false;
            config.analyze_inheritance = true;
            config.analyze_includes = false;
            config.analyze_imports = true;
            config.analyze_exports = true;
            break;
            
        case Language::CPP:
            config.analyze_classes = true;
            config.analyze_functions = true;
            config.analyze_namespaces = true;
            config.analyze_templates = true;
            config.analyze_inheritance = true;
            config.analyze_includes = true;
            config.analyze_imports = false;
            config.analyze_exports = false;
            config.include_private_members = true;
            break;
            
        case Language::C:
            config.analyze_classes = false;
            config.analyze_functions = true;
            config.analyze_namespaces = false;
            config.analyze_templates = false;
            config.analyze_inheritance = false;
            config.analyze_includes = true;
            config.analyze_imports = false;
            config.analyze_exports = false;
            break;
            
        case Language::PYTHON:
            config.analyze_classes = true;
            config.analyze_functions = true;
            config.analyze_namespaces = false;
            config.analyze_templates = false;
            config.analyze_inheritance = true;
            config.analyze_includes = false;
            config.analyze_imports = true;
            config.analyze_exports = false;
            break;
            
        case Language::CSHARP:
            config.analyze_classes = true;
            config.analyze_functions = true;
            config.analyze_namespaces = true;
            config.analyze_templates = true;  // ジェネリクス
            config.analyze_inheritance = true;
            config.analyze_includes = false;
            config.analyze_imports = true;    // using文
            config.analyze_exports = false;
            config.include_private_members = true;
            break;
            
        case Language::GO:
            config.analyze_classes = false;
            config.analyze_functions = true;
            config.analyze_namespaces = false;
            config.analyze_templates = false;
            config.analyze_inheritance = false;
            config.analyze_includes = false;
            config.analyze_imports = true;
            config.analyze_exports = false;
            break;
            
        case Language::RUST:
            config.analyze_classes = true;    // struct/enum
            config.analyze_functions = true;
            config.analyze_namespaces = true; // mod
            config.analyze_templates = true;  // generics
            config.analyze_inheritance = false;
            config.analyze_includes = false;
            config.analyze_imports = true;    // use文
            config.analyze_exports = false;
            config.include_private_members = true;
            break;
            
        case Language::UNKNOWN:
            // デフォルト設定
            break;
    }
    
    return config;
}

//=============================================================================
// 🏗️ CppAnalysisResult Implementation
//=============================================================================

void CppAnalysisResult::update_statistics() {
    cpp_stats.namespace_count = static_cast<uint32_t>(namespaces.size());
    
    cpp_stats.class_count = 0;
    cpp_stats.struct_count = 0;
    cpp_stats.union_count = 0;
    cpp_stats.template_count = 0;
    cpp_stats.private_member_count = 0;
    cpp_stats.public_member_count = 0;
    cpp_stats.virtual_function_count = 0;
    
    for (const auto& cls : cpp_classes) {
        switch (cls.class_type) {
            case CppClass::CLASS:
                cpp_stats.class_count++;
                break;
            case CppClass::STRUCT:
                cpp_stats.struct_count++;
                break;
            case CppClass::UNION:
                cpp_stats.union_count++;
                break;
        }
        
        if (cls.is_template) {
            cpp_stats.template_count++;
        }
        
        // メンバー統計（簡易実装）
        cpp_stats.private_member_count += cls.methods.size() / 2; // 簡易推測
        cpp_stats.public_member_count += cls.methods.size() / 2;  // 簡易推測
        cpp_stats.virtual_function_count += cls.methods.size() / 4; // 簡易推測
    }
    
    cpp_stats.function_count = static_cast<uint32_t>(cpp_functions.size());
    cpp_stats.include_count = static_cast<uint32_t>(includes.size());
}

} // namespace nekocode