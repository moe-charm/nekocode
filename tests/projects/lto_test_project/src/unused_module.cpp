#include "../include/unused_module.hpp"

// These functions are completely unused - should be detected by LTO
void completely_unused_function() {
    std::cout << "This function is completely unused!" << std::endl;
}

void another_unused_function() {
    std::cout << "Another completely unused function!" << std::endl;
}

// More unused functions
void helper_function_never_called() {
    std::cout << "Helper function that's never called" << std::endl;
}

void debug_function() {
    std::cout << "Debug function - should be detected as unused" << std::endl;
}

// Completely unused class implementations
void CompletelyUnusedClass::method1() {
    std::cout << "Unused class method 1" << std::endl;
}

void CompletelyUnusedClass::method2() {
    std::cout << "Unused class method 2" << std::endl;
    helper_function_never_called(); // This call won't save helper function
}

int CompletelyUnusedClass::calculate(int a, int b) {
    return a + b + unused_member_variable;
}