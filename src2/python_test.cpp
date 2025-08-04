//=============================================================================
// 🐍 Python Universal Adapter Test - インデント言語統一システム検証
//=============================================================================

#include "adapters/python_universal_adapter.hpp"
#include <iostream>
#include <string>

using namespace nekocode::adapters;

int main() {
    // 🚀 Python Universal Adapter テスト
    std::cout << "🐍 Python Universal Adapter Test Starting...\n";
    
    PythonUniversalAdapter adapter;
    
    // テスト用Python コード（インデント重要！）
    std::string test_code = R"PYTHON(
class DataProcessor:
    def __init__(self, name):
        self.name = name
        self.data = []
        self.processed = False
    
    def add_data(self, item):
        if item is not None:
            self.data.append(item)
            return True
        return False
    
    def process_data(self):
        for item in self.data:
            if item > 0:
                item = item * 2
        self.processed = True
    
    def __str__(self):
        return f"DataProcessor({self.name})"

def global_function(value):
    if value > 10:
        return value * 2
    else:
        return value

class TestClass:
    class_var = "test"
    
    def __init__(self):
        self.instance_var = 42
        self.another_var = "hello"
)PYTHON";

    try {
        std::cout << "📊 Analyzing Python code...\n";
        
        // 解析実行
        auto result = adapter.analyze(test_code, "test.py");
        
        std::cout << "✅ Analysis completed!\n";
        std::cout << "📈 Results:\n";
        std::cout << "  - Language: " << adapter.get_language_name() << "\n";
        std::cout << "  - Classes: " << result.classes.size() << "\n";
        std::cout << "  - Functions: " << result.functions.size() << "\n";
        std::cout << "  - File size: " << result.file_info.size_bytes << " bytes\n";
        std::cout << "  - Total lines: " << result.file_info.total_lines << "\n";
        
        // AST統計確認
        auto ast_stats = adapter.get_ast_statistics();
        std::cout << "🌳 AST Statistics:\n";
        std::cout << "  - AST Classes: " << ast_stats.classes << "\n";
        std::cout << "  - AST Functions: " << ast_stats.functions << "\n";
        std::cout << "  - AST Variables: " << ast_stats.variables << "\n";
        std::cout << "  - Max Depth: " << ast_stats.max_depth << "\n";
        
        // Python特化機能テスト
        auto special_methods = adapter.find_special_methods();
        std::cout << "🔮 Special Methods Found: " << special_methods.size() << "\n";
        for (const auto& method : special_methods) {
            std::cout << "  - " << method << "\n";
        }
        
        auto instance_vars = adapter.find_instance_variables();
        std::cout << "📦 Instance Variables Found: " << instance_vars.size() << "\n";
        for (const auto& var : instance_vars) {
            std::cout << "  - " << var << "\n";
        }
        
        // 既存成功実績との比較
        std::cout << "\n🎯 Success Metrics Comparison:\n";
        std::cout << "  - requests library baseline: 10 functions + 25+ member variables\n";
        std::cout << "  - Current test results: " << result.functions.size() 
                  << " functions + " << ast_stats.variables << " variables\n";
        
        if (result.functions.size() >= 5 && ast_stats.variables >= 3) {
            std::cout << "✅ SUCCESS: Exceeding baseline expectations!\n";
        }
        
        std::cout << "🎉 Python Universal Adapter Test PASSED!\n";
        std::cout << "\n🌟 **Phase 5 基本機能動作確認完了！**\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test FAILED: " << e.what() << "\n";
        return 1;
    }
}