//=============================================================================
// 🏭 Analyzer Factory - 言語別アナライザー生成ファクトリー
//
// 言語に応じた適切なアナライザーを生成
//=============================================================================

#include "nekocode/analyzers/base_analyzer.hpp"
// #include "nekocode/analyzers/javascript_analyzer.hpp" // regex版は削除済み
#include "nekocode/analyzers/javascript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/typescript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/python_analyzer.hpp"
#include "nekocode/analyzers/python_pegtl_analyzer.hpp"
#include "nekocode/analyzers/cpp_language_analyzer.hpp"
#include "nekocode/analyzers/cpp_pegtl_analyzer.hpp"
// #include "nekocode/analyzers/csharp_analyzer.hpp" // regex版は削除済み
// #include "nekocode/analyzers/csharp_pegtl_analyzer.hpp" // 一時的にコメントアウト
// #include "nekocode/analyzers/unity_analyzer.hpp" // 一時的にコメントアウト
#include "nekocode/analyzers/go_analyzer.hpp"
#include "nekocode/analyzers/rust_analyzer.hpp"
#include <algorithm>

// 🔧 グローバルデバッグフラグ定義
bool g_debug_mode = false;
#include <cctype>

namespace nekocode {

//=============================================================================
// 🏭 AnalyzerFactory Implementation
//=============================================================================

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_analyzer(Language language) {
    switch (language) {
        case Language::JAVASCRIPT:
            // PEGTL版を使用（std::regex版から移行）
            return std::make_unique<JavaScriptPEGTLAnalyzer>();
            
        case Language::TYPESCRIPT:
            // PEGTL版を使用（JavaScript拡張）
            return std::make_unique<TypeScriptPEGTLAnalyzer>();
            
        case Language::CPP:
            // PEGTL版を使用（Claude Code支援作戦）
            return std::make_unique<CppPEGTLAnalyzer>();
            
        case Language::C:
            return std::make_unique<CLanguageAnalyzer>();
            
        case Language::PYTHON:
            // TODO: PEGTL版実装予定（現在は基本版を使用）
            return std::make_unique<PythonAnalyzer>();
            
        case Language::CSHARP:
            // 一時的にnullptrを返す（コンパイルエラー回避）
            return nullptr;
            
        case Language::GO:
            // Go言語解析エンジン（Goroutine & Channel detection）
            return std::make_unique<GoAnalyzer>();
            
        case Language::RUST:
            // Rust言語解析エンジン（trait, impl, macro detection）
            return std::make_unique<RustAnalyzer>();
            
        default:
            return nullptr;
    }
}

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_analyzer_from_extension(const std::string& extension) {
    // 拡張子を小文字に変換
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // JavaScript
    if (ext == ".js" || ext == ".mjs" || ext == ".jsx" || ext == ".cjs") {
        // PEGTL版を使用（std::regex版から移行）
        return std::make_unique<JavaScriptPEGTLAnalyzer>();
    }
    
    // TypeScript
    if (ext == ".ts" || ext == ".tsx" || ext == ".mts" || ext == ".cts") {
        // PEGTL版を使用（JavaScript拡張）
        return std::make_unique<TypeScriptPEGTLAnalyzer>();
    }
    
    // C++
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || 
        ext == ".hpp" || ext == ".hxx" || ext == ".hh" || ext == ".h++") {
        // PEGTL版を使用（Claude Code支援作戦）
        return std::make_unique<CppPEGTLAnalyzer>();
    }
    
    // C (注意: .h は曖昧なので内容で判断が必要)
    if (ext == ".c") {
        return std::make_unique<CLanguageAnalyzer>();
    }
    
    // Python
    if (ext == ".py" || ext == ".pyw" || ext == ".pyi") {
        // TODO: PEGTL版実装予定（現在は基本版を使用）
        return std::make_unique<PythonAnalyzer>();
    }
    
    // C#
    if (ext == ".cs" || ext == ".csx") {
        // 一時的にnullptrを返す（コンパイルエラー回避）
        return nullptr;
    }
    
    // Go
    if (ext == ".go") {
        // Go言語解析エンジン（Goroutine & Channel detection）
        return std::make_unique<GoAnalyzer>();
    }
    
    // Rust
    if (ext == ".rs") {
        // Rust言語解析エンジン（trait, impl, macro detection）
        return std::make_unique<RustAnalyzer>();
    }
    
    // .h ファイルはデフォルトでC++として扱う
    if (ext == ".h") {
        // PEGTL版を使用（Claude Code支援作戦）
        return std::make_unique<CppPEGTLAnalyzer>();
    }
    
    return nullptr;
}

//=============================================================================
// 🎮 Unity 特化ファクトリー関数
//=============================================================================

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_unity_analyzer() {
    std::cerr << "🎮 Unity Analyzer temporarily disabled" << std::endl;
    return nullptr;
}

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_unity_analyzer_from_file(
    const std::string& filename, 
    const std::string& content_preview
) {
    std::cerr << "🎮 Unity Analyzer temporarily disabled" << std::endl;
    return nullptr;
}

//=============================================================================
// 🔧 Private Utility Functions
//=============================================================================

std::string AnalyzerFactory::get_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return filename.substr(dot_pos);
}

} // namespace nekocode