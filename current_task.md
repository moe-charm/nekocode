# 🚀 MoveClass全言語徹底テスト計画

**最終更新**: 2025-08-09 08:00  
**状況**: 🔥 **全6言語でMoveClassテスト実施中**

---

## 📋 **テスト概要**

MoveClass機能を全6言語で実戦的にテストし、各言語特有の課題を発見・解決する。

### **テスト環境**
- **テストプロジェクト**: `/tmp/test-projects/` (1.4GB実プロジェクト)
- **実験用ディレクトリ**: `/tmp/test-moveclass-[言語]/`
- **方針**: 破壊的変更OK（test-projectsなら実験し放題）

---

## 🟨 **1. JavaScript/TypeScript テスト**

### **テストケース1: React Component移動**
```javascript
// 移動元: /tmp/test-projects/react/fixtures/stacks/Components.js
export class NativeClass extends React.Component { ... }
export class FrozenClass extends React.Component { ... }

// 移動先: NativeComponents.js
// 課題: React import自動追加、export文の扱い
```
**✅ 実施済み**: NativeClass移動成功
**✅ PCRE2革命**: JavaScript import解析をPCRE2化済み

### **テストケース2: TypeScript Interface移動**
```typescript
// 移動元: /tmp/test-projects/TypeScript/src/compiler/types.ts
export interface Type { ... }
export interface Symbol { ... }

// 移動先: interfaces/CoreTypes.ts
// 課題: 型の相互参照、import type vs import
```

### **テストケース3: 循環依存の検出**
```javascript
// A.js imports B.js
// B.js imports A.js
// 課題: 移動時に循環依存を壊さない
```

### **期待される動作**
- [x] ES6 import/export正確な解析
- [ ] 相対パス自動調整（./→../）
- [ ] index.jsのバレルexport更新
- [ ] JSDocコメント保持

---

## 🐍 **2. Python テスト**

### **テストケース1: Flask Model移動**
```python
# 移動元: /tmp/test-projects/flask/src/flask/app.py
class Flask:
    def __init__(self):
        self.config = Config()
    
    def route(self, rule):
        ...

# 移動先: core/application.py
# 課題: __init__.py更新、相対import
```
**⏳ 実施中**: 
- Flaskクラス(81-1536行)を確認
- MoveClassコマンド未実装のため手動テスト中
- /tmp/test_python_moveclass.pyでテストケース作成済み

### **テストケース2: 循環import解決**
```python
# models/user.py
from .post import Post  # 循環参照

class User:
    def get_posts(self) -> List[Post]:
        ...

# 移動時に TYPE_CHECKING使用を提案
from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from .post import Post
```

### **テストケース3: __all__更新**
```python
# __init__.py
__all__ = ['User', 'Post', 'Comment']
# Userを移動したら自動更新
```

### **期待される動作**
- [ ] from...import文の正確な解析
- [ ] 相対/絶対import判定
- [ ] __init__.py自動更新
- [ ] docstring保持

---

## ⚙️ **3. C++ テスト**

### **テストケース1: nlohmann/json Class移動**
```cpp
// 移動元: /tmp/test-projects/json/single_include/nlohmann/json.hpp
// 20201行目: basic_jsonクラス（938KB、123クラス、770関数）
namespace nlohmann {
    template<...>
    class basic_json { 
        ...
    };
    using json = basic_json<>;
}

// 移動先: json_core.hpp
// 課題: namespace維持、template、friend宣言
```
**⏳ 実施中**:
- 938KBの巨大ファイル解析成功
- 123クラス、770関数検出
- basic_jsonは20201行目

### **テストケース2: Template Class移動**
```cpp
// 移動元: container.hpp
template<typename T>
class Container {
    std::vector<T> items;
};

// 特殊化も一緒に移動
template<>
class Container<std::string> { ... };
```

### **テストケース3: ヘッダーガード更新**
```cpp
// 移動時に自動更新
#ifndef OLD_PATH_HPP → #ifndef NEW_PATH_HPP
#define OLD_PATH_HPP → #define NEW_PATH_HPP
```

### **期待される動作**
- [ ] include文の<>と""の使い分け
- [ ] namespace階層の維持
- [ ] 前方宣言の追跡
- [ ] インライン実装の扱い

---

## 🎯 **4. C# テスト**

### **テストケース1: NLog Logger移動**
```csharp
// 移動元: /tmp/test-projects/NLog/src/NLog/Logger.cs
namespace NLog {
    public class Logger {
        private LogFactory factory;
        ...
    }
}

// 移動先: Core/Logger.cs
// 課題: namespace変更、using更新、partial class
```

### **テストケース2: ASP.NET Controller移動**
```csharp
// 移動元: Controllers/HomeController.cs
[ApiController]
[Route("[controller]")]
public class HomeController : ControllerBase { }

// 属性（Attribute）も正しく移動
```

### **テストケース3: Generic制約の保持**
```csharp
public class Repository<T> where T : class, IEntity, new() { }
```

### **期待される動作**
- [ ] namespace階層の調整
- [ ] using static/global using対応
- [ ] partial class検出と警告
- [ ] 属性（Attributes）保持

---

## 🐹 **5. Go テスト**

### **テストケース1: Gin Handler移動**
```go
// 移動元: /tmp/test-projects/gin/gin.go
package gin

type Engine struct {
    RouterGroup
    pool sync.Pool
}

func (engine *Engine) Run(addr ...string) error { }

// 移動先: engine/core.go
// 課題: package変更、メソッドレシーバー
```

### **テストケース2: Interface実装移動**
```go
type Handler interface {
    ServeHTTP(ResponseWriter, *Request)
}

type MyHandler struct{}
func (h *MyHandler) ServeHTTP(w ResponseWriter, r *Request) {}
// インターフェースと実装の関係維持
```

### **テストケース3: 内部package**
```go
// internal/配下への移動時の可視性変更
package internal
// 外部からアクセス不可になることを警告
```

### **期待される動作**
- [ ] package文の自動更新
- [ ] import括弧内のソート
- [ ] メソッドレシーバーの追跡
- [ ] internal可視性の警告

---

## 🦀 **6. Rust テスト**

### **テストケース1: Serde Struct移動**
```rust
// 移動元: /tmp/test-projects/serde/serde/src/lib.rs
pub struct Serializer {
    output: Vec<u8>,
}

impl Serializer {
    pub fn new() -> Self { }
}

// 移動先: ser/serializer.rs
// 課題: mod宣言更新、pub use再export
```

### **テストケース2: Trait実装の移動**
```rust
trait MyTrait {
    fn method(&self);
}

struct MyStruct;
impl MyTrait for MyStruct {
    fn method(&self) { }
}
// trait定義と実装の関係維持
```

### **テストケース3: マクロ使用struct**
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    #[serde(default)]
    pub name: String,
}
// derive属性とserde属性の保持
```

### **期待される動作**
- [ ] mod.rs自動更新
- [ ] use文のパス調整
- [ ] pub/pub(crate)可視性維持
- [ ] derive/属性マクロ保持

---

## 🧪 **共通テストシナリオ**

### **A. 基本移動テスト**
1. 単一クラス/構造体を新規ファイルへ移動
2. import/include自動更新確認
3. 元ファイルでの削除確認

### **B. 依存関係テスト**
1. 相互参照するクラスの移動
2. 循環依存の検出と警告
3. 依存グラフ可視化（DOT形式）

### **C. エラーハンドリング**
1. 移動先ファイル既存時の動作
2. 書き込み権限なし時の動作
3. 構文エラーファイルでの動作

### **D. ロールバックテスト**
1. 移動実行後のundo
2. 部分的失敗時の復元
3. バックアップファイル作成

---

## 📊 **テスト実行手順**

```bash
# 1. 各言語のテスト環境準備
for lang in js python cpp csharp go rust; do
    mkdir -p /tmp/test-moveclass-$lang
    cp -r /tmp/test-projects/$lang/* /tmp/test-moveclass-$lang/
done

# 2. Session作成と解析
./bin/nekocode_ai session-create [ファイル]

# 3. MoveClassプレビュー
./bin/nekocode_ai moveclass-preview [session-id] [symbol-id] [target-file]

# 4. 実行確認
./bin/nekocode_ai moveclass-confirm [preview-id]

# 5. 結果検証
diff -u original.js moved.js
```

---

## ✅ **成功基準**

### **必須要件**
- [ ] 全6言語で基本移動成功
- [ ] import/include自動更新動作
- [ ] プレビュー表示が分かりやすい
- [ ] エラー時の適切なメッセージ

### **品質基準**
- [ ] 移動後もビルド/実行可能
- [ ] コードフォーマット維持
- [ ] コメント・属性の保持
- [ ] パフォーマンス（1秒以内）

### **ボーナス目標**
- [ ] 複数Symbol同時移動
- [ ] リファクタリング提案
- [ ] 移動履歴の記録
- [ ] VSCode拡張連携

---

## 🔥 **現在の進捗**

| 言語 | 基本移動 | import更新 | 依存解析 | エラー処理 | 総合評価 |
|------|---------|-----------|---------|-----------|----------|
| JavaScript | ✅ | ✅ | ⏳ | ⏳ | 60% |
| Python | ⏳ | ✅ | ✅ | ⏳ | 35% |
| C++ | ⏳ | ⏳ | ✅ | ⏳ | 20% |
| C# | ⏳ | ⏳ | ⏳ | ⏳ | 0% |
| Go | ⏳ | ⏳ | ⏳ | ⏳ | 0% |
| Rust | ⏳ | ⏳ | ⏳ | ⏳ | 0% |

**次のアクション**: C++ nlohmann/json移動テスト

---

## 📝 **発見した課題と解決策**

### **課題1: 言語別import構文の多様性**
- **問題**: ES6、Python、Go等でimport構文が大きく異なる
- **解決**: ImportAnalyzer::parse_importsを言語別に特化

### **課題2: 名前空間の扱い**
- **問題**: C++/C#のnamespace、Goのpackage、Rustのmod
- **解決**: RefactoringUtils::adjust_namespaceで統一処理

### **課題3: プレビュー表示**
- **問題**: 「よくわからん」フィードバック
- **解決**: 前後5行表示、色分け、説明付きフォーマット実装済み✅

### **課題4: PCRE2革命**
- **問題**: C++ std::regexが「失敗しまくってた」
- **解決**: PCRE2 Python風APIで全面置換✅
  - direct_replace.cpp: 完全PCRE2化
  - dependency_graph.cpp: JavaScript import解析PCRE2化
  - adjust_namespace関数: デッドコード削除

---

**テスト担当**: Claude + User collaborative testing  
**期限**: 本日中に全言語基本テスト完了目標