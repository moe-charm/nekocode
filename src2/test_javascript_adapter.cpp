//=============================================================================
// 🧪 JavaScript Universal Adapter Test - 統一システム検証
//=============================================================================

#include "adapters/javascript_universal_adapter.hpp"
#include <iostream>
#include <string>

using namespace nekocode::adapters;

int main() {
    // 🔥 JavaScript Universal Adapter テスト
    std::cout << "🚀 JavaScript Universal Adapter Test Starting...\n";
    
    JavaScriptUniversalAdapter adapter;
    
    // テスト用JavaScript コード
    std::string test_code = R"(
class MyClass {
    constructor(name) {
        this.name = name;
    }
    
    async getData() {
        const result = await fetch('/api/data');
        return result.json();
    }
    
    processData = (data) => {
        return data.map(item => item.value);
    }
}

function processArray(arr) {
    return arr.filter(x => x > 0)
              .map(x => x * 2);
}

const asyncFunc = async () => {
    try {
        const data = await processData();
        return data;
    } catch (error) {
        console.error(error);
    }
};
)";

    try {
        std::cout << "📊 Analyzing JavaScript code...\n";
        
        // 解析実行
        auto result = adapter.analyze(test_code, "test.js");
        
        std::cout << "✅ Analysis completed!\n";
        std::cout << "📈 Results:\n";
        std::cout << "  - Language: " << adapter.get_language_name() << "\n";
        std::cout << "  - Classes: " << result.classes.size() << "\n";
        std::cout << "  - Functions: " << result.functions.size() << "\n";
        std::cout << "  - File size: " << result.file_info.size_bytes << " bytes\n";
        
        // AST統計確認
        auto ast_stats = adapter.get_ast_statistics();
        std::cout << "🌳 AST Statistics:\n";
        std::cout << "  - AST Classes: " << ast_stats.classes << "\n";
        std::cout << "  - AST Functions: " << ast_stats.functions << "\n";
        std::cout << "  - AST Variables: " << ast_stats.variables << "\n";
        std::cout << "  - Max Depth: " << ast_stats.max_depth << "\n";
        
        // JavaScript特化機能テスト
        auto async_functions = adapter.find_async_functions();
        std::cout << "⚡ Async Functions Found: " << async_functions.size() << "\n";
        for (const auto& func : async_functions) {
            std::cout << "  - " << func << "\n";
        }
        
        std::cout << "🎉 JavaScript Universal Adapter Test PASSED!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test FAILED: " << e.what() << "\n";
        return 1;
    }
}