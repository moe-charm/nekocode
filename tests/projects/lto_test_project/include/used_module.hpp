#pragma once
#include <iostream>

// Used function declaration
void used_module_function();

// Used class declaration
class UsedModuleClass {
public:
    void do_something();
    void unused_method(); // This method is declared but never used
};