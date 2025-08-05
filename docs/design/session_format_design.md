# 🎯 セッションフォーマット設計 - 確信度対応

## 🤔 問題：ツールごとに異なる確信度

### 現実
```
C++ LTO:     100%精度（確信度の概念なし）
Python Vulture: 60%〜90%確信度
Go staticcheck: ？
Rust cargo:    ？
```

## 📊 セッションフォーマット候補

### Option 1: シンプル（文字列のまま）
```json
{
  "dead_code": {
    "tool": "Vulture",
    "unused_items": [
      "test.py:6: unused import 'os' (90% confidence)",
      "test.py:14: unused function 'unused_function' (60% confidence)"
    ]
  }
}
```

**メリット**: シンプル、実装簡単
**デメリット**: パース困難、統一性なし

### Option 2: 構造化
```json
{
  "dead_code": {
    "tool": "Vulture",
    "items": [
      {
        "type": "unused_import",
        "name": "os",
        "file": "test.py", 
        "line": 6,
        "confidence": 90,
        "message": "unused import 'os'"
      },
      {
        "type": "unused_function",
        "name": "unused_function",
        "file": "test.py",
        "line": 14, 
        "confidence": 60,
        "message": "unused function 'unused_function'"
      }
    ]
  }
}
```

**メリット**: 構造化、フィルタリング可能
**デメリット**: パース複雑、ツール依存

### Option 3: 統一フォーマット（推奨）
```json
{
  "dead_code": {
    "tool": "Vulture",
    "summary": {
      "total_found": 6,
      "high_confidence": 2,  // ≥80%
      "medium_confidence": 4, // 60-79%
      "low_confidence": 0     // <60%
    },
    "items": [
      {
        "name": "os",
        "type": "import",
        "location": "test.py:6",
        "confidence": 90,
        "category": "high"
      },
      {
        "name": "unused_function", 
        "type": "function",
        "location": "test.py:14",
        "confidence": 60,
        "category": "medium"
      }
    ]
  }
}
```

### Option 4: 超シンプル（数字横）
```json
{
  "dead_code": {
    "tool": "Vulture",
    "items": [
      "unused import 'os' (90%)",
      "unused function 'unused_function' (60%)",
      "unused class 'UnusedClass' (60%)"
    ]
  }
}
```

## 🎯 他ツールとの統一性

### C++ LTO（確信度なし）
```json
{
  "dead_code": {
    "tool": "LTO", 
    "items": [
      "unused function 'dead_func' (100%)",  // 100%で統一
      "unused variable 'dead_var' (100%)"
    ]
  }
}
```

### Go staticcheck（要調査）
```bash
# 実際の出力例
main.go:10:6: unused function foo (U1000)
```

### 統一案
```json
{
  "dead_code": {
    "language": "python",
    "tool": "vulture",
    "items": [
      {
        "item": "unused import 'os'",
        "confidence": 90,
        "location": "test.py:6"
      }
    ],
    "confidence_scale": "percentage" // or "categorical"
  }
}
```

## 💡 推奨案：**シンプル+カテゴリ**

```json
{
  "dead_code": {
    "tool": "Vulture",
    "language": "python", 
    "items": {
      "high_confidence": [    // ≥80%
        "unused import 'os' (90%) at test.py:6",
        "unused import 'unused_module' (90%) at test.py:8"
      ],
      "medium_confidence": [  // 60-79%
        "unused function 'unused_function' (60%) at test.py:14",
        "unused class 'UnusedClass' (60%) at test.py:23"
      ],
      "low_confidence": []    // <60%
    },
    "summary": {
      "total": 6,
      "high": 2,
      "medium": 4, 
      "low": 0
    }
  }
}
```

## 🚀 実装方針

### 1. まずは超シンプル
```json
{
  "dead_code": {
    "items": [
      "unused import 'os' (90%)",
      "unused function 'foo' (60%)"
    ]
  }
}
```

### 2. 必要に応じて構造化
- AIが解析しやすいよう
- ユーザーがフィルタリングしやすいよう

## 🤔 質問

1. **確信度の扱い**: 数字そのまま？ カテゴリ分け？
2. **統一性**: 全ツールで確信度表現統一？
3. **シンプルさ**: どこまで構造化する？

**とりあえず「数字横に出すだけ」が一番実用的かもにゃ！**