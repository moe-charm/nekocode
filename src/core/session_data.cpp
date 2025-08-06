//=============================================================================
// 📱 Session Data実装 - セッション情報＆JSONシリアライゼーション
//
// SessionManagerから分離したデータ実装
// 責任: セッションデータのJSONシリアライゼーション実装
//=============================================================================

#include "nekocode/session_data.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace nekocode {

//=============================================================================
// 📱 SessionData JSONシリアライゼーション実装
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
        
        // 🌟 関数詳細情報を保存（UniversalFunctionInfo::to_json()使用）
        nlohmann::json functions_json = nlohmann::json::array();
        for (const auto& func : single_file_result.functions) {
            // 🆕 UniversalFunctionInfoのto_json()メソッドを使用
            // これにより自動的にmetadataも保存される！
            nlohmann::json func_json = func.to_json();
            functions_json.push_back(func_json);
        }
        single_file_json["functions"] = functions_json;
        
        j["single_file_result"] = single_file_json;
    }
    
    j["quick_stats"] = quick_stats;
    
    // デッドコード解析結果（存在する場合）
    if (!dead_code_info.is_null()) {
        j["dead_code_info"] = dead_code_info;
    }
    
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
    
    // デッドコード解析結果復元
    if (j.contains("dead_code_info")) {
        data.dead_code_info = j["dead_code_info"];
    }
    
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
        // 単一ファイルの場合の復元
        if (j.contains("single_file_result")) {
            const auto& single = j["single_file_result"];
            
            // ファイル情報復元
            if (single.contains("file_info")) {
                const auto& info = single["file_info"];
                data.single_file_result.file_info.name = info["name"];
                data.single_file_result.file_info.path = info["path"].get<std::string>();
                data.single_file_result.file_info.size_bytes = info["size_bytes"];
                data.single_file_result.file_info.total_lines = info["total_lines"];
                data.single_file_result.file_info.code_lines = info["code_lines"];
                data.single_file_result.file_info.comment_lines = info["comment_lines"];
                data.single_file_result.file_info.empty_lines = info["empty_lines"];
            }
            
            // 統計情報復元
            if (single.contains("stats")) {
                const auto& stats = single["stats"];
                data.single_file_result.stats.class_count = stats["class_count"];
                data.single_file_result.stats.function_count = stats["function_count"];
                data.single_file_result.stats.import_count = stats["import_count"];
                data.single_file_result.stats.export_count = stats["export_count"];
                data.single_file_result.stats.unique_calls = stats["unique_calls"];
                data.single_file_result.stats.total_calls = stats["total_calls"];
            }
            
            // 複雑度情報復元
            if (single.contains("complexity")) {
                const auto& complexity = single["complexity"];
                data.single_file_result.complexity.cyclomatic_complexity = complexity["cyclomatic_complexity"];
                data.single_file_result.complexity.max_nesting_depth = complexity["max_nesting_depth"];
                data.single_file_result.complexity.update_rating();
            }
            
            // クラス情報復元
            if (single.contains("classes")) {
                for (const auto& class_json : single["classes"]) {
                    ClassInfo cls;
                    cls.name = class_json["name"];
                    cls.parent_class = class_json.value("parent_class", "");
                    cls.start_line = class_json["start_line"];
                    cls.end_line = class_json["end_line"];
                    
                    // メソッド情報復元
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
                    
                    // メンバ変数情報復元
                    if (class_json.contains("member_variables")) {
                        for (const auto& var_json : class_json["member_variables"]) {
                            MemberVariable var;
                            var.name = var_json["name"];
                            var.type = var_json["type"];
                            var.declaration_line = var_json["declaration_line"];
                            var.is_static = var_json.value("is_static", false);
                            var.is_const = var_json.value("is_const", false);
                            var.access_modifier = var_json.value("access_modifier", "private");
                            if (var_json.contains("used_by_methods")) {
                                var.used_by_methods = var_json["used_by_methods"].get<std::vector<std::string>>();
                            }
                            if (var_json.contains("modified_by_methods")) {
                                var.modified_by_methods = var_json["modified_by_methods"].get<std::vector<std::string>>();
                            }
                            cls.member_variables.push_back(var);
                        }
                    }
                    
                    data.single_file_result.classes.push_back(cls);
                }
            }
            
            // 関数情報復元  
            if (single.contains("functions")) {
                for (const auto& func_json : single["functions"]) {
                    FunctionInfo func;
                    func.name = func_json["name"];
                    func.start_line = func_json["start_line"];
                    func.end_line = func_json["end_line"];
                    func.complexity.cyclomatic_complexity = func_json["complexity"];
                    func.complexity.update_rating();
                    if (func_json.contains("parameters")) {
                        func.parameters = func_json["parameters"];
                    }
                    func.is_async = func_json.value("is_async", false);
                    func.is_arrow_function = func_json.value("is_arrow_function", false);
                    data.single_file_result.functions.push_back(func);
                }
            }
            
            // 関数呼び出し情報復元
            if (single.contains("function_calls")) {
                for (const auto& call_json : single["function_calls"]) {
                    FunctionCall call;
                    call.function_name = call_json["function_name"];
                    call.line_number = call_json["line_number"];
                    call.is_method_call = call_json.value("is_method_call", false);
                    if (call_json.contains("object_name")) {
                        call.object_name = call_json["object_name"];
                    }
                    data.single_file_result.function_calls.push_back(call);
                }
            }
        }
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
// 🕒 時刻変換ユーティリティ実装
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

} // namespace nekocode