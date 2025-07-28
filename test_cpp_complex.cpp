#include <iostream>
#include <vector>
#include <memory>

namespace myproject {

class BaseClass {
public:
    virtual void process() = 0;
    virtual ~BaseClass() = default;
};

template<typename T>
class Container : public BaseClass {
private:
    std::vector<T> data;
    
public:
    void add(const T& item) {
        data.push_back(item);
    }
    
    void process() override {
        for (const auto& item : data) {
            if (item.is_valid()) {
                item.execute();
            }
        }
    }
    
    size_t size() const noexcept {
        return data.size();
    }
};

struct Config {
    std::string name;
    int timeout;
    bool enabled;
};

void initialize_system() {
    try {
        auto container = std::make_unique<Container<Config>>();
        container->add({"test", 1000, true});
        container->process();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

} // namespace myproject

int main() {
    myproject::initialize_system();
    return 0;
}