//=============================================================================
// 🎯 Command Dispatcher実装 - コマンド実行分散処理
//=============================================================================

#include "nekocode/command_dispatcher.hpp"
#include "nekocode/command_line_args.hpp"
#include <iostream>
#include <filesystem>

// main_ai.cpp からの外部関数宣言（リファクタリング後に整理予定）
extern void show_help();
extern void show_supported_languages();
extern int analyze_target(const std::string& target_path, const nekocode::CommandLineArgs& args);
extern int create_session(const std::string& target_path, const nekocode::CommandLineArgs& args);
extern int check_session_status(const std::string& session_id);
extern int execute_session_command(const std::string& session_id, const std::string& command);

namespace nekocode {

//=============================================================================
// 🎯 メインディスパッチャー実装
//=============================================================================

int CommandDispatcher::dispatch(int argc, char* argv[]) {
    // 引数なしの場合はヘルプ表示
    if (argc < 2) {
        return dispatch_help();
    }
    
    std::string action = argv[1];
    
    // ヘルプ表示
    if (action == "-h" || action == "--help") {
        return dispatch_help();
    }
    
    // 言語一覧表示
    if (action == "languages") {
        return dispatch_languages();
    }
    
    // アクション別分岐
    if (action == "analyze") {
        if (argc < 3) {
            return handle_missing_argument("analyze", "target path");
        }
        return dispatch_analyze(argv[2], argc - 2, argv + 2);
    }
    else if (action == "session-create") {
        if (argc < 3) {
            return handle_missing_argument("session-create", "target path");
        }
        return dispatch_session_create(argv[2], argc - 2, argv + 2);
    }
    else if (action == "session-status") {
        if (argc < 3) {
            return handle_missing_argument("session-status", "session ID");
        }
        return dispatch_session_status(argv[2], argc - 2, argv + 2);
    }
    else if (action == "session-command") {
        if (argc < 4) {
            std::cerr << "Error: session-command requires session ID and command" << std::endl;
            std::cerr << "Usage: nekocode_ai session-command <session_id> <command>" << std::endl;
            return 1;
        }
        return dispatch_session_command(argv[2], argv[3]);
    }
    
    return handle_unknown_command(action);
}

//=============================================================================
// 🔍 個別コマンド処理実装
//=============================================================================

int CommandDispatcher::dispatch_analyze(const std::string& target_path, int argc, char* argv[]) {
    CommandLineArgs args = CommandLineArgs::parse(argc, argv);
    args.target_path = target_path;  // ターゲットパスを設定
    return analyze_target(target_path, args);
}

int CommandDispatcher::dispatch_session_create(const std::string& target_path, int argc, char* argv[]) {
    // session-create 専用引数解析
    CommandLineArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--progress") {
            args.enable_progress = true;
        } else if (arg == "--io-threads" && i + 1 < argc) {
            args.io_threads = std::stoul(argv[++i]);
        } else if (arg == "--cpu-threads" && i + 1 < argc) {
            args.cpu_threads = std::stoul(argv[++i]);
        } else if (arg == "--no-check") {
            args.skip_precheck = true;
        } else if (arg == "--force") {
            args.force_execution = true;
        } else if (arg == "--check-only") {
            args.check_only = true;
        }
    }
    return create_session(target_path, args);
}


int CommandDispatcher::dispatch_session_status(const std::string& session_id, int argc, char* argv[]) {
    return check_session_status(session_id);
}

int CommandDispatcher::dispatch_session_command(const std::string& session_id, const std::string& command) {
    return execute_session_command(session_id, command);
}

int CommandDispatcher::dispatch_languages() {
    show_supported_languages();
    return 0;
}

int CommandDispatcher::dispatch_help() {
    show_help();
    return 1;
}

//=============================================================================
// 🛠️ エラーハンドリング実装
//=============================================================================

int CommandDispatcher::handle_missing_argument(const std::string& command, const std::string& expected) {
    std::cerr << "Error: Missing " << expected << " for " << command << std::endl;
    std::cerr << "Usage: nekocode_ai " << command << " <" << expected << "> [options]" << std::endl;
    return 1;
}

int CommandDispatcher::handle_unknown_command(const std::string& command) {
    std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
    std::cerr << "Run 'nekocode_ai --help' for usage information." << std::endl;
    return 1;
}

} // namespace nekocode