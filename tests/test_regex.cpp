//=============================================================================
// 🧪 正規表現問題デバッグ用テストプログラム
//=============================================================================

#include <iostream>
#include <regex>
#include <string>
#include <exception>
#include <vector>

// テスト用マクロ
#define TEST_REGEX(name, pattern) \
    try { \
        std::regex test_regex(pattern); \
        std::cout << "✅ " << name << ": OK" << std::endl; \
    } catch (const std::regex_error& e) { \
        std::cout << "❌ " << name << ": FAILED - " << e.what() << std::endl; \
        std::cout << "   Pattern: " << pattern << std::endl; \
    }

int main() {
    std::cout << "🧪 正規表現テスト開始..." << std::endl;
    std::cout << "================================" << std::endl;
    
    // 基本的な正規表現
    TEST_REGEX("基本パターン", R"(\w+)");
    TEST_REGEX("括弧付き", R"((\w+))");
    TEST_REGEX("関数呼び出し", R"((\w+)\s*\()");
    
    // JavaScript解析用
    std::cout << "\n📜 JavaScript解析パターン:" << std::endl;
    TEST_REGEX("クラス定義", R"(class\s+(\w+)(?:\s+extends\s+(\w+))?\s*\{)");
    TEST_REGEX("関数定義", R"(function\s+(\w+)\s*\(([^)]*)\)\s*\{)");
    TEST_REGEX("アロー関数", R"((?:const|let|var)\s+(\w+)\s*=\s*\([^)]*\)\s*=>)");
    TEST_REGEX("メソッド呼び出し", R"((\w+)\.(\w+)\s*\()");
    
    // 問題のありそうなパターン
    std::cout << "\n⚠️ 問題の可能性があるパターン:" << std::endl;
    TEST_REGEX("CommonJS require (修正前)", R"(require\s*\(\s*['""]([^'""]+)['""])\s*\))");
    TEST_REGEX("CommonJS require (修正後)", R"(require\s*\(\s*['""]([^'""]+)['""]\s*\))");
    TEST_REGEX("ES6 import", R"(import\s+(?:[\w\s,{}*]+\s+from\s+)?['""]([^'""]+)['""])");
    TEST_REGEX("Function (問題の可能性)", R"(function\s+(\w+)\s*\([^)]*\)\s*\{[\s\S]*?\1\.prototype\.(\w+))");
    TEST_REGEX("CommonJS Export", R"((?:module\.)?exports(?:\.(\w+))?\s*=)");
    
    // C++解析用
    std::cout << "\n🔧 C++解析パターン:" << std::endl;
    TEST_REGEX("名前空間", R"(namespace\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
    TEST_REGEX("関数(C++)", R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\s*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?\s*[{;])");
    TEST_REGEX("インクルード", R"(#include\s*([<"])([^>"]+)[>"])");
    TEST_REGEX("Raw string literal", R"(R\"[^(]*\(.*?\)[^\"]*\")");
    
    // 動的正規表現（エスケープテスト）
    std::cout << "\n🔄 動的正規表現テスト:" << std::endl;
    std::string class_name = "TestClass";
    try {
        std::string pattern1 = class_name + R"(\.prototype\.(\w+)\s*=)";
        std::regex test1(pattern1);
        std::cout << "✅ プロトタイプパターン: OK" << std::endl;
    } catch (const std::regex_error& e) {
        std::cout << "❌ プロトタイプパターン: FAILED - " << e.what() << std::endl;
    }
    
    try {
        // エスケープ付き
        std::string escaped_name = std::regex_replace(class_name, std::regex(R"([\.\+\*\?\[\^\]\$\(\)\{\}\|\\])"), R"(\$&)");
        std::string pattern2 = escaped_name + R"(\.prototype\.(\w+)\s*=)";
        std::regex test2(pattern2);
        std::cout << "✅ エスケープ付きパターン: OK" << std::endl;
    } catch (const std::regex_error& e) {
        std::cout << "❌ エスケープ付きパターン: FAILED - " << e.what() << std::endl;
    }
    
    std::cout << "\n✨ テスト完了" << std::endl;
    return 0;
}