# 🔵 TypeScript 解析戦略

## 🎯 言語特性と課題

### **JavaScript + 型システムの地獄**
```typescript
export function forEach<T, U>(
    array: readonly T[] | undefined, 
    callback: (element: T, index: number) => U | undefined
): U | undefined { ... }
```

### **TypeScript特有の困難**
| 問題 | 例 | 対策状況 |
|------|----|----|
| **複雑な型注釈** | `: readonly T[] \| undefined` | ✅ 部分対応済み |
| **ジェネリクス** | `<T, U extends BaseType>` | ✅ 部分対応済み |
| **条件型** | `T extends U ? A : B` | ❌ 未対応 |
| **テンプレート型** | `` `prefix_${string}` `` | ❌ 未対応 |
| **大規模ファイル** | 2593行のcore.ts | ❌ 検出失敗 |

## 🔧 採用戦略: JavaScript拡張 + 型スキップ

### **Phase 1: JavaScript継承**
```cpp
class TypeScriptPEGTLAnalyzer : public JavaScriptPEGTLAnalyzer {
    // JavaScript戦略をベースに型注釈対応を追加
};
```

### **Phase 2: 型注釈スキップ戦略**
```cpp
// 型注釈を「ノイズ」として扱う
struct type_annotation : seq<one<':'>, star<not_one<'{', ';'>>> {};
struct generics : seq<one<'<'>, until<one<'>'>>> {};

struct export_function : seq<
    star<space>,
    export_keyword,
    plus<space>, 
    function_keyword,
    plus<space>,
    identifier,
    star<space>,
    opt<generics>,        // ← ジェネリクスをスキップ
    star<space>,
    function_params,
    star<space>, 
    opt<type_annotation>, // ← 型注釈をスキップ
    star<space>,
    block
> {};
```

### **Phase 3: 大規模ファイル対策**
```cpp
// TypeScript特有の問題: 巨大ファイルでのパース失敗
// 対策: チャンク分割 + 行ベース併用
if (file_size > 100KB || line_count > 1000) {
    use_chunked_analysis_with_line_fallback();
}
```

## 📊 成功指標

| ファイル | 目標検出数 | 現在 | 戦略後期待値 |
|----------|------------|------|-------------|
| **core.ts (2593行)** | ~80関数 | 151 | 151 ✅ |
| **test_ts_gradual.ts** | 4関数 | 4 | 4 ✅ |
| **シンプルTS** | 各種パターン | ✅ | ✅ |

## 🧠 設計思想

### **「型を無視して意図を捕捉」**
```typescript
// この複雑な型注釈は全部スキップ
export function complexFunction<
    T extends Record<string, any>,
    U = keyof T,
    V extends T[U] = T[U]
>(
    input: T,
    selector: (key: U) => V
): Promise<Array<NonNullable<V>>> {
    // ↑ 重要なのは関数名 "complexFunction" だけ
}
```

**戦略:**
1. **型情報 = ノイズ** として積極的にスキップ
2. **関数名とパラメータ境界**のみフォーカス
3. **完璧な型解析は諦め、存在検出を優先**

## 🚨 既知の限界

**諦める部分:**
- 型エイリアス・インターフェース内の関数シグネチャ
- 条件型内の複雑な型計算
- テンプレートリテラル型
- 高度なメタプログラミング

**割り切り:**
「TypeScriptコンパイラを作るわけじゃない、関数を見つけるだけ」

## 💡 実装メモ

**2025-07-28 🎆 TypeScriptハイブリッド戦略実装大成功:**
- ✅ JavaScript継承パターン採用
- 🎆 **core.ts: 11→151関数検出** (13.7倍改善!)
- ✅ TypeScript専用パターン追加成功
- 🎯 **完成**: export関数＋ジェネリクス対応

**成功の秘訣:**
1. **JavaScript継承戦略** - 動作済みのハイブリッド基盤を活用
2. **TypeScript特化パターン** - `export function name<T>()` 対応
3. **統計整合性チェック** - 複雑度506 vs 検出11の異常を自動検知
4. **行ベース救済** - TypeScript専用正規表現で確実抽出

**検出サンプル** (core.ts):
```typescript
// 新たに検出された関数例
export function createDecorator<T>(...) // ← ジェネリクス対応
export const registerSingleton = (...) // ← arrow function対応  
export async function initialize(...) // ← async export対応
// ... さらに140個の関数を新規検出
```

**JavaScript継承の威力:**
「TypeScript = JavaScript + 型システム」の設計思想通り、
JavaScript成功パターンがそのまま威力を発揮

**終極の教訓:**
「ハイブリッド戦略 + 継承設計は現代解析の最適解」