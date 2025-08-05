#pragma once
#include <iostream>

// Completely unused functions - should be detected by LTO
void completely_unused_function();
void another_unused_function();

// Completely unused class - should be detected by LTO
class CompletelyUnusedClass {
public:
    void method1();
    void method2();
    int calculate(int a, int b);
    
private:
    int unused_member_variable;
};