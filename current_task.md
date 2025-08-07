# 🌟 Universal Symbol Revolution - 段階的実装計画

**目標**: Rust struct-impl関連付けから始まる、全言語統一シンボル管理システム構築

---

## 🎯 Phase 1: 最小修正 - Rustのimpl分類修正

### **現在の問題**
- Rustのimplメソッドが`functions[]`に混在している
- `struct DatabaseManager`のメソッドが独立関数として分類される
- 正しくは`classes[].methods[]`に入るべき

### **実装内容**
```cpp
// src/analyzers/rust/rust_analyzer.cpp
void RustAnalyzer::analyze() {
    // 1. 既存の解析はそのまま維持
    analyze_structs(content);
    analyze_impls(content);
    
    // 2. 🆕 impl分類修正処理を追加
    fix_impl_method_classification();
}

void RustAnalyzer::fix_impl_method_classification() {
    // implメソッドをfunctions[]からclasses[].methods[]に移動
    for (const auto& impl : impls_) {
        auto* target_struct = find_struct_by_name(impl.struct_name);
        if (target_struct) {
            // implのメソッドをstructのmethodsに移動
            for (const auto& method_name : impl.methods) {
                auto method_func = extract_function_from_list(method_name);
                if (method_func.has_value()) {
                    target_struct->methods.push_back(method_func.value());
                }
            }
        }
    }
}
```

### **期待する結果**
```json
// 修正前 ❌
{
  "classes": [{"name": "DatabaseManager", "methods": []}],
  "functions": [
    {"name": "new", "start_line": 16},      // implメソッドが混在
    {"name": "connect", "start_line": 30},  // implメソッドが混在
    {"name": "standalone_function", "start_line": 221}
  ]
}

// 修正後 ✅
{
  "classes": [
    {
      "name": "DatabaseManager", 
      "methods": [
        {"name": "new", "start_line": 16},
        {"name": "connect", "start_line": 30}
      ]
    }
  ],
  "functions": [
    {"name": "standalone_function", "start_line": 221}  // スタンドアロンのみ
  ]
}
```

**工数見積**: 2-3時間
**リスク**: 低（既存コード影響最小）

---

## 🎯 Phase 2: 中規模修正 - metadata活用でparent_struct追加

### **目標**
- UniversalFunctionInfo.metadataを活用
- 言語固有情報の統一的格納
- 将来のmove-struct機能基盤構築

### **実装内容**
```cpp
// Phase1の成果を活用してメタデータ追加
void RustAnalyzer::enhance_function_metadata() {
    // クラスメソッドにparent_struct情報追加
    for (auto& cls : result.classes) {
        for (auto& method : cls.methods) {
            method.metadata["parent_struct"] = cls.name;
            method.metadata["impl_type"] = determine_impl_type(method, cls);
            method.metadata["language"] = "rust";
            method.metadata["access_modifier"] = determine_access_modifier(method);
        }
    }
    
    // traitメソッドの場合はtrait名も追加
    for (const auto& impl : impls_) {
        if (!impl.trait_name.empty()) {
            // trait実装メソッドの場合
            enhance_trait_methods_metadata(impl);
        }
    }
}
```

### **期待するJSON出力**
```json
{
  "classes": [
    {
      "name": "DatabaseManager",
      "methods": [
        {
          "name": "new",
          "start_line": 16,
          "metadata": {
            "parent_struct": "DatabaseManager",
            "impl_type": "inherent",
            "language": "rust",
            "access_modifier": "pub"
          }
        }
      ]
    }
  ]
}
```

**工数見積**: 3-4時間
**リスク**: 低（既存構造維持）

---

## 🎯 Phase 3: 大規模設計 - UniversalSymbolInfo統一

### **目標**
- 全言語統一シンボル管理
- 階層構造の完全表現
- 次世代解析システム基盤

### **設計思想**
```cpp
enum class SymbolType {
    // 構造要素
    CLASS, STRUCT, INTERFACE, ENUM, TRAIT,
    // 関数要素
    FUNCTION, METHOD, CONSTRUCTOR, DESTRUCTOR,
    // 変数要素
    VARIABLE, MEMBER_VAR, PARAMETER, PROPERTY,
    // 組織要素
    NAMESPACE, MODULE, IMPL_BLOCK
};

struct UniversalSymbolInfo {
    SymbolType symbol_type;
    std::string name;
    std::string parent_context = "";
    LineNumber start_line = 0, end_line = 0;
    
    // 🌳 階層構造
    std::vector<UniversalSymbolInfo> children;
    
    // 🎨 言語固有情報
    std::unordered_map<std::string, std::string> metadata;
    
    // ⚡ 動的情報
    std::vector<std::string> parameters;
    ComplexityInfo complexity;
    bool is_async = false;
};
```

### **変換レイヤー設計**
```cpp
class UniversalSymbolConverter {
public:
    // Phase1,2の成果を活用した変換
    std::vector<UniversalSymbolInfo> convert_analysis_result(
        const AnalysisResult& result, Language lang);
    
private:
    UniversalSymbolInfo convert_rust_struct(const ClassInfo& cls);
    UniversalSymbolInfo convert_rust_function(const FunctionInfo& func);
    // 他言語用変換メソッド...
};
```

### **期待するJSON出力**
```json
{
  "symbols": [
    {
      "symbol_type": "struct",
      "name": "DatabaseManager", 
      "start_line": 7,
      "metadata": {"language": "rust", "visibility": "pub"},
      "children": [
        {
          "symbol_type": "method",
          "name": "new",
          "start_line": 16,
          "parent_context": "DatabaseManager",
          "metadata": {
            "impl_type": "inherent",
            "access_modifier": "pub"
          }
        },
        {
          "symbol_type": "member_var",
          "name": "host",
          "start_line": 8,
          "parent_context": "DatabaseManager"
        }
      ]
    },
    {
      "symbol_type": "function",
      "name": "standalone_function",
      "start_line": 221,
      "parent_context": ""
    }
  ]
}
```

**工数見積**: 1-2週間
**リスク**: 中（大規模変更、十分なテストが必要）

---

## 🎯 Phase 4: 最終統一 - SessionData構造統一

### **目標**
- single_file_result と directory_result の統一
- API簡素化・保守性向上
- 既存コードの段階的移行

### **統一後設計**
```cpp
struct SessionData {
    std::string session_id;
    std::filesystem::path target_path;
    bool is_directory;
    
    // 🆕 統一結果格納
    DirectoryAnalysis analysis_result;  // 単一ファイルもfiles[0]に格納
    
    // 🆕 Universal Symbol情報
    std::vector<UniversalSymbolInfo> universal_symbols;
    
    // 互換性維持
    nlohmann::json quick_stats;
    std::vector<CommandHistory> command_history;
};
```

### **移行戦略**
```cpp
// 既存APIは互換ラッパーで維持
class SessionDataCompat {
public:
    // 既存コード用の互換インターフェース
    const AnalysisResult& get_single_file_result() const {
        if (!is_directory && !analysis_result.files.empty()) {
            return analysis_result.files[0];
        }
        throw std::runtime_error("Not a single file session");
    }
    
    // 新APIは直接アクセス
    const DirectoryAnalysis& get_analysis_result() const {
        return analysis_result;
    }
};
```

**工数見積**: 1週間
**リスク**: 中（既存コード影響大、慎重な移行が必要）

---

## 📋 実装スケジュール

| Phase | 期間 | 主要成果物 | リスク |
|-------|------|-----------|--------|
| Phase 1 | 1日 | Rust impl分類修正 | 低 |
| Phase 2 | 2日 | metadata拡張 | 低 |
| Phase 3 | 2週間 | UniversalSymbolInfo | 中 |
| Phase 4 | 1週間 | SessionData統一 | 中 |

**総工数**: 約3週間
**最小MVP**: Phase 1完了で基本機能実現

---

## ⚠️ 重要な注意点

### **後方互換性の徹底維持**
- 既存のmove-class機能は動作継続必須
- SessionDataのJSON形式は段階的拡張
- 既存APIは互換ラッパーで保護

### **テスト駆動開発**
- 各Phase完了時に動作確認
- Rustのテストファイルで検証
- レグレッションテスト必須

### **段階的リリース**
- Phase 1完了時点でリリース可能
- Phase 2でメタデータ機能提供
- Phase 3で次世代機能解放

---

**最終更新**: 2025-08-08  
**状況**: Phase 1準備完了 - 実装開始準備OK!