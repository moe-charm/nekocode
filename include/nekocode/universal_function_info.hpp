//=============================================================================
// 🌟 UniversalFunctionInfo - 全言語統一関数情報クラス
// 
// Universal AST Revolution の中核コンポーネント
// 全8言語（JS/TS/C++/C/Python/C#/Go/Rust）で統一された関数情報形式
//
// 設計原則：
// ✅ 高速・軽量（構文解析ベース）
// ✅ 後方互換性（既存FunctionInfoと100%互換）
// ✅ 拡張可能（新言語・新フィールド対応）
// ✅ JSON直接変換（session_data.cppで使用）
// 
// Author: Claude + User collaborative design
// Date: 2025-08-06
//=============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "nekocode/types.hpp"  // LineNumber, ComplexityInfo

//=============================================================================
// 🌟 UniversalFunctionInfo - 全言語統一関数情報
//=============================================================================

/**
 * @brief 全プログラミング言語で統一された関数情報クラス
 * 
 * 軽量で高速な構文解析ベースの関数情報を格納。
 * 既存のFunctionInfoと100%互換性があり、段階的移行が可能。
 * 
 * 対応言語: JavaScript, TypeScript, C++, C, Python, C#, Go, Rust
 * 
 * @note デッドコード解析などの重い処理は含めない（後から合成）
 */
struct UniversalFunctionInfo {
    //=========================================================================
    // 🏗️ 基本情報（全言語共通・必須）
    //=========================================================================
    
    /// 関数名
    std::string name;
    
    /// 開始行番号（1-based）
    LineNumber start_line = 0;
    
    /// 終了行番号（1-based、0=未計算）
    LineNumber end_line = 0;
    
    /// 複雑度情報（循環的複雑度、認知的複雑度など）
    ComplexityInfo complexity;
    
    /// パラメータリスト（型なし、名前のみ）
    std::vector<std::string> parameters;
    
    //=========================================================================
    // 🎯 言語共通フラグ（軽量な構文情報）
    //=========================================================================
    
    /// 非同期関数（JavaScript/TypeScript/C#/Rust）
    bool is_async = false;
    
    /// アロー関数（JavaScript/TypeScript）
    bool is_arrow_function = false;
    
    //=========================================================================
    // 🧩 拡張用メタデータ（将来対応・カスタム情報）
    //=========================================================================
    
    /**
     * @brief 拡張用メタデータマップ
     * 
     * 軽量な文字列情報のみ格納。言語固有情報や検出モード等に使用。
     * 
     * 例：
     * - "pattern_type": "arrow_function" | "class_method" | "standalone"
     * - "detection_mode": "ast_based" | "line_based"
     * - "access_modifier": "public" | "private" | "protected"
     * - "is_static": "true" | "false"
     * - "is_generator": "true" | "false" (Python/JavaScript)
     * - "is_unsafe": "true" | "false" (Rust)
     * - "is_virtual": "true" | "false" (C++)
     */
    std::unordered_map<std::string, std::string> metadata;
    
    //=========================================================================
    // 🏗️ コンストラクタ（後方互換性）
    //=========================================================================
    
    /// デフォルトコンストラクタ
    UniversalFunctionInfo() = default;
    
    /// 関数名指定コンストラクタ（既存FunctionInfoと互換）
    explicit UniversalFunctionInfo(const std::string& func_name) 
        : name(func_name) {}
    
    /// 完全指定コンストラクタ
    UniversalFunctionInfo(const std::string& func_name, 
                         LineNumber start, 
                         LineNumber end = 0)
        : name(func_name), start_line(start), end_line(end) {}
    
    //=========================================================================
    // 🚀 言語別ファクトリーメソッド（将来拡張用）
    //=========================================================================
    
    /// JavaScript用初期化
    static UniversalFunctionInfo create_for_javascript(const std::string& name, 
                                                      LineNumber line,
                                                      bool is_async = false,
                                                      bool is_arrow = false) {
        UniversalFunctionInfo info(name, line);
        info.is_async = is_async;
        info.is_arrow_function = is_arrow;
        info.metadata["detection_mode"] = "line_based";
        return info;
    }
    
    /// Python用初期化  
    static UniversalFunctionInfo create_for_python(const std::string& name,
                                                  LineNumber line,
                                                  bool is_generator = false) {
        UniversalFunctionInfo info(name, line);
        if (is_generator) {
            info.metadata["is_generator"] = "true";
        }
        info.metadata["detection_mode"] = "line_based";
        return info;
    }
    
    /// C++用初期化
    static UniversalFunctionInfo create_for_cpp(const std::string& name,
                                               LineNumber start,
                                               LineNumber end,
                                               bool is_virtual = false,
                                               bool is_static = false) {
        UniversalFunctionInfo info(name, start, end);
        if (is_virtual) info.metadata["is_virtual"] = "true";
        if (is_static) info.metadata["is_static"] = "true";
        info.metadata["detection_mode"] = "ast_based";
        return info;
    }
    
    //=========================================================================
    // 📊 JSON変換（session_data.cpp用）
    //=========================================================================
    
    /**
     * @brief JSON形式に変換
     * 
     * session_data.cppで使用。既存のFunctionInfo JSON形式と完全互換。
     * 
     * @return nlohmann::json JSON オブジェクト
     */
    nlohmann::json to_json() const {
        nlohmann::json j;
        
        // 基本情報
        j["name"] = name;
        j["start_line"] = start_line;
        j["end_line"] = end_line;
        j["complexity"] = complexity.cyclomatic_complexity;
        j["parameters"] = parameters;
        
        // フラグ情報
        j["is_async"] = is_async;
        j["is_arrow_function"] = is_arrow_function;
        
        // 🆕 メタデータも保存（既存のバグを修正）
        j["metadata"] = metadata;
        
        return j;
    }
    
    /**
     * @brief JSONから復元
     * 
     * @param j JSON オブジェクト
     * @return UniversalFunctionInfo 復元された関数情報
     */
    static UniversalFunctionInfo from_json(const nlohmann::json& j) {
        UniversalFunctionInfo info;
        
        // 必須フィールド
        if (j.contains("name")) {
            info.name = j["name"].get<std::string>();
        }
        if (j.contains("start_line")) {
            info.start_line = j["start_line"].get<LineNumber>();
        }
        if (j.contains("end_line")) {
            info.end_line = j["end_line"].get<LineNumber>();
        }
        
        // 複雑度（オブジェクトまたは数値）
        if (j.contains("complexity")) {
            if (j["complexity"].is_number()) {
                info.complexity.cyclomatic_complexity = j["complexity"].get<int>();
                info.complexity.update_rating();
            } else if (j["complexity"].is_object()) {
                // 完全なComplexityInfoオブジェクト
                auto comp_json = j["complexity"];
                if (comp_json.contains("cyclomatic_complexity")) {
                    info.complexity.cyclomatic_complexity = 
                        comp_json["cyclomatic_complexity"].get<int>();
                }
                if (comp_json.contains("cognitive_complexity")) {
                    info.complexity.cognitive_complexity = 
                        comp_json["cognitive_complexity"].get<int>();
                }
                if (comp_json.contains("max_nesting_depth")) {
                    info.complexity.max_nesting_depth = 
                        comp_json["max_nesting_depth"].get<int>();
                }
                info.complexity.update_rating();
            }
        }
        
        // オプションフィールド
        if (j.contains("parameters") && j["parameters"].is_array()) {
            info.parameters = j["parameters"].get<std::vector<std::string>>();
        }
        if (j.contains("is_async")) {
            info.is_async = j["is_async"].get<bool>();
        }
        if (j.contains("is_arrow_function")) {
            info.is_arrow_function = j["is_arrow_function"].get<bool>();
        }
        if (j.contains("metadata") && j["metadata"].is_object()) {
            info.metadata = j["metadata"].get<std::unordered_map<std::string, std::string>>();
        }
        
        return info;
    }
    
    //=========================================================================
    // 🔍 ユーティリティメソッド
    //=========================================================================
    
    /**
     * @brief 有効性チェック
     * 
     * 最低限の有効な関数情報を持っているかチェック。
     * 
     * @return bool 有効な場合true
     */
    bool is_valid() const {
        return !name.empty() && start_line > 0;
    }
    
    /**
     * @brief 関数の行数を取得
     * 
     * @return LineNumber 行数（end_line未設定の場合は0）
     */
    LineNumber get_line_count() const {
        if (end_line > 0 && end_line >= start_line) {
            return end_line - start_line + 1;
        }
        return 0;
    }
    
    /**
     * @brief メタデータから値を安全に取得
     * 
     * @param key キー名
     * @param default_value デフォルト値
     * @return std::string 値（存在しない場合はデフォルト値）
     */
    std::string get_metadata(const std::string& key, 
                           const std::string& default_value = "") const {
        auto it = metadata.find(key);
        return (it != metadata.end()) ? it->second : default_value;
    }
    
    /**
     * @brief メタデータからbool値を取得
     * 
     * @param key キー名
     * @param default_value デフォルト値
     * @return bool 値（"true"の場合true、その他false）
     */
    bool get_metadata_bool(const std::string& key, bool default_value = false) const {
        std::string value = get_metadata(key);
        if (value.empty()) return default_value;
        return value == "true";
    }
    
    //=========================================================================
    // 🎯 言語固有プロパティアクセサー（便利メソッド）
    //=========================================================================
    
    /// 静的メソッドかどうか（C++/C#/Java等）
    bool is_static() const {
        return get_metadata_bool("is_static");
    }
    
    /// 仮想関数かどうか（C++）
    bool is_virtual() const {
        return get_metadata_bool("is_virtual");
    }
    
    /// ジェネレータ関数かどうか（Python/JavaScript）
    bool is_generator() const {
        return get_metadata_bool("is_generator");
    }
    
    /// アクセス修飾子を取得（デフォルト: "public"）
    std::string get_access_modifier() const {
        return get_metadata("access_modifier", "public");
    }
    
    /// 検出モードを取得（"ast_based" or "line_based"）
    std::string get_detection_mode() const {
        return get_metadata("detection_mode", "line_based");
    }
};