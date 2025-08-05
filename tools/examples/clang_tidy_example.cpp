// Clang-Tidyが検出・修正できる問題の例

#include <iostream>
#include <memory>
#include <vector>

// 1. 古いスタイルのコード（modernize-*）
class OldStyle {
    int* ptr;  // ❌ 生ポインタ → unique_ptr推奨
public:
    OldStyle() : ptr(new int(42)) {}
    ~OldStyle() { delete ptr; }  // ❌ 手動メモリ管理
};

// 2. パフォーマンス問題（performance-*）
void inefficient_function(std::vector<int> vec) {  // ❌ 値渡し → const&推奨
    for (int i = 0; i < vec.size(); i++) {  // ❌ size()を毎回呼ぶ
        std::cout << vec[i];
    }
}

// 3. バグの可能性（bugprone-*）
void potential_bug() {
    int arr[10];
    for (int i = 0; i <= 10; i++) {  // ❌ 配列外アクセス！
        arr[i] = i;
    }
}

// 4. 未使用コード（misc-unused-*）
static void unused_static_function() {  // ❌ 未使用関数
    std::cout << "Never called!";
}

// 5. C++11以降の機能を使うべき（modernize-use-*）
void old_loop() {
    std::vector<int> vec = {1, 2, 3};
    // ❌ 古いスタイルのループ
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it;
    }
    // ✅ Clang-Tidyが提案: for (auto& item : vec) { ... }
}

// 6. 読みやすさ（readability-*）
int x = 1;  // ❌ グローバル変数は避けるべき

class BadNaming {  // ❌ クラス名は大文字始まり推奨
    int mVariable;  // ❌ メンバ変数の命名規則
public:
    int getVariable() { return mVariable; }  // ❌ constメソッドにすべき
};

// 7. Google/LLVM等のコーディングスタイル準拠
namespace my_namespace {  // スタイルガイドに従った命名
class MyClass {
    // コーディングスタイルチェック
};
}

int main() {
    // Clang-Tidyが検出する様々な問題の例
    return 0;
}