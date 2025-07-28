# 🟡 JavaScript解析エンジン

## 🎯 設計思想・配置理由

### **なぜ独立フォルダにしたか**
1. **技術的複雑性**: JavaScriptはIIFE、クロージャ、動的プロパティ等の特殊パターンが多い
2. **ハイブリッド戦略の証明**: lodash.js 0→489関数検出の歴史的大成功を果たした
3. **メンテナンス性**: JavaScript特化のバグ修正・機能追加が頻繁に発生
4. **教育価値**: 他言語への成功パターン移植の参考実装として重要

### **ファイル構成と役割**
```
javascript/
├── javascript_pegtl_analyzer.cpp    # メイン実装（PEGTL + ハイブリッド戦略）
├── javascript_analyzer.cpp          # レガシー実装（参考・比較用）
└── README.md                        # この設計理由書
```

### **JavaScript特有の解析課題**
- **IIFE内関数**: `(function(){ function hidden(){} })()`
- **クロージャ**: `const f = function() { return function() {} }`
- **動的プロパティ**: `obj.method = function(){}`
- **ES6+構文**: アロー関数、分割代入、テンプレートリテラル

### **ハイブリッド戦略の核心**
```cpp
bool needs_line_based_fallback(const AnalysisResult& result, const std::string& content) {
    // 統計整合性チェック: 複雑度 vs 検出数の妥当性検証
    uint32_t complexity = result.complexity.cyclomatic_complexity;
    size_t detected_functions = result.functions.size();
    
    if (complexity > 100 && detected_functions < 10) return true;  // ← IIFE検出
    if (content.find(";(function()") != std::string::npos) return true;
    
    return false;
}
```

## 🏆 成果実績
- **lodash.js**: 0→489関数検出 (∞倍改善!)
- **複雑JavaScriptライブラリ**: 高精度解析実現
- **他言語移植**: TypeScript・C++・C#への成功パターン提供

## 💡 将来展望
- ES2024対応
- Node.js特化機能
- React/Vue.js JSX解析