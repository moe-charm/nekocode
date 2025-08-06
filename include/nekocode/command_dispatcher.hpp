#pragma once

//=============================================================================
// 🎯 Command Dispatcher - コマンド実行分散処理
//
// main関数の巨大な分岐処理を分離したコマンド実行責任クラス
// 責任: コマンドラインアクションの適切な処理関数への振り分け
//=============================================================================

#include "types.hpp"
#include <string>
#include <vector>

namespace nekocode {

//=============================================================================
// 🎯 Command Dispatcher - メインコマンド分散処理クラス
//=============================================================================

class CommandDispatcher {
public:
    CommandDispatcher() = default;
    ~CommandDispatcher() = default;

    //=========================================================================
    // 🎯 メインディスパッチャー
    //=========================================================================
    
    /// コマンドライン全体の処理を分散実行
    int dispatch(int argc, char* argv[]);

private:
    //=========================================================================
    // 🔍 各コマンド処理
    //=========================================================================
    
    /// analyze コマンド処理
    int dispatch_analyze(const std::string& target_path, int argc, char* argv[]);
    
    /// session-create コマンド処理
    int dispatch_session_create(const std::string& target_path, int argc, char* argv[]);
    
    
    /// session-status コマンド処理
    int dispatch_session_status(const std::string& session_id, int argc, char* argv[]);
    
    /// session-command コマンド処理
    int dispatch_session_command(const std::string& session_id, const std::string& command);
    
    /// languages コマンド処理
    int dispatch_languages();
    
    /// ヘルプ表示
    int dispatch_help();
    
    //=========================================================================
    // 📝 直接編集コマンド（セッション不要）
    //=========================================================================
    
    /// replace コマンド処理（直接実行）
    int dispatch_replace(int argc, char* argv[]);
    
    /// replace-preview コマンド処理
    int dispatch_replace_preview(int argc, char* argv[]);
    
    /// replace-confirm コマンド処理
    int dispatch_replace_confirm(int argc, char* argv[]);
    
    /// insert コマンド処理（直接実行）
    int dispatch_insert(int argc, char* argv[]);
    
    /// insert-preview コマンド処理
    int dispatch_insert_preview(int argc, char* argv[]);
    
    /// insert-confirm コマンド処理
    int dispatch_insert_confirm(int argc, char* argv[]);
    
    /// movelines コマンド処理（直接実行）
    int dispatch_movelines(int argc, char* argv[]);
    
    /// movelines-preview コマンド処理
    int dispatch_movelines_preview(int argc, char* argv[]);
    
    /// movelines-confirm コマンド処理
    int dispatch_movelines_confirm(int argc, char* argv[]);

#ifdef NEKOCODE_USE_MEMORY_SYSTEM
    /// memory コマンド処理（Memory System統合）
    int dispatch_memory(int argc, char* argv[]);
#endif

    //=========================================================================
    // 🛠️ 内部ヘルパー
    //=========================================================================
    
    /// 引数が不足している場合のエラー処理
    int handle_missing_argument(const std::string& command, const std::string& expected);
    
    /// 不明なコマンドのエラー処理
    int handle_unknown_command(const std::string& command);
};

} // namespace nekocode