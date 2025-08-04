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
#include "session_data.hpp"
#include "session_commands.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <memory>

namespace nekocode {

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
    SessionCommands session_commands_;
    
    // セッションファイル管理
    std::filesystem::path get_session_file(const std::string& session_id) const;
    SessionData load_session(const std::string& session_id) const;
    void save_session(const SessionData& session) const;
    
    // SessionCommandsにコマンド処理を委譲
    
    // analyze用ヘルパー関数
    nlohmann::json analyze_file(const AnalysisResult& file, bool deep) const;
    nlohmann::json analyze_directory(const DirectoryAnalysis& dir_result, bool deep) const;
    
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