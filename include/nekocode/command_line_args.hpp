#pragma once

//=============================================================================
// 🎯 Command Line Arguments - コマンドライン引数構造体
//
// main_ai.cppとcommand_dispatcher.cppで共有される引数構造体
// 責任: コマンドライン引数の解析と保持
//=============================================================================

#include <string>
#include <cstdint>

namespace nekocode {

//=============================================================================
// 🎯 Command Line Arguments - コマンドライン引数構造体
//=============================================================================

struct CommandLineArgs {
    std::string target_path;
    std::string output_format = "json";
    std::string language = "auto";          // 言語指定
    bool show_help = false;
    bool compact_mode = false;
    bool stats_only = false;
    bool enable_parallel = true;
    uint32_t io_threads = 4;                // 🆕 同時ファイル読み込み数（デフォルト: 4）
    uint32_t cpu_threads = 0;               // 🆕 解析スレッド数（0 = 自動）
    bool show_performance = false;
    bool list_languages = false;           // サポート言語一覧
    bool enable_progress = false;           // プログレス表示
    bool debug_mode = false;                // --debug: デバッグログ表示モード
    
    // 事前チェック関連
    bool skip_precheck = false;             // --no-check: 事前チェックスキップ
    bool force_execution = false;           // --force: 確認なしで強制実行
    bool check_only = false;                // --check-only: チェックのみ実行
    
    /// コマンドライン引数を解析
    static CommandLineArgs parse(int argc, char* argv[]);
};

/// CommandDispatcher用のラッパー関数
CommandLineArgs parse_args(int argc, char* argv[]);

} // namespace nekocode