// LTO Dead Code Detection Test - Main File
#include <iostream>
#include "../include/used_module.hpp"
#include "../include/unused_module.hpp"

// Used global function
void used_global_function() {
    std::cout << "This global function is used!" << std::endl;
}

// Unused global function - should be detected by LTO
void unused_global_function() {
    std::cout << "This global function is NOT used!" << std::endl;
}

// Used class
class UsedGlobalClass {
public:
    void method() {
        std::cout << "Used global class method" << std::endl;
    }
};

// Unused class - should be detected by LTO
class UnusedGlobalClass {
public:
    void method() {
        std::cout << "Unused global class method" << std::endl;
    }
    
    void another_method() {
        std::cout << "Another unused method" << std::endl;
    }
};

int main() {
    std::cout << "=== LTO Dead Code Detection Test ===" << std::endl;
    
    // Use some functions to make them "live"
    used_global_function();
    
    UsedGlobalClass used_obj;
    used_obj.method();
    
    // Call used module functions
    used_module_function();
    UsedModuleClass used_module_obj;
    used_module_obj.do_something();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}