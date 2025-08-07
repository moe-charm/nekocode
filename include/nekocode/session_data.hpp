#pragma once

//=============================================================================
// 📱 Session Data - セッション情報＆JSONシリアライゼーション
//
// SessionManagerから分離したデータクラス
// 責任: セッションデータの構造定義とJSONシリアライゼーション
//=============================================================================

#include "types.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>

namespace nekocode {

//=============================================================================
// 📱 Session Data - セッション情報
//=============================================================================

struct SessionData {
    std::string session_id;
    std::string session_type = "ai_optimized";
    Timestamp created_at;
    std::filesystem::path target_path;
    
    // 解析データ
    AnalysisResult single_file_result;        // 単一ファイルの場合
    DirectoryAnalysis directory_result;       // ディレクトリの場合
    bool is_directory = false;
    
    // セッション管理
    struct CommandHistory {
        std::string command;
        Timestamp timestamp;
        std::string result_type;
    };
    std::vector<CommandHistory> command_history;
    
    // クイック統計（高速アクセス用）
    nlohmann::json quick_stats;
    
    // デッドコード解析結果（完全解析時のみ）
    nlohmann::json dead_code_info;
    
    // Phase 3: Universal Symbol情報（Rustのみ対応）
    std::shared_ptr<class SymbolTable> universal_symbols;
    
    // Rustセッション作成時に自動的にSymbolTableを生成
    void enhance_with_symbols();
    
    // JSONシリアライズ
    nlohmann::json to_json() const;
    static SessionData from_json(const nlohmann::json& j);
};

//=============================================================================
// 🕒 時刻変換ユーティリティ
//=============================================================================

/// Timestamp を文字列に変換
std::string timestamp_to_string(const Timestamp& timestamp);

/// 文字列を Timestamp に変換
Timestamp string_to_timestamp(const std::string& str);

} // namespace nekocode