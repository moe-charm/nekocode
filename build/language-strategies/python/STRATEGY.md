# 🐍 Python 解析戦略

## 🎯 言語特性と課題

### **インデント地獄との戦い**
```python
class OuterClass:
    def method1(self):
        def inner_function():
            if True:
                def deeply_nested():
                    pass
                return deeply_nested
        return inner_function
    
    def method2(self):
        pass
```

### **Python特有の困難**
| 問題 | 例 | 対策状況 |
|------|----|----|
| **インデント構文** | `class A:\n    def f():` | ✅ PEGTL対応済み |
| **動的関数定義** | `setattr(obj, 'func', lambda: x)` | ❌ 諦め |
| **デコレータ** | `@property\ndef getter():` | ❌ 未対応 |
| **ネスト関数** | `def outer(): def inner():` | ⚠️ 要検証 |
| **クラスメソッド** | `@classmethod\ndef cm():` | ❌ 未対応 |

## 🔧 採用戦略: 超シンプル化作戦

### **Phase 1: 極小文法（無限ループ回避）**
```cpp
// 過去の失敗: 複雑なPython文法 → 無限ループ
// 新方針: 超絶シンプル化

struct python_minimal : seq<
    ignore,
    opt<python_element>,
    star<any>  // 残りは何でもOK（安全策）
> {};
```

**検出対象:**
- ✅ `def function_name():` - 基本関数
- ✅ `class ClassName:` - 基本クラス  
- ✅ `from module import name` - インポート
- ❓ `async def async_func():` - 非同期関数

### **Phase 2: インデント無視戦略**
```cpp
// Pythonの美学を捨てて実用性を取る
struct python_function : seq<
    star<space>,  // インデントは単なる空白として処理
    TAO_PEGTL_STRING("def"),
    plus<space>,
    identifier,
    one<'('>,
    until<one<')'>>,
    one<')'>,
    star<space>,
    one<':'>
> {};
```

### **Phase 3: デコレータスキップ**
```cpp
// @decorator は前処理で除去
std::string preprocess_python(const std::string& content) {
    std::vector<std::string> lines;
    for (const auto& line : split_lines(content)) {
        if (!starts_with(trim(line), "@")) {  // デコレータ行を除去
            lines.push_back(line);
        }
    }
    return join(lines, "\n");
}
```

## 📊 現在の状況

**2025-07-28時点:**
- ✅ PEGTL移行完了（インデント地獄攻略済み）
- ✅ 基本的なクラス・関数検出は動作
- ⚠️ 実際のPythonプロジェクトでの検証が不足

**テスト対象:**
- `requests` ライブラリ（ダウンロード済み）
- リアルワールドPythonコードでの動作検証

## 🧠 設計思想

### **「Pythonらしさより実用性」**
```python
# Pythonの美しいインデント構造
class Beautiful:
    def method(self):
        if condition:
            for item in items:
                result = process(item)
                yield result
                
# ↓ 我々の解析では
# class Beautiful: ← これだけ検出できればOK
# def method(self): ← これだけ検出できればOK
```

**割り切り:**
1. **インデント構造は無視** - 存在検出のみ
2. **動的生成は諦め** - 静的な定義のみ
3. **デコレータは前処理で除去** - ノイズとして扱う

## 🚨 諦める部分

**完璧を求めず、実用性重視:**
- `exec()`, `eval()` 内の動的定義
- メタクラスによる動的生成
- 複雑なデコレータチェーン
- `__getattr__` による動的メソッド

## 📊 成功指標

| ファイル | 目標検出数 | 期待値 |
|----------|------------|--------|
| **requests/api.py** | ~20関数 | 15+ |
| **simple test** | 3-5要素 | 完全検出 |

## 💡 実装メモ

**教訓:**
「Pythonのインデント地獄は一度攻略済み。今度は実戦投入」

**次のアクション:**
1. requestsライブラリでの実戦テスト
2. デコレータ前処理の実装
3. 必要に応じてフォールバック追加