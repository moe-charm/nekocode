#pragma once

//=============================================================================
// 🐱 Edit Commands - NekoCode独自編集機能
//
// SessionCommandsから分離した編集専用コマンド群
// 責任: ファイル編集、置換、挿入、行移動等の実装
//=============================================================================

#include "session_data.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace nekocode {

//=============================================================================
// 🐱 Edit Commands - NekoCode独自編集機能クラス
//=============================================================================

class EditCommands {
public:
    EditCommands() = default;
    ~EditCommands() = default;

    //=========================================================================
    // 🐱 NekoCode独自編集機能
    //=========================================================================
    
    /// シンプル正規表現置換
    nlohmann::json cmd_replace(const SessionData& session,
                              const std::string& file_path,
                              const std::string& pattern,
                              const std::string& replacement) const;
    
    /// 置換プレビュー（軽量応答）
    nlohmann::json cmd_replace_preview(const SessionData& session,
                                      const std::string& file_path,
                                      const std::string& pattern,
                                      const std::string& replacement) const;
    
    /// 置換実行確定（プレビューID使用）
    nlohmann::json cmd_replace_confirm(const SessionData& session,
                                      const std::string& preview_id) const;
    
    /// 編集履歴一覧
    nlohmann::json cmd_edit_history(const SessionData& session) const;
    
    /// 編集詳細表示（preview_idまたはedit_id）
    nlohmann::json cmd_edit_show(const SessionData& session,
                                const std::string& id) const;
    
    /// 統一挿入プレビュー（start/end/行番号/パターン対応）
    nlohmann::json cmd_insert_preview(const SessionData& session,
                                     const std::string& file_path,
                                     const std::string& position,
                                     const std::string& content) const;
    
    /// 挿入実行確定（プレビューID使用）
    nlohmann::json cmd_insert_confirm(const SessionData& session,
                                     const std::string& preview_id) const;
    
    /// 行移動プレビュー（srcfile 開始行 行数 dstfile 挿入行）
    nlohmann::json cmd_movelines_preview(const SessionData& session,
                                        const std::string& srcfile,
                                        const std::string& start_line,
                                        const std::string& line_count,
                                        const std::string& dstfile,
                                        const std::string& insert_line) const;
    
    /// 行移動実行確定（プレビューID使用）
    nlohmann::json cmd_movelines_confirm(const SessionData& session,
                                        const std::string& preview_id) const;
};

} // namespace nekocode