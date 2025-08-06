//=============================================================================
// 🎯 Command Dispatcher実装 - コマンド実行分散処理
//=============================================================================

#include "nekocode/command_dispatcher.hpp"
#include "nekocode/command_line_args.hpp"
#include "nekocode/session_data.hpp"
#include "nekocode/session_commands.hpp"
#include <iostream>
#include <filesystem>

// 🧠 Memory System Integration
#ifdef NEKOCODE_USE_MEMORY_SYSTEM
#include "nekocode/memory_command.hpp"
#endif

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
    // 📝 直接編集コマンド（セッション不要）
    else if (action == "replace") {
        return dispatch_replace(argc, argv);
    }
    else if (action == "replace-preview") {
        return dispatch_replace_preview(argc, argv);
    }
    else if (action == "replace-confirm") {
        return dispatch_replace_confirm(argc, argv);
    }
    else if (action == "insert") {
        return dispatch_insert(argc, argv);
    }
    else if (action == "insert-preview") {
        return dispatch_insert_preview(argc, argv);
    }
    else if (action == "insert-confirm") {
        return dispatch_insert_confirm(argc, argv);
    }
    else if (action == "movelines") {
        return dispatch_movelines(argc, argv);
    }
    else if (action == "movelines-preview") {
        return dispatch_movelines_preview(argc, argv);
    }
    else if (action == "movelines-confirm") {
        return dispatch_movelines_confirm(argc, argv);
    }
#ifdef NEKOCODE_USE_MEMORY_SYSTEM
    else if (action == "memory") {
        return dispatch_memory(argc - 1, argv + 1);  // Skip "memory" and pass remaining args
    }
#endif
    
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
// 📝 直接編集コマンド実装（セッション不要）
//=============================================================================

int CommandDispatcher::dispatch_replace(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: replace requires file, pattern, and replacement" << std::endl;
        std::cerr << "Usage: nekocode replace <file> <pattern> <replacement>" << std::endl;
        return 1;
    }
    
    // 最小限のSessionDataを作成
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto result = commands.cmd_replace(minimal_session, argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_replace_preview(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: replace-preview requires file, pattern, and replacement" << std::endl;
        std::cerr << "Usage: nekocode replace-preview <file> <pattern> <replacement>" << std::endl;
        return 1;
    }
    
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto result = commands.cmd_replace_preview(minimal_session, argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_replace_confirm(int argc, char* argv[]) {
    // Phase 2で実装: プレビューIDまたは直接実行の両モード対応
    if (argc == 3) {
        // Mode 1: プレビューID指定
        SessionData minimal_session;
        minimal_session.session_id = "direct_exec";
        minimal_session.target_path = std::filesystem::current_path();
        minimal_session.is_directory = true;  // ディレクトリモードで動作
        
        SessionCommands commands;
        auto result = commands.cmd_replace_confirm(minimal_session, argv[2]);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } else if (argc >= 5) {
        // Mode 2: 直接実行（プレビューなし）
        return dispatch_replace(argc, argv);  // replaceと同じ処理
    } else {
        std::cerr << "Error: replace-confirm requires preview ID or file/pattern/replacement" << std::endl;
        std::cerr << "Usage: nekocode replace-confirm <preview_id>" << std::endl;
        std::cerr << "   or: nekocode replace-confirm <file> <pattern> <replacement>" << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_insert(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: insert requires file, position, and content" << std::endl;
        std::cerr << "Usage: nekocode insert <file> <position> <content>" << std::endl;
        return 1;
    }
    
    // 直接実行: プレビュー作成してすぐconfirm
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto preview = commands.cmd_insert_preview(minimal_session, argv[2], argv[3], argv[4]);
    
    if (preview.contains("preview_id")) {
        auto result = commands.cmd_insert_confirm(minimal_session, preview["preview_id"]);
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } else {
        std::cout << preview.dump(2) << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_insert_preview(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: insert-preview requires file, position, and content" << std::endl;
        std::cerr << "Usage: nekocode insert-preview <file> <position> <content>" << std::endl;
        return 1;
    }
    
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto result = commands.cmd_insert_preview(minimal_session, argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_insert_confirm(int argc, char* argv[]) {
    if (argc == 3) {
        // Mode 1: プレビューID指定
        SessionData minimal_session;
        minimal_session.session_id = "direct_exec";
        minimal_session.target_path = std::filesystem::current_path();
        minimal_session.is_directory = true;  // ディレクトリモードで動作
        
        SessionCommands commands;
        auto result = commands.cmd_insert_confirm(minimal_session, argv[2]);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } else if (argc >= 5) {
        // Mode 2: 直接実行
        return dispatch_insert(argc, argv);
    } else {
        std::cerr << "Error: insert-confirm requires preview ID or file/position/content" << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_movelines(int argc, char* argv[]) {
    if (argc < 7) {
        std::cerr << "Error: movelines requires source file, start line, count, dest file, and insert line" << std::endl;
        std::cerr << "Usage: nekocode movelines <srcfile> <start> <count> <dstfile> <position>" << std::endl;
        return 1;
    }
    
    // 直接実行: プレビュー作成してすぐconfirm
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto preview = commands.cmd_movelines_preview(minimal_session, argv[2], argv[3], argv[4], argv[5], argv[6]);
    
    if (preview.contains("preview_id")) {
        auto result = commands.cmd_movelines_confirm(minimal_session, preview["preview_id"]);
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } else {
        std::cout << preview.dump(2) << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_movelines_preview(int argc, char* argv[]) {
    if (argc < 7) {
        std::cerr << "Error: movelines-preview requires source file, start line, count, dest file, and insert line" << std::endl;
        std::cerr << "Usage: nekocode movelines-preview <srcfile> <start> <count> <dstfile> <position>" << std::endl;
        return 1;
    }
    
    SessionData minimal_session;
    minimal_session.session_id = "direct_exec";
    minimal_session.target_path = std::filesystem::current_path();
    minimal_session.is_directory = true;  // ディレクトリモードで動作
    
    SessionCommands commands;
    auto result = commands.cmd_movelines_preview(minimal_session, argv[2], argv[3], argv[4], argv[5], argv[6]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_movelines_confirm(int argc, char* argv[]) {
    if (argc == 3) {
        // Mode 1: プレビューID指定
        SessionData minimal_session;
        minimal_session.session_id = "direct_exec";
        minimal_session.target_path = std::filesystem::current_path();
        minimal_session.is_directory = true;  // ディレクトリモードで動作
        
        SessionCommands commands;
        auto result = commands.cmd_movelines_confirm(minimal_session, argv[2]);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } else if (argc >= 7) {
        // Mode 2: 直接実行
        return dispatch_movelines(argc, argv);
    } else {
        std::cerr << "Error: movelines-confirm requires preview ID or full parameters" << std::endl;
        return 1;
    }
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

//=============================================================================
// 🧠 Memory System Integration
//=============================================================================

#ifdef NEKOCODE_USE_MEMORY_SYSTEM
int CommandDispatcher::dispatch_memory(int argc, char* argv[]) {
    try {
        MemoryCommand memory_cmd;
        
        // Convert char* argv[] to vector<string>
        // Skip argv[0] which is "memory", start from argv[1] which is the actual command
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {  // Start from index 1, not 0
            args.push_back(std::string(argv[i]));
        }
        
        bool success = memory_cmd.execute(args);
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Memory System Error: " << e.what() << std::endl;
        return 1;
    }
}
#endif

} // namespace nekocode