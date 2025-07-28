//=============================================================================
// 🎮 Session Manager実装 - 対話式解析セッション管理
//=============================================================================

#include "nekocode/session_manager.hpp"
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
// 📱 SessionData実装
//=============================================================================

nlohmann::json SessionData::to_json() const {
    nlohmann::json j;
    
    j["session_id"] = session_id;
    j["session_type"] = session_type;
    j["created_at"] = timestamp_to_string(created_at);
    j["target_path"] = target_path.string();
    j["is_directory"] = is_directory;
    
    // 解析データ
    if (is_directory) {
        j["directory_result"] = {
            {"summary", {
                {"total_files", directory_result.summary.total_files},
                {"total_lines", directory_result.summary.total_lines},
                {"total_size", directory_result.summary.total_size},
                {"large_files", directory_result.summary.large_files},
                {"complex_files", directory_result.summary.complex_files},
                {"total_classes", directory_result.summary.total_classes},
                {"total_functions", directory_result.summary.total_functions}
            }},
            {"files_count", directory_result.files.size()}
        };
        
        // ファイル詳細（完全保存）
        nlohmann::json files_json = nlohmann::json::array();
        for (const auto& file : directory_result.files) {
            nlohmann::json file_json;
            file_json["file_info"] = {
                {"name", file.file_info.name},
                {"path", file.file_info.path.string()},
                {"size_bytes", file.file_info.size_bytes},
                {"total_lines", file.file_info.total_lines},
                {"code_lines", file.file_info.code_lines},
                {"comment_lines", file.file_info.comment_lines},
                {"empty_lines", file.file_info.empty_lines}
            };
            file_json["stats"] = {
                {"class_count", file.stats.class_count},
                {"function_count", file.stats.function_count},
                {"import_count", file.stats.import_count},
                {"export_count", file.stats.export_count},
                {"unique_calls", file.stats.unique_calls},
                {"total_calls", file.stats.total_calls}
            };
            file_json["complexity"] = {
                {"cyclomatic_complexity", file.complexity.cyclomatic_complexity},
                {"max_nesting_depth", file.complexity.max_nesting_depth},
                {"rating", file.complexity.to_string()}
            };
            files_json.push_back(file_json);
        }
        j["directory_files"] = files_json;
    } else {
        j["single_file_result"] = {
            {"file_name", single_file_result.file_info.name},
            {"lines", single_file_result.file_info.total_lines},
            {"size", single_file_result.file_info.size_bytes},
            {"complexity", single_file_result.complexity.cyclomatic_complexity},
            {"rating", single_file_result.complexity.to_string()},
            {"classes", single_file_result.stats.class_count},
            {"functions", single_file_result.stats.function_count}
        };
    }
    
    j["quick_stats"] = quick_stats;
    
    // コマンド履歴
    nlohmann::json history_json = nlohmann::json::array();
    for (const auto& cmd : command_history) {
        history_json.push_back({
            {"command", cmd.command},
            {"timestamp", timestamp_to_string(cmd.timestamp)},
            {"result_type", cmd.result_type}
        });
    }
    j["command_history"] = history_json;
    
    return j;
}

SessionData SessionData::from_json(const nlohmann::json& j) {
    SessionData data;
    
    data.session_id = j["session_id"];
    data.session_type = j.value("session_type", "ai_optimized");
    data.created_at = string_to_timestamp(j["created_at"]);
    data.target_path = j["target_path"].get<std::string>();
    data.is_directory = j.value("is_directory", false);
    data.quick_stats = j["quick_stats"];
    
    // 解析データ復元
    if (data.is_directory) {
        // サマリー復元
        if (j.contains("directory_result") && j["directory_result"].contains("summary")) {
            const auto& summary = j["directory_result"]["summary"];
            data.directory_result.summary.total_files = summary["total_files"];
            data.directory_result.summary.total_lines = summary["total_lines"];
            data.directory_result.summary.total_size = summary["total_size"];
            data.directory_result.summary.large_files = summary["large_files"];
            data.directory_result.summary.complex_files = summary["complex_files"];
            data.directory_result.summary.total_classes = summary["total_classes"];
            data.directory_result.summary.total_functions = summary["total_functions"];
        }
        
        // ファイル詳細復元
        if (j.contains("directory_files")) {
            for (const auto& file_json : j["directory_files"]) {
                AnalysisResult file_result;
                
                // ファイル情報
                if (file_json.contains("file_info")) {
                    const auto& info = file_json["file_info"];
                    file_result.file_info.name = info["name"];
                    file_result.file_info.path = info["path"].get<std::string>();
                    file_result.file_info.size_bytes = info["size_bytes"];
                    file_result.file_info.total_lines = info["total_lines"];
                    file_result.file_info.code_lines = info["code_lines"];
                    file_result.file_info.comment_lines = info["comment_lines"];
                    file_result.file_info.empty_lines = info["empty_lines"];
                }
                
                // 統計情報
                if (file_json.contains("stats")) {
                    const auto& stats = file_json["stats"];
                    file_result.stats.class_count = stats["class_count"];
                    file_result.stats.function_count = stats["function_count"];
                    file_result.stats.import_count = stats["import_count"];
                    file_result.stats.export_count = stats["export_count"];
                    file_result.stats.unique_calls = stats["unique_calls"];
                    file_result.stats.total_calls = stats["total_calls"];
                }
                
                // 複雑度情報
                if (file_json.contains("complexity")) {
                    const auto& complexity = file_json["complexity"];
                    file_result.complexity.cyclomatic_complexity = complexity["cyclomatic_complexity"];
                    file_result.complexity.max_nesting_depth = complexity["max_nesting_depth"];
                    file_result.complexity.update_rating();
                }
                
                data.directory_result.files.push_back(file_result);
            }
        }
    } else {
        // 単一ファイルの場合（今回は省略、必要なら後で実装）
    }
    
    // コマンド履歴復元
    if (j.contains("command_history")) {
        for (const auto& cmd_json : j["command_history"]) {
            CommandHistory cmd;
            cmd.command = cmd_json["command"];
            cmd.timestamp = string_to_timestamp(cmd_json["timestamp"]);
            cmd.result_type = cmd_json["result_type"];
            data.command_history.push_back(cmd);
        }
    }
    
    return data;
}

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
    session.target_path = target_path;
    session.is_directory = false;
    session.single_file_result = result;
    session.quick_stats = extract_quick_stats(result);
    
    save_session(session);
    
    return session.session_id;
}

std::string SessionManager::create_session(const std::filesystem::path& target_path, 
                                           const DirectoryAnalysis& result) {
    SessionData session;
    session.session_id = generate_session_id();
    session.created_at = std::chrono::system_clock::now();
    session.target_path = target_path;
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
            result = cmd_stats(session);
        } else if (command == "files") {
            result = cmd_files(session);
        } else if (command == "complexity") {
            result = cmd_complexity(session);
        } else if (command == "structure") {
            result = cmd_structure(session);
        } else if (command == "calls") {
            result = cmd_calls(session);
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
                result = cmd_find_symbols(session, symbol, options, debug_mode);
            }
        } else if (command == "include-graph") {
            result = cmd_include_graph(session);
        } else if (command == "include-cycles") {
            result = cmd_include_cycles(session);
        } else if (command == "include-impact") {
            result = cmd_include_impact(session);
        } else if (command == "include-unused") {
            result = cmd_include_unused(session);
        } else if (command == "include-optimize") {
            result = cmd_include_optimize(session);
        } else if (command == "help") {
            result = cmd_help();
        } else {
            result = {
                {"error", "Unknown command: " + command},
                {"available_commands", {"stats", "files", "complexity", 
                                        "structure", "calls", "find <term>", 
                                        "include-graph", "include-cycles", "include-impact",
                                        "include-unused", "include-optimize", "help"}}
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

nlohmann::json SessionManager::cmd_stats(const SessionData& session) const {
    return {
        {"command", "stats"},
        {"result", session.quick_stats},
        {"summary", session.is_directory ? 
            "Project: " + std::to_string(session.directory_result.summary.total_files) + " files" :
            "File: " + session.single_file_result.file_info.name}
    };
}

nlohmann::json SessionManager::cmd_files(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "files"},
            {"result", {{session.single_file_result.file_info.name, {
                {"lines", session.single_file_result.file_info.total_lines},
                {"size", session.single_file_result.file_info.size_bytes}
            }}}},
            {"summary", "Single file analysis"}
        };
    }
    
    nlohmann::json files_json;
    int large_count = 0;
    
    for (const auto& file : session.directory_result.files) {
        files_json[file.file_info.name] = {
            {"lines", file.file_info.total_lines},
            {"size", file.file_info.size_bytes},
            {"complexity", file.complexity.cyclomatic_complexity}
        };
        
        if (file.file_info.total_lines > 500) {
            large_count++;
        }
    }
    
    return {
        {"command", "files"},
        {"result", files_json},
        {"large_files", large_count},
        {"summary", std::to_string(session.directory_result.files.size()) + 
                    " files, " + std::to_string(large_count) + " large files (>500 lines)"}
    };
}

nlohmann::json SessionManager::cmd_complexity(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "complexity"},
            {"result", {
                {"cyclomatic", session.single_file_result.complexity.cyclomatic_complexity},
                {"nesting", session.single_file_result.complexity.max_nesting_depth},
                {"rating", session.single_file_result.complexity.to_string()}
            }},
            {"summary", "Complexity: " + 
                std::to_string(session.single_file_result.complexity.cyclomatic_complexity) + 
                " (" + session.single_file_result.complexity.to_string() + ")"}
        };
    }
    
    nlohmann::json complex_files = nlohmann::json::array();
    
    for (const auto& file : session.directory_result.files) {
        complex_files.push_back({
            {"file", file.file_info.name},
            {"complexity", file.complexity.cyclomatic_complexity},
            {"rating", file.complexity.to_string()}
        });
    }
    
    return {
        {"command", "complexity"},
        {"result", complex_files},
        {"summary", "Analyzed " + std::to_string(session.directory_result.files.size()) + 
                    " files for complexity"}
    };
}

nlohmann::json SessionManager::cmd_structure(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "structure"},
            {"result", {
                {"classes", session.single_file_result.stats.class_count},
                {"functions", session.single_file_result.stats.function_count},
                {"imports", session.single_file_result.stats.import_count},
                {"exports", session.single_file_result.stats.export_count}
            }},
            {"summary", "Classes: " + 
                std::to_string(session.single_file_result.stats.class_count) + 
                ", Functions: " + 
                std::to_string(session.single_file_result.stats.function_count)}
        };
    }
    
    return {
        {"command", "structure"},
        {"result", {
            {"total_classes", session.directory_result.summary.total_classes},
            {"total_functions", session.directory_result.summary.total_functions}
        }},
        {"summary", "Total classes: " + 
            std::to_string(session.directory_result.summary.total_classes) + 
            ", Total functions: " + 
            std::to_string(session.directory_result.summary.total_functions)}
    };
}

nlohmann::json SessionManager::cmd_calls(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "calls"},
            {"result", {
                {"unique_calls", session.single_file_result.stats.unique_calls},
                {"total_calls", session.single_file_result.stats.total_calls}
            }},
            {"summary", "Unique calls: " + 
                std::to_string(session.single_file_result.stats.unique_calls) + 
                ", Total: " + 
                std::to_string(session.single_file_result.stats.total_calls)}
        };
    }
    
    uint32_t total_unique = 0;
    uint32_t total_calls = 0;
    
    for (const auto& file : session.directory_result.files) {
        total_unique += file.stats.unique_calls;
        total_calls += file.stats.total_calls;
    }
    
    return {
        {"command", "calls"},
        {"result", {
            {"total_unique", total_unique},
            {"total_calls", total_calls}
        }},
        {"summary", "Total unique calls: " + std::to_string(total_unique) + 
                    ", Total calls: " + std::to_string(total_calls)}
    };
}

nlohmann::json SessionManager::cmd_find(const SessionData& session, const std::string& term) const {
    nlohmann::json matches = nlohmann::json::array();
    
    if (!session.is_directory) {
        // 単一ファイルで検索（簡易実装）
        if (session.single_file_result.file_info.name.find(term) != std::string::npos) {
            matches.push_back({
                {"file", session.single_file_result.file_info.name},
                {"type", "filename"}
            });
        }
    } else {
        // ディレクトリで検索
        for (const auto& file : session.directory_result.files) {
            if (file.file_info.name.find(term) != std::string::npos) {
                matches.push_back({
                    {"file", file.file_info.name},
                    {"type", "filename"}
                });
            }
        }
    }
    
    return {
        {"command", "find"},
        {"search_term", term},
        {"result", matches},
        {"summary", "Found " + std::to_string(matches.size()) + " matches"}
    };
}

nlohmann::json SessionManager::cmd_help() const {
    return {
        {"command", "help"},
        {"commands", {
            {"stats", "Show quick statistics"},
            {"files", "List all files with details"},
            {"complexity", "Show complexity analysis"},
            {"structure", "Show code structure (classes/functions)"},
            {"calls", "Show function call analysis"},
            {"find <symbol>", "Search for symbol usage (functions/variables)"},
            {"find <symbol> -f", "Search for function usage only"},
            {"find <symbol> -v", "Search for variable usage only"},
            {"find <symbol> -o FILE", "Save results to file"},
            {"find <symbol> PATH", "Search in specific path"},
            {"include-graph", "Show include dependency graph"},
            {"include-cycles", "Detect circular dependencies"},
            {"include-impact", "Analyze file change impact"},
            {"include-unused", "Detect unused includes"},
            {"include-optimize", "Get optimization suggestions"},
            {"help", "Show this help"}
        }}
    };
}

nlohmann::json SessionManager::cmd_include_graph(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "include-graph"},
            {"error", "Include analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // Include解析実行
    IncludeAnalyzer analyzer;
    IncludeAnalyzer::Config config;
    config.analyze_system_headers = false;
    config.detect_circular = true;
    analyzer.set_config(config);
    
    auto result = analyzer.analyze_directory(session.target_path);
    
    return {
        {"command", "include-graph"},
        {"result", analyzer.get_include_graph(result)},
        {"summary", "Analyzed " + std::to_string(result.total_files) + 
                   " files with " + std::to_string(result.total_includes) + " includes"}
    };
}

nlohmann::json SessionManager::cmd_include_cycles(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "include-cycles"},
            {"error", "Include analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // Include解析実行
    IncludeAnalyzer analyzer;
    IncludeAnalyzer::Config config;
    config.detect_circular = true;
    analyzer.set_config(config);
    
    auto result = analyzer.analyze_directory(session.target_path);
    
    return {
        {"command", "include-cycles"},
        {"result", analyzer.get_circular_dependencies(result)},
        {"summary", result.circular_dependencies.empty() ? 
                   "No circular dependencies found! 🎉" :
                   "Found " + std::to_string(result.circular_dependencies.size()) + " circular dependencies ⚠️"}
    };
}

nlohmann::json SessionManager::cmd_include_impact(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "include-impact"},
            {"error", "Include analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // ホットスポットヘッダーを返す（簡易実装）
    IncludeAnalyzer analyzer;
    auto result = analyzer.analyze_directory(session.target_path);
    
    nlohmann::json impact_json;
    impact_json["command"] = "include-impact";
    impact_json["hotspot_headers"] = nlohmann::json::array();
    
    for (const auto& hotspot : result.hotspot_headers) {
        if (hotspot.included_by_count > 0) {  // Top 10
            impact_json["hotspot_headers"].push_back({
                {"file", hotspot.file_path},
                {"included_by", hotspot.included_by_count},
                {"impact_score", hotspot.impact_score}
            });
        }
    }
    
    impact_json["summary"] = "Top impact headers - changes to these affect many files";
    
    return impact_json;
}

nlohmann::json SessionManager::cmd_include_unused(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "include-unused"},
            {"error", "Include analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // Include解析実行
    IncludeAnalyzer analyzer;
    IncludeAnalyzer::Config config;
    config.detect_unused = true;
    analyzer.set_config(config);
    
    auto result = analyzer.analyze_directory(session.target_path);
    
    return {
        {"command", "include-unused"},
        {"result", analyzer.get_unused_includes(result)},
        {"summary", result.unused_includes.empty() ? 
                   "No unused includes detected! 🎉" :
                   "Found " + std::to_string(result.unused_includes.size()) + " potentially unused includes"}
    };
}

nlohmann::json SessionManager::cmd_include_optimize(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "include-optimize"},
            {"error", "Include analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // Include解析実行
    IncludeAnalyzer analyzer;
    IncludeAnalyzer::Config config;
    config.detect_circular = true;
    config.detect_unused = true;
    config.suggest_optimizations = true;
    analyzer.set_config(config);
    
    auto result = analyzer.analyze_directory(session.target_path);
    
    nlohmann::json optimize_json;
    optimize_json["command"] = "include-optimize";
    
    // 最適化提案
    nlohmann::json suggestions = nlohmann::json::array();
    
    // 循環依存の解決提案
    if (!result.circular_dependencies.empty()) {
        suggestions.push_back({
            {"type", "break_circular_dependencies"},
            {"count", result.circular_dependencies.size()},
            {"severity", "high"},
            {"suggestion", "Consider using forward declarations or restructuring to break cycles"}
        });
    }
    
    // 不要include削除提案
    if (!result.unused_includes.empty()) {
        suggestions.push_back({
            {"type", "remove_unused_includes"},
            {"count", result.unused_includes.size()},
            {"severity", "medium"},
            {"suggestion", "Remove unused includes to improve compilation time"},
            {"estimated_impact", std::to_string(result.unused_includes.size() * 5) + "% faster compilation"}
        });
    }
    
    // ホットスポット最適化提案
    for (const auto& hotspot : result.hotspot_headers) {
        if (hotspot.included_by_count > 10) {
            suggestions.push_back({
                {"type", "optimize_hotspot"},
                {"file", hotspot.file_path},
                {"included_by", hotspot.included_by_count},
                {"severity", "high"},
                {"suggestion", "Consider splitting this header or using forward declarations"}
            });
        }
    }
    
    optimize_json["suggestions"] = suggestions;
    optimize_json["total_suggestions"] = suggestions.size();
    optimize_json["summary"] = suggestions.empty() ? 
                              "Your include structure is well optimized! 🎉" :
                              "Found " + std::to_string(suggestions.size()) + " optimization opportunities";
    
    return optimize_json;
}

std::string SessionManager::generate_session_id() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "ai_session_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    return ss.str();
}

nlohmann::json SessionManager::extract_quick_stats(const AnalysisResult& result) const {
    return {
        {"type", "single_file"},
        {"file", result.file_info.name},
        {"lines", result.file_info.total_lines},
        {"complexity", result.complexity.cyclomatic_complexity},
        {"rating", result.complexity.to_string()},
        {"classes", result.stats.class_count},
        {"functions", result.stats.function_count}
    };
}

nlohmann::json SessionManager::extract_quick_stats(const DirectoryAnalysis& result) const {
    return {
        {"type", "directory"},
        {"files", result.summary.total_files},
        {"lines", result.summary.total_lines},
        {"size", result.summary.total_size},
        {"large_files", result.summary.large_files},
        {"classes", result.summary.total_classes},
        {"functions", result.summary.total_functions}
    };
}

//=============================================================================
// 🛠️ Helper Functions実装
//=============================================================================

std::string timestamp_to_string(const Timestamp& ts) {
    auto time_t = std::chrono::system_clock::to_time_t(ts);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

Timestamp string_to_timestamp(const std::string& str) {
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

nlohmann::json SessionManager::cmd_find_symbols(const SessionData& session, 
                                                const std::string& symbol,
                                                const std::vector<std::string>& options,
                                                bool debug) const {
    if (debug) {
        std::cerr << "[DEBUG cmd_find_symbols] Starting symbol search for: " << symbol << std::endl;
        std::cerr << "[DEBUG cmd_find_symbols] Options count: " << options.size() << std::endl;
    }
    
    try {
        // シンボル検索オプション構築
        SymbolFinder::FindOptions find_options;
        find_options.display_limit = 50;  // Claude Code向けデフォルト
        
        // オプション解析
        for (size_t i = 0; i < options.size(); ++i) {
            const auto& opt = options[i];
            
            if (opt == "-f" || opt == "--function") {
                find_options.type = SymbolFinder::SymbolType::FUNCTION;
            }
            else if (opt == "-v" || opt == "--variable") {
                find_options.type = SymbolFinder::SymbolType::VARIABLE;
            }
            else if ((opt == "-o" || opt == "--output") && i + 1 < options.size()) {
                find_options.output_file = options[++i];
            }
            else if (opt.find("--limit=") == 0) {
                find_options.display_limit = std::stoul(opt.substr(8));
            }
            else if (opt == "--limit" && i + 1 < options.size()) {
                find_options.display_limit = std::stoul(options[++i]);
            }
            else if (opt == "--debug") {
                find_options.debug = true;
            }
            else if (!opt.empty() && opt[0] != '-') {
                // 数字のみの引数は除外（オプションの値の可能性）
                bool is_only_digits = std::all_of(opt.begin(), opt.end(), ::isdigit);
                if (!is_only_digits) {
                    find_options.search_paths.push_back(opt);
                }
            }
        }
        
        // SymbolFinder作成・実行
        SymbolFinder finder;
        
        // セッションからファイル情報を設定
        std::vector<FileInfo> files;
        if (session.is_directory) {
            if (debug) {
                std::cerr << "[DEBUG cmd_find_symbols] Directory mode, files count: " 
                         << session.directory_result.files.size() << std::endl;
                std::cerr << "[DEBUG cmd_find_symbols] Session target path: " << session.target_path << std::endl;
            }
            
            for (const auto& file : session.directory_result.files) {
                FileInfo file_with_full_path = file.file_info;
                
                // ファイル名のみの場合、セッションのtarget_pathを使って構築
                std::string path_str = file.file_info.path.string();
                if (!file.file_info.path.is_absolute() && 
                    path_str.find('/') == std::string::npos && 
                    path_str.find('\\') == std::string::npos) {
                    file_with_full_path.path = session.target_path / file.file_info.path;
                    if (debug) {
                        std::cerr << "[DEBUG cmd_find_symbols] Converted filename to path: " 
                                 << file.file_info.path << " -> " << file_with_full_path.path << std::endl;
                    }
                } else {
                    if (debug) {
                        std::cerr << "[DEBUG cmd_find_symbols] Using existing path: " << file.file_info.path << std::endl;
                    }
                }
                
                files.push_back(file_with_full_path);
                if (debug) {
                    std::cerr << "[DEBUG cmd_find_symbols] Added file: " << file_with_full_path.path << std::endl;
                }
            }
        } else {
            if (debug) {
                std::cerr << "[DEBUG cmd_find_symbols] Single file mode: " 
                         << session.single_file_result.file_info.path << std::endl;
            }
            files.push_back(session.single_file_result.file_info);
        }
        
        if (debug) {
            std::cerr << "[DEBUG cmd_find_symbols] Total files to search: " << files.size() << std::endl;
        }
        finder.setFiles(files);
        
        // 検索実行
        if (debug) {
            std::cerr << "[DEBUG cmd_find_symbols] Starting find operation..." << std::endl;
        }
        auto results = finder.find(symbol, find_options);
        if (debug) {
            std::cerr << "[DEBUG cmd_find_symbols] Find operation completed. Total matches: " 
                     << results.total_count << std::endl;
        }
        
        // JSON結果構築
        nlohmann::json result_json;
        result_json["command"] = "find";
        result_json["symbol"] = symbol;
        result_json["total_matches"] = results.total_count;
        
        // 統計情報
        if (results.function_count > 0 || results.variable_count > 0) {
            result_json["statistics"] = {
                {"functions", results.function_count},
                {"variables", results.variable_count}
            };
        }
        
        // 結果リスト（制限付き）
        nlohmann::json matches = nlohmann::json::array();
        size_t display_count = std::min(results.total_count, find_options.display_limit);
        
        for (size_t i = 0; i < display_count && i < results.locations.size(); ++i) {
            const auto& loc = results.locations[i];
            matches.push_back({
                {"file", loc.file_path},
                {"line", loc.line_number},
                {"content", loc.line_content},
                {"type", loc.symbol_type == SymbolFinder::SymbolType::FUNCTION ? "function" : "variable"},
                {"use_type", 
                    loc.use_type == SymbolFinder::UseType::DECLARATION ? "declaration" :
                    loc.use_type == SymbolFinder::UseType::ASSIGNMENT ? "assignment" :
                    loc.use_type == SymbolFinder::UseType::CALL ? "call" : "reference"
                }
            });
        }
        
        result_json["matches"] = matches;
        
        // 省略情報
        if (display_count < results.total_count) {
            size_t omitted = results.total_count - display_count;
            std::string filename = find_options.output_file.empty() ? 
                                  "find_results_" + symbol + ".txt" : find_options.output_file;
            
            result_json["omitted"] = {
                {"count", omitted},
                {"saved_to", filename},
                {"message", "残り" + std::to_string(omitted) + "件はファイルに保存されました"}
            };
            
            // ファイル出力実行（SymbolFinderは内部で処理）
            if (find_options.output_file.empty()) {
                // 自動生成ファイル名で再実行
                SymbolFinder::FindOptions auto_save_options = find_options;
                auto_save_options.output_file = filename;
                auto_save_options.display_limit = std::numeric_limits<size_t>::max();  // 全件出力
                finder.find(symbol, auto_save_options);
            }
        }
        
        // サマリー
        if (results.isEmpty()) {
            result_json["summary"] = "'" + symbol + "' は見つかりませんでした";
        } else {
            result_json["summary"] = "Found " + std::to_string(results.total_count) + " matches for '" + symbol + "'";
        }
        
        return result_json;
        
    } catch (const std::exception& e) {
        return {
            {"command", "find"},
            {"error", std::string("Symbol search failed: ") + e.what()},
            {"symbol", symbol}
        };
    }
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

} // namespace nekocode