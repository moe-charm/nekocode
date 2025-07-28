#include <iostream>
#include <regex>
#include <string>

int main() {
    // C#のメソッドパターン（現在のcsharp_analyzer.cppから）
    std::regex method_pattern(
        R"((?:(?:public|private|protected|internal|static|virtual|override|abstract|async)\s+)*)"
        R"((?:[\w\.<>]+\??)\s+)"          // 戻り値型（null許容型対応）
        R"((\w+)\s*)"
        R"(\([^)]*\)\s*)"
        R"((?:\{|=>))"                   // メソッド本体開始
    );
    
    // テストケース
    std::string test_code = R"(
        public async Task<ActionResult<UserDto>> GetUser(int id)
        {
            // method body
        }
        
        private void SimpleMethod()
        {
        }
    )";
    
    std::cout << "Testing C# method pattern...\n";
    std::cout << "Test code:\n" << test_code << "\n\n";
    
    std::sregex_iterator iter(test_code.begin(), test_code.end(), method_pattern);
    std::sregex_iterator end;
    
    int count = 0;
    while (iter != end) {
        std::cout << "Match " << ++count << ": " << iter->str() << "\n";
        std::cout << "Method name: " << (*iter)[1].str() << "\n\n";
        ++iter;
    }
    
    if (count == 0) {
        std::cout << "No matches found!\n";
        
        // より単純なパターンでテスト
        std::regex simple_pattern(R"((\w+)\s*\([^)]*\)\s*\{)");
        std::sregex_iterator simple_iter(test_code.begin(), test_code.end(), simple_pattern);
        
        std::cout << "\nTrying simpler pattern...\n";
        while (simple_iter != end) {
            std::cout << "Simple match: " << simple_iter->str() << "\n";
            ++simple_iter;
        }
    }
    
    return 0;
}