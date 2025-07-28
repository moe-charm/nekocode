# 🟨 JavaScript 解析戦略

## 🎯 言語特性と課題

### **最大の敵: IIFE (Immediately Invoked Function Expression)**
```javascript
;(function() {
  function basePropertyOf(object) { ... }  // ← 検出困難
  function baseTrim(string) { ... }        // ← 数百の関数が隠れてる
})();
```

### **JavaScript特有の困難**
| 問題 | 例 | 対策 |
|------|----|----|
| **IIFE内関数** | `(function(){ function f(){} })()` | 行ベース補完 |
| **ネスト関数** | `function outer(){ function inner(){} }` | 括弧レベル追跡 |
| **動的export** | `for(var k in funcs) exports[k]=funcs[k]` | 諦める |
| **セミコロン省略** | `function f()\n{` | 柔軟な空白処理 |
| **Arrow関数** | `const f = () => {}` | 専用ルール |

## 🔧 採用戦略: ハイブリッド方式

### **Phase 1: PEGTL軽量化**
```cpp
// 目標: 完璧解析 → キーワード検出器
struct function_keywords : sor<
    TAO_PEGTL_STRING("function"),
    TAO_PEGTL_STRING("export function"), 
    TAO_PEGTL_STRING("async function")
> {};
```

**検出対象:**
- ✅ `function name()` - 基本関数
- ✅ `export function name()` - ES6エクスポート
- ✅ `async function name()` - 非同期関数
- ✅ `const name = () =>` - アロー関数

### **Phase 2: 行ベース補完**
```cpp
// IIFE内関数の救済作戦
for (const auto& line : lines) {
    // インデント付き関数も検出
    if (std::regex_match(line, std::regex("^\\s*function\\s+(\\w+)"))) {
        // PEGTLで取りこぼした関数を追加
    }
}
```

### **Phase 3: 統計整合性チェック**
```cpp
// 複雑度 vs 検出数の妥当性検証
if (complexity > 500 && functions.size() < 10) {
    // 明らかに取りこぼし → フォールバック実行
    use_aggressive_line_based_analysis();
}
```

## 📊 成功指標

| ファイル | 目標検出数 | 現在 | 戦略後期待値 |
|----------|------------|------|-------------|
| **lodash.js** | ~300関数 | 0 | 200+ |
| **simple test** | 4関数 | 4 | 4 ✅ |
| **export test** | 2関数 | 2 | 2 ✅ |

## 🚨 諦める部分

**完璧を求めず、実用性を重視:**
- 動的に生成される関数名
- eval()内の関数定義  
- 複雑すぎるクロージャ
- 難読化されたコード

## 💡 実装メモ

**2025-07-28 🎆 ハイブリッド戦略実装大成功:**
- ✅ シンプルなケースは完動
- 🎆 **lodash.js: 0→489関数検出** (無限倍改善!)
- ✅ 既存機能の互換性維持  
- 🎯 **完成**: 行ベース補完が完璧動作

**成功の秘訣:**
1. **統計整合性チェック** - 複雑度2432 vs 検出0の異常を自動検知
2. **IIFEパターン検出** - `;(function()` や `(function(){` を自動識別
3. **行ベース救済** - 正規表現で確実に関数名を抽出
4. **重複防止** - 既存関数との重複を防ぐ機構

**検出サンプル** (lodash.js):
```
apply (line 485)
arrayAggregator (line 505) 
arrayEach (line 525)
arrayEachRight (line 546)
arrayEvery (line 567)
... さらに484個の関数検出
```

**終極の教訓:**
「ハイブリッド戦略は現実的コード解析の最適解」
「完璧でない解析でも、動作する解析の方が遺かに価値が高い」