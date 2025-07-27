//=============================================================================
// 🧪 NekoCode Core 正規表現デバッグプログラム
//=============================================================================

#include <iostream>
#include <regex>
#include <string>
#include <exception>
#include "../include/nekocode/core.hpp"

int main() {
    std::cout << "🧪 NekoCode Core 正規表現初期化テスト開始..." << std::endl;
    std::cout << "================================" << std::endl;
    
    try {
        std::cout << "1️⃣ JavaScriptAnalyzer 作成中..." << std::endl;
        nekocode::JavaScriptAnalyzer js_analyzer;
        std::cout << "✅ JavaScriptAnalyzer 作成成功!" << std::endl;
        
        std::cout << "\n2️⃣ NekoCodeCore 作成中..." << std::endl;
        nekocode::AnalysisConfig config;
        config.analyze_complexity = false;
        config.analyze_dependencies = false;
        config.analyze_function_calls = false;
        nekocode::NekoCodeCore core(config);
        std::cout << "✅ NekoCodeCore 作成成功!" << std::endl;
        
        std::cout << "\n3️⃣ シンプルなコード解析中..." << std::endl;
        std::string simple_code = "function test() { return 42; }";
        auto result = core.analyze_content(simple_code, "test.js");
        
        if (result.is_success()) {
            std::cout << "✅ 解析成功!" << std::endl;
            std::cout << "   総行数: " << result.value().file_info.total_lines << std::endl;
            std::cout << "   関数数: " << result.value().functions.size() << std::endl;
        } else {
            std::cout << "❌ 解析失敗: " << result.error().message << std::endl;
        }
        
    } catch (const std::regex_error& e) {
        std::cout << "❌ 正規表現エラー: " << e.what() << std::endl;
        std::cout << "   コード: " << e.code() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ エラー: " << e.what() << std::endl;
    }
    
    std::cout << "\n✨ テスト完了" << std::endl;
    return 0;
}