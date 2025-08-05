# 🎯 デッドコード検出ファースト戦略

## 📊 現在の状況

### ✅ 完了
- **C++**: LTO統合済み（100%精度）
- **技術実証**: Clang-Tidy YAML解析完了（後回し決定）

### 🔄 次の優先順位
1. **Python Vulture統合** 🐍
2. **Go staticcheck統合** 🐹
3. **統一フォーマット設計** 📋

## 🚀 Python Vulture統合（最優先）

### Vultureの特徴
```bash
# インストール
pip install vulture

# 基本使用
vulture src/

# ホワイトリスト生成
vulture --make-whitelist src/ > whitelist.py
vulture src/ whitelist.py
```

### 検出できるもの
- 未使用関数
- 未使用クラス
- 未使用変数
- 未使用インポート
- 未使用プロパティ

### 実装例
```python
class PythonVultureAnalyzer:
    def analyze_deadcode(self, python_file):
        # vulture実行
        result = subprocess.run([
            'vulture', python_file, '--json'
        ], capture_output=True, text=True)
        
        # JSON解析
        unused_items = json.loads(result.stdout)
        
        return {
            'tool': 'vulture',
            'language': 'python',
            'unused_functions': [...],
            'unused_classes': [...],
            'unused_imports': [...]
        }
```

## 🐹 Go staticcheck統合

### staticcheckの未使用検出
```bash
# インストール  
go install honnef.co/go/tools/cmd/staticcheck@latest

# 未使用コード検出
staticcheck -checks=U1000,U1001,U1002 ./...
```

### 検出できるもの
- U1000: 未使用関数・メソッド
- U1001: 未使用変数
- U1002: 未使用constants

## 📋 統一フォーマット

### 共通インターフェース
```bash
nekocode_ai analyze <path> --complete
```

### 統一JSON出力
```json
{
  "analysis_mode": "complete",
  "structure": {
    "functions": 25,
    "classes": 8,
    "files": 12
  },
  "dead_code": {
    "language": "python",
    "detection_tool": "vulture", 
    "unused_functions": [
      {"name": "deprecated_func", "file": "utils.py", "line": 42}
    ],
    "unused_classes": [
      {"name": "OldClass", "file": "legacy.py", "line": 15}
    ],
    "unused_imports": [
      {"name": "unused_module", "file": "main.py", "line": 3}
    ],
    "summary": {
      "total_unused": 8,
      "potential_cleanup": "15% code reduction"
    }
  }
}
```

## 💡 シンプルな価値提案

### ユーザーへのメッセージ
```
🎯 「完全解析」= 完全なデッドコード検出

✅ C++: LTO で100%精度のデッドコード検出
✅ Python: Vulture で未使用関数・クラス・インポート検出  
✅ Go: staticcheck で未使用関数・変数検出
✅ 全言語: 統一インターフェース（--completeフラグ）

→ プロジェクトから不要コードを完全除去！
```

## 🏆 成功指標

1. **検出率**: 各言語で90%以上の未使用コード検出
2. **統一性**: 全言語で同じインターフェース
3. **実用性**: CI/CDで使える速度・精度
4. **シンプル性**: 複雑な設定不要

## 次のアクション

最優先: **Python Vulture統合**から開始にゃ！

これで「泣く子も禿げる完全解析」の第一段階として、「完全デッドコード検出」が実現するにゃ！🚀