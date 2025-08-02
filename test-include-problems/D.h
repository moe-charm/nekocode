#pragma once

// 🔄 循環依存チェーン継続: D.h -> E.h
#include "E.h" 
#include <iostream>
#include <list>       // 不要includeのテスト（使わない）
#include <forward_list> // 不要includeのテスト（使わない）

class D {
private:
    E* e_ptr;
    
public:
    D();
    void useE();
};