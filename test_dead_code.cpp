// デッドコード検出テスト
#include <iostream>
#include <string>
#include <vector>

// 使用される関数
void used_function() {
    std::cout << "This function is used" << std::endl;
}

// 未使用関数（検出対象）
void unused_function() {
    std::cout << "This function is NOT used!" << std::endl;
}

// 使用されるクラス
class UsedClass {
public:
    void method() {
        std::cout << "Used class method" << std::endl;
    }
};

// 未使用クラス（検出対象）
class UnusedClass {
public:
    void method() {
        std::cout << "Unused class method" << std::endl;
    }
};

// 別の使用される関数
int calculate(int a, int b) {
    return a + b;
}

// 未使用のテンプレート関数（検出対象）
template<typename T>
T unused_template(T a, T b) {
    return a + b;
}

// メイン関数
int main() {
    // 使用される関数を呼び出し
    used_function();
    
    // クラスの使用
    UsedClass obj;
    obj.method();
    
    // 計算関数の使用
    int result = calculate(5, 3);
    std::cout << "Result: " << result << std::endl;
    
    return 0;
}

// 未使用のグローバル変数（検出対象）
int unused_global_variable = 42;

// 使用されるグローバル変数
const char* program_name = "DeadCodeTest";