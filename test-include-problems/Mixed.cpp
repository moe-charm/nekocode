// 混在テスト：使うものと使わないものが混在
#include <iostream>    // 使う
#include <vector>      // 不要
#include <string>      // 使う
#include <map>         // 不要
#include <set>         // 不要
#include <algorithm>   // 不要
#include <memory>      // 不要

void mixedExample() {
    // iostreamを使用
    std::cout << "Hello, World!" << std::endl;
    
    // stringを使用
    std::string message = "Testing";
    
    // その他のincludeは使わない（不要）
}