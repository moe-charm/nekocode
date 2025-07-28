#include <iostream>
#include "include/nekocode/core.hpp"
#include "include/nekocode/pegtl_analyzer.hpp"

int main() {
    nekocode::NekoCodeCore core(nekocode::AnalysisConfig{});
    
    auto result = core.analyze_file("/mnt/workdisk/public_share/nyacore-workspace/nekocode-cpp-github/test_python.py");
    
    if (result.is_success()) {
        const auto& analysis = result.value();
        std::cout << "Analysis successful!" << std::endl;
        std::cout << "Language: " << static_cast<int>(analysis.language) << std::endl;
        std::cout << "Classes: " << analysis.classes.size() << std::endl;
        std::cout << "Functions: " << analysis.functions.size() << std::endl;
        std::cout << "Imports: " << analysis.imports.size() << std::endl;
        
        std::cout << "\nClasses found:" << std::endl;
        for (const auto& cls : analysis.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")" << std::endl;
        }
        
        std::cout << "\nFunctions found:" << std::endl;
        for (const auto& func : analysis.functions) {
            std::cout << "  - " << func.name << " (line " << func.start_line << ")" << std::endl;
        }
    } else {
        std::cout << "Analysis failed: " << result.error().message << std::endl;
    }
    
    return 0;
}