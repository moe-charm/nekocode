// C++ メンバ変数検出テスト
#include <string>
#include <vector>
#include <memory>

class CppTestClass {
private:
    std::string name;
    int age;
    bool isActive;

protected:
    std::vector<int> scores;
    float rating;

public:
    static int instanceCount;
    const std::string DEFAULT_NAME = "Unknown";
    std::shared_ptr<int> dataPtr;

    CppTestClass(const std::string& n, int a) : name(n), age(a), isActive(true) {
        instanceCount++;
    }

    std::string getName() const {
        return name;
    }

    void setAge(int newAge) {
        if (newAge >= 0) {
            age = newAge;
        }
    }

    static int getInstanceCount() {
        return instanceCount;
    }
};

struct SimpleStruct {
    int x;
    int y;
    std::string label;
    
    SimpleStruct() : x(0), y(0), label("default") {}
    
    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }
};

template<typename T>
class TemplateClass {
private:
    T data;
    std::vector<T> items;

public:
    void addItem(const T& item) {
        items.push_back(item);
    }
    
    T getData() const {
        return data;
    }
};