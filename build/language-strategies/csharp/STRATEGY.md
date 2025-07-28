# 🟣 C# 解析戦略

## 🎯 言語特性と課題

### **PEGTL移行の成功事例**
```csharp
namespace MyNamespace 
{
    public class MyClass : BaseClass, IInterface
    {
        public async Task<List<T>> GenericMethodAsync<T>(
            IEnumerable<T> source,
            Func<T, bool> predicate
        ) where T : class, new()
        {
            // ...
        }
    }
}
```

### **C#特有の特徴**
| 要素 | 例 | 対応状況 |
|------|----|----|
| **名前空間** | `namespace Company.Product` | ✅ 対応済み |
| **修飾子** | `public static async` | ✅ 対応済み |
| **ジェネリクス** | `List<T> where T : class` | ⚠️ 部分対応 |
| **プロパティ** | `public int Value { get; set; }` | ❌ 未対応 |
| **イベント** | `public event EventHandler Click;` | ❌ 未対応 |
| **using文** | `using System.Collections;` | ✅ 対応済み |

## 🔧 採用戦略: 段階的完成

### **Phase 1: 基本要素検出（完了）**
```cpp
// 成功済みの実装
struct csharp_class : seq<
    opt<seq<public_keyword, plus<space>>>,
    class_keyword,
    plus<space>,
    identifier,
    // ... 継承・実装も対応
> {};
```

**検出済み要素:**
- ✅ `public class ClassName` - 基本クラス
- ✅ `namespace NamespaceName` - 名前空間
- ✅ `public void MethodName()` - メソッド
- ✅ `using System;` - using文

### **Phase 2: 高度な機能（今後）**
```csharp
// 対応予定の要素
public string PropertyName { get; set; }  // プロパティ
public event EventHandler EventName;      // イベント
public delegate void DelegateName();      // デリゲート
```

### **Phase 3: 現実的な制約**
```cpp
// ジェネリクス制約は「ノイズ」として処理
struct generic_constraints : seq<
    TAO_PEGTL_STRING("where"),
    until<one<'{'>>  // where句全体をスキップ
> {};
```

## 📊 現在の状況

**2025-07-28時点:**
- ✅ 最初のPEGTL移行成功言語
- ✅ 基本的なクラス・メソッド検出は動作
- ⚠️ 最近の動作状況は未検証
- 🎯 成功パターンを他言語に展開中

**検証必要:**
.NET Runtimeサンプルでの動作確認

## 🧠 設計思想

### **「C#の優等生ぶりを活用」**
```csharp
// C#の良い所:
// 1. 構文が比較的規則正しい
// 2. キーワードが明確
// 3. ブロック構造が分かりやすい
// 4. JavaScriptのような「なんでもあり」感が少ない

public class WellStructured 
{
    public void ClearMethod() 
    {
        // 予測可能な構造
    }
}
```

**戦略:**
1. **規則正しさを活用** - 予測可能なパターン
2. **キーワード検出重視** - `public`, `class`, `namespace`
3. **ジェネリクス制約はスキップ** - 複雑すぎる部分は諦め

## 🚨 既知の限界

**諦める部分:**
- LINQ式内の複雑なラムダ
- 動的生成（Reflection.Emit）
- 複雑なジェネリクス制約
- Expression Trees

**でも十分:**
「通常のC#コードの80%は確実に検出可能」

## 📊 成功指標

| ファイル | 目標検出数 | 期待値 |
|----------|------------|--------|
| **.NET Runtime sample** | 多数 | 高精度検出 |
| **simple test** | 完全 | 100% |

## 💡 実装メモ

**成功の理由:**
1. **段階的実装** - 極小→シンプル→完全
2. **C#の規則正しさ** - 予測可能な構文
3. **適度な諦め** - 完璧主義を避けた

**他言語への教訓:**
「C#のアプローチを参考に、各言語の特徴に合わせて調整」

**次のアクション:**
1. .NET Runtimeでの動作検証
2. プロパティ・イベント検出の追加検討
3. 成功パターンの他言語展開