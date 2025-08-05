# 🔍 各言語デッドコード検出ツール調査

## 🎯 調査項目

各ツールについて以下を確認：
1. **出力フォーマット** - どんな形式で結果が出るか
2. **確信度表現** - パーセンテージ？レベル？なし？
3. **位置情報** - ファイル:行番号の形式
4. **検出項目** - 関数、クラス、変数、インポート等
5. **エラーハンドリング** - ツールなしの場合の動作

## 📋 調査対象

### ✅ 完了
- **C++ LTO**: 100%精度、位置情報なし、関数・変数のみ
- **Python Vulture**: 60-90%確信度、位置情報あり、全項目

### 🔄 調査予定
1. **Go staticcheck**
2. **Rust cargo** 
3. **C# .NET analyzer**
4. **JavaScript ts-prune**
5. **TypeScript ts-prune**

## 🚀 調査方法

各言語のサンプルコード作成 → ツール実行 → 出力解析

### Go調査
```go
// test_go_deadcode.go
package main

import (
    "fmt"
    "unused_package"  // 未使用
)

func usedFunction() {
    fmt.Println("used")
}

func unusedFunction() {  // 未使用
    fmt.Println("unused")
}

func main() {
    usedFunction()
}
```

### Rust調査
```rust
// test_rust_deadcode.rs
#[allow(unused_imports)]
use std::collections::HashMap;  // 未使用

fn used_function() {
    println!("used");
}

fn unused_function() {  // 未使用
    println!("unused");
}

fn main() {
    used_function();
}
```

### JavaScript調査
```javascript
// test_js_deadcode.js
const used = require('used-module');
const unused = require('unused-module');  // 未使用

function usedFunction() {
    return "used";
}

function unusedFunction() {  // 未使用
    return "unused";
}

module.exports = { usedFunction };
```

## 💡 期待される発見

- **統一可能な要素**: 全ツール共通の情報
- **差異**: ツール固有の表現方法
- **課題**: 統一困難な部分

## 📊 最終目標

全調査完了後に **統一セッションフォーマット v2** を設計！