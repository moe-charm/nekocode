#pragma once

// 🔄 循環依存のテスト: A.h -> B.h -> A.h
#include "B.h"
#include <iostream>
#include <vector>     // 不要includeのテスト（使わない）
#include <string>     // 不要includeのテスト（使わない）
#include <map>        // 不要includeのテスト（使わない）

class A {
private:
    B* b_ptr;
    
public:
    A();
    void doSomething();
    void callB();
};