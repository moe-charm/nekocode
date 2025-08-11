# 🌳 Tree-sitter全言語対応計画

## 📊 **現状確認**

### ✅ **完了済み**
- **JavaScript/TypeScript**: Tree-sitter完全実装済み（26倍高速）

### 🚧 **対応待ち言語**
- **Python**: PEST → Tree-sitter移行
- **C++**: PEST → Tree-sitter移行  
- **C#**: PEST → Tree-sitter移行
- **Go**: PEST → Tree-sitter移行
- **Rust**: PEST → Tree-sitter移行

## 🎯 **移行優先順位**

### **Phase 1: Python (最優先)**
- **理由**: 使用頻度高、Tree-sitterサポート充実
- **tree-sitter-python**: 成熟したパーサー
- **期待効果**: 10-20倍高速化

### **Phase 2: C++ (企業重要)**  
- **理由**: 複雑な構文、現在PEST重い
- **tree-sitter-cpp**: 利用可能
- **期待効果**: 5-15倍高速化

### **Phase 3: C# (Unity/企業)**
- **理由**: ゲーム開発・企業システム
- **tree-sitter-c-sharp**: 利用可能
- **期待効果**: 8-12倍高速化

### **Phase 4: Go + Rust (モダン言語)**
- **理由**: 構文シンプル、移行容易
- **tree-sitter-go/rust**: 充実
- **期待効果**: 12-25倍高速化

## 🏗️ **実装アーキテクチャ**

### **統一パターン**
```rust
// 各言語で共通の実装パターン
pub struct TreeSitter[Language]Analyzer {
    parser: Parser,
}

impl TreeSitter[Language]Analyzer {
    pub fn new() -> Result<Self> {
        let mut parser = Parser::new();
        parser.set_language(&tree_sitter_[language]::LANGUAGE.into())?;
        Ok(Self { parser })
    }
    
    pub async fn analyze(&mut self, content: &str, file_path: &str) -> Result<AnalysisResult> {
        // Tree-sitter解析 → AST構築 → AnalysisResult変換
    }
}
```

### **段階的移行戦略**
1. **新アナライザー作成** (`tree_sitter_[lang]_analyzer.rs`)
2. **既存PEST並列動作** (フォールバック保持)
3. **`--parser tree-sitter`で選択可能**
4. **十分テスト後デフォルト化**
5. **PEST版削除**

## 📦 **必要な依存関係**

### **Cargo.toml追加**
```toml
[dependencies]
# 既存
tree-sitter = "0.23"
tree-sitter-javascript = "0.23" 
tree-sitter-typescript = "0.23"

# Phase 1
tree-sitter-python = "0.23"

# Phase 2  
tree-sitter-cpp = "0.23"

# Phase 3
tree-sitter-c-sharp = "0.23"

# Phase 4
tree-sitter-go = "0.23"
tree-sitter-rust = "0.23"
```

## ⚡ **期待される性能向上**

| 言語 | 現在(PEST) | Tree-sitter予想 | 改善倍率 |
|------|-----------|----------------|----------|
| JavaScript/TypeScript | 60.7s | **1.2s** | **🚀 50.6倍** |
| Python | ~30s | **2-3s** | **🚀 10-15倍** |
| C++ | ~25s | **3-5s** | **🚀 5-8倍** |
| C# | ~20s | **2-3s** | **🚀 7-10倍** |
| Go | ~15s | **1-2s** | **🚀 8-15倍** |
| Rust | ~18s | **1-2s** | **🚀 9-18倍** |

## 🎯 **Phase 1実装例: Python**

### **作成ファイル**
```
src/analyzers/python/tree_sitter_analyzer.rs
```

### **実装内容**
```rust
use tree_sitter::{Language, Parser, Tree};
use crate::core::types::*;

pub struct TreeSitterPythonAnalyzer {
    parser: Parser,
}

impl TreeSitterPythonAnalyzer {
    pub fn new() -> Result<Self> {
        let mut parser = Parser::new();
        parser.set_language(&tree_sitter_python::LANGUAGE.into())?;
        Ok(Self { parser })
    }
    
    pub async fn analyze(&mut self, content: &str, file_path: &str) -> Result<AnalysisResult> {
        let tree = self.parser.parse(content, None)
            .ok_or_else(|| anyhow::anyhow!("Failed to parse Python"))?;
            
        self.extract_from_tree(&tree, content, file_path).await
    }
    
    async fn extract_from_tree(&self, tree: &Tree, content: &str, file_path: &str) -> Result<AnalysisResult> {
        // Python専用AST解析ロジック
        // - 関数検出 (def, async def, lambda)
        // - クラス検出 (class)
        // - import/from import
        // - 変数検出
    }
}
```

## 🎉 **全言語Tree-sitter化後のメリット**

### **🚀 性能革命**
- **全言語で10-50倍高速化**
- **大規模プロジェクトが数秒で解析完了**
- **リアルタイム解析が実用レベル**

### **📈 精度向上**  
- **構文解析エラー激減**
- **複雑なネスト構造も完全解析**
- **最新言語機能に対応**

### **🔧 保守性向上**
- **統一されたアーキテクチャ**
- **言語固有バグの削減** 
- **新言語追加が容易**

## 📅 **実装スケジュール**

| フェーズ | 期間 | 実装内容 |
|---------|------|----------|
| **Phase 1** | 1-2週間 | Python Tree-sitter化 |
| **Phase 2** | 2-3週間 | C++ Tree-sitter化 | 
| **Phase 3** | 1-2週間 | C# Tree-sitter化 |
| **Phase 4** | 1週間 | Go + Rust Tree-sitter化 |
| **Cleanup** | 1週間 | PEST版削除、文書更新 |

**🎯 目標: 6-8週間で全言語Tree-sitter完全移行達成！**

---
**最終更新**: 2025-08-11  
**現状**: JavaScript/TypeScript完了、Python Phase1準備中  
**目標**: 全言語で10-50倍高速化達成