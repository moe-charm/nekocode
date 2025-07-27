// Template analysis test file
#include <iostream>
#include <vector>

// Class template
template<typename T, typename U>
class MyContainer {
public:
    T data;
    U metadata;
};

// Function template
template<typename T>
T max_value(T a, T b) {
    return a > b ? a : b;
}

// Variadic template
template<typename T, typename... Args>
void print_all(T first, Args... args) {
    std::cout << first;
    if constexpr (sizeof...(args) > 0) {
        print_all(args...);
    }
}

// Template specialization
template<>
class MyContainer<int, std::string> {
public:
    int data;
    std::string metadata;
    void special_method() {}
};

int main() {
    MyContainer<int, std::string> container;
    auto result = max_value(10, 20);
    print_all("Hello", " ", "World", "\n");
    return 0;
}