#pragma once

// 🔄 循環依存チェーン完成: E.h -> C.h (3つの循環完成)
#include "C.h"
#include <iostream>
#include <unordered_map>  // 不要includeのテスト（使わない）
#include <unordered_set>  // 不要includeのテスト（使わない）
#include <tuple>          // 不要includeのテスト（使わない）

class E {
private:
    C* c_ptr;
    
public:
    E();
    void useC();
};