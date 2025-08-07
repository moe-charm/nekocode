//=============================================================================
// 🎮 Session Manager実装 - 対話式解析セッション管理
//=============================================================================

#include "nekocode/session_manager.hpp"
#include "nekocode/session_data.hpp"
#include "nekocode/session_commands.hpp"
#include "nekocode/include_analyzer.hpp"
#include "nekocode/symbol_finder.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <iostream>
#include <algorithm>

namespace nekocode {

//=============================================================================
// 🎮 SessionManager実装
//=============================================================================

SessionManager::SessionManager() {
    sessions_dir_ = std::filesystem::current_path() / "sessions";
    std::filesystem::create_directories(sessions_dir_);
}

SessionManager::~SessionManager() = default;

std::string SessionManager::create_session(const std::filesystem::path& target_path, 
                                           const AnalysisResult& result) {
    SessionData session;
    session.session_id = generate_session_id();
    session.created_at = std::chrono::system_clock::now();
    session.target_path = std::filesystem::absolute(target_path);  // 🔧 絶対パスに変換
    session.is_directory = false;
    session.single_file_result = result;
    session.quick_stats = extract_quick_stats(result);
    
    // Phase 3: Rust言語の場合はUniversal Symbolを生成
    session.enhance_with_symbols();
    
    save_session(session);
    
    return session.session_id;
}

std::string SessionManager::create_session(const std::filesystem::path& target_path, 
                                           const DirectoryAnalysis& result) {
    SessionData session;
    session.session_id = generate_session_id();
    session.created_at = std::chrono::system_clock::now();
    session.target_path = std::filesystem::absolute(target_path);  // 🔧 絶対パスに変換
    session.is_directory = true;
    session.directory_result = result;
    session.quick_stats = extract_quick_stats(result);
    
    save_session(session);
    
    return session.session_id;
}

nlohmann::json SessionManager::execute_command(const std::string& session_id, 
                                                const std::string& command) {
    try {
        if (!session_exists(session_id)) {
            return {{"error", "Session not found: " + session_id}};
        }
        
        SessionData session = load_session(session_id);
        nlohmann::json result;
        
        // コマンド解析・実行
        if (command == "stats") {
            result = session_commands_.cmd_stats(session);
        } else if (command == "files") {
            result = session_commands_.cmd_files(session);
        } else if (command == "complexity") {
            result = session_commands_.cmd_complexity(session);
        } else if (command.substr(0, 11) == "complexity ") {
            // complexity --methods <file> コマンドのパース
            std::string args = command.substr(11);
            
            // トークン分割
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {  
                tokens.push_back(current_token);
            }
            
            // オプション解析
            bool methods_flag = false;
            std::string filename;
            
            for (const auto& token : tokens) {
                if (token == "--methods") {
                    methods_flag = true;
                } else if (token.size() > 0 && token[0] != '-') {
                    filename = token;
                }
            }
            
            if (methods_flag) {
                result = session_commands_.cmd_complexity_methods(session, filename);
            } else {
                result = {{"error", "complexity: 使用法: complexity --methods [filename]"}};
            }
        } else if (command == "structure") {
            result = session_commands_.cmd_structure(session);
        } else if (command.substr(0, 10) == "structure ") {
            // structure --detailed <file> コマンドのパース
            std::string args = command.substr(10);
            
            // トークン分割（findコマンドと同じロジック）
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            // オプション解析
            bool detailed = false;
            std::string filename;
            
            for (const auto& token : tokens) {
                if (token == "--detailed") {
                    detailed = true;
                } else if (token.size() > 0 && token[0] != '-') {
                    filename = token;
                }
            }
            
            if (detailed && !filename.empty()) {
                result = session_commands_.cmd_structure_detailed(session, filename);
            } else if (detailed) {
                result = session_commands_.cmd_structure_detailed(session, "");  // 全ファイル
            } else {
                result = {{"error", "structure: 使用法: structure --detailed [filename]"}};
            }
        } else if (command == "calls") {
            result = session_commands_.cmd_calls(session);
        } else if (command.substr(0, 6) == "calls ") {
            // calls --detailed <function> コマンドのパース
            std::string args = command.substr(6);
            
            // トークン分割
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            // オプション解析
            bool detailed_flag = false;
            std::string function_name;
            
            for (const auto& token : tokens) {
                if (token == "--detailed") {
                    detailed_flag = true;
                } else if (token.size() > 0 && token[0] != '-') {
                    function_name = token;
                }
            }
            
            if (detailed_flag && !function_name.empty()) {
                result = session_commands_.cmd_calls_detailed(session, function_name);
            } else if (detailed_flag) {
                result = {{"error", "calls: 使用法: calls --detailed <function_name>"}};
            } else {
                result = {{"error", "calls: 使用法: calls --detailed <function_name>"}};
            }
        } else if (command.substr(0, 5) == "find ") {
            // find コマンドのパース
            std::string args = command.substr(5);
            
            // 事前にdebugフラグをチェック（早期判定）
            bool early_debug = args.find("--debug") != std::string::npos;
            
            if (early_debug) {
                std::cerr << "[DEBUG] find command received: " << command << std::endl;
                std::cerr << "[DEBUG] args after 'find ': " << args << std::endl;
            }
            
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            // 簡易的なトークン分割
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            if (early_debug) {
                std::cerr << "[DEBUG] tokens parsed: " << tokens.size() << " tokens" << std::endl;
                for (size_t i = 0; i < tokens.size(); ++i) {
                    std::cerr << "[DEBUG]   token[" << i << "]: '" << tokens[i] << "'" << std::endl;
                }
            }
            
            if (tokens.empty()) {
                result = {{"error", "find: シンボル名を指定してください"}};
            } else {
                std::string symbol = tokens[0];
                std::vector<std::string> options(tokens.begin() + 1, tokens.end());
                
                if (early_debug) {
                    std::cerr << "[DEBUG] symbol: '" << symbol << "'" << std::endl;
                    std::cerr << "[DEBUG] options: " << options.size() << " options" << std::endl;
                    for (const auto& opt : options) {
                        std::cerr << "[DEBUG]   option: '" << opt << "'" << std::endl;
                    }
                }
                
                // debugフラグをチェック
                bool debug_mode = false;
                for (const auto& opt : options) {
                    if (opt == "--debug") {
                        debug_mode = true;
                        break;
                    }
                }
                
                // オプションをチェックして、シンボル検索か通常検索か判定
                bool is_symbol_search = false;
                for (const auto& opt : options) {
                    if (opt == "-f" || opt == "-v" || opt == "-a" || 
                        opt == "--function" || opt == "--variable" || opt == "--all") {
                        is_symbol_search = true;
                        if (debug_mode) {
                            std::cerr << "[DEBUG] Symbol search triggered by option: " << opt << std::endl;
                        }
                        break;
                    }
                }
                
                // パスが指定されている場合もシンボル検索とする
                for (const auto& opt : options) {
                    if (!opt.empty() && opt[0] != '-') {
                        is_symbol_search = true;
                        if (debug_mode) {
                            std::cerr << "[DEBUG] Symbol search triggered by path: " << opt << std::endl;
                        }
                        break;
                    }
                }
                
                if (debug_mode) {
                    std::cerr << "[DEBUG] is_symbol_search: " << is_symbol_search << std::endl;
                    std::cerr << "[DEBUG] tokens.size(): " << tokens.size() << std::endl;
                    std::cerr << "[DEBUG] Condition check: is_symbol_search=" << is_symbol_search 
                             << " OR tokens.size()>1=" << (tokens.size() > 1) << std::endl;
                    
                    // 常にシンボル検索を使用（ファイル名検索は古い機能）
                    std::cerr << "[DEBUG] Always using symbol search" << std::endl;
                }
                result = session_commands_.cmd_find_symbols(session, symbol, options, debug_mode);
            }
        } else if (command == "include-graph") {
            result = session_commands_.cmd_include_graph(session);
        } else if (command == "include-cycles") {
            result = session_commands_.cmd_include_cycles(session);
        } else if (command == "include-impact") {
            result = session_commands_.cmd_include_impact(session);
        } else if (command == "include-unused") {
            result = session_commands_.cmd_include_unused(session);
        } else if (command == "include-optimize") {
            result = session_commands_.cmd_include_optimize(session);
        } else if (command == "duplicates") {
            result = session_commands_.cmd_duplicates(session);
        } else if (command.substr(0, 11) == "large-files") {
            // large-files コマンドのパース
            int threshold = 500;  // デフォルト500行
            size_t pos = command.find("--threshold");
            if (pos != std::string::npos) {
                size_t val_pos = command.find_first_not_of(" ", pos + 11);
                if (val_pos != std::string::npos) {
                    try {
                        threshold = std::stoi(command.substr(val_pos));
                    } catch (...) {
                        // パースエラーの場合はデフォルト値を使用
                    }
                }
            }
            result = session_commands_.cmd_large_files(session, threshold);
        } else if (command == "todo") {
            result = session_commands_.cmd_todo(session);
        } else if (command == "complexity-ranking") {
            result = session_commands_.cmd_complexity_ranking(session);
        } else if (command.substr(0, 7) == "analyze") {
            // analyze [filename] [--deep] [--complete]
            std::string args = command.substr(7);
            std::string filename;
            bool deep = false;
            bool complete = false;
            
            // --deep フラグをチェック
            size_t deep_pos = args.find("--deep");
            if (deep_pos != std::string::npos) {
                deep = true;
                // --deepを削除
                args = args.substr(0, deep_pos) + args.substr(deep_pos + 6);
            }
            
            // --complete フラグをチェック
            size_t complete_pos = args.find("--complete");
            if (complete_pos != std::string::npos) {
                complete = true;
                // --completeを削除
                args = args.substr(0, complete_pos) + args.substr(complete_pos + 10);
            }
            
            // ファイル名を抽出（先頭と末尾の空白を除去）
            size_t start = args.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t end = args.find_last_not_of(" \t");
                filename = args.substr(start, end - start + 1);
            }
            
            result = session_commands_.cmd_analyze(session, filename, deep, complete);
        } else if (command.substr(0, 18) == "dependency-analyze") {
            // dependency-analyze [filename]
            std::string args = command.substr(18);
            std::string filename;
            
            // ファイル名を抽出（先頭と末尾の空白を除去）
            size_t start = args.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t end = args.find_last_not_of(" \t");
                filename = args.substr(start, end - start + 1);
            }
            
            result = session_commands_.cmd_dependency_analyze(session, filename);
        } else if (command.substr(0, 11) == "move-class ") {
            // move-class <class_name> <src_file> <dst_file>
            std::string args_str = command.substr(11);
            
            // 引数を分割（スペース区切り）
            std::vector<std::string> args;
            std::stringstream ss(args_str);
            std::string arg;
            while (ss >> arg) {
                args.push_back(arg);
            }
            
            result = session_commands_.cmd_move_class(session, args);
        } else if (command == "help") {
            result = session_commands_.cmd_help();
        } else if (command.substr(0, 16) == "replace-preview ") {
            // replace-preview <file_path> <pattern> <replacement>
            std::string args = command.substr(16);
            
            // シンプルなトークン分割（3つの引数）
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            if (tokens.size() != 3) {
                result = {
                    {"error", "replace-preview: 使用法: replace-preview <file_path> <pattern> <replacement>"},
                    {"example", "replace-preview src/test.cpp \"old_function\" \"new_function\""}
                };
            } else {
                result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
                // DISABLED: session_commands_.cmd_replace_preview(session, tokens[0], tokens[1], tokens[2]);
            }
        } else if (command.substr(0, 16) == "replace-confirm ") {
            // replace-confirm <preview_id>
            std::string preview_id = command.substr(16);
            result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
            // DISABLED: session_commands_.cmd_replace_confirm(session, preview_id);
        } else if (command == "edit-history") {
            result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
            // DISABLED: session_commands_.cmd_edit_history(session);
        } else if (command.substr(0, 10) == "edit-show ") {
            // edit-show <id>
            std::string id = command.substr(10);
            result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
            // DISABLED: session_commands_.cmd_edit_show(session, id);
        } else if (command.substr(0, 15) == "insert-preview ") {
            // insert-preview <file> <position> <content>
            std::string args = command.substr(15);
            
            // トークン分割（3つの引数）
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            if (tokens.size() != 3) {
                result = {
                    {"error", "insert-preview: 使用法: insert-preview <file> <position> <content>"},
                    {"example", "insert-preview test.cpp end \"// Footer\""},
                    {"positions", {"start", "end", "行番号", "before:pattern", "after:pattern"}}
                };
            } else {
                result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
                // DISABLED: session_commands_.cmd_insert_preview(session, tokens[0], tokens[1], tokens[2]);
            }
        } else if (command.substr(0, 15) == "insert-confirm ") {
            // insert-confirm <preview_id>
            std::string preview_id = command.substr(15);
            result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
            // DISABLED: session_commands_.cmd_insert_confirm(session, preview_id);
        } else if (command.substr(0, 18) == "movelines-preview ") {
            // movelines-preview <srcfile> <start_line> <line_count> <dstfile> <insert_line>
            std::string args = command.substr(18);
            
            // トークン分割（5つの引数）
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            if (tokens.size() != 5) {
                result = {
                    {"error", "movelines-preview: 使用法: movelines-preview <srcfile> <start_line> <line_count> <dstfile> <insert_line>"},
                    {"example", "movelines-preview utils.js 45 20 helpers.js 10"}
                };
            } else {
                result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
                // DISABLED: session_commands_.cmd_movelines_preview(session, tokens[0], tokens[1], tokens[2], tokens[3], tokens[4]);
            }
        } else if (command.substr(0, 18) == "movelines-confirm ") {
            // movelines-confirm <preview_id>
            std::string preview_id = command.substr(18);
            result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
            // DISABLED: session_commands_.cmd_movelines_confirm(session, preview_id);
        } else if (command.substr(0, 8) == "replace ") {
            // replace <file_path> <pattern> <replacement>
            std::string args = command.substr(8);
            
            // シンプルなトークン分割（3つの引数）
            std::vector<std::string> tokens;
            std::string current_token;
            bool in_quotes = false;
            
            for (char c : args) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ' ' && !in_quotes) {
                    if (!current_token.empty()) {
                        tokens.push_back(current_token);
                        current_token.clear();
                    }
                } else {
                    current_token += c;
                }
            }
            if (!current_token.empty()) {
                tokens.push_back(current_token);
            }
            
            if (tokens.size() != 3) {
                result = {
                    {"error", "replace: 使用法: replace <file_path> <pattern> <replacement>"},
                    {"example", "replace src/test.cpp \"old_function\" \"new_function\""}
                };
            } else {
                result = {{"info", "編集機能はNekoCode MCPサーバーで提供されています"}};
                // DISABLED: session_commands_.cmd_replace(session, tokens[0], tokens[1], tokens[2]);
            }
        } else if (command.substr(0, 10) == "ast-query ") {
            // ast-query <query_path>
            std::string query_path = command.substr(10);
            result = session_commands_.cmd_ast_query(session, query_path);
        } else if (command == "ast-stats") {
            result = session_commands_.cmd_ast_stats(session);
        } else if (command.substr(0, 15) == "scope-analysis ") {
            // scope-analysis <line_number>
            std::string line_str = command.substr(15);
            uint32_t line_number = std::stoul(line_str);
            result = session_commands_.cmd_scope_analysis(session, line_number);
        } else if (command.substr(0, 9) == "ast-dump ") {
            // ast-dump <format>
            std::string format = command.substr(9);
            result = session_commands_.cmd_ast_dump(session, format);
        } else if (command == "ast-dump") {
            result = session_commands_.cmd_ast_dump(session);
        } else {
            result = {
                {"error", "Unknown command: " + command},
                {"available_commands", {"stats", "files", "complexity", 
                                        "structure", "calls", "find <term>", 
                                        "move-class <class> <src> <dst>",
                                        "include-graph", "include-cycles", "include-impact",
                                        "include-unused", "include-optimize", "duplicates", 
                                        "large-files", "todo", "complexity-ranking", 
                                        "analyze", "dependency-analyze", "help",
                                        "ast-query <path>", "ast-stats", "scope-analysis <line>", "ast-dump [format]",
                                        "replace-preview <file> <pattern> <replacement>", 
                                        "replace-confirm <preview_id>", "edit-history", "edit-show <id>",
                                        "movelines-preview <srcfile> <start_line> <line_count> <dstfile> <insert_line>",
                                        "movelines-confirm <preview_id>"}}
            };
        }
        
        // 履歴更新
        SessionData::CommandHistory history;
        history.command = command;
        history.timestamp = std::chrono::system_clock::now();
        history.result_type = result.contains("error") ? "error" : "success";
        session.command_history.push_back(history);
        
        save_session(session);
        
        return result;
        
    } catch (const std::exception& e) {
        return {{"error", std::string("Command execution failed: ") + e.what()}};
    }
}

bool SessionManager::session_exists(const std::string& session_id) const {
    return std::filesystem::exists(get_session_file(session_id));
}

std::vector<std::string> SessionManager::list_sessions() const {
    std::vector<std::string> sessions;
    
    for (const auto& entry : std::filesystem::directory_iterator(sessions_dir_)) {
        if (entry.path().extension() == ".json") {
            sessions.push_back(entry.path().stem().string());
        }
    }
    
    return sessions;
}

//=============================================================================
// 🔒 Private実装
//=============================================================================

std::filesystem::path SessionManager::get_session_file(const std::string& session_id) const {
    return sessions_dir_ / (session_id + ".json");
}

SessionData SessionManager::load_session(const std::string& session_id) const {
    std::ifstream file(get_session_file(session_id));
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open session file");
    }
    
    nlohmann::json j;
    file >> j;
    
    return SessionData::from_json(j);
}

void SessionManager::save_session(const SessionData& session) const {
    std::ofstream file(get_session_file(session.session_id));
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create session file");
    }
    
    file << session.to_json().dump(2);
}

std::vector<FileInfo> SessionManager::getProjectFiles(const std::string& session_id) {
    try {
        if (!session_exists(session_id)) {
            return {};
        }
        
        SessionData session = load_session(session_id);
        std::vector<FileInfo> files;
        
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                files.push_back(file.file_info);
            }
        } else {
            files.push_back(session.single_file_result.file_info);
        }
        
        return files;
        
    } catch (const std::exception&) {
        return {};
    }
}

std::string SessionManager::generate_session_id() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "session_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    return ss.str();
}

nlohmann::json SessionManager::extract_quick_stats(const AnalysisResult& result) const {
    return {
        {"type", "file"},
        {"language", static_cast<int>(result.language)},
        {"lines", result.file_info.total_lines},
        {"size", result.file_info.size_bytes},
        {"complexity", result.complexity.cyclomatic_complexity},
        {"functions", result.stats.function_count},
        {"classes", result.stats.class_count}
    };
}

nlohmann::json SessionManager::extract_quick_stats(const DirectoryAnalysis& result) const {
    return {
        {"type", "directory"},
        {"files", result.summary.total_files},
        {"lines", result.summary.total_lines},
        {"size", result.summary.total_size},
        {"functions", result.summary.total_functions},
        {"classes", result.summary.total_classes}
    };
}

} // namespace nekocode