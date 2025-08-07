// Debug Test: RustAnalyzer直接テスト
#include "include/nekocode/analyzers/rust_analyzer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <rust_file>" << std::endl;
        return 1;
    }
    
    // ファイル読み込み
    std::ifstream file(argv[1]);
    std::string content;
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
    } else {
        std::cerr << "Failed to read file: " << argv[1] << std::endl;
        return 1;
    }
    
    // RustAnalyzer直接テスト
    nekocode::RustAnalyzer analyzer;
    auto result = analyzer.analyze(content, argv[1]);
    
    // クラス情報出力
    std::cout << "=== CLASSES ===" << std::endl;
    for (const auto& cls : result.classes) {
        std::cout << "Class: " << cls.name << " (line " << cls.start_line << ")" << std::endl;
        std::cout << "  Methods count: " << cls.methods.size() << std::endl;
        for (const auto& method : cls.methods) {
            std::cout << "    - " << method.name << " (line " << method.start_line << ")" << std::endl;
        }
    }
    
    // 関数情報出力
    std::cout << "\\n=== FUNCTIONS ===" << std::endl;
    for (const auto& func : result.functions) {
        std::cout << "Function: " << func.name << " (line " << func.start_line << ")" << std::endl;
    }
    
    std::cout << "\\nTotal classes: " << result.classes.size() << std::endl;
    std::cout << "Total functions: " << result.functions.size() << std::endl;
    
    return 0;
}