#include "../include/used_module.hpp"

// Used function implementation
void used_module_function() {
    std::cout << "Used module function called!" << std::endl;
}

// Unused function in this module - should be detected by LTO
void unused_module_internal_function() {
    std::cout << "This internal function is never called!" << std::endl;
}

// Used class method implementation
void UsedModuleClass::do_something() {
    std::cout << "UsedModuleClass doing something..." << std::endl;
}

// Unused method implementation - should be detected by LTO
void UsedModuleClass::unused_method() {
    std::cout << "This method is never called!" << std::endl;
}