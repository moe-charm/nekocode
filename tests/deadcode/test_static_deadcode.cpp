// 静的デッドコード検出テスト
#include <iostream>

// 使用される静的関数
static void used_static_function() {
    std::cout << "This static function is used" << std::endl;
}

// 未使用静的関数（検出対象）
static void unused_static_function() {
    std::cout << "This static function is NOT used!" << std::endl;
}

// 未使用静的変数（検出対象）
static int unused_static_variable = 42;

// 使用される静的変数
static const char* program_name = "StaticDeadCodeTest";

int main() {
    // 使用される静的関数を呼び出し
    used_static_function();
    
    // 使用される変数を使用
    std::cout << "Program: " << program_name << std::endl;
    
    return 0;
}