// テストファイル：C++用コメントアウト行検出機能のテスト

#include <iostream>
#include <string>
#include <vector>

class TestClass {
public:
    TestClass() {
        this->data = "initialized";
        // std::cout << "debug: constructor called" << std::endl;
        // __debugbreak();
    }
    
    /*
    void oldMethod() {
        return this->processData();
    }
    */
    
    std::string processData() {
        // TODO: implement proper validation 
        // const auto result = this->validateInput(this->data);
        
        /* 
        if (!result.isValid) {
            throw std::runtime_error("Invalid data");
        }
        */
        
        return this->data;
    }
    
    // This is just a regular comment explaining the code
    bool validateInput(const std::string& input) {
        return input.length() > 0;
    }
    
private:
    std::string data;
};

// int main() { return 0; }

namespace test {
    /*
    template<typename T>
    class OldTemplate {
        T value;
    public:
        void setValue(T val) { value = val; }
    };
    */
    
    // struct OldStruct { int x, y; };
}