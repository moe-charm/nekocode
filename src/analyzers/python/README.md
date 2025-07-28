# 🟢 Python解析エンジン

## 🎯 設計思想・配置理由

### **なぜPython独立フォルダが必要か**
1. **インデント構文の特殊性**: 他言語と根本的に異なるブロック構造
2. **AI・データサイエンス需要**: 機械学習プロジェクトの解析ニーズ急増
3. **動的言語の課題**: JavaScript同様の複雑性があるが、より規則的
4. **ハイブリッド戦略適用準備**: 成功パターンの移植候補として有望

### **ファイル構成と役割**
```
python/
├── python_analyzer.cpp             # 現在の実装
├── python_pegtl_analyzer.cpp       # 将来のPEGTL + ハイブリッド実装
└── README.md                       # この設計理由書
```

### **Python特有の解析課題**
- **インデント依存**: `if condition:\n    def nested_func():`
- **動的メソッド**: `setattr(obj, 'method', lambda: None)`
- **装飾子**: `@property`, `@staticmethod`, `@dataclass`
- **リスト内包表記**: `[func(x) for x in data if condition(x)]`
- **型ヒント**: `def process(data: List[Dict[str, Any]]) -> Optional[str]:`

### **予想される実装戦略**
```python
# 🎯 Pythonの良い所を活用
def clear_structure():      # 明確な関数定義
    pass

class WellDefined:         # 予測可能なクラス構造
    def __init__(self):    # 標準的なコンストラクタ
        pass
    
    @property
    def value(self):       # 装飾子パターン認識
        return self._value
```

### **期待されるハイブリッド戦略**
```cpp
// Python特化の統計整合性チェック
bool needs_python_line_based_fallback(...) {
    // インデント複雑度 vs 検出数の整合性
    if (indentation_depth > 4 && detected_functions < 5) return true;
    
    // Python特有パターン検出
    if (content.find("def ") != std::string::npos && detected_functions == 0) return true;
    if (content.find("class ") != std::string::npos && detected_classes == 0) return true;
    
    return false;
}
```

### **現在の実装状況**
**✅ 既存機能**:
- 基本的な関数・クラス検出
- python_analyzer.cpp で部分実装済み

**🚧 改善予定**:
- PEGTL実装の追加
- ハイブリッド戦略の適用
- 型ヒント・装飾子対応

### **設計哲学: Pythonのシンプルさを活用**
```python
# Pythonの特徴:
# 1. 「読みやすいコードを書く」文化
# 2. 明示的なキーワード (def, class, import)
# 3. 一貫性のあるコーディングスタイル (PEP 8)
# 4. JavaScriptほど「なんでもあり」ではない

# 対応しやすい構造例
def simple_function(param: str) -> str:
    """明確なドキュメント"""
    return param.upper()

class DataProcessor:
    """明確なクラス定義"""
    def process(self, data):
        return [item for item in data if item]
```

## 🏆 期待される成果
- **基本Python**: 高精度な関数・クラス検出
- **科学計算ライブラリ**: NumPy・Pandas・Scikit-learn解析対応  
- **Webフレームワーク**: Django・Flask・FastAPI特化機能

## 💡 将来展望
- Python 3.12+ 最新機能対応
- Jupyter Notebook解析
- AI/ML プロジェクト特化機能
- 型検査ツール（mypy・pyright）連携