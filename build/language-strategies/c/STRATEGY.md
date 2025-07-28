# ⚫ C言語 解析戦略

## 🎯 言語特性と課題

### **最もシンプルな言語（のはず）**
```c
#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int data;
    struct node* next;
} Node;

int add(int a, int b) {
    return a + b;
}

void process_list(Node* head) {
    // ...
}
```

### **C言語の特徴**
| 要素 | 例 | 複雑度 |
|------|----|----|
| **関数定義** | `int func(int a, char* b)` | 😊 シンプル |
| **構造体** | `struct name { ... };` | 😊 シンプル |
| **typedef** | `typedef struct {...} Name;` | 🤔 中程度 |
| **ポインタ** | `int* (*func)(char**, int)` | 😱 地獄 |
| **マクロ** | `#define MAX(a,b) ((a)>(b)?(a):(b))` | 😱😱 超地獄 |
| **関数ポインタ** | `int (*func_ptr)(int, int)` | 😱😱😱 |

## 🔧 採用戦略: シンプル重視

### **Phase 1: 基本要素のみ**
```cpp
// 目標: C++の複雑さを排除して、純粋なC要素のみ
struct c_function : seq<
    star<space>,
    return_type,          // int, void, char*, etc.
    plus<space>,
    identifier,           // 関数名
    one<'('>,
    until<one<')'>>,      // パラメータ（中身は解析しない）
    one<')'>,
    star<space>,
    one<'{'>              // 関数本体開始
> {};
```

**検出対象:**
- ✅ `int function(int a)` - 基本関数
- ✅ `struct name { ... };` - 構造体
- ✅ `typedef struct { ... } Name;` - typedef構造体
- ❌ 関数ポインタ（諦め）

### **Phase 2: プリプロセッサ除去**
```cpp
// C言語の最大の敵: プリプロセッサマクロ
std::string preprocess_c(const std::string& content) {
    std::vector<std::string> lines;
    
    for (const auto& line : split_lines(content)) {
        std::string trimmed = trim(line);
        
        // プリプロセッサ指令を除去
        if (!trimmed.starts_with("#")) {
            lines.push_back(line);
        }
    }
    return join(lines, "\n");
}
```

### **Phase 3: 複雑な型は諦める**
```c
// ❌ 解析しない（複雑すぎる）
int (*signal(int sig, int (*func)(int)))(int);
void (*(*func_array[10])())(int);

// ✅ 解析する（シンプル）
int add(int a, int b);
void print_message(char* msg);
struct point { int x, y; };
```

## 📊 期待値設定

**C言語は「楽勝」のはず:**
- 構文がシンプル
- キーワードが少ない
- オブジェクト指向の複雑さなし
- テンプレートなし

**でも:**
- 関数ポインタの構文は狂気
- マクロは別言語レベル
- `typedef`の複雑な使用法

## 🧠 設計思想

### **「Cの美学：シンプル・イズ・ベスト」**
```c
// Cの良い所:
// 1. 構文が明確
// 2. 関数定義が分かりやすい
// 3. 構造体定義が単純
// 4. 余計な機能がない

// 活用方針:
// シンプルな構文を最大限活用
// 複雑な部分は潔く諦める
```

**戦略:**
1. **基本に徹する** - int, char, struct, function
2. **プリプロセッサは前処理で除去** - `#include`, `#define`
3. **関数ポインタは諦める** - 人間でも読みにくい

## 🚨 諦める部分

**複雑すぎて解析不可能:**
- 関数ポインタの複雑な宣言
- マクロ展開後の動的生成
- `__attribute__` 等のコンパイラ拡張
- inline アセンブリ

**でも十分:**
「通常のCコードの90%は簡単に検出可能」

## 📊 成功指標

| ファイル | 目標検出数 | 期待値 |
|----------|------------|--------|
| **標準的なCファイル** | ほぼ全要素 | 95%+ |
| **関数ポインタ多用** | 部分的 | 60%+ |
| **マクロ地獄** | 最低限 | 30%+ |

## 💡 実装メモ

**2025-07-28現在:**
- ⚠️ C言語の動作状況未検証
- 🎯 C++より遥かに簡単なはず
- 🚀 「息抜き」として実装予定

**期待:**
「C言語だけは確実に成功させる！」

**次のアクション:**
1. シンプルなCファイルでテスト
2. プリプロセッサ除去の実装
3. 複雑な型宣言のスキップ機能

**教訓（予想）:**
「シンプルな言語はシンプルに解析する」