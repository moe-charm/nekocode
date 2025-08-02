#pragma once

// 🔄 循環依存のテスト: B.h -> A.h (完成させる)
#include "A.h"
#include <iostream>
#include <set>        // 不要includeのテスト（使わない）
#include <algorithm>  // 不要includeのテスト（使わない）
#include <memory>     // 不要includeのテスト（使わない）

class B {
private:
    A* a_ptr;
    
public:
    B();
    void processA();
    void callA();
};