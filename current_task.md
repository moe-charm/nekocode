# 🚀 Phase 3: Rust先行UniversalSymbol実装計画

**最終更新**: 2025-08-08  
**状況**: ✅ Phase 1,2完了 → Phase 3 Rust縦断実装開始

---

## 📋 Phase 1,2の成果（完了済み）

### ✅ **Phase 1: Rust impl分類修正**
- implメソッドをfunctions[]からclasses[].methods[]に正しく分類
- 2025-08-08 コミット済み

### ✅ **Phase 2: Rust metadata拡張**  
- parent_struct, impl_type, trait_name等のメタデータ追加
- JSONフォーマッターでmetadata出力対応
- 2025-08-08 コミット済み

---

## 🎯 Phase 3: Rust先行実装戦略

### **なぜRust先行？**
1. **最も準備が整っている** - Phase 1,2でRustは完全実装済み
2. **端から端まで動くものを作れる** - 設計の妥当性を実証
3. **リスク最小** - 他言語は既存実装のまま動作継続
4. **学習効果** - Rustで得た知見を他言語展開に活用

---

## 📊 実装ステップ（Rust縦断方式）

### **Step 3.1: UniversalSymbol基盤構築（1日）**

#### 3.1.1 universal_symbol.hpp作成
```cpp
// include/nekocode/universal_symbol.hpp
namespace nekocode {

enum class SymbolType {
    // Rust向け優先
    STRUCT,     // Rustのstruct
    TRAIT,      // Rustのtrait
    IMPL_BLOCK, // Rustのimplブロック
    METHOD,     // メソッド
    FUNCTION,   // 独立関数
    MEMBER_VAR, // メンバ変数
    
    // 後で他言語向けに追加
    CLASS,      // JS/TS/C++/C#のclass
    INTERFACE,  // TS/C#のinterface
    ENUM,       // 列挙型
    NAMESPACE,  // 名前空間
    MODULE,     // モジュール
    
    UNKNOWN
};

struct UniversalSymbolInfo {
    // 基本情報
    SymbolType symbol_type = SymbolType::UNKNOWN;
    std::string name;
    std::string qualified_name;  // 例: DatabaseManager::new
    
    // 位置情報
    LineNumber start_line = 0;
    LineNumber end_line = 0;
    
    // 階層情報（IDベース管理）
    std::string symbol_id;      // 例: "struct_DatabaseManager"
    std::string parent_id;      // 親シンボルのID
    std::vector<std::string> child_ids;  // 子シンボルのID
    
    // Phase 2のmetadataを活用
    std::unordered_map<std::string, std::string> metadata;
    
    // 追加情報
    std::vector<std::string> parameters;
    ComplexityInfo complexity;
    
    // ID生成ヘルパー
    static std::string generate_id(SymbolType type, const std::string& name);
};

}
```

#### 3.1.2 symbol_table.hpp作成
```cpp
// include/nekocode/symbol_table.hpp
namespace nekocode {

class SymbolTable {
private:
    // IDベースの管理
    std::unordered_map<std::string, UniversalSymbolInfo> symbols_;
    std::vector<std::string> root_symbols_;  // トップレベルシンボル
    
    // 検索用インデックス
    std::unordered_map<std::string, std::vector<std::string>> name_index_;
    
public:
    // 基本操作
    void add_symbol(const UniversalSymbolInfo& symbol);
    UniversalSymbolInfo* get_symbol(const std::string& id);
    const UniversalSymbolInfo* get_symbol(const std::string& id) const;
    
    // 階層操作
    std::vector<UniversalSymbolInfo> get_children(const std::string& parent_id) const;
    std::vector<UniversalSymbolInfo> get_roots() const;
    
    // 検索
    std::vector<UniversalSymbolInfo> find_by_name(const std::string& name) const;
    std::vector<UniversalSymbolInfo> get_all_symbols() const;
    
    // JSON出力用
    nlohmann::json to_json() const;
};

}
```

---

### **Step 3.2: Rust専用変換レイヤー（2日）**

#### 3.2.1 rust_symbol_converter.hpp
```cpp
// src/converters/rust_symbol_converter.hpp
namespace nekocode {

class RustSymbolConverter {
private:
    // ID生成管理
    std::unordered_set<std::string> used_ids_;
    
    std::string generate_unique_id(const std::string& base);
    
public:
    // 既存のAnalysisResultから変換
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // 逆変換（互換性のため）
    AnalysisResult convert_to_analysis_result(const SymbolTable& symbols);
    
private:
    // 変換ヘルパー
    UniversalSymbolInfo convert_struct(const ClassInfo& cls);
    UniversalSymbolInfo convert_method(const FunctionInfo& method, const std::string& parent_struct);
    UniversalSymbolInfo convert_function(const FunctionInfo& func);
    UniversalSymbolInfo convert_member_var(const MemberVariable& var, const std::string& parent_struct);
};

}
```

#### 3.2.2 実装の要点
```cpp
// src/converters/rust_symbol_converter.cpp
SymbolTable RustSymbolConverter::convert_from_analysis_result(const AnalysisResult& result) {
    SymbolTable table;
    
    // 1. structを変換
    for (const auto& cls : result.classes) {
        UniversalSymbolInfo struct_sym;
        struct_sym.symbol_type = SymbolType::STRUCT;
        struct_sym.name = cls.name;
        struct_sym.symbol_id = generate_unique_id("struct_" + cls.name);
        struct_sym.start_line = cls.start_line;
        struct_sym.end_line = cls.end_line;
        
        // 2. メンバ変数を子要素として追加
        for (const auto& var : cls.member_variables) {
            UniversalSymbolInfo var_sym = convert_member_var(var, struct_sym.symbol_id);
            struct_sym.child_ids.push_back(var_sym.symbol_id);
            table.add_symbol(var_sym);
        }
        
        // 3. メソッドを子要素として追加（Phase 1,2の成果活用）
        for (const auto& method : cls.methods) {
            UniversalSymbolInfo method_sym = convert_method(method, struct_sym.symbol_id);
            
            // Phase 2のmetadataを引き継ぎ
            method_sym.metadata = method.metadata;
            
            struct_sym.child_ids.push_back(method_sym.symbol_id);
            table.add_symbol(method_sym);
        }
        
        table.add_symbol(struct_sym);
    }
    
    // 4. 独立関数を変換
    for (const auto& func : result.functions) {
        table.add_symbol(convert_function(func));
    }
    
    return table;
}
```

---

### **Step 3.3: SessionData統合（2日）**

#### 3.3.1 AnalysisResult拡張
```cpp
// include/nekocode/types.hpp（既存ファイルに追加）
struct AnalysisResult {
    // 既存フィールドは全て維持
    FileInfo file_info;
    std::vector<ClassInfo> classes;
    std::vector<FunctionInfo> functions;
    // ...
    
    // 🆕 Rust専用UniversalSymbol（オプショナル）
    std::shared_ptr<SymbolTable> universal_symbols;
};
```

#### 3.3.2 SessionData拡張
```cpp
// include/nekocode/session.hpp（既存ファイルに追加）
struct SessionData {
    // 既存フィールドは全て維持
    std::string session_id;
    std::filesystem::path target_path;
    // ...
    
    // 🆕 UniversalSymbol情報（Rustのみ対応）
    std::shared_ptr<SymbolTable> universal_symbols;
    
    // Rustセッション作成時に自動変換
    void enhance_with_symbols() {
        if (language == Language::RUST && single_file_result.has_value()) {
            RustSymbolConverter converter;
            universal_symbols = std::make_shared<SymbolTable>(
                converter.convert_from_analysis_result(single_file_result.value())
            );
        }
    }
};
```

---

### **Step 3.4: JSON出力とテスト（1日）**

#### 3.4.1 JSON出力拡張
```cpp
// src/formatters/formatters.cpp
std::string AIReportFormatter::format_single_file(const AnalysisResult& result) {
    nlohmann::json json_result;
    
    // 既存出力は全て維持
    json_result["classes"] = format_classes(result.classes);
    json_result["functions"] = format_functions(result.functions);
    
    // 🆕 Rustの場合のみsymbols追加
    if (result.universal_symbols) {
        json_result["symbols"] = result.universal_symbols->to_json();
    }
    
    return json_result.dump(2);
}
```

#### 3.4.2 期待されるJSON出力
```json
{
  "classes": [...],     // 既存出力維持
  "functions": [...],   // 既存出力維持
  "symbols": [          // 🆕 Rust専用追加
    {
      "symbol_type": "struct",
      "name": "DatabaseManager",
      "symbol_id": "struct_DatabaseManager",
      "start_line": 7,
      "end_line": 40,
      "children": [
        {
          "symbol_type": "member_var",
          "name": "host",
          "parent_id": "struct_DatabaseManager"
        },
        {
          "symbol_type": "method",
          "name": "new",
          "parent_id": "struct_DatabaseManager",
          "metadata": {
            "parent_struct": "DatabaseManager",
            "impl_type": "inherent",
            "access_modifier": "pub"
          }
        }
      ]
    }
  ]
}
```

---

### **Step 3.5: 他言語展開（各言語1-2日）**

#### 展開順序（Rustの実装を参考に）
1. **JavaScript/TypeScript** - classベースで類似
2. **Python** - class/functionが明確
3. **C++** - 複雑だが重要
4. **C#** - C++と類似
5. **Go** - struct/interfaceベース

各言語でRustと同様の手順：
1. metadata追加（Phase 2相当）
2. SymbolConverter実装
3. テストと検証

---

## 📊 スケジュール

| Step | 内容 | 期間 | 状況 |
|------|------|------|------|
| 3.1 | UniversalSymbol基盤 | 1日 | 🔄 開始予定 |
| 3.2 | Rust変換レイヤー | 2日 | ⏳ 待機 |
| 3.3 | SessionData統合 | 2日 | ⏳ 待機 |
| 3.4 | JSON出力・テスト | 1日 | ⏳ 待機 |
| 3.5 | 他言語展開 | 5-10日 | ⏳ 待機 |

**Rust完全対応まで**: 6日  
**全言語対応まで**: 11-16日

---

## ⚠️ リスク管理

### **既存機能への影響**
- ❌ 既存のclasses/functions出力は変更しない
- ❌ 既存のSessionData構造は破壊しない
- ✅ Rustのみ追加機能として実装

### **ロールバック計画**
- 各Stepごとに独立コミット
- universal_symbolsはnullptrチェックで保護
- 問題があれば該当コミットのみrevert

---

## 🎯 次のアクション

### **今すぐ開始: Step 3.1.1**
1. `include/nekocode/universal_symbol.hpp`作成
2. 基本的な構造体定義
3. Rust向けSymbolType定義
4. コンパイル確認

### **続いて: Step 3.1.2**
1. `include/nekocode/symbol_table.hpp`作成
2. 基本的なシンボル管理機能
3. 階層構造の管理

---

**準備完了！Rust先行実装を開始します！** 🚀