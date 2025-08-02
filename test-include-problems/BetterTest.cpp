// より良い不要includeテスト（ファイル名ベースのシンボル推測対応）
#include "UsedHeader.h"    // 使う - UsedHeaderファイル名で推測できる
#include "UnusedHeader.h"  // 使わない - UnusedHeaderファイル名だが実際は使わない
#include "A.h"             // 使わない - Aクラスを推測するが使わない

void betterTestFunction() {
    // UsedHeaderもUsedClassも使わない
    // ファイル名ベースの推測でも「使ってない」と判定されるはず
    int x = 42;
}