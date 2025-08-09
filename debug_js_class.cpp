#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "src/analyzers/javascript/javascript_pegtl_analyzer.hpp"

int main() {
    std::cout << "🔍 JavaScript Class Detection Debug\n";
    std::cout << "=====================================\n\n";
    
    // Test case 1: Simple class
    std::string test1 = R"(
class SimpleClass {
    constructor() {
        this.value = 42;
    }
}
)";
    
    // Test case 2: Export class
    std::string test2 = R"(
export class ExportedClass {
    method() {
        return "test";
    }
}
)";
    
    // Test case 3: Class extending React.Component (問題のケース)
    std::string test3 = R"(
export class NativeClass extends React.Component {
    render() {
        return this.props.children;
    }
}
)";
    
    // Test case 4: Class extending simple identifier
    std::string test4 = R"(
export class TestClass extends Component {
    render() {
        return "test";
    }
}
)";
    
    std::cout << "Test 1: Simple Class\n";
    std::cout << "--------------------\n";
    {
        nekocode::JavaScriptPEGTLAnalyzer analyzer;
        auto result = analyzer.analyze(test1, "test1.js");
        std::cout << "Classes found: " << result.classes.size() << "\n";
        for (const auto& cls : result.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "Test 2: Exported Class\n";
    std::cout << "----------------------\n";
    {
        nekocode::JavaScriptPEGTLAnalyzer analyzer;
        auto result = analyzer.analyze(test2, "test2.js");
        std::cout << "Classes found: " << result.classes.size() << "\n";
        for (const auto& cls : result.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "Test 3: Class extends React.Component (PROBLEM CASE)\n";
    std::cout << "----------------------------------------------------\n";
    {
        nekocode::JavaScriptPEGTLAnalyzer analyzer;
        auto result = analyzer.analyze(test3, "test3.js");
        std::cout << "Classes found: " << result.classes.size() << "\n";
        for (const auto& cls : result.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")\n";
        }
        if (result.classes.empty()) {
            std::cout << "  ❌ NO CLASSES DETECTED! This is the bug!\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "Test 4: Class extends Component (without dot)\n";
    std::cout << "---------------------------------------------\n";
    {
        nekocode::JavaScriptPEGTLAnalyzer analyzer;
        auto result = analyzer.analyze(test4, "test4.js");
        std::cout << "Classes found: " << result.classes.size() << "\n";
        for (const auto& cls : result.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")\n";
        }
        std::cout << "\n";
    }
    
    // Test with actual Components.js content
    std::cout << "Test 5: Actual Components.js Content\n";
    std::cout << "------------------------------------\n";
    std::string components_content = R"(// Example

export const Throw = React.lazy(() => {
  throw new Error('Example');
});

export const Component = React.memo(function Component({children}) {
  return children;
});

export function DisplayName({children}) {
  return children;
}
DisplayName.displayName = 'Custom Name';

export class NativeClass extends React.Component {
  render() {
    return this.props.children;
  }
}

export class FrozenClass extends React.Component {
  constructor() {
    super();
  }
  render() {
    return this.props.children;
  }
}
Object.freeze(FrozenClass.prototype);
)";
    
    {
        nekocode::JavaScriptPEGTLAnalyzer analyzer;
        auto result = analyzer.analyze(components_content, "Components.js");
        std::cout << "Classes found: " << result.classes.size() << " (Expected: 2)\n";
        for (const auto& cls : result.classes) {
            std::cout << "  - " << cls.name << " (line " << cls.start_line << ")\n";
        }
        std::cout << "Functions found: " << result.functions.size() << "\n";
        for (const auto& func : result.functions) {
            std::cout << "  - " << func.name << " (line " << func.start_line << ")\n";
        }
        
        if (result.classes.size() != 2) {
            std::cout << "\n❌ BUG CONFIRMED: Expected 2 classes (NativeClass, FrozenClass) but found " 
                     << result.classes.size() << "\n";
        }
    }
    
    return 0;
}