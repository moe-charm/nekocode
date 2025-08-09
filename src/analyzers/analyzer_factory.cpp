//=============================================================================
// 🏭 Analyzer Factory - 言語別アナライザー生成ファクトリー
//
// 言語に応じた適切なアナライザーを生成
//=============================================================================

#include "base_analyzer.hpp"
// #include "nekocode/analyzers/javascript_analyzer.hpp" // regex版は削除済み
#include "javascript/javascript_pegtl_analyzer.hpp"
#include "typescript/typescript_pegtl_analyzer.hpp"
// #include "nekocode/analyzers/python_analyzer.hpp" // レガシー削除
#include "nekocode/analyzers/python_pegtl_analyzer.hpp"
#include "nekocode/analyzers/cpp_language_analyzer.hpp"
#include "nekocode/analyzers/cpp_pegtl_analyzer.hpp"
// #include "nekocode/analyzers/csharp_analyzer.hpp" // regex版は削除済み
#include "nekocode/analyzers/csharp_pegtl_analyzer.hpp"
#include "nekocode/analyzers/unity_analyzer.hpp"
#include "nekocode/analyzers/go_analyzer.hpp"
#include "nekocode/analyzers/rust_analyzer.hpp"

// 🌟 Universal AST Adapters - 6言語統一システム
#include "../adapters/javascript_universal_adapter.hpp"
#include "../adapters/python_universal_adapter.hpp"
#include "../adapters/cpp_universal_adapter.hpp"
#include "../adapters/csharp_universal_adapter.hpp"
#include "../adapters/go_universal_adapter.hpp"
#include "../adapters/rust_universal_adapter.hpp"

#include <algorithm>
#include <cstdlib> // for std::getenv

// 🔧 グローバルデバッグフラグ定義  
bool g_debug_mode = false;
bool g_quiet_mode = true;   // Claude Code用：デフォルトでstderr出力抑制（Geminiクラッシュ対策）
#include <cctype>

namespace nekocode {

//=============================================================================
// 🏭 AnalyzerFactory Implementation
//=============================================================================

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_analyzer(Language language) {
    // 🌟 Universal AST モード（常に有効：成熟したPEGTLを呼ぶ統一アーキテクチャ）
    // 旧デバッグ用環境変数チェックは削除
    constexpr bool use_universal_ast = true;
    
    switch (language) {
        case Language::JAVASCRIPT:
            // 🚀 Universal AST Adapter (成熟したPEGTLを呼び出し)
            return std::make_unique<adapters::JavaScriptUniversalAdapter>();
            
        case Language::TYPESCRIPT:
            // TODO: TypeScript Universal Adapter実装待ち
            // 現在はPEGTL版を使用（JavaScript拡張）
            return std::make_unique<TypeScriptPEGTLAnalyzer>();
            
        case Language::CPP:
            // ⚙️ Universal AST Adapter (CppPEGTLAnalyzerを呼び出し)
            return std::make_unique<adapters::CppUniversalAdapter>();
            
        case Language::C:
            // TODO: C Universal Adapter実装待ち
            return std::make_unique<CLanguageAnalyzer>();
            
        case Language::PYTHON:
            // 🐍 Universal AST Adapter (PythonPEGTLAnalyzerを呼び出し)
            return std::make_unique<adapters::PythonUniversalAdapter>();
            
        case Language::CSHARP:
            // 💎 Universal AST Adapter (CSharpPEGTLAnalyzerを呼び出し)
            return std::make_unique<adapters::CSharpUniversalAdapter>();
            
        case Language::GO:
            // 🟢 Universal AST Adapter (GoAnalyzerを呼び出し)
            return std::make_unique<adapters::GoUniversalAdapter>();
            
        case Language::RUST:
            // 🦀 Universal AST Adapter (RustAnalyzerを呼び出し)
            return std::make_unique<adapters::RustUniversalAdapter>();
            
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
        // 🚀 Universal AST Adapter を使用
        return std::make_unique<adapters::JavaScriptUniversalAdapter>();
    }
    
    // TypeScript
    if (ext == ".ts" || ext == ".tsx" || ext == ".mts" || ext == ".cts") {
        // PEGTL版を使用（JavaScript拡張）
        return std::make_unique<TypeScriptPEGTLAnalyzer>();
    }
    
    // C++
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || 
        ext == ".hpp" || ext == ".hxx" || ext == ".hh" || ext == ".h++") {
        // ⚙️ Universal AST Adapter を使用
        return std::make_unique<adapters::CppUniversalAdapter>();
    }
    
    // C (注意: .h は曖昧なので内容で判断が必要)
    if (ext == ".c") {
        return std::make_unique<CLanguageAnalyzer>();
    }
    
    // Python
    if (ext == ".py" || ext == ".pyw" || ext == ".pyi") {
        // 🐍 Universal AST Adapter を使用
        return std::make_unique<adapters::PythonUniversalAdapter>();
    }
    
    // C#
    if (ext == ".cs" || ext == ".csx") {
        // 💎 Universal AST Adapter を使用
        return std::make_unique<adapters::CSharpUniversalAdapter>();
    }
    
    // Go
    if (ext == ".go") {
        // 🟢 Universal AST Adapter を使用
        return std::make_unique<adapters::GoUniversalAdapter>();
    }
    
    // Rust
    if (ext == ".rs") {
        // 🦀 Universal AST Adapter を使用
        return std::make_unique<adapters::RustUniversalAdapter>();
    }
    
    // .h ファイルはデフォルトでC++として扱う
    if (ext == ".h") {
        // ⚙️ Universal AST Adapter を使用
        return std::make_unique<adapters::CppUniversalAdapter>();
    }
    
    return nullptr;
}

//=============================================================================
// 🎮 Unity 特化ファクトリー関数
//=============================================================================

std::unique_ptr<BaseAnalyzer> AnalyzerFactory::create_unity_analyzer() {
    // Unity Analyzer creation
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
        // Unity project detected
        return std::make_unique<UnityAnalyzer>();
    }
    
    // Unity プロジェクトでない場合は通常のC#アナライザーを返す
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