# 🧪 Phase 5完了後: Session Mode包括的テスト計画

**最終更新**: 2025-08-08 18:00  
**状況**: ✅ **Phase 5 Universal Symbol完成** → 🧪 **Session Mode統合テスト開始**

---

## 🎯 **Session Mode包括的テスト目的**

Phase 5で実装したUniversal Symbol Native Generationが、実際のSession環境で正しく動作することを確認する。これはPhase 6 move-class機能の前提条件となる重要なテスト。

---

## 🧪 **全言語Session Modeテスト詳細計画**

### 📋 **テスト手順テンプレート**
各言語で以下の手順を実行:

```bash
# 1. テストファイル作成
cat > /tmp/test_[lang].[ext] << 'EOF'
[言語別テストコード]
EOF

# 2. Session作成
./bin/nekocode_ai session-create /tmp/test_[lang].[ext]
# → Session ID取得

# 3. 通常解析結果確認
./bin/nekocode_ai session-command [SESSION_ID] stats

# 4. Universal Symbols確認
./bin/nekocode_ai session-command [SESSION_ID] list-symbols

# 5. JSON出力でUniversal Symbols検証
./bin/nekocode_ai session-command [SESSION_ID] show-json | jq '.universal_symbols'

# 6. Symbol詳細確認
./bin/nekocode_ai session-command [SESSION_ID] get-symbol [SYMBOL_ID]
```

---

## 🟨 **1. JavaScript Session Test**

### テストコード
```javascript
// /tmp/test_js.js
class UserManager {
    constructor() {
        this.users = [];
        this.lastId = 0;
    }
    
    addUser(name, email) {
        const user = {
            id: ++this.lastId,
            name,
            email
        };
        this.users.push(user);
        return user;
    }
    
    findUserById(id) {
        return this.users.find(u => u.id === id);
    }
}

export function createManager() {
    return new UserManager();
}

const globalManager = new UserManager();
```

### 期待される結果
- **Classes**: UserManager
- **Functions**: addUser, findUserById, createManager
- **Universal Symbols**: 
  - class_UserManager_0 (CLASS)
  - method_addUser_0 (FUNCTION)
  - method_findUserById_0 (FUNCTION)
  - function_createManager_0 (FUNCTION)

---

## 🐍 **2. Python Session Test**

### テストコード
```python
# /tmp/test_py.py
class DataProcessor:
    def __init__(self):
        self.data = []
        self.processed_count = 0
    
    def process_item(self, item):
        processed = self._transform(item)
        self.data.append(processed)
        self.processed_count += 1
        return processed
    
    def _transform(self, item):
        return str(item).upper()
    
    def get_statistics(self):
        return {
            'count': self.processed_count,
            'items': len(self.data)
        }

def create_processor():
    return DataProcessor()

processor = DataProcessor()
```

### 期待される結果
- **Classes**: DataProcessor
- **Functions**: __init__, process_item, _transform, get_statistics, create_processor
- **Universal Symbols**:
  - class_DataProcessor_0 (CLASS)
  - method___init___0 (FUNCTION)
  - method_process_item_0 (FUNCTION)
  - method__transform_0 (FUNCTION)
  - method_get_statistics_0 (FUNCTION)
  - function_create_processor_0 (FUNCTION)

---

## ⚙️ **3. C++ Session Test**

### テストコード
```cpp
// /tmp/test_cpp.cpp
#include <vector>
#include <string>

namespace Core {
    class EventHandler {
    private:
        std::vector<std::string> events;
        int eventCount;
        
    public:
        EventHandler() : eventCount(0) {}
        
        void addEvent(const std::string& event) {
            events.push_back(event);
            eventCount++;
        }
        
        std::string getLastEvent() const {
            if (!events.empty()) {
                return events.back();
            }
            return "";
        }
        
        int getEventCount() const {
            return eventCount;
        }
    };
    
    EventHandler* createHandler() {
        return new EventHandler();
    }
}
```

### 期待される結果
- **Classes**: EventHandler
- **Functions**: EventHandler (constructor), addEvent, getLastEvent, getEventCount, createHandler
- **Universal Symbols**:
  - class_EventHandler_0 (CLASS)
  - method_EventHandler_0 (FUNCTION/Constructor)
  - method_addEvent_0 (FUNCTION)
  - method_getLastEvent_0 (FUNCTION)
  - method_getEventCount_0 (FUNCTION)
  - function_createHandler_0 (FUNCTION)

---

## 🎯 **4. C# Session Test**

### テストコード
```csharp
// /tmp/test_cs.cs
using System;
using System.Collections.Generic;

namespace TestApp {
    public class OrderManager {
        private List<Order> orders;
        private int nextOrderId;
        
        public OrderManager() {
            orders = new List<Order>();
            nextOrderId = 1;
        }
        
        public Order CreateOrder(string customer, decimal amount) {
            var order = new Order {
                Id = nextOrderId++,
                Customer = customer,
                Amount = amount,
                CreatedAt = DateTime.Now
            };
            orders.Add(order);
            return order;
        }
        
        public Order FindOrder(int id) {
            return orders.Find(o => o.Id == id);
        }
        
        public int OrderCount => orders.Count;
    }
    
    public class Order {
        public int Id { get; set; }
        public string Customer { get; set; }
        public decimal Amount { get; set; }
        public DateTime CreatedAt { get; set; }
    }
}
```

### 期待される結果
- **Classes**: OrderManager, Order
- **Functions**: OrderManager (constructor), CreateOrder, FindOrder
- **Properties**: OrderCount, Id, Customer, Amount, CreatedAt
- **Universal Symbols**:
  - class_OrderManager_0 (CLASS)
  - class_Order_0 (CLASS)
  - method_OrderManager_0 (FUNCTION/Constructor)
  - method_CreateOrder_0 (FUNCTION)
  - method_FindOrder_0 (FUNCTION)
  - property_OrderCount_0 (PROPERTY)
  - property_Id_0 (PROPERTY)
  - property_Customer_0 (PROPERTY)
  - property_Amount_0 (PROPERTY)
  - property_CreatedAt_0 (PROPERTY)

---

## 🐹 **5. Go Session Test**

### テストコード
```go
// /tmp/test_go.go
package main

import (
    "fmt"
    "sync"
)

type TaskManager struct {
    tasks []Task
    mu    sync.Mutex
    nextID int
}

type Task struct {
    ID   int
    Name string
    Done bool
}

func NewTaskManager() *TaskManager {
    return &TaskManager{
        tasks:  make([]Task, 0),
        nextID: 1,
    }
}

func (tm *TaskManager) AddTask(name string) int {
    tm.mu.Lock()
    defer tm.mu.Unlock()
    
    task := Task{
        ID:   tm.nextID,
        Name: name,
        Done: false,
    }
    tm.tasks = append(tm.tasks, task)
    tm.nextID++
    return task.ID
}

func (tm *TaskManager) CompleteTask(id int) bool {
    tm.mu.Lock()
    defer tm.mu.Unlock()
    
    for i := range tm.tasks {
        if tm.tasks[i].ID == id {
            tm.tasks[i].Done = true
            return true
        }
    }
    return false
}

func main() {
    manager := NewTaskManager()
    fmt.Println(manager)
}
```

### 期待される結果
- **Structs**: TaskManager, Task
- **Functions**: NewTaskManager, AddTask, CompleteTask, main
- **Universal Symbols**:
  - struct_TaskManager_0 (CLASS/STRUCT)
  - struct_Task_0 (CLASS/STRUCT)
  - function_NewTaskManager_0 (FUNCTION)
  - method_AddTask_0 (FUNCTION)
  - method_CompleteTask_0 (FUNCTION)
  - function_main_0 (FUNCTION)

---

## 🦀 **6. Rust Session Test**

### テストコード
```rust
// /tmp/test_rust.rs
use std::collections::HashMap;

pub struct CacheManager {
    cache: HashMap<String, String>,
    hit_count: usize,
    miss_count: usize,
}

impl CacheManager {
    pub fn new() -> Self {
        CacheManager {
            cache: HashMap::new(),
            hit_count: 0,
            miss_count: 0,
        }
    }
    
    pub fn get(&mut self, key: &str) -> Option<String> {
        if let Some(value) = self.cache.get(key) {
            self.hit_count += 1;
            Some(value.clone())
        } else {
            self.miss_count += 1;
            None
        }
    }
    
    pub fn set(&mut self, key: String, value: String) {
        self.cache.insert(key, value);
    }
    
    pub fn stats(&self) -> (usize, usize) {
        (self.hit_count, self.miss_count)
    }
}

trait Cacheable {
    fn cache_key(&self) -> String;
}

fn create_cache() -> CacheManager {
    CacheManager::new()
}
```

### 期待される結果
- **Structs**: CacheManager
- **Traits**: Cacheable
- **Functions**: new, get, set, stats, cache_key, create_cache
- **Universal Symbols**:
  - struct_CacheManager_0 (CLASS/STRUCT)
  - trait_Cacheable_0 (INTERFACE/TRAIT)
  - method_new_0 (FUNCTION)
  - method_get_0 (FUNCTION)
  - method_set_0 (FUNCTION)
  - method_stats_0 (FUNCTION)
  - method_cache_key_0 (FUNCTION)
  - function_create_cache_0 (FUNCTION)

---

## 🔍 **検証ポイント**

### 1. **Universal Symbols生成確認**
- [ ] 各言語でUniversal Symbolsが生成される
- [ ] symbol_idが一意で重複しない
- [ ] symbol_typeが正しく設定される（CLASS, FUNCTION, PROPERTY等）

### 2. **Session永続化確認**
- [ ] Session作成後、Universal SymbolsがSessionに保存される
- [ ] session-commandでUniversal Symbolsにアクセス可能
- [ ] Session再読み込み後もUniversal Symbolsが維持される

### 3. **JSON出力確認**
- [ ] `universal_symbols`セクションが存在
- [ ] `symbols`配列に全シンボルが含まれる
- [ ] 各シンボルに必要なフィールドが存在（symbol_id, symbol_type, name, start_line等）

### 4. **エラーハンドリング**
- [ ] 存在しないsymbol_idアクセス時の適切なエラー
- [ ] 空ファイルや構文エラーファイルでのグレースフルな処理

---

## 📊 **テスト結果記録表**

| 言語 | Session作成 | Symbols生成数 | list-symbols | get-symbol | JSON出力 | 総合評価 |
|------|------------|--------------|-------------|------------|----------|----------|
| JavaScript | ✅ | 1 (class only) | ❌ Not impl | ❌ Not impl | ⚠️ Partial | ❌ **失敗** |
| Python | ⏳ | - | ⏳ | ⏳ | ⏳ | ⏳ |
| C++ | ⏳ | - | ⏳ | ⏳ | ⏳ | ⏳ |
| C# | ⏳ | - | ⏳ | ⏳ | ⏳ | ⏳ |
| Go | ⏳ | - | ⏳ | ⏳ | ⏳ | ⏳ |
| Rust | ⏳ | - | ⏳ | ⏳ | ⏳ | ⏳ |

**凡例**: ✅ 成功 / ⚠️ 部分的成功 / ❌ 失敗 / ⏳ 未実施

---

## 🚨 **既知の問題・要注意点**

### 🔥 **新規発見: JavaScript Universal Symbol不完全実装**
**Session Modeテストで判明（2025-08-08 18:30）**

#### 問題詳細:
1. **メソッドが検出されない**
   - クラスのメソッド（constructor, addUser, findUserById等）がUniversal Symbolsに含まれない
   - クラス自体は正しく検出される（class_UserManager_0）
   - トップレベル関数は通常のfunctions配列には含まれる

2. **Session Command未実装**
   - `list-symbols`コマンドが存在しない
   - `get-symbol`コマンドが存在しない
   - Universal Symbolsへのアクセス方法がSession経由で提供されていない

3. **JSON出力の不整合**
   - `symbols`キーでUniversal Symbolsが出力される（`universal_symbols`ではない）
   - classのみで、methodsが含まれない

#### テスト結果:
```json
// 期待される出力
{
  "symbols": [
    { "symbol_id": "class_UserManager_0", "symbol_type": "class", ... },
    { "symbol_id": "method_constructor_0", "symbol_type": "function", ... },
    { "symbol_id": "method_addUser_0", "symbol_type": "function", ... },
    { "symbol_id": "method_findUserById_0", "symbol_type": "function", ... }
  ]
}

// 実際の出力
{
  "symbols": [
    { "symbol_id": "class_UserManager_0", "symbol_type": "class", ... }
  ]
}
```

### 既存の問題:

1. **C# PEGTLパース問題**
   - 現在Line-based fallbackに依存
   - Universal Symbols生成は動作するが精度に注意

2. **デバッグ出力**
   - 条件付きコンパイル化完了
   - 本番環境では`NEKOCODE_DEBUG_SYMBOLS`未定義で使用

3. **メモリ管理**
   - shared_ptr使用でメモリリーク防止
   - 大規模ファイルでのテストも必要

---

## 🎯 **テスト完了基準**

- ✅ 全6言語でSession Mode動作確認
- ✅ Universal Symbolsの生成・アクセス確認
- ✅ エラーケースのハンドリング確認
- ✅ パフォーマンス問題なし

**テスト完了後**: Phase 6 move-class機能設計へ移行

---

**次のステップ**: JavaScriptから順次Session Modeテスト実施！