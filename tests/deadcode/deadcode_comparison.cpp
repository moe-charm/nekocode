// Clang-Tidy vs LTO の検出能力比較

#include <iostream>

// 1. ファイル内static関数（未使用）
static void local_unused_func() {  // Clang-Tidy ✅ / LTO ✅
    std::cout << "Never called";
}

// 2. グローバル関数（未使用）
void global_unused_func() {        // Clang-Tidy ❌ / LTO ✅
    std::cout << "Might be used externally?";
}

// 3. 使用される関数
void used_function() {
    std::cout << "This is used";
}

// 4. 未使用だが、インライン展開される可能性
inline void inline_unused() {      // Clang-Tidy ❌ / LTO ✅
    std::cout << "Inline but unused";
}

// 5. テンプレート（インスタンス化されない）
template<typename T>
void unused_template() {           // Clang-Tidy ❌ / LTO ✅
    std::cout << "Template never instantiated";
}

// 6. クラス内
class MyClass {
private:
    int unused_member = 42;        // Clang-Tidy ✅ / LTO ❓
    
    void unused_private() {        // Clang-Tidy ✅ / LTO ✅
        std::cout << "Private unused";
    }
    
public:
    void unused_public() {         // Clang-Tidy ❌ / LTO ✅
        std::cout << "Public but unused";
    }
};

// 7. 相互参照（でも未使用）
void circular_a();
void circular_b() { circular_a(); }
void circular_a() { circular_b(); }  // Clang-Tidy ❌ / LTO ✅（両方削除）

// 8. コンストラクタで使用される
int initialized_var = [](){        // Clang-Tidy ❌ / LTO ❌
    std::cout << "Init";
    return 1;
}();

// 9. デバッグ用（ifdef で無効）
#ifdef DEBUG
void debug_only() {                // Clang-Tidy ✅ / LTO N/A
    std::cout << "Debug";
}
#endif

int main() {
    used_function();
    // MyClassはインスタンス化されない
    return 0;
}