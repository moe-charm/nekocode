//=============================================================================
// 🧪 全正規表現パターンテスト
//=============================================================================

#include <iostream>
#include <regex>
#include <string>
#include <vector>

struct RegexTest {
    std::string name;
    std::string pattern;
};

int main() {
    std::cout << "🧪 全正規表現パターンテスト" << std::endl;
    std::cout << "================================" << std::endl;
    
    // core.cppから抽出したすべての正規表現パターン
    std::vector<RegexTest> patterns = {
        // JavaScriptAnalyzer
        {"ES6クラス", R"(class\s+(\w+)(?:\s+extends\s+(\w+))?\s*\{)"},
        {"プロトタイプ(簡略版)", R"(function\s+(\w+)\s*\([^)]*\)\s*\{)"},
        {"関数定義", R"(function\s+(\w+)\s*\(([^)]*)\)\s*\{)"},
        {"アロー関数", R"((?:const|let|var)\s+(\w+)\s*=\s*\([^)]*\)\s*=>)"},
        {"async関数", R"(async\s+function\s+(\w+)\s*\([^)]*\)\s*\{)"},
        {"ES6 import", R"(import\s+(?:[\w\s,{}*]+\s+from\s+)?['""]([^'""]+)['""])"},
        {"CommonJS require(正しい)", R"(require\s*\(\s*['""]([^'""]+)['""]\s*\))"},
        {"ES6 export", R"(export\s+(?:default\s+)?(?:const|let|var|function|class)?\s*(\w+))"},
        {"関数呼び出し", R"((\w+)\s*\()"},
        {"メソッド呼び出し", R"((\w+)\.(\w+)\s*\()"},
        
        // 動的生成される可能性のあるパターン
        {"プロトタイプメソッド", R"(MyClass\.prototype\.(\w+)\s*=)"},
        {"クラス開始", R"(class\s+MyClass(?:\s+extends\s+\w+)?\s*\{)"},
        {"メソッド検索", R"((\w+)\s*\([^)]*\)\s*\{)"},
        {"CommonJS export", R"((?:module\.)?exports(?:\.(\w+))?\s*=)"},
        
        // CppAnalyzer
        {"名前空間", R"(namespace\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)"},
        {"C++クラス", R"((class|struct|union)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*([^{]+))?\s*\{)"},
        {"C++関数", R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\s*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?\s*[{;])"},
        {"インクルード", R"(#include\s*([<"])([^>"]+)[>"])"},
        {"テンプレート", R"(template\s*<[^>]*>)"},
        
        // コメント除去用
        {"単行コメント", R"(//.*$)"},
        {"複数行コメント", R"(/\*[\s\S]*?\*/)"},
        {"文字列リテラル", R"("(?:[^"\\]|\\.)*")"},
        {"文字リテラル", R"('(?:[^'\\]|\\.)*')"},
        
        // 問題のある可能性のあるパターン
        {"CommonJS require(間違い)", R"(require\s*\(\s*['""]([^'""]+)['""])\s*\))"},
        {"エスケープ用", R"([\.\+\*\?\[\^\]\$\(\)\{\}\|\\])"},
        {"Raw string literal", R"(R\"[^(]*\(.*?\)[^\"]*\")"}
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : patterns) {
        try {
            std::regex re(test.pattern, std::regex_constants::ECMAScript);
            std::cout << "✅ " << test.name << std::endl;
            passed++;
        } catch (const std::regex_error& e) {
            std::cout << "❌ " << test.name << " - エラー: " << e.what() << std::endl;
            std::cout << "   パターン: " << test.pattern << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n📊 結果: " << passed << "/" << (passed + failed) << " 成功" << std::endl;
    
    if (failed > 0) {
        std::cout << "❌ " << failed << " 個の正規表現でエラーが発生しました。" << std::endl;
    } else {
        std::cout << "✅ すべての正規表現が正常です！" << std::endl;
    }
    
    return failed;
}