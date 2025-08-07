//=============================================================================
// 🌟 NekoCode Universal Symbol System
//
// Phase 3: 言語統一シンボル管理システム
// Rust先行実装 - 他言語は順次追加
//=============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "nekocode/types.hpp"  // LineNumber, ComplexityInfo等

namespace nekocode {

//=============================================================================
// 🎯 Symbol Type Enumeration
//=============================================================================

enum class SymbolType {
    // Rust向け優先
    STRUCT,      // Rustのstruct
    TRAIT,       // Rustのtrait  
    IMPL_BLOCK,  // Rustのimplブロック
    METHOD,      // メソッド（impl内の関数）
    FUNCTION,    // 独立関数（トップレベル関数）
    MEMBER_VAR,  // メンバ変数（structのフィールド）
    
    // 他言語向け（将来追加）
    CLASS,       // JS/TS/C++/C#のclass
    INTERFACE,   // TS/C#/Go/Javaのinterface
    ENUM,        // 列挙型
    NAMESPACE,   // C++/C#の名前空間
    MODULE,      // Python/Goのモジュール
    PACKAGE,     // Javaのパッケージ
    
    // 共通要素
    CONSTRUCTOR, // コンストラクタ
    DESTRUCTOR,  // デストラクタ
    PROPERTY,    // プロパティ（getter/setter）
    PARAMETER,   // 関数パラメータ
    VARIABLE,    // ローカル変数
    CONSTANT,    // 定数
    
    // その他
    UNKNOWN
};

//=============================================================================
// 🌟 Universal Symbol Information
//=============================================================================

struct UniversalSymbolInfo {
    // 基本情報
    SymbolType symbol_type = SymbolType::UNKNOWN;
    std::string name;                // シンボル名（例: "new", "DatabaseManager"）
    std::string qualified_name;      // 完全修飾名（例: "DatabaseManager::new"）
    
    // 位置情報
    LineNumber start_line = 0;
    LineNumber end_line = 0;
    
    // 階層情報（IDベース管理）
    std::string symbol_id;            // ユニークID（例: "struct_DatabaseManager_1"）
    std::string parent_id;            // 親シンボルのID（ルートなら空文字）
    std::vector<std::string> child_ids;  // 子シンボルのIDリスト
    
    // Phase 2のmetadataを活用（言語固有情報）
    std::unordered_map<std::string, std::string> metadata;
    // 例: Rustの場合
    // - "parent_struct": "DatabaseManager"
    // - "impl_type": "inherent" | "trait"
    // - "trait_name": "Clone"（trait implの場合）
    // - "access_modifier": "pub" | "pub(crate)" | "private"
    // - "return_type": "Self" | "Result<(), Error>"
    // - "is_async": "true" | "false"
    // - "is_unsafe": "true" | "false"
    
    // 追加情報
    std::vector<std::string> parameters;  // 関数パラメータリスト
    ComplexityInfo complexity;            // 複雑度情報
    
    // ヘルパー関数
    
    // SymbolTypeを文字列に変換
    static std::string symbol_type_to_string(SymbolType type);
    
    // ID生成ヘルパー（型_名前_連番の形式）
    static std::string generate_id(SymbolType type, const std::string& name, int sequence = 0);
    
    // 階層の深さを取得（デバッグ用）
    size_t get_depth() const;
    
    // メタデータの安全な取得
    std::string get_metadata(const std::string& key, const std::string& default_value = "") const;
    
    // JSON変換用
    nlohmann::json to_json() const;
};

//=============================================================================
// 実装
//=============================================================================

inline std::string UniversalSymbolInfo::symbol_type_to_string(SymbolType type) {
    switch (type) {
        case SymbolType::STRUCT:      return "struct";
        case SymbolType::TRAIT:       return "trait";
        case SymbolType::IMPL_BLOCK:  return "impl_block";
        case SymbolType::METHOD:      return "method";
        case SymbolType::FUNCTION:    return "function";
        case SymbolType::MEMBER_VAR:  return "member_var";
        case SymbolType::CLASS:       return "class";
        case SymbolType::INTERFACE:   return "interface";
        case SymbolType::ENUM:        return "enum";
        case SymbolType::NAMESPACE:   return "namespace";
        case SymbolType::MODULE:      return "module";
        case SymbolType::PACKAGE:     return "package";
        case SymbolType::CONSTRUCTOR: return "constructor";
        case SymbolType::DESTRUCTOR:  return "destructor";
        case SymbolType::PROPERTY:    return "property";
        case SymbolType::PARAMETER:   return "parameter";
        case SymbolType::VARIABLE:    return "variable";
        case SymbolType::CONSTANT:    return "constant";
        case SymbolType::UNKNOWN:     return "unknown";
        default:                      return "unknown";
    }
}

inline std::string UniversalSymbolInfo::generate_id(SymbolType type, const std::string& name, int sequence) {
    std::string id = symbol_type_to_string(type) + "_" + name;
    
    // 名前に特殊文字が含まれる場合は置換
    for (char& c : id) {
        if (!std::isalnum(c) && c != '_') {
            c = '_';
        }
    }
    
    // 連番が指定されていれば追加
    if (sequence > 0) {
        id += "_" + std::to_string(sequence);
    }
    
    return id;
}

inline std::string UniversalSymbolInfo::get_metadata(const std::string& key, const std::string& default_value) const {
    auto it = metadata.find(key);
    return (it != metadata.end()) ? it->second : default_value;
}

inline nlohmann::json UniversalSymbolInfo::to_json() const {
    nlohmann::json j;
    
    j["symbol_type"] = symbol_type_to_string(symbol_type);
    j["name"] = name;
    
    if (!qualified_name.empty()) {
        j["qualified_name"] = qualified_name;
    }
    
    j["symbol_id"] = symbol_id;
    
    if (!parent_id.empty()) {
        j["parent_id"] = parent_id;
    }
    
    if (start_line > 0) {
        j["start_line"] = start_line;
    }
    
    if (end_line > 0) {
        j["end_line"] = end_line;
    }
    
    if (!child_ids.empty()) {
        j["child_ids"] = child_ids;
    }
    
    if (!metadata.empty()) {
        j["metadata"] = metadata;
    }
    
    if (!parameters.empty()) {
        j["parameters"] = parameters;
    }
    
    // 複雑度情報があれば追加
    if (complexity.cyclomatic_complexity > 0 || 
        complexity.cognitive_complexity > 0 || 
        complexity.max_nesting_depth > 0) {
        j["complexity"] = {
            {"cyclomatic", complexity.cyclomatic_complexity},
            {"cognitive", complexity.cognitive_complexity},
            {"max_nesting", complexity.max_nesting_depth}
        };
    }
    
    return j;
}

inline size_t UniversalSymbolInfo::get_depth() const {
    // parent_idの数を数えて階層の深さを推定（簡易実装）
    if (parent_id.empty()) return 0;
    
    // より正確な実装はSymbolTableで管理
    return 1;  // 仮実装
}

} // namespace nekocode