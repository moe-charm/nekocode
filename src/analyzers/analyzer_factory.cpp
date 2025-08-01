//=============================================================================
// 🏭 Analyzer Factory - 言語別アナライザー生成ファクトリー
//
// 言語に応じた適切なアナライザーを生成
//=============================================================================

#include "base_analyzer.hpp"
// #include "nekocode/analyzers/javascript_analyzer.hpp" // regex版は削除済み
#include "javascript/javascript_pegtl_analyzer.hpp"
#include "typescript/typescript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/python_analyzer.hpp"
#include "nekocode/analyzers/python_pegtl_analyzer.hpp"
#include "nekocode/analyzers/cpp_language_analyzer.hpp"
#include "nekocode/analyzers/cpp_pegtl_analyzer.hpp"
// #include "nekocode/analyzers/csharp_analyzer.hpp" // regex版は削除済み
#include "nekocode/analyzers/csharp_pegtl_analyzer.hpp"
#include "nekocode/analyzers/unity_analyzer.hpp"
#include "nekocode/analyzers/go_analyzer.hpp"
#include "nekocode/analyzers/rust_analyzer.hpp"
#include <algorithm>

// 🔧 グローバルデバッグフラグ定義  
bool g_debug_mode = false;
bool g_quiet_mode = true;   // Claude Code用：デフォルトでstderr出力抑制（Geminiクラッシュ対策）
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
            // PEGTL版を使用（メンバ変数検出対応）
            return std::make_unique<PythonPEGTLAnalyzer>();
            
        case Language::CSHARP:
            // PEGTL版を使用（std::regex版から移行）
            return std::make_unique<CSharpPEGTLAnalyzer>();
            
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
        // PEGTL版を使用（メンバ変数検出対応）
        return std::make_unique<PythonPEGTLAnalyzer>();
    }
    
    // C#
    if (ext == ".cs" || ext == ".csx") {
        // PEGTL版を使用（メンバ変数検出対応）
        return std::make_unique<CSharpPEGTLAnalyzer>();
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
    std::cerr << "🎮 Creating Unity Analyzer" << std::endl;
    return std::make_unique<UnityAnalyzer>();
}

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_unity_analyzer_from_file(
    const std::string& filename, 
    const std::string& content_preview
) {
    // Unity プロジェクトの判定
    if (content_preview.find("using UnityEngine") != std::string::npos ||
        content_preview.find(": MonoBehaviour") != std::string::npos ||
        content_preview.find(": ScriptableObject") != std::string::npos ||
        content_preview.find("[SerializeField]") != std::string::npos) {
        std::cerr << "🎮 Unity project detected, creating Unity Analyzer for: " << filename << std::endl;
        return std::make_unique<UnityAnalyzer>();
    }
    
    // Unity プロジェクトでない場合は通常のC#アナライザーを返す
    std::cerr << "⚙️ Standard C# file detected, using C# PEGTL Analyzer for: " << filename << std::endl;
    return std::make_unique<CSharpPEGTLAnalyzer>();
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