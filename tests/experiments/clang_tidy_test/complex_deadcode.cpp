// より複雑なデッドコードパターン
#include <iostream>
#include <vector>
#include <memory>

// 1. 名前空間内の未使用関数
namespace utils {
    void used_util() {
        std::cout << "Used utility\n";
    }
    
    void unused_util() {  // デッドコード
        std::cout << "Unused utility\n";
    }
}

// 2. テンプレート（使用/未使用）
template<typename T>
T used_template(T a, T b) {
    return a + b;
}

template<typename T>
T unused_template(T x) {  // デッドコード
    return x * 2;
}

// 3. クラス階層
class Base {
public:
    virtual void used_virtual() {
        std::cout << "Base::used\n";
    }
    
    virtual void unused_virtual() {  // デッドコード？
        std::cout << "Base::unused\n";
    }
};

class Derived : public Base {
public:
    void used_virtual() override {
        std::cout << "Derived::used\n";
    }
};

// 4. 静的メンバ関数
class Helper {
public:
    static void used_static() {
        std::cout << "Used static\n";
    }
    
    static void unused_static() {  // デッドコード
        std::cout << "Unused static\n";
    }
};

// 5. インライン関数
inline void used_inline() {
    std::cout << "Used inline\n";
}

inline void unused_inline() {  // デッドコード
    std::cout << "Unused inline\n";
}

// 6. 相互依存（デッドコードループ）
void dead_loop_a();
void dead_loop_b() { dead_loop_a(); }
void dead_loop_a() { dead_loop_b(); }

// 7. マクロ定義関数
#define UNUSED_MACRO() std::cout << "Macro unused"

int main() {
    // 実際に使用されるもののみ
    utils::used_util();
    
    int sum = used_template<int>(1, 2);
    std::cout << "Sum: " << sum << "\n";
    
    Derived d;
    Base* b = &d;
    b->used_virtual();
    
    Helper::used_static();
    
    used_inline();
    
    return 0;
}