// Clang-Tidyが修正提案するコード例
#include <iostream>
#include <memory>
#include <vector>

// 1. modernize-use-nullptr
void old_style_null() {
    int* p1 = NULL;      // → nullptr
    int* p2 = 0;         // → nullptr
}

// 2. modernize-use-auto
void old_style_types() {
    std::vector<int>::iterator it = std::vector<int>().begin();  // → auto
    std::unique_ptr<int> ptr = std::make_unique<int>(42);       // → auto
}

// 3. performance-unnecessary-copy-initialization
void inefficient_copy() {
    std::string original = "Hello";
    std::string copy = original;  // 不要なコピー（const&推奨）
    std::cout << copy;
}

// 4. readability-container-size-empty
void check_container() {
    std::vector<int> vec;
    if (vec.size() == 0) {  // → vec.empty()
        std::cout << "Empty";
    }
}

// 5. modernize-loop-convert
void old_loop() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // 古いスタイルのループ
    for (std::vector<int>::iterator it = numbers.begin(); 
         it != numbers.end(); ++it) {
        std::cout << *it << " ";  // → for (auto& num : numbers)
    }
}

// 6. readability-redundant-string-cstr
void redundant_cstr() {
    std::string str = "test";
    std::cout << str.c_str();  // .c_str()は不要
}

// 7. misc-unused-parameters
void unused_param(int used, int unused) {  // unusedパラメータ
    std::cout << used;
}

// 8. bugprone-integer-division
void integer_division() {
    int a = 5;
    int b = 2;
    double result = a / b;  // 整数除算！結果は2.0
    std::cout << result;
}

int main() {
    old_style_null();
    old_style_types();
    inefficient_copy();
    check_container();
    old_loop();
    redundant_cstr();
    unused_param(42, 0);
    integer_division();
    
    return 0;
}