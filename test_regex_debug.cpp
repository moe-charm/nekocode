#include <iostream>
#include <regex>
#include <string>

int main() {
    // 修正後の正規表現をテスト
    std::regex class_pattern = std::regex(
        R"((?:(?:public|private|protected|internal|abstract|sealed|static|partial)\s+)*)"
        R"(class\s+(\w+)(?:\s*<[^>]*>)?(?:\s*:\s*([^{]+))?\s*\{)"
    );
    
    // テストコード
    std::string test_code = "namespace Test { public class SimpleClass { } }";
    
    std::sregex_iterator iter(test_code.begin(), test_code.end(), class_pattern);
    std::sregex_iterator end;
    
    int count = 0;
    while (iter != end) {
        count++;
        std::cout << "Found class: " << (*iter)[1].str() << std::endl;
        ++iter;
    }
    
    std::cout << "Total classes found: " << count << std::endl;
    return 0;
}