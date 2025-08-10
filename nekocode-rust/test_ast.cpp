// C++ AST Test File
#include <iostream>
#include <string>

class TestClass {
private:
    std::string name;
    
public:
    TestClass(const std::string& n) : name(n) {}
    
    std::string greet() const {
        return "Hello, " + name + "!";
    }
    
    static int staticMethod() {
        return 42;
    }
};

int normalFunction(int x, int y) {
    return x + y;
}

template<typename T>
T templateFunction(T a, T b) {
    return a + b;
}

namespace TestNamespace {
    void namespaceFunction() {
        std::cout << "In namespace" << std::endl;
    }
}