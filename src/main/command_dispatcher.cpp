//=============================================================================
// 🎯 Command Dispatcher実装 - コマンド実行分散処理
//=============================================================================

#include "nekocode/command_dispatcher.hpp"
#include "nekocode/command_line_args.hpp"
#include "nekocode/session_data.hpp"
#include "nekocode/session_commands.hpp"
#include <iostream>
#include <filesystem>

// 🔥 Direct Edit機能（セッション不要）
#include "../core/commands/direct_edit/direct_edit_common.hpp"

// 📦 MoveClass Handler
#include "nekocode/commands/moveclass_handler.hpp"

// 🔧 Config Manager
#include "../core/config_manager.hpp"

// Direct Mode関数の前方宣言
namespace nekocode {
namespace DirectEdit {
    nlohmann::json replace_preview(const std::string& file_path, const std::string& pattern, const std::string& replacement);
    nlohmann::json replace_confirm(const std::string& preview_id);
    nlohmann::json replace_direct(const std::string& file_path, const std::string& pattern, const std::string& replacement);
    
    nlohmann::json insert_preview(const std::string& file_path, const std::string& position, const std::string& content);
    nlohmann::json insert_confirm(const std::string& preview_id);
    nlohmann::json insert_direct(const std::string& file_path, const std::string& position, const std::string& content);
    
    nlohmann::json movelines_preview(const std::string& srcfile, int start_line, int line_count, const std::string& dstfile, int insert_line);
    nlohmann::json movelines_confirm(const std::string& preview_id);
    nlohmann::json movelines_direct(const std::string& srcfile, int start_line, int line_count, const std::string& dstfile, int insert_line);
}
}

using namespace nekocode::DirectEdit;

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
        // コマンドと引数を結合（引用符が必要な場合は追加）
        std::string full_command = argv[3];
        for (int i = 4; i < argc; i++) {
            full_command += " ";
            // スペースを含む引数は引用符で囲む
            std::string arg = argv[i];
            if (arg.find(' ') != std::string::npos && arg[0] != '"') {
                full_command += "\"" + arg + "\"";
            } else {
                full_command += arg;
            }
        }
        return dispatch_session_command(argv[2], full_command);
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
    else if (action == "moveclass") {
        return dispatch_moveclass(argc, argv);
    }
    else if (action == "moveclass-preview") {
        return dispatch_moveclass_preview(argc, argv);
    }
    else if (action == "moveclass-confirm") {
        return dispatch_moveclass_confirm(argc, argv);
    }
    // 🔧 設定管理コマンド
    else if (action == "config") {
        return dispatch_config(argc, argv);
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
    
    // 🚀 Direct Mode - セッション不要の直接置換
    auto result = replace_direct(argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_replace_preview(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: replace-preview requires file, pattern, and replacement" << std::endl;
        std::cerr << "Usage: nekocode replace-preview <file> <pattern> <replacement>" << std::endl;
        return 1;
    }
    
    // 🔄 Direct Mode - セッション不要の置換プレビュー
    auto result = replace_preview(argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_replace_confirm(int argc, char* argv[]) {
    if (argc == 3) {
        // Mode 1: プレビューID指定で確定実行
        auto result = replace_confirm(argv[2]);
        
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
    
    // 🚀 Direct Mode - セッション不要の直接挿入
    auto result = insert_direct(argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_insert_preview(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: insert-preview requires file, position, and content" << std::endl;
        std::cerr << "Usage: nekocode insert-preview <file> <position> <content>" << std::endl;
        return 1;
    }
    
    // 📥 Direct Mode - セッション不要の挿入プレビュー
    auto result = insert_preview(argv[2], argv[3], argv[4]);
    
    std::cout << result.dump(2) << std::endl;
    return result.contains("error") ? 1 : 0;
}

int CommandDispatcher::dispatch_insert_confirm(int argc, char* argv[]) {
    if (argc == 3) {
        // Mode 1: プレビューID指定で確定実行
        auto result = insert_confirm(argv[2]);
        
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
    
    // 🔄 Direct Mode - セッション不要の直接行移動
    try {
        int start_line = std::stoi(argv[3]);
        int line_count = std::stoi(argv[4]);
        int insert_line = std::stoi(argv[6]);
        
        auto result = movelines_direct(argv[2], start_line, line_count, argv[5], insert_line);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing numeric arguments: " << e.what() << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_movelines_preview(int argc, char* argv[]) {
    if (argc < 7) {
        std::cerr << "Error: movelines-preview requires source file, start line, count, dest file, and insert line" << std::endl;
        std::cerr << "Usage: nekocode movelines-preview <srcfile> <start> <count> <dstfile> <position>" << std::endl;
        return 1;
    }
    
    // 🔄 Direct Mode - セッション不要の行移動プレビュー
    try {
        int start_line = std::stoi(argv[3]);
        int line_count = std::stoi(argv[4]);
        int insert_line = std::stoi(argv[6]);
        
        auto result = movelines_preview(argv[2], start_line, line_count, argv[5], insert_line);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing numeric arguments: " << e.what() << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_movelines_confirm(int argc, char* argv[]) {
    if (argc == 3) {
        // Mode 1: プレビューID指定で確定実行
        auto result = movelines_confirm(argv[2]);
        
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
// 📦 MoveClass コマンド実装
//=============================================================================

int CommandDispatcher::dispatch_moveclass(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: moveclass requires session_id, symbol_id, and target_file" << std::endl;
        std::cerr << "Usage: nekocode_ai moveclass <session_id> <symbol_id> <target_file>" << std::endl;
        return 1;
    }
    
    // セッションベースのクラス移動
    try {
        std::string session_id = argv[2];
        std::string symbol_id = argv[3];
        std::string target_file = argv[4];
        
        MoveClassHandler handler;
        auto result = handler.execute(session_id, symbol_id, target_file);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error in moveclass: " << e.what() << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_moveclass_preview(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Error: moveclass-preview requires session_id, symbol_id, and target_file" << std::endl;
        std::cerr << "Usage: nekocode_ai moveclass-preview <session_id> <symbol_id> <target_file>" << std::endl;
        return 1;
    }
    
    try {
        std::string session_id = argv[2];
        std::string symbol_id = argv[3];
        std::string target_file = argv[4];
        
        MoveClassHandler handler;
        auto result = handler.preview(session_id, symbol_id, target_file);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error in moveclass-preview: " << e.what() << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_moveclass_confirm(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: moveclass-confirm requires preview_id" << std::endl;
        std::cerr << "Usage: nekocode_ai moveclass-confirm <preview_id>" << std::endl;
        return 1;
    }
    
    try {
        std::string preview_id = argv[2];
        
        MoveClassHandler handler;
        auto result = handler.confirm(preview_id);
        
        std::cout << result.dump(2) << std::endl;
        return result.contains("error") ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error in moveclass-confirm: " << e.what() << std::endl;
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
// 🔧 Config System Implementation
//=============================================================================

int CommandDispatcher::dispatch_config(int argc, char* argv[]) {
    try {
        // configコマンドのみの場合はshow
        if (argc == 2) {
            return dispatch_config_show();
        }
        
        std::string subcommand = argv[2];
        
        if (subcommand == "show") {
            return dispatch_config_show();
        }
        else if (subcommand == "set") {
            if (argc < 5) {
                std::cerr << "Error: config set requires key and value" << std::endl;
                std::cerr << "Usage: nekocode_ai config set <key> <value>" << std::endl;
                std::cerr << "Example: nekocode_ai config set memory.edit_history.max_size_mb 20" << std::endl;
                return 1;
            }
            return dispatch_config_set(argv[3], argv[4]);
        }
        else if (subcommand == "help" || subcommand == "--help") {
            std::cout << "🔧 Config Management" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  config show              Show current configuration" << std::endl;
            std::cout << "  config set <key> <value> Set configuration value" << std::endl;
            std::cout << std::endl;
            std::cout << "Available keys:" << std::endl;
            std::cout << "  memory.edit_history.max_size_mb    Max size for edit history (MB)" << std::endl;
            std::cout << "  memory.edit_history.min_files_keep Minimum files to keep" << std::endl;
            std::cout << "  memory.edit_previews.max_size_mb   Max size for preview files (MB)" << std::endl;
            std::cout << "  performance.default_io_threads      Default I/O thread count" << std::endl;
            std::cout << "  performance.storage_type            Storage type (ssd/hdd/auto)" << std::endl;
            std::cout << std::endl;
            std::cout << "Config file location: bin/nekocode_config.json" << std::endl;
            return 0;
        }
        else {
            std::cerr << "Error: Unknown config subcommand '" << subcommand << "'" << std::endl;
            std::cerr << "Use 'nekocode_ai config help' for usage information" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }
}

int CommandDispatcher::dispatch_config_show() {
    auto& config = ConfigManager::instance();
    
    std::cout << config.to_string();
    
    // 現在の使用状況も表示
    auto stats = DirectEdit::get_edit_history_stats();
    std::cout << "\n📊 Current Usage:\n";
    std::cout << "  Edit History:  " << stats.history_size_bytes / 1024.0 / 1024.0 
              << " MB (" << stats.history_files << " files)\n";
    std::cout << "  Preview Files: " << stats.preview_size_bytes / 1024.0 / 1024.0 
              << " MB (" << stats.preview_files << " files)\n";
    
    return 0;
}

int CommandDispatcher::dispatch_config_set(const std::string& key, const std::string& value) {
    auto& config = ConfigManager::instance();
    
    if (config.set_value(key, value)) {
        config.save_to_file();
        std::cout << "✅ Configuration updated:" << std::endl;
        std::cout << "   " << key << " = " << value << std::endl;
        return 0;
    } else {
        std::cerr << "❌ Invalid configuration key or value" << std::endl;
        std::cerr << "Use 'nekocode_ai config help' for available keys" << std::endl;
        return 1;
    }
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