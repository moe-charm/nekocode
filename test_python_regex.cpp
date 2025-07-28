#include <iostream>
#include <regex>
#include <string>

int main() {
    std::string test_code = R"(
class Configuration:
    """設定クラス"""
    def __init__(self):
        pass

def calculate_complexity(code_lines):
    return 1

async def main():
    pass
)";

    // クラス検出テスト
    std::regex class_regex(R"(^class\s+(\w+)(?:\s*\([^)]*\))?\s*:)", std::regex::multiline);
    std::sregex_iterator class_iter(test_code.begin(), test_code.end(), class_regex);
    std::sregex_iterator end;
    
    std::cout << "Classes found:" << std::endl;
    while (class_iter != end) {
        std::cout << "  - " << (*class_iter)[1].str() << std::endl;
        ++class_iter;
    }
    
    // 関数検出テスト
    std::regex func_regex(R"(^(?:async\s+)?def\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[^:]+)?\s*:)", std::regex::multiline);
    std::sregex_iterator func_iter(test_code.begin(), test_code.end(), func_regex);
    
    std::cout << "\nFunctions found:" << std::endl;
    while (func_iter != end) {
        std::cout << "  - " << (*func_iter)[1].str() << std::endl;
        ++func_iter;
    }
    
    return 0;
}