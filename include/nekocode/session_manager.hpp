#pragma once

//=============================================================================
// 🎮 Session Manager - 対話式解析セッション管理
//
// 実行ファイル２個大作戦: AI専用対話式機能
//
// 特徴:
// - JSONセッション永続化
// - コマンド履歴管理
// - インクリメンタル解析
// - Claude Code最適化
//=============================================================================

#include "types.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <memory>

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
    
    // JSONシリアライズ
    nlohmann::json to_json() const;
    static SessionData from_json(const nlohmann::json& j);
};

//=============================================================================
// 🎮 Session Manager - セッション管理クラス
//=============================================================================

class SessionManager {
public:
    SessionManager();
    ~SessionManager();
    
    // セッション管理
    std::string create_session(const std::filesystem::path& target_path, 
                               const AnalysisResult& result);
    std::string create_session(const std::filesystem::path& target_path, 
                               const DirectoryAnalysis& result);
    
    // コマンド実行
    nlohmann::json execute_command(const std::string& session_id, 
                                    const std::string& command);
    
    // セッションアクセス
    bool session_exists(const std::string& session_id) const;
    std::vector<std::string> list_sessions() const;
    
    // プロジェクトファイル取得（find用）
    std::vector<FileInfo> getProjectFiles(const std::string& session_id);
    
private:
    std::filesystem::path sessions_dir_;
    
    // セッションファイル管理
    std::filesystem::path get_session_file(const std::string& session_id) const;
    SessionData load_session(const std::string& session_id) const;
    void save_session(const SessionData& session) const;
    
    // コマンド実行
    nlohmann::json cmd_stats(const SessionData& session) const;
    nlohmann::json cmd_files(const SessionData& session) const;
    nlohmann::json cmd_complexity(const SessionData& session) const;
    nlohmann::json cmd_structure(const SessionData& session) const;
    nlohmann::json cmd_calls(const SessionData& session) const;
    nlohmann::json cmd_find(const SessionData& session, const std::string& term) const;
    nlohmann::json cmd_find_symbols(const SessionData& session, 
                                    const std::string& symbol,
                                    const std::vector<std::string>& options,
                                    bool debug = false) const;
    nlohmann::json cmd_help() const;
    
    // Include解析コマンド
    nlohmann::json cmd_include_graph(const SessionData& session) const;
    nlohmann::json cmd_include_cycles(const SessionData& session) const;
    nlohmann::json cmd_include_impact(const SessionData& session) const;
    nlohmann::json cmd_include_unused(const SessionData& session) const;
    nlohmann::json cmd_include_optimize(const SessionData& session) const;
    nlohmann::json cmd_duplicates(const SessionData& session) const;
    nlohmann::json cmd_large_files(const SessionData& session, int threshold) const;
    nlohmann::json cmd_todo(const SessionData& session) const;
    nlohmann::json cmd_complexity_ranking(const SessionData& session) const;
    
    // ユーティリティ
    std::string generate_session_id() const;
    nlohmann::json extract_quick_stats(const AnalysisResult& result) const;
    nlohmann::json extract_quick_stats(const DirectoryAnalysis& result) const;
};

//=============================================================================
// 🛠️ Helper Functions
//=============================================================================

// タイムスタンプ変換
std::string timestamp_to_string(const Timestamp& ts);
Timestamp string_to_timestamp(const std::string& str);

} // namespace nekocode