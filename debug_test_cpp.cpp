//=============================================================================
// 🤖 C++ Complex Test Sample - テンプレート・クラス・複雑度テスト
//=============================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

// 🔧 テンプレートクラス
template<typename T, size_t N>
class FixedArray {
private:
    T data_[N];
    size_t size_;

public:
    FixedArray() : size_(0) {}
    
    // 🎯 テンプレート関数
    template<typename U>
    void push_back(U&& value) {
        if (size_ < N) {
            data_[size_++] = std::forward<U>(value);
        }
    }
    
    // 🔄 複雑な条件分岐
    T& at(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        
        // ネストした条件分岐で複雑度上昇
        if (index == 0) {
            return data_[0];
        } else if (index == 1) {
            return data_[1];
        } else if (index < size_ / 2) {
            return data_[index];
        } else {
            return data_[size_ - 1 - index];
        }
    }
    
    size_t size() const { return size_; }
};

// 🏭 抽象基底クラス
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
    virtual void print() const = 0;
};

// 🔺 派生クラス
class Triangle : public Shape {
private:
    double a_, b_, c_;
    
public:
    Triangle(double a, double b, double c) : a_(a), b_(b), c_(c) {}
    
    double area() const override {
        // ヘロンの公式（複雑な計算）
        double s = (a_ + b_ + c_) / 2.0;
        return sqrt(s * (s - a_) * (s - b_) * (s - c_));
    }
    
    double perimeter() const override {
        return a_ + b_ + c_;
    }
    
    void print() const override {
        std::cout << "Triangle(" << a_ << ", " << b_ << ", " << c_ << ")" << std::endl;
    }
};

// 🎯 複雑なアルゴリズム関数
template<typename Container, typename Predicate>
auto complex_filter_sort(Container& container, Predicate pred) -> decltype(container) {
    Container result;
    
    // 複雑なフィルタリングロジック
    for (auto it = container.begin(); it != container.end(); ++it) {
        if (pred(*it)) {
            // ネストしたループで複雑度上昇
            bool should_add = true;
            for (const auto& existing : result) {
                if (*it == existing) {
                    should_add = false;
                    break;
                }
            }
            
            if (should_add) {
                // さらにネストした条件
                if (*it > 0) {
                    result.push_back(*it);
                } else if (*it < 0) {
                    result.push_back(-(*it));
                } else {
                    // ゼロの場合の特別処理
                    if (result.empty()) {
                        result.push_back(0);
                    }
                }
            }
        }
    }
    
    // ソート
    std::sort(result.begin(), result.end());
    
    return result;
}

// 🔄 再帰関数
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    } else if (n == 2) {
        return 1;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

// 🎮 メイン関数（高複雑度）
int main() {
    try {
        // テンプレートクラステスト
        FixedArray<int, 10> arr;
        
        for (int i = 0; i < 15; ++i) {  // 意図的にサイズオーバー
            arr.push_back(i * 2);
        }
        
        // 複雑な条件分岐
        for (size_t i = 0; i < arr.size(); ++i) {
            try {
                int value = arr.at(i);
                
                if (value % 2 == 0) {
                    std::cout << "Even: " << value;
                } else {
                    std::cout << "Odd: " << value;
                }
                
                // フィボナッチ数チェック
                if (value == fibonacci(value)) {
                    std::cout << " (Fibonacci!)";
                }
                
                std::cout << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                continue;
            }
        }
        
        // 図形テスト
        std::vector<std::unique_ptr<Shape>> shapes;
        shapes.push_back(std::make_unique<Triangle>(3.0, 4.0, 5.0));
        shapes.push_back(std::make_unique<Triangle>(1.0, 1.0, 1.0));
        
        for (const auto& shape : shapes) {
            shape->print();
            std::cout << "Area: " << shape->area() << std::endl;
            std::cout << "Perimeter: " << shape->perimeter() << std::endl;
        }
        
        // 複雑フィルターテスト
        std::vector<int> numbers = {-5, -2, 0, 1, 3, 5, 7, 3, 1};
        auto filtered = complex_filter_sort(numbers, [](int x) { 
            return x >= -2 && x <= 7; 
        });
        
        std::cout << "Filtered numbers: ";
        for (int n : filtered) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 2;
    }
    
    return 0;
}