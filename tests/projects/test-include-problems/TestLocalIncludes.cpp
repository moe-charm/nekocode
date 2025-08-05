// ローカルヘッダーの不要includeテスト
#include "UsedHeader.h"    // 使う
#include "UnusedHeader.h"  // 使わない（不要）
#include "A.h"             // not used

void testFunction() {
    // UsedClassのみ使用
    UsedClass obj;
    obj.printMessage();
    int value = obj.getValue();
    
    // These classes are not used in this function
}