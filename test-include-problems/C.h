#pragma once

// 🔄 別の循環依存チェーン: C.h -> D.h -> E.h -> C.h
#include "D.h"
#include <iostream>
#include <queue>      // 不要includeのテスト（使わない）
#include <stack>      // 不要includeのテスト（使わない）
#include <deque>      // 不要includeのテスト（使わない）

class C {
private:
    D* d_ptr;
    
public:
    C();
    void useD();
};