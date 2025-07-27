//=============================================================================
// 🧪 最小限の正規表現テスト
//=============================================================================

#include <iostream>
#include <regex>
#include <string>

void test_regex(const std::string& name, const std::string& pattern, const std::string& test_str) {
    try {
        std::regex re(pattern);
        std::smatch match;
        if (std::regex_search(test_str, match, re)) {
            std::cout << "✅ " << name << " - マッチ成功: " << match[0].str() << std::endl;
        } else {
            std::cout << "🔍 " << name << " - マッチなし" << std::endl;
        }
    } catch (const std::regex_error& e) {
        std::cout << "❌ " << name << " - エラー: " << e.what() << std::endl;
        std::cout << "   パターン: " << pattern << std::endl;
    }
}

int main() {
    std::cout << "🧪 正規表現エラー特定テスト" << std::endl;
    std::cout << "================================" << std::endl;
    
    // テスト用コード
    std::string js_code = R"(
        const fs = require('fs');
        function MyClass() { }
        MyClass.prototype.method = function() { };
        class NewClass extends BaseClass { }
    )";
    
    // 問題のありそうな正規表現を順番にテスト
    std::cout << "\n1️⃣ CommonJS require パターン:" << std::endl;
    test_regex("require (間違い)", R"(require\s*\(\s*['""]([^'""]+)['""])\s*\))", js_code);
    test_regex("require (修正版)", R"(require\s*\(\s*['""]([^'""]+)['""]\s*\))", js_code);
    
    std::cout << "\n2️⃣ プロトタイプパターン:" << std::endl;
    test_regex("prototype (後方参照あり)", R"(function\s+(\w+)\s*\([^)]*\)\s*\{[\s\S]*?\1\.prototype\.(\w+))", js_code);
    test_regex("prototype (簡略版)", R"(function\s+(\w+)\s*\([^)]*\)\s*\{)", js_code);
    test_regex("prototype メソッド", R"((\w+)\.prototype\.(\w+)\s*=)", js_code);
    
    std::cout << "\n3️⃣ その他のパターン:" << std::endl;
    test_regex("関数呼び出し", R"((\w+)\s*\()", js_code);
    test_regex("メソッド呼び出し", R"((\w+)\.(\w+)\s*\()", js_code);
    
    // 動的正規表現テスト
    std::cout << "\n4️⃣ 動的正規表現:" << std::endl;
    std::string class_name = "MyClass";
    std::string pattern1 = class_name + R"(\.prototype\.(\w+)\s*=)";
    test_regex("動的prototype", pattern1, js_code);
    
    // エスケープテスト
    std::cout << "\n5️⃣ エスケープパターンテスト:" << std::endl;
    test_regex("エスケープ用", R"([\.\+\*\?\[\^\]\$\(\)\{\}\|\\])", "test.class");
    
    return 0;
}