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
#include "nekocode/analyzers/unity_analyzer.hpp"
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
        // TODO: Unity プロジェクト自動検出を実装
        // 現在は基本的な C# アナライザーを使用
        return std::make_unique<CSharpPEGTLAnalyzer>();
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
    std::cout << "🎮 Creating Unity Analyzer (Composition Design)" << std::endl;
    return std::make_unique<UnityAnalyzer>();
}

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_unity_analyzer_from_file(
    const std::string& filename, 
    const std::string& content_preview
) {
    std::string ext = get_extension(filename);
    
    // C# ファイルの場合、Unity コンテンツを検査
    if (ext == ".cs") {
        if (content_preview.find("using UnityEngine") != std::string::npos ||
            content_preview.find(": MonoBehaviour") != std::string::npos ||
            content_preview.find(": ScriptableObject") != std::string::npos) {
            std::cout << "🎮 Unity content detected! Using Unity Analyzer" << std::endl;
            return create_unity_analyzer();
        }
    }
    
    // 通常の C# アナライザーを返す
    return create_analyzer_from_extension(ext);
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