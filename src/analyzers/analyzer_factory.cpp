//=============================================================================
// 🏭 Analyzer Factory - 言語別アナライザー生成ファクトリー
//
// 言語に応じた適切なアナライザーを生成
//=============================================================================

#include "nekocode/analyzers/base_analyzer.hpp"
#include "nekocode/analyzers/javascript_analyzer.hpp"
#include "nekocode/analyzers/javascript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/typescript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/python_analyzer.hpp"
#include "nekocode/analyzers/python_pegtl_analyzer.hpp"
#include "nekocode/analyzers/cpp_language_analyzer.hpp"
#include "nekocode/analyzers/cpp_pegtl_analyzer.hpp"
#include "nekocode/analyzers/csharp_analyzer.hpp"
#include "nekocode/analyzers/csharp_pegtl_analyzer.hpp"
// #include "nekocode/analyzers/unity_analyzer.hpp"  // 一時的に無効化
#include <algorithm>
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
            // PEGTL版を使用（インデント地獄攻略）
            return std::make_unique<PythonPEGTLAnalyzer>();
            
        case Language::CSHARP:
            // PEGTL版を使用（std::regex版から移行）
            return std::make_unique<CSharpPEGTLAnalyzer>();
            
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
        // PEGTL版を使用（インデント地獄攻略）
        return std::make_unique<PythonPEGTLAnalyzer>();
    }
    
    // C#
    if (ext == ".cs" || ext == ".csx") {
        // PEGTL版を使用（std::regex版から移行）
        return std::make_unique<CSharpPEGTLAnalyzer>();
    }
    
    // .h ファイルはデフォルトでC++として扱う
    if (ext == ".h") {
        // PEGTL版を使用（Claude Code支援作戦）
        return std::make_unique<CppPEGTLAnalyzer>();
    }
    
    return nullptr;
}

} // namespace nekocode