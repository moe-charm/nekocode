# 🟣 C#解析エンジン

## 🎯 設計思想・配置理由

### **なぜC#が独立フォルダを必要とするか**
1. **企業開発の中核言語**: .NET Framework/.NET Core/5+での大規模システム開発
2. **構文規則性活用**: JavaScriptと異なり予測可能な構文→PEGTL実装に最適
3. **ハイブリッド戦略適用**: JavaScript成功パターンの移植で安定した成果
4. **現在進行形の改善**: PEGTL文法の根本修正プロジェクト実施中

### **ファイル構成と役割**
```
csharp/
├── csharp_pegtl_analyzer.cpp       # PEGTL実装（ハイブリッド戦略付き）
├── csharp_analyzer.cpp             # レガシー実装（比較・退避用）
└── README.md                       # この設計理由書
```

### **C#特有の解析課題と対応戦略**
- **修飾子組み合わせ**: `public static async Task<T>` → 順序柔軟な文法設計
- **ジェネリクス**: `List<Dictionary<string, T>>` → ネスト対応パーサー
- **プロパティ記法**: `=> expression;` と `{ get; set; }` → 2パターン解析
- **LINQ式**: `data.Where(x => x.IsValid)` → 複雑だが諦める設計
- **名前空間**: `namespace Company.Product.Module` → ドット記法対応

### **現在の実装状況 (2025-07-28)**
```cpp
// ✅ 成功部分: ハイブリッド戦略完璧動作
bool needs_csharp_line_based_fallback(...) {
    if (complexity > 30 && detected_classes == 0) return true;
    if ((has_class || has_namespace) && detected_classes == 0) return true;
    return false;
}

// ⚠️ 改善中: PEGTL文法の現実対応
struct modifiers : star<seq<sor<access_modifier, other_modifier>, plus<space>>> {};
struct normal_method : seq<modifiers, type_name, plus<space>, identifier, ...>;
struct constructor : seq<modifiers, identifier, method_params, ...>;
```

### **現在の成果と課題**
**✅ 成功実績**:
- test_csharp_hybrid.cs: 5クラス・2関数検出
- .NET Runtime OleDbTestBase.cs: 2クラス検出
- 統計整合性チェック: 完璧動作

**⚠️ 改善課題**:
- PEGTL解析が常に失敗 → regex依存
- メソッド検出不完全（コンストラクタ未検出）
- プロパティ=>記法の部分対応

### **設計哲学: C#の優等生ぶりを活用**
```csharp
// C#の良い所:
// 1. 構文が比較的規則正しい
// 2. キーワードが明確
// 3. ブロック構造が分かりやすい  
// 4. JavaScriptのような「なんでもあり」感が少ない

public class WellStructured {
    public void ClearMethod() {
        // 予測可能な構造
    }
}
```

## 🏆 期待される最終成果
- **PEGTL Success Rate**: 現在0% → 目標80%+
- **メソッド検出精度**: 現在2関数 → 目標15+関数 (test_csharp_hybrid.cs)
- **.NET Runtime適用**: 大規模プロジェクト解析対応

## 💡 将来展望
- C# 12.0最新機能対応
- ASP.NET Core特化解析
- Entity Framework解析機能