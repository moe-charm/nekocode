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
            
            // クラス詳細情報を追加
            nlohmann::json classes_json = nlohmann::json::array();
            for (const auto& cls : file.classes) {
                nlohmann::json class_json;
                class_json["name"] = cls.name;
                class_json["parent_class"] = cls.parent_class;
                class_json["start_line"] = cls.start_line;
                class_json["end_line"] = cls.end_line;
                
                nlohmann::json methods_json = nlohmann::json::array();
                for (const auto& method : cls.methods) {
                    nlohmann::json method_json;
                    method_json["name"] = method.name;
                    method_json["start_line"] = method.start_line;
                    method_json["end_line"] = method.end_line;
                    method_json["complexity"] = method.complexity.cyclomatic_complexity;
                    method_json["parameters"] = method.parameters;
                    methods_json.push_back(method_json);
                }
                class_json["methods"] = methods_json;
                
                // メンバ変数情報を追加
                nlohmann::json member_vars_json = nlohmann::json::array();
                for (const auto& var : cls.member_variables) {
                    nlohmann::json var_json;
                    var_json["name"] = var.name;
                    var_json["type"] = var.type;
                    var_json["declaration_line"] = var.declaration_line;
                    var_json["is_static"] = var.is_static;
                    var_json["is_const"] = var.is_const;
                    var_json["access_modifier"] = var.access_modifier;
                    // Phase2の情報は必要に応じて追加
                    if (!var.used_by_methods.empty()) {
                        var_json["used_by_methods"] = var.used_by_methods;
                    }
                    if (!var.modified_by_methods.empty()) {
                        var_json["modified_by_methods"] = var.modified_by_methods;
                    }
                    member_vars_json.push_back(var_json);
                }
                class_json["member_variables"] = member_vars_json;
                
                classes_json.push_back(class_json);
            }
            file_json["classes"] = classes_json;
            
            // 関数詳細情報を追加
            nlohmann::json functions_json = nlohmann::json::array();
            for (const auto& func : file.functions) {
                nlohmann::json func_json;
                func_json["name"] = func.name;
                func_json["start_line"] = func.start_line;
                func_json["end_line"] = func.end_line;
                func_json["complexity"] = func.complexity.cyclomatic_complexity;
                func_json["parameters"] = func.parameters;
                functions_json.push_back(func_json);
            }
            file_json["functions"] = functions_json;
            
            // 関数呼び出し情報を追加
            nlohmann::json calls_json = nlohmann::json::array();
            for (const auto& call : file.function_calls) {
                nlohmann::json call_json;
                call_json["function_name"] = call.function_name;
                call_json["line_number"] = call.line_number;
                call_json["is_method_call"] = call.is_method_call;
                if (!call.object_name.empty()) {
                    call_json["object_name"] = call.object_name;
                }
                calls_json.push_back(call_json);
            }
            file_json["function_calls"] = calls_json;
            
            files_json.push_back(file_json);
        }
        j["directory_files"] = files_json;
    } else {
        // 単一ファイル：完全な詳細情報も保存（analyze機能対応）
        nlohmann::json single_file_json;
        single_file_json["file_info"] = {
            {"name", single_file_result.file_info.name},
            {"path", single_file_result.file_info.path.string()},
            {"size_bytes", single_file_result.file_info.size_bytes},
            {"total_lines", single_file_result.file_info.total_lines},
            {"code_lines", single_file_result.file_info.code_lines},
            {"comment_lines", single_file_result.file_info.comment_lines},
            {"empty_lines", single_file_result.file_info.empty_lines}
        };
        single_file_json["stats"] = {
            {"class_count", single_file_result.stats.class_count},
            {"function_count", single_file_result.stats.function_count},
            {"import_count", single_file_result.stats.import_count},
            {"export_count", single_file_result.stats.export_count},
            {"unique_calls", single_file_result.stats.unique_calls},
            {"total_calls", single_file_result.stats.total_calls}
        };
        single_file_json["complexity"] = {
            {"cyclomatic_complexity", single_file_result.complexity.cyclomatic_complexity},
            {"max_nesting_depth", single_file_result.complexity.max_nesting_depth},
            {"rating", single_file_result.complexity.to_string()}
        };
        
        // クラス詳細情報を保存
        nlohmann::json classes_json = nlohmann::json::array();
        for (const auto& cls : single_file_result.classes) {
            nlohmann::json class_json;
            class_json["name"] = cls.name;
            class_json["parent_class"] = cls.parent_class;
            class_json["start_line"] = cls.start_line;
            class_json["end_line"] = cls.end_line;
            
            nlohmann::json methods_json = nlohmann::json::array();
            for (const auto& method : cls.methods) {
                nlohmann::json method_json;
                method_json["name"] = method.name;
                method_json["start_line"] = method.start_line;
                method_json["end_line"] = method.end_line;
                method_json["complexity"] = method.complexity.cyclomatic_complexity;
                method_json["parameters"] = method.parameters;
                methods_json.push_back(method_json);
            }
            class_json["methods"] = methods_json;
            
            // メンバ変数情報を保存
            nlohmann::json member_vars_json = nlohmann::json::array();
            for (const auto& var : cls.member_variables) {
                nlohmann::json var_json;
                var_json["name"] = var.name;
                var_json["type"] = var.type;
                var_json["declaration_line"] = var.declaration_line;
                var_json["is_static"] = var.is_static;
                var_json["is_const"] = var.is_const;
                var_json["access_modifier"] = var.access_modifier;
                // Phase2の情報は必要に応じて追加
                if (!var.used_by_methods.empty()) {
                    var_json["used_by_methods"] = var.used_by_methods;
                }
                if (!var.modified_by_methods.empty()) {
                    var_json["modified_by_methods"] = var.modified_by_methods;
                }
                member_vars_json.push_back(var_json);
            }
            class_json["member_variables"] = member_vars_json;
            
            classes_json.push_back(class_json);
        }
        single_file_json["classes"] = classes_json;
        
        // 関数詳細情報を保存
        nlohmann::json functions_json = nlohmann::json::array();
        for (const auto& func : single_file_result.functions) {
            nlohmann::json func_json;
            func_json["name"] = func.name;
            func_json["start_line"] = func.start_line;
            func_json["end_line"] = func.end_line;
            func_json["complexity"] = func.complexity.cyclomatic_complexity;
            func_json["parameters"] = func.parameters;
            func_json["is_async"] = func.is_async;
            func_json["is_arrow_function"] = func.is_arrow_function;
            functions_json.push_back(func_json);
        }
        single_file_json["functions"] = functions_json;
        
        j["single_file_result"] = single_file_json;
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
                
                // クラス詳細情報復元
                if (file_json.contains("classes")) {
                    for (const auto& class_json : file_json["classes"]) {
                        ClassInfo cls;
                        cls.name = class_json["name"];
                        if (class_json.contains("parent_class")) {
                            cls.parent_class = class_json["parent_class"];
                        }
                        cls.start_line = class_json["start_line"];
                        cls.end_line = class_json["end_line"];
                        
                        // メソッド復元
                        if (class_json.contains("methods")) {
                            for (const auto& method_json : class_json["methods"]) {
                                FunctionInfo method;
                                method.name = method_json["name"];
                                method.start_line = method_json["start_line"];
                                method.end_line = method_json["end_line"];
                                method.complexity.cyclomatic_complexity = method_json["complexity"];
                                method.complexity.update_rating();
                                if (method_json.contains("parameters")) {
                                    method.parameters = method_json["parameters"];
                                }
                                cls.methods.push_back(method);
                            }
                        }
                        
                        // メンバ変数復元
                        if (class_json.contains("member_variables")) {
                            for (const auto& var_json : class_json["member_variables"]) {
                                MemberVariable var;
                                var.name = var_json["name"];
                                var.type = var_json["type"];
                                var.declaration_line = var_json["declaration_line"];
                                var.is_static = var_json.value("is_static", false);
                                var.is_const = var_json.value("is_const", false);
                                var.access_modifier = var_json.value("access_modifier", "private");
                                
                                // Phase2の情報があれば復元
                                if (var_json.contains("used_by_methods")) {
                                    var.used_by_methods = var_json["used_by_methods"].get<std::vector<std::string>>();
                                }
                                if (var_json.contains("modified_by_methods")) {
                                    var.modified_by_methods = var_json["modified_by_methods"].get<std::vector<std::string>>();
                                }
                                cls.member_variables.push_back(var);
                            }
                        }
                        
                        file_result.classes.push_back(cls);
                    }
                }
                
                // 関数詳細情報復元
                if (file_json.contains("functions")) {
                    for (const auto& func_json : file_json["functions"]) {
                        FunctionInfo func;
                        func.name = func_json["name"];
                        func.start_line = func_json["start_line"];
                        func.end_line = func_json["end_line"];
                        func.complexity.cyclomatic_complexity = func_json["complexity"];
                        func.complexity.update_rating();
                        if (func_json.contains("parameters")) {
                            func.parameters = func_json["parameters"];
                        }
                        file_result.functions.push_back(func);
                    }
                }
                
                // 関数呼び出し情報復元
                if (file_json.contains("function_calls")) {
                    for (const auto& call_json : file_json["function_calls"]) {
                        FunctionCall call;
                        call.function_name = call_json["function_name"];
                        call.line_number = call_json["line_number"];
                        call.is_method_call = call_json["is_method_call"];
                        if (call_json.contains("object_name")) {
                            call.object_name = call_json["object_name"];
                        }
                        file_result.function_calls.push_back(call);
                    }
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
    session.target_path = std::filesystem::absolute(target_path);  // 🔧 絶対パスに変換
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
            result = cmd_stats(session);
        } else if (command == "files") {
            result = cmd_files(session);
        } else if (command == "complexity") {
            result = cmd_complexity(session);
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
                result = cmd_complexity_methods(session, filename);
            } else {
                result = {{"error", "complexity: 使用法: complexity --methods [filename]"}};
            }
        } else if (command == "structure") {
            result = cmd_structure(session);
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
                result = cmd_structure_detailed(session, filename);
            } else if (detailed) {
                result = cmd_structure_detailed(session, "");  // 全ファイル
            } else {
                result = {{"error", "structure: 使用法: structure --detailed [filename]"}};
            }
        } else if (command == "calls") {
            result = cmd_calls(session);
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
                result = cmd_calls_detailed(session, function_name);
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
        } else if (command == "duplicates") {
            result = cmd_duplicates(session);
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
            result = cmd_large_files(session, threshold);
        } else if (command == "todo") {
            result = cmd_todo(session);
        } else if (command == "complexity-ranking") {
            result = cmd_complexity_ranking(session);
        } else if (command.substr(0, 7) == "analyze") {
            // analyze [filename] [--deep]
            std::string args = command.substr(7);
            std::string filename;
            bool deep = false;
            
            // --deep フラグをチェック
            size_t deep_pos = args.find("--deep");
            if (deep_pos != std::string::npos) {
                deep = true;
                // --deepを削除
                args = args.substr(0, deep_pos) + args.substr(deep_pos + 6);
            }
            
            // ファイル名を抽出（先頭と末尾の空白を除去）
            size_t start = args.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t end = args.find_last_not_of(" \t");
                filename = args.substr(start, end - start + 1);
            }
            
            result = cmd_analyze(session, filename, deep);
        } else if (command == "help") {
            result = cmd_help();
        } else {
            result = {
                {"error", "Unknown command: " + command},
                {"available_commands", {"stats", "files", "complexity", 
                                        "structure", "calls", "find <term>", 
                                        "include-graph", "include-cycles", "include-impact",
                                        "include-unused", "include-optimize", "duplicates", 
                                        "large-files", "todo", "complexity-ranking", 
                                        "analyze", "help"}}
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
    std::cerr << "[DEBUG] cmd_complexity called! is_directory=" << session.is_directory << std::endl;
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
    
    // 🔥 ファイルを複雑度順にソート（降順：高い複雑度から表示）
    std::vector<AnalysisResult> sorted_files = session.directory_result.files;
    std::cerr << "[DEBUG] Before sort: files count = " << sorted_files.size() << std::endl;
    if (!sorted_files.empty()) {
        std::cerr << "[DEBUG] First file complexity: " << sorted_files[0].complexity.cyclomatic_complexity << std::endl;
    }
    std::sort(sorted_files.begin(), sorted_files.end(), 
              [](const AnalysisResult& a, const AnalysisResult& b) {
                  return a.complexity.cyclomatic_complexity > b.complexity.cyclomatic_complexity;
              });
    std::cerr << "[DEBUG] After sort: files count = " << sorted_files.size() << std::endl;
    if (!sorted_files.empty()) {
        std::cerr << "[DEBUG] First file complexity after sort: " << sorted_files[0].complexity.cyclomatic_complexity << std::endl;
    }
    
    nlohmann::json complex_files = nlohmann::json::array();
    
    for (const auto& file : sorted_files) {
        complex_files.push_back({
            {"file", file.file_info.name},
            {"complexity", file.complexity.cyclomatic_complexity},
            {"rating", file.complexity.to_string()}
        });
    }
    
    return {
        {"command", "complexity"},
        {"result", complex_files},
        {"debug_info", {
            {"function_called", "cmd_complexity"},
            {"files_count", sorted_files.size()},
            {"first_file_complexity", sorted_files.empty() ? 0 : sorted_files[0].complexity.cyclomatic_complexity}
        }},
        {"summary", "Analyzed " + std::to_string(session.directory_result.files.size()) + 
                    " files for complexity (sorted by complexity, highest first)"}
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

nlohmann::json SessionManager::cmd_analyze(const SessionData& session, 
                                            const std::string& filename, 
                                            bool deep) const {
    nlohmann::json result;
    result["command"] = "analyze";
    result["mode"] = deep ? "deep" : "basic";
    
    // 単一ファイルモード
    if (!session.is_directory) {
        return analyze_file(session.single_file_result, deep);
    }
    
    // ディレクトリモード
    if (filename.empty()) {
        // 全ファイルのサマリー
        return analyze_directory(session.directory_result, deep);
    } else {
        // 特定ファイルの分析
        for (const auto& file : session.directory_result.files) {
            if (file.file_info.name == filename || 
                file.file_info.path.filename() == filename) {
                return analyze_file(file, deep);
            }
        }
        result["error"] = "ファイル '" + filename + "' が見つかりません";
        return result;
    }
}

nlohmann::json SessionManager::analyze_file(const AnalysisResult& file, bool deep) const {
    nlohmann::json result;
    result["command"] = "analyze";
    result["mode"] = deep ? "deep" : "basic";
    result["file"] = file.file_info.name;
    result["total_lines"] = file.file_info.total_lines;
    
    // クラス別統計
    nlohmann::json classes_json = nlohmann::json::array();
    for (const auto& cls : file.classes) {
        nlohmann::json class_json;
        class_json["name"] = cls.name;
        class_json["lines"] = cls.end_line - cls.start_line;
        
        // 基本メトリクス計算
        ClassMetrics metrics;
        metrics.member_variable_count = cls.member_variables.size();
        metrics.method_count = cls.methods.size();
        metrics.total_lines = cls.end_line - cls.start_line;
        metrics.calculate_responsibility();
        
        class_json["metrics"] = {
            {"member_variables", metrics.member_variable_count},
            {"methods", metrics.method_count},
            {"responsibility_score", metrics.responsibility_score},
            {"lines", metrics.total_lines}
        };
        
        // 変数リスト（基本情報のみ）
        nlohmann::json vars_json = nlohmann::json::array();
        for (const auto& var : cls.member_variables) {
            vars_json.push_back({
                {"name", var.name},
                {"type", var.type},
                {"line", var.declaration_line},
                {"access", var.access_modifier}
            });
        }
        class_json["member_variables"] = vars_json;
        
        // 判定
        if (metrics.responsibility_score > 500) {
            class_json["warning"] = "責務が大きすぎる可能性があります";
        }
        
        classes_json.push_back(class_json);
    }
    
    result["classes"] = classes_json;
    
    // ファイル全体のサマリー
    int total_classes = 0;
    int total_member_vars = 0;
    int total_methods = 0;
    int max_responsibility = 0;
    std::string most_complex_class;
    
    for (const auto& cls : file.classes) {
        total_classes++;
        total_member_vars += cls.member_variables.size();
        total_methods += cls.methods.size();
        
        int responsibility = cls.member_variables.size() * cls.methods.size();
        if (responsibility > max_responsibility) {
            max_responsibility = responsibility;
            most_complex_class = cls.name;
        }
    }
    
    result["summary"] = {
        {"total_classes", total_classes},
        {"total_member_variables", total_member_vars},
        {"total_methods", total_methods},
        {"highest_responsibility", max_responsibility},
        {"most_complex_class", most_complex_class}
    };
    
    if (deep) {
        // Phase 2の詳細解析を追加（将来実装）
        result["deep_analysis"] = {
            {"message", "詳細解析機能は後日実装予定です"}
        };
    }
    
    return result;
}

nlohmann::json SessionManager::analyze_directory(const DirectoryAnalysis& dir_result, bool deep) const {
    nlohmann::json result;
    result["command"] = "analyze";
    result["mode"] = deep ? "deep" : "basic"; 
    result["type"] = "directory";
    result["total_files"] = dir_result.summary.total_files;
    
    // 各ファイルの統計を集計
    nlohmann::json files_json = nlohmann::json::array();
    int total_classes = 0;
    int total_member_vars = 0;
    int total_methods = 0;
    int max_responsibility = 0;
    std::string most_complex_file;
    std::string most_complex_class;
    
    for (const auto& file : dir_result.files) {
        nlohmann::json file_summary;
        file_summary["name"] = file.file_info.name;
        file_summary["classes"] = file.classes.size();
        
        int file_member_vars = 0;
        int file_methods = 0;
        int file_max_responsibility = 0;
        std::string file_complex_class;
        
        for (const auto& cls : file.classes) {
            total_classes++;
            file_member_vars += cls.member_variables.size();
            file_methods += cls.methods.size();
            total_member_vars += cls.member_variables.size();
            total_methods += cls.methods.size();
            
            int responsibility = cls.member_variables.size() * cls.methods.size();
            if (responsibility > file_max_responsibility) {
                file_max_responsibility = responsibility;
                file_complex_class = cls.name;
            }
            if (responsibility > max_responsibility) {
                max_responsibility = responsibility;
                most_complex_file = file.file_info.name;
                most_complex_class = cls.name;
            }
        }
        
        file_summary["member_variables"] = file_member_vars;
        file_summary["methods"] = file_methods;
        file_summary["max_responsibility"] = file_max_responsibility;
        
        if (file_max_responsibility > 500) {
            file_summary["warning"] = "高責務クラスが含まれています: " + file_complex_class;
        }
        
        files_json.push_back(file_summary);
    }
    
    result["files"] = files_json;
    result["summary"] = {
        {"total_classes", total_classes},
        {"total_member_variables", total_member_vars},
        {"total_methods", total_methods},
        {"highest_responsibility", max_responsibility},
        {"most_complex_file", most_complex_file},
        {"most_complex_class", most_complex_class}
    };
    
    // 警告メッセージ
    if (max_responsibility > 1000) {
        result["warning"] = "⚠️ 非常に高い責務を持つクラスが検出されました";
    } else if (max_responsibility > 500) {
        result["info"] = "責務が大きいクラスが検出されました";
    }
    
    return result;
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
            {"duplicates", "Find duplicate or backup files"},
            {"large-files", "Show files over threshold lines (default 500)"},
            {"large-files --threshold 1000", "Show files over 1000 lines"},
            {"todo", "Find TODO/FIXME/XXX comments in code"},
            {"complexity-ranking", "Show functions ranked by complexity"},
            {"analyze", "Analyze class responsibility (member vars × methods)"},
            {"analyze <file>", "Analyze specific file's class responsibility"},
            {"analyze <file> --deep", "Deep analysis with usage patterns (Phase 2)"},
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

nlohmann::json SessionManager::cmd_duplicates(const SessionData& session) const {
    if (!session.is_directory) {
        return {
            {"command", "duplicates"},
            {"error", "Duplicates analysis is only available for directory sessions"},
            {"hint", "Create a session with a directory path to use this feature"}
        };
    }
    
    // 重複候補パターン
    const std::vector<std::string> duplicate_patterns = {
        "_backup", "_Backup", "_BACKUP",
        "_Fixed", "_fixed", "_FIXED",
        "_Original", "_original", "_ORIGINAL",
        "_old", "_Old", "_OLD",
        "_copy", "_Copy", "_COPY",
        "_v2", "_v3", "_v4", "_v5", "_v6", "_v7", "_v8", "_v9",
        "_v10", "_v11", "_v12", "_v13", "_v14", "_v15", "_v16", "_v17", "_v18", "_v19",
        "_v20", "_v21", "_v22", "_v23", "_v24", "_v25",
        "_tmp", "_temp", "_test",
        ".bak", ".backup", ".old", ".orig"
    };
    
    nlohmann::json duplicates_json;
    duplicates_json["command"] = "duplicates";
    duplicates_json["duplicates"] = nlohmann::json::array();
    
    std::map<std::string, std::vector<const AnalysisResult*>> base_name_to_files;
    
    // ファイルをベース名でグループ化
    for (const auto& file : session.directory_result.files) {
        std::string filename = file.file_info.name;
        std::string base_name = filename;
        
        // パターンを削除してベース名を取得
        for (const auto& pattern : duplicate_patterns) {
            size_t pos = base_name.rfind(pattern);
            if (pos != std::string::npos) {
                // 拡張子を保持
                size_t ext_pos = filename.rfind('.');
                if (ext_pos != std::string::npos && ext_pos > pos) {
                    base_name = filename.substr(0, pos) + filename.substr(ext_pos);
                } else {
                    base_name = filename.substr(0, pos);
                }
                break;
            }
        }
        
        base_name_to_files[base_name].push_back(&file);
    }
    
    // 重複候補を検出
    for (const auto& [base_name, files] : base_name_to_files) {
        if (files.size() > 1) {
            // 最大の行数を持つファイルを見つける
            const AnalysisResult* primary_file = files[0];
            for (const auto* file : files) {
                if (file->file_info.total_lines > primary_file->file_info.total_lines) {
                    primary_file = file;
                }
            }
            
            nlohmann::json duplicate_group;
            duplicate_group["primary_file"] = {
                {"name", primary_file->file_info.name},
                {"lines", primary_file->file_info.total_lines},
                {"size", primary_file->file_info.size_bytes}
            };
            
            duplicate_group["duplicates"] = nlohmann::json::array();
            
            for (const auto* file : files) {
                if (file != primary_file) {
                    // 類似度を計算（簡易版：行数の比率）
                    double similarity = 0.0;
                    if (primary_file->file_info.total_lines > 0) {
                        similarity = static_cast<double>(file->file_info.total_lines) / 
                                   static_cast<double>(primary_file->file_info.total_lines) * 100.0;
                    }
                    
                    duplicate_group["duplicates"].push_back({
                        {"name", file->file_info.name},
                        {"lines", file->file_info.total_lines},
                        {"size", file->file_info.size_bytes},
                        {"similarity", std::round(similarity)}
                    });
                }
            }
            
            if (!duplicate_group["duplicates"].empty()) {
                duplicates_json["duplicates"].push_back(duplicate_group);
            }
        }
    }
    
    // サマリー
    size_t total_duplicate_files = 0;
    size_t total_duplicate_size = 0;
    
    for (const auto& group : duplicates_json["duplicates"]) {
        for (const auto& dup : group["duplicates"]) {
            total_duplicate_files++;
            total_duplicate_size += dup["size"].get<size_t>();
        }
    }
    
    duplicates_json["summary"] = {
        {"duplicate_groups", duplicates_json["duplicates"].size()},
        {"total_duplicate_files", total_duplicate_files},
        {"total_duplicate_size", total_duplicate_size},
        {"size_human", total_duplicate_size > 1024*1024 ? 
            std::to_string(total_duplicate_size / (1024*1024)) + " MB" :
            std::to_string(total_duplicate_size / 1024) + " KB"}
    };
    
    if (duplicates_json["duplicates"].empty()) {
        duplicates_json["message"] = "No duplicate files found! 🎉";
    } else {
        duplicates_json["message"] = "Found " + std::to_string(total_duplicate_files) + 
                                    " potential duplicate files (" + 
                                    duplicates_json["summary"]["size_human"].get<std::string>() + ")";
    }
    
    return duplicates_json;
}

//=============================================================================
// 🔍 詳細構造解析コマンド - Claude Code君向け
//=============================================================================

nlohmann::json SessionManager::cmd_structure_detailed(const SessionData& session, const std::string& filename) const {
    nlohmann::json result_json;
    result_json["command"] = "structure-detailed";
    
    if (!session.is_directory) {
        // 単一ファイルの場合
        const auto& analysis = session.single_file_result;
        result_json["file"] = analysis.file_info.name;
        result_json["classes"] = nlohmann::json::array();
        
        // クラス詳細情報
        for (const auto& cls : analysis.classes) {
            nlohmann::json class_json;
            class_json["name"] = cls.name;
            class_json["line_start"] = cls.start_line;
            class_json["line_end"] = cls.end_line;
            class_json["parent_class"] = cls.parent_class;
            
            // クラス全体の複雑度（メソッドの合計）
            uint32_t total_complexity = 0;
            nlohmann::json methods_json = nlohmann::json::array();
            
            for (const auto& method : cls.methods) {
                nlohmann::json method_json;
                method_json["name"] = method.name;
                method_json["line"] = method.start_line;  
                method_json["line_end"] = method.end_line;
                method_json["complexity"] = method.complexity.cyclomatic_complexity;
                method_json["parameters"] = method.parameters;
                
                // 関数呼び出し情報を追加（calls）
                nlohmann::json calls_json = nlohmann::json::array();
                for (const auto& call : analysis.function_calls) {
                    if (call.line_number >= method.start_line && call.line_number <= method.end_line) {
                        calls_json.push_back({
                            {"function", call.function_name},
                            {"line", call.line_number}
                        });
                    }
                }
                method_json["calls"] = calls_json;
                
                // called_from は計算が複雑なので将来実装
                method_json["called_from"] = nlohmann::json::array();
                
                methods_json.push_back(method_json);
                total_complexity += method.complexity.cyclomatic_complexity;
            }
            
            class_json["complexity"] = total_complexity;
            class_json["methods"] = methods_json;
            result_json["classes"].push_back(class_json);
        }
        
        // 自由関数（クラス外の関数）
        nlohmann::json functions_json = nlohmann::json::array();
        for (const auto& func : analysis.functions) {
            nlohmann::json func_json;
            func_json["name"] = func.name;
            func_json["line"] = func.start_line;
            func_json["line_end"] = func.end_line;
            func_json["complexity"] = func.complexity.cyclomatic_complexity;
            func_json["parameters"] = func.parameters;
            
            // 関数呼び出し情報
            nlohmann::json calls_json = nlohmann::json::array();
            for (const auto& call : analysis.function_calls) {
                if (call.line_number >= func.start_line && call.line_number <= func.end_line) {
                    calls_json.push_back({
                        {"function", call.function_name},
                        {"line", call.line_number}
                    });
                }
            }
            func_json["calls"] = calls_json;
            func_json["called_from"] = nlohmann::json::array();
            
            functions_json.push_back(func_json);
        }
        result_json["functions"] = functions_json;
        
    } else {
        // ディレクトリの場合：指定されたファイルを検索
        if (filename.empty()) {
            result_json["error"] = "ディレクトリ解析では --detailed にファイル名を指定してください";
            return result_json;
        }
        
        // 指定されたファイルを検索
        bool found = false;
        for (const auto& file_result : session.directory_result.files) {
            if (file_result.file_info.name == filename || 
                file_result.file_info.path.filename() == filename) {
                // 見つかったファイルの詳細情報を出力
                result_json["file"] = file_result.file_info.name;
                result_json["classes"] = nlohmann::json::array();
                
                // クラス詳細情報
                for (const auto& cls : file_result.classes) {
                    nlohmann::json class_json;
                    class_json["name"] = cls.name;
                    class_json["line_start"] = cls.start_line;
                    class_json["line_end"] = cls.end_line;
                    class_json["parent_class"] = cls.parent_class;
                    
                    // クラス全体の複雑度（メソッドの合計）
                    uint32_t total_complexity = 0;
                    nlohmann::json methods_json = nlohmann::json::array();
                    
                    for (const auto& method : cls.methods) {
                        nlohmann::json method_json;
                        method_json["name"] = method.name;
                        method_json["line"] = method.start_line;  
                        method_json["line_end"] = method.end_line;
                        method_json["complexity"] = method.complexity.cyclomatic_complexity;
                        method_json["parameters"] = method.parameters;
                        
                        // 関数呼び出し情報を追加（calls）
                        nlohmann::json calls_json = nlohmann::json::array();
                        for (const auto& call : file_result.function_calls) {
                            if (call.line_number >= method.start_line && call.line_number <= method.end_line) {
                                calls_json.push_back({
                                    {"function", call.function_name},
                                    {"line", call.line_number}
                                });
                            }
                        }
                        method_json["calls"] = calls_json;
                        
                        // called_from は計算が複雑なので将来実装
                        method_json["called_from"] = nlohmann::json::array();
                        
                        methods_json.push_back(method_json);
                        total_complexity += method.complexity.cyclomatic_complexity;
                    }
                    
                    class_json["complexity"] = total_complexity;
                    class_json["methods"] = methods_json;
                    result_json["classes"].push_back(class_json);
                }
                
                // 自由関数（クラス外の関数）
                nlohmann::json functions_json = nlohmann::json::array();
                for (const auto& func : file_result.functions) {
                    nlohmann::json func_json;
                    func_json["name"] = func.name;
                    func_json["line"] = func.start_line;
                    func_json["line_end"] = func.end_line;
                    func_json["complexity"] = func.complexity.cyclomatic_complexity;
                    func_json["parameters"] = func.parameters;
                    
                    // 関数呼び出し情報
                    nlohmann::json calls_json = nlohmann::json::array();
                    for (const auto& call : file_result.function_calls) {
                        if (call.line_number >= func.start_line && call.line_number <= func.end_line) {
                            calls_json.push_back({
                                {"function", call.function_name},
                                {"line", call.line_number}
                            });
                        }
                    }
                    func_json["calls"] = calls_json;
                    func_json["called_from"] = nlohmann::json::array();
                    
                    functions_json.push_back(func_json);
                }
                result_json["functions"] = functions_json;
                
                found = true;
                break;
            }
        }
        
        if (!found) {
            result_json["error"] = "指定されたファイル '" + filename + "' が見つかりません";
            return result_json;
        }
    }
    
    result_json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
        
    return result_json;
}

//=============================================================================
// 📊 ファイル別メソッド複雑度ランキング - Claude Code君向け
//=============================================================================

nlohmann::json SessionManager::cmd_complexity_methods(const SessionData& session, const std::string& filename) const {
    nlohmann::json result_json;
    result_json["command"] = "complexity-methods";
    result_json["file"] = filename.empty() ? "all" : filename;
    result_json["method_complexity"] = nlohmann::json::array();
    
    // メソッドの複雑度情報を集める
    std::vector<std::tuple<std::string, std::string, uint32_t, uint32_t, std::string>> method_complexities;
    
    if (!session.is_directory) {
        // 単一ファイルの場合
        const auto& analysis = session.single_file_result;
        std::string file_name = analysis.file_info.name;
        
        // ファイル名フィルタ
        if (!filename.empty() && file_name != filename && 
            analysis.file_info.path.filename() != filename) {
            result_json["error"] = "指定されたファイル '" + filename + "' が見つかりません";
            return result_json;
        }
        
        // クラスメソッドを収集
        for (const auto& cls : analysis.classes) {
            for (const auto& method : cls.methods) {
                std::string reason = "Method in class " + cls.name;
                if (method.complexity.cyclomatic_complexity > 20) {
                    reason += " (High complexity - consider refactoring)";
                } else if (method.complexity.cyclomatic_complexity > 10) {
                    reason += " (Moderate complexity)";
                } else {
                    reason += " (Low complexity)";
                }
                
                method_complexities.emplace_back(
                    cls.name + "::" + method.name,
                    file_name,
                    method.complexity.cyclomatic_complexity,
                    method.start_line,
                    reason
                );
            }
        }
        
        // 自由関数を収集
        for (const auto& func : analysis.functions) {
            std::string reason = "Free function";
            if (func.complexity.cyclomatic_complexity > 20) {
                reason += " (High complexity - consider refactoring)";
            } else if (func.complexity.cyclomatic_complexity > 10) {
                reason += " (Moderate complexity)";
            } else {
                reason += " (Low complexity)";
            }
            
            method_complexities.emplace_back(
                func.name,
                file_name,
                func.complexity.cyclomatic_complexity,
                func.start_line,
                reason
            );
        }
        
    } else {
        // ディレクトリの場合
        for (const auto& file_result : session.directory_result.files) {
            std::string file_name = file_result.file_info.name;
            
            // ファイル名フィルタ
            if (!filename.empty() && file_name != filename && 
                file_result.file_info.path.filename() != filename) {
                continue;
            }
            
            // クラスメソッドを収集
            for (const auto& cls : file_result.classes) {
                for (const auto& method : cls.methods) {
                    std::string reason = "Method in class " + cls.name;
                    if (method.complexity.cyclomatic_complexity > 20) {
                        reason += " (High complexity - consider refactoring)";
                    } else if (method.complexity.cyclomatic_complexity > 10) {
                        reason += " (Moderate complexity)";
                    } else {
                        reason += " (Low complexity)";
                    }
                    
                    method_complexities.emplace_back(
                        cls.name + "::" + method.name,
                        file_name,
                        method.complexity.cyclomatic_complexity,
                        method.start_line,
                        reason
                    );
                }
            }
            
            // 自由関数を収集
            for (const auto& func : file_result.functions) {
                std::string reason = "Free function";
                if (func.complexity.cyclomatic_complexity > 20) {
                    reason += " (High complexity - consider refactoring)";
                } else if (func.complexity.cyclomatic_complexity > 10) {
                    reason += " (Moderate complexity)";
                } else {
                    reason += " (Low complexity)";
                }
                
                method_complexities.emplace_back(
                    func.name,
                    file_name,
                    func.complexity.cyclomatic_complexity,
                    func.start_line,
                    reason
                );
            }
        }
    }
    
    // 複雑度でソート（降順）
    std::sort(method_complexities.begin(), method_complexities.end(),
              [](const auto& a, const auto& b) {
                  return std::get<2>(a) > std::get<2>(b);
              });
    
    // JSON出力
    for (const auto& method : method_complexities) {
        result_json["method_complexity"].push_back({
            {"method", std::get<0>(method)},
            {"file", std::get<1>(method)},
            {"complexity", std::get<2>(method)},
            {"line", std::get<3>(method)},
            {"reason", std::get<4>(method)}
        });
    }
    
    // 統計情報
    result_json["summary"] = {
        {"total_methods", method_complexities.size()},
        {"high_complexity", std::count_if(method_complexities.begin(), method_complexities.end(),
                                         [](const auto& m) { return std::get<2>(m) > 20; })},
        {"moderate_complexity", std::count_if(method_complexities.begin(), method_complexities.end(),
                                            [](const auto& m) { return std::get<2>(m) > 10 && std::get<2>(m) <= 20; })},
        {"low_complexity", std::count_if(method_complexities.begin(), method_complexities.end(),
                                       [](const auto& m) { return std::get<2>(m) <= 10; })}
    };
    
    result_json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return result_json;
}

//=============================================================================
// 📞 関数呼び出し詳細解析 - Claude Code君向け
//=============================================================================

nlohmann::json SessionManager::cmd_calls_detailed(const SessionData& session, const std::string& function_name) const {
    nlohmann::json result_json;
    result_json["command"] = "calls-detailed";
    result_json["function"] = function_name;
    result_json["calls"] = nlohmann::json::array();
    result_json["called_by"] = nlohmann::json::array();
    
    // 関数の基本情報を検索
    bool function_found = false;
    nlohmann::json function_info;
    
    auto analyze_calls_for_file = [&](const AnalysisResult& analysis, const std::string& file_name) {
        // クラスメソッドを検索
        for (const auto& cls : analysis.classes) {
            for (const auto& method : cls.methods) {
                std::string full_method_name = cls.name + "::" + method.name;
                
                if (method.name == function_name || full_method_name == function_name) {
                    function_found = true;
                    function_info = {
                        {"name", full_method_name},
                        {"file", file_name},
                        {"line", method.start_line},
                        {"line_end", method.end_line},
                        {"complexity", method.complexity.cyclomatic_complexity},
                        {"class", cls.name},
                        {"type", "method"},
                        {"parameters", method.parameters}
                    };
                    
                    // この関数が呼び出している関数を収集
                    for (const auto& call : analysis.function_calls) {
                        if (call.line_number >= method.start_line && call.line_number <= method.end_line) {
                            result_json["calls"].push_back({
                                {"function", call.function_name},
                                {"file", file_name},
                                {"line", call.line_number}
                            });
                        }
                    }
                }
            }
        }
        
        // 自由関数を検索
        for (const auto& func : analysis.functions) {
            if (func.name == function_name) {
                function_found = true;
                function_info = {
                    {"name", func.name},
                    {"file", file_name},
                    {"line", func.start_line},
                    {"line_end", func.end_line},
                    {"complexity", func.complexity.cyclomatic_complexity},
                    {"type", "function"},
                    {"parameters", func.parameters}
                };
                
                // この関数が呼び出している関数を収集
                for (const auto& call : analysis.function_calls) {
                    if (call.line_number >= func.start_line && call.line_number <= func.end_line) {
                        result_json["calls"].push_back({
                            {"function", call.function_name},
                            {"file", file_name},
                            {"line", call.line_number}
                        });
                    }
                }
            }
        }
        
        // called_by の解析（この関数を呼び出している箇所を検索）
        for (const auto& call : analysis.function_calls) {
            if (call.function_name == function_name || 
                call.function_name.find("::" + function_name) != std::string::npos) {
                
                // 呼び出し元の関数を特定（簡易実装）
                std::string caller_function = "unknown";
                
                // クラスメソッド内かチェック
                for (const auto& cls : analysis.classes) {
                    for (const auto& method : cls.methods) {
                        if (call.line_number >= method.start_line && call.line_number <= method.end_line) {
                            caller_function = cls.name + "::" + method.name;
                            break;
                        }
                    }
                    if (caller_function != "unknown") break;
                }
                
                // 自由関数内かチェック
                if (caller_function == "unknown") {
                    for (const auto& func : analysis.functions) {
                        if (call.line_number >= func.start_line && call.line_number <= func.end_line) {
                            caller_function = func.name;
                            break;
                        }
                    }
                }
                
                result_json["called_by"].push_back({
                    {"function", caller_function},
                    {"file", file_name},
                    {"line", call.line_number}
                });
            }
        }
    };
    
    if (!session.is_directory) {
        // 単一ファイルの場合
        analyze_calls_for_file(session.single_file_result, session.single_file_result.file_info.name);
    } else {
        // ディレクトリの場合：全ファイルを検索
        for (const auto& file_result : session.directory_result.files) {
            analyze_calls_for_file(file_result, file_result.file_info.name);
        }
    }
    
    if (!function_found) {
        result_json["error"] = "指定された関数 '" + function_name + "' が見つかりません";
        return result_json;
    }
    
    result_json["function_info"] = function_info;
    
    // 統計情報
    result_json["summary"] = {
        {"total_calls", result_json["calls"].size()},
        {"called_by_count", result_json["called_by"].size()},
        {"complexity", function_info.value("complexity", 0)}
    };
    
    // 分割提案（基本的な情報のみ）
    result_json["refactoring_suggestion"] = {
        {"complexity_level", function_info.value("complexity", 0) > 20 ? "high" : 
                           (function_info.value("complexity", 0) > 10 ? "moderate" : "low")},
        {"suggestion", function_info.value("complexity", 0) > 20 ? 
                      "High complexity - consider breaking into smaller functions" :
                      "Complexity is acceptable"}
    };
    
    result_json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return result_json;
}

nlohmann::json SessionManager::cmd_large_files(const SessionData& session, int threshold) const {
    if (!session.is_directory) {
        // 単一ファイルの場合も対応
        if (session.single_file_result.file_info.total_lines >= threshold) {
            return {
                {"command", "large-files"},
                {"threshold", threshold},
                {"files", {{
                    {"name", session.single_file_result.file_info.name},
                    {"lines", session.single_file_result.file_info.total_lines},
                    {"size", session.single_file_result.file_info.size_bytes},
                    {"complexity", session.single_file_result.complexity.cyclomatic_complexity},
                    {"rating", session.single_file_result.complexity.to_string()}
                }}},
                {"summary", "1 file over " + std::to_string(threshold) + " lines"}
            };
        } else {
            return {
                {"command", "large-files"},
                {"threshold", threshold},
                {"files", nlohmann::json::array()},
                {"summary", "No files over " + std::to_string(threshold) + " lines"}
            };
        }
    }
    
    nlohmann::json large_files_json;
    large_files_json["command"] = "large-files";
    large_files_json["threshold"] = threshold;
    large_files_json["files"] = nlohmann::json::array();
    
    // 大きいファイルを複雑度順にソート
    std::vector<const AnalysisResult*> large_files;
    for (const auto& file : session.directory_result.files) {
        if (file.file_info.total_lines >= threshold) {
            large_files.push_back(&file);
        }
    }
    
    // 複雑度順にソート（高い順）
    std::sort(large_files.begin(), large_files.end(),
              [](const AnalysisResult* a, const AnalysisResult* b) {
                  return a->complexity.cyclomatic_complexity > b->complexity.cyclomatic_complexity;
              });
    
    // 結果を構築
    for (const auto* file : large_files) {
        // 複雑度に基づく絵文字を選択
        std::string emoji;
        if (file->complexity.cyclomatic_complexity > 100) {
            emoji = "🔴";  // 非常に高い複雑度
        } else if (file->complexity.cyclomatic_complexity > 50) {
            emoji = "🟠";  // 高い複雑度
        } else if (file->complexity.cyclomatic_complexity > 20) {
            emoji = "🟡";  // 中程度の複雑度
        } else {
            emoji = "🟢";  // 低い複雑度
        }
        
        large_files_json["files"].push_back({
            {"name", file->file_info.name},
            {"lines", file->file_info.total_lines},
            {"size", file->file_info.size_bytes},
            {"complexity", file->complexity.cyclomatic_complexity},
            {"rating", file->complexity.to_string()},
            {"emoji", emoji},
            {"functions", file->stats.function_count},
            {"classes", file->stats.class_count}
        });
    }
    
    // サマリー統計
    if (large_files.empty()) {
        large_files_json["summary"] = "No files over " + std::to_string(threshold) + " lines";
    } else {
        size_t total_lines = 0;
        size_t total_complexity = 0;
        size_t high_complexity_count = 0;
        
        for (const auto* file : large_files) {
            total_lines += file->file_info.total_lines;
            total_complexity += file->complexity.cyclomatic_complexity;
            if (file->complexity.cyclomatic_complexity > 50) {
                high_complexity_count++;
            }
        }
        
        large_files_json["summary"] = "Found " + std::to_string(large_files.size()) + 
                                     " files over " + std::to_string(threshold) + " lines";
        large_files_json["statistics"] = {
            {"total_large_files", large_files.size()},
            {"total_lines", total_lines},
            {"average_complexity", large_files.empty() ? 0 : total_complexity / large_files.size()},
            {"high_complexity_files", high_complexity_count}
        };
        
        if (high_complexity_count > 0) {
            large_files_json["warning"] = "⚠️  " + std::to_string(high_complexity_count) + 
                                         " files have high complexity and should be refactored";
        }
    }
    
    return large_files_json;
}

nlohmann::json SessionManager::cmd_todo(const SessionData& session) const {
    nlohmann::json todo_json;
    todo_json["command"] = "todo";
    todo_json["todos"] = nlohmann::json::array();
    
    // TODOパターン
    const std::vector<std::string> todo_patterns = {
        "TODO", "FIXME", "XXX", "HACK", "BUG", "OPTIMIZE",
        "REFACTOR", "NOTE", "REVIEW", "QUESTION"
    };
    
    // ファイルを処理
    std::vector<FileInfo> files;
    if (session.is_directory) {
        for (const auto& file : session.directory_result.files) {
            files.push_back(file.file_info);
        }
    } else {
        files.push_back(session.single_file_result.file_info);
    }
    
    size_t total_todos = 0;
    std::map<std::string, size_t> type_count;
    
    // 各ファイルを検索
    for (const auto& file_info : files) {
        std::ifstream file(file_info.path);
        if (!file.is_open()) continue;
        
        std::string line;
        int line_number = 0;
        
        while (std::getline(file, line)) {
            line_number++;
            
            // 各パターンを検索
            for (const auto& pattern : todo_patterns) {
                size_t pos = line.find(pattern);
                if (pos != std::string::npos) {
                    // パターンの後に:またはスペースがあることを確認（誤検出防止）
                    if (pos + pattern.length() < line.length()) {
                        char next_char = line[pos + pattern.length()];
                        if (next_char != ':' && next_char != ' ' && next_char != '(' && next_char != '\t') {
                            continue;
                        }
                    }
                    
                    // TODOの内容を抽出
                    size_t content_start = pos + pattern.length();
                    while (content_start < line.length() && 
                           (line[content_start] == ':' || line[content_start] == ' ' || line[content_start] == '\t')) {
                        content_start++;
                    }
                    
                    std::string content = line.substr(content_start);
                    
                    // 行の前後の空白を削除
                    size_t first = content.find_first_not_of(" \t");
                    size_t last = content.find_last_not_of(" \t");
                    if (first != std::string::npos && last != std::string::npos) {
                        content = content.substr(first, last - first + 1);
                    }
                    
                    // 緊急度を判定
                    std::string priority = "normal";
                    if (pattern == "FIXME" || pattern == "BUG") {
                        priority = "high";
                    } else if (pattern == "HACK" || pattern == "XXX") {
                        priority = "medium";
                    }
                    
                    todo_json["todos"].push_back({
                        {"file", file_info.name},
                        {"line", line_number},
                        {"type", pattern},
                        {"content", content},
                        {"priority", priority},
                        {"full_line", line}
                    });
                    
                    total_todos++;
                    type_count[pattern]++;
                    
                    break; // 1行に複数のパターンがある場合は最初のものだけ
                }
            }
        }
    }
    
    // 統計情報
    todo_json["summary"] = {
        {"total_todos", total_todos},
        {"files_with_todos", files.size()}
    };
    
    // タイプ別カウント
    todo_json["by_type"] = nlohmann::json::object();
    for (const auto& [type, count] : type_count) {
        todo_json["by_type"][type] = count;
    }
    
    // 優先度別にグループ化
    nlohmann::json high_priority = nlohmann::json::array();
    nlohmann::json medium_priority = nlohmann::json::array();
    nlohmann::json normal_priority = nlohmann::json::array();
    
    for (const auto& todo : todo_json["todos"]) {
        if (todo["priority"] == "high") {
            high_priority.push_back(todo);
        } else if (todo["priority"] == "medium") {
            medium_priority.push_back(todo);
        } else {
            normal_priority.push_back(todo);
        }
    }
    
    todo_json["grouped"] = {
        {"high", high_priority},
        {"medium", medium_priority},
        {"normal", normal_priority}
    };
    
    // メッセージ
    if (total_todos == 0) {
        todo_json["message"] = "No TODO comments found! 🎉";
    } else {
        todo_json["message"] = "Found " + std::to_string(total_todos) + " TODO comments";
        if (high_priority.size() > 0) {
            todo_json["warning"] = "⚠️  " + std::to_string(high_priority.size()) + 
                                  " high priority items need attention (FIXME/BUG)";
        }
    }
    
    return todo_json;
}

nlohmann::json SessionManager::cmd_complexity_ranking(const SessionData& session) const {
    nlohmann::json ranking_json;
    ranking_json["command"] = "complexity-ranking";
    ranking_json["functions"] = nlohmann::json::array();
    
    // すべての関数を収集
    struct FunctionComplexity {
        std::string file_name;
        std::string function_name;
        int line_number;
        int complexity;
        std::string language;
    };
    
    std::vector<FunctionComplexity> all_functions;
    
    if (session.is_directory) {
        for (const auto& file : session.directory_result.files) {
            // 各ファイルの関数情報がない場合はスキップ
            // 注：現在の実装では関数レベルの複雑度は保存されていないため、
            // ファイル全体の複雑度を関数数で割った推定値を使用
            if (file.stats.function_count > 0) {
                // 推定値：ファイル全体の複雑度を関数数で割る
                int estimated_complexity_per_function = 
                    file.complexity.cyclomatic_complexity / file.stats.function_count;
                
                // 仮の関数エントリを作成（将来的には実際の関数データを使用）
                for (uint32_t i = 0; i < file.stats.function_count && i < 10; i++) {
                    FunctionComplexity func;
                    func.file_name = file.file_info.name;
                    func.function_name = "function_" + std::to_string(i + 1);
                    func.line_number = 0;  // 不明
                    func.complexity = estimated_complexity_per_function;
                    
                    // 言語の判定（C++17互換）
                    std::string name = file.file_info.name;
                    size_t pos = name.find_last_of('.');
                    std::string ext = (pos != std::string::npos) ? name.substr(pos) : "";
                    
                    if (ext == ".cpp" || ext == ".hpp" || ext == ".cc" || ext == ".h") {
                        func.language = "C++";
                    } else if (ext == ".js") {
                        func.language = "JavaScript";
                    } else if (ext == ".ts") {
                        func.language = "TypeScript";
                    } else if (ext == ".py") {
                        func.language = "Python";
                    } else if (ext == ".cs") {
                        func.language = "C#";
                    } else if (ext == ".go") {
                        func.language = "Go";
                    } else if (ext == ".rs") {
                        func.language = "Rust";
                    } else {
                        func.language = "Unknown";
                    }
                    
                    all_functions.push_back(func);
                }
            }
        }
    } else {
        // 単一ファイルの場合
        if (session.single_file_result.stats.function_count > 0) {
            int estimated_complexity = 
                session.single_file_result.complexity.cyclomatic_complexity / 
                session.single_file_result.stats.function_count;
            
            for (uint32_t i = 0; i < session.single_file_result.stats.function_count && i < 10; i++) {
                FunctionComplexity func;
                func.file_name = session.single_file_result.file_info.name;
                func.function_name = "function_" + std::to_string(i + 1);
                func.line_number = 0;
                func.complexity = estimated_complexity;
                func.language = "Unknown";
                all_functions.push_back(func);
            }
        }
    }
    
    // 複雑度でソート（降順）
    std::sort(all_functions.begin(), all_functions.end(),
              [](const FunctionComplexity& a, const FunctionComplexity& b) {
                  return a.complexity > b.complexity;
              });
    
    // 上位50個（または全部）を結果に追加
    size_t limit = std::min(all_functions.size(), size_t(50));
    for (size_t i = 0; i < limit; i++) {
        const auto& func = all_functions[i];
        
        // 複雑度レベルの判定
        std::string level;
        std::string emoji;
        if (func.complexity > 50) {
            level = "Very High";
            emoji = "🔴";
        } else if (func.complexity > 20) {
            level = "High";
            emoji = "🟠";
        } else if (func.complexity > 10) {
            level = "Medium";
            emoji = "🟡";
        } else {
            level = "Low";
            emoji = "🟢";
        }
        
        ranking_json["functions"].push_back({
            {"rank", i + 1},
            {"file", func.file_name},
            {"function", func.function_name},
            {"complexity", func.complexity},
            {"level", level},
            {"emoji", emoji},
            {"language", func.language}
        });
    }
    
    // 統計情報
    if (all_functions.empty()) {
        ranking_json["summary"] = "No functions found for complexity analysis";
    } else {
        int total_complexity = 0;
        int high_complexity_count = 0;
        
        for (const auto& func : all_functions) {
            total_complexity += func.complexity;
            if (func.complexity > 20) {
                high_complexity_count++;
            }
        }
        
        ranking_json["summary"] = {
            {"total_functions", all_functions.size()},
            {"displayed", limit},
            {"average_complexity", all_functions.empty() ? 0 : total_complexity / all_functions.size()},
            {"high_complexity_functions", high_complexity_count}
        };
        
        if (high_complexity_count > 0) {
            ranking_json["warning"] = "⚠️  " + std::to_string(high_complexity_count) + 
                                     " functions have high complexity and should be refactored";
        }
        
        ranking_json["note"] = "Note: Function-level complexity is estimated from file complexity / function count";
    }
    
    return ranking_json;
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