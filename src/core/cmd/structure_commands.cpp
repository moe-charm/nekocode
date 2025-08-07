//=============================================================================
// 🏗️ Structure Commands実装 - 構造解析・関数呼び出し解析コマンド
//=============================================================================

#include "nekocode/session_commands.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <set>
#include <filesystem>

namespace nekocode {

//=============================================================================
// 🏗️ 構造解析コマンド実装
//=============================================================================

nlohmann::json SessionCommands::cmd_structure(const SessionData& session) const {
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

nlohmann::json SessionCommands::cmd_calls(const SessionData& session) const {
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
            {"total_unique_calls", total_unique},
            {"total_calls", total_calls}
        }},
        {"summary", "Total unique calls: " + std::to_string(total_unique) + 
                    ", Total calls: " + std::to_string(total_calls)}
    };
}

nlohmann::json SessionCommands::cmd_complexity_ranking(const SessionData& session) const {
    nlohmann::json ranking_json;
    ranking_json["command"] = "complexity-ranking";
    ranking_json["functions"] = nlohmann::json::array();
    
    // すべての関数を収集
    struct FunctionComplexity {
        std::string filename;
        std::string function_name;
        uint32_t complexity;
        std::string full_name() const {
            return filename + "::" + function_name;
        }
    };
    
    std::vector<FunctionComplexity> all_functions;
    
    if (session.is_directory) {
        for (const auto& file : session.directory_result.files) {
            for (const auto& cls : file.classes) {
                for (const auto& method : cls.methods) {
                    all_functions.push_back({
                        file.file_info.name,
                        cls.name + "::" + method.name,
                        method.complexity.cyclomatic_complexity
                    });
                }
            }
            
            for (const auto& func : file.functions) {
                all_functions.push_back({
                    file.file_info.name,
                    func.name,
                    func.complexity.cyclomatic_complexity
                });
            }
        }
    } else {
        const auto& file = session.single_file_result;
        for (const auto& cls : file.classes) {
            for (const auto& method : cls.methods) {
                all_functions.push_back({
                    file.file_info.name,
                    cls.name + "::" + method.name,
                    method.complexity.cyclomatic_complexity
                });
            }
        }
        
        for (const auto& func : file.functions) {
            all_functions.push_back({
                file.file_info.name,
                func.name,
                func.complexity.cyclomatic_complexity
            });
        }
    }
    
    // 複雑度で降順ソート
    std::sort(all_functions.begin(), all_functions.end(),
        [](const FunctionComplexity& a, const FunctionComplexity& b) {
            return a.complexity > b.complexity;
        });
    
    // 上位50個または全て
    size_t max_functions = std::min(all_functions.size(), static_cast<size_t>(50));
    for (size_t i = 0; i < max_functions; ++i) {
        const auto& func = all_functions[i];
        ranking_json["functions"].push_back({
            {"rank", i + 1},
            {"file", func.filename},
            {"function", func.function_name},
            {"complexity", func.complexity}
        });
    }
    
    ranking_json["summary"] = "Top " + std::to_string(max_functions) + 
                              " functions by complexity (out of " + 
                              std::to_string(all_functions.size()) + " total)";
    
    return ranking_json;
}

nlohmann::json SessionCommands::cmd_large_files(const SessionData& session, int threshold) const {
    nlohmann::json result = {
        {"command", "large-files"},
        {"threshold", threshold},
        {"large_files", nlohmann::json::array()}
    };
    
    std::vector<nlohmann::json> large_files;
    int total_files = 0;
    int large_count = 0;
    
    if (session.is_directory) {
        // ディレクトリ解析の場合
        for (const auto& file : session.directory_result.files) {
            total_files++;
            if (file.file_info.total_lines >= threshold) {
                large_count++;
                large_files.push_back({
                    {"file", file.file_info.path.string()},
                    {"lines", file.file_info.total_lines},
                    {"size_bytes", file.file_info.size_bytes},
                    {"complexity", file.complexity.cyclomatic_complexity},
                    {"functions", file.stats.function_count},
                    {"classes", file.stats.class_count}
                });
            }
        }
    } else {
        // 単一ファイル解析の場合
        total_files = 1;
        if (session.single_file_result.file_info.total_lines >= threshold) {
            large_count = 1;
            const auto& file = session.single_file_result;
            large_files.push_back({
                {"file", file.file_info.path.string()},
                {"lines", file.file_info.total_lines},
                {"size_bytes", file.file_info.size_bytes},
                {"complexity", file.complexity.cyclomatic_complexity},
                {"functions", file.stats.function_count},
                {"classes", file.stats.class_count}
            });
        }
    }
    
    // 行数でソート（降順）
    std::sort(large_files.begin(), large_files.end(), 
        [](const nlohmann::json& a, const nlohmann::json& b) {
            return a["lines"] > b["lines"];
        });
    
    result["large_files"] = large_files;
    result["summary"] = {
        {"total_files", total_files},
        {"large_files_count", large_count},
        {"percentage", total_files > 0 ? (large_count * 100.0 / total_files) : 0.0},
        {"threshold_lines", threshold}
    };
    
    return result;
}

nlohmann::json SessionCommands::cmd_structure_detailed(const SessionData& session, const std::string& filename) const {
    nlohmann::json result = {
        {"command", "structure-detailed"},
        {"files", nlohmann::json::array()}
    };
    
    auto process_file = [&](const AnalysisResult& file) {
        nlohmann::json file_detail = {
            {"filename", file.file_info.name},
            {"size_bytes", file.file_info.size_bytes},
            {"total_lines", file.file_info.total_lines},
            {"code_lines", file.file_info.code_lines},
            {"complexity", {
                {"cyclomatic_complexity", file.complexity.cyclomatic_complexity},
                {"max_nesting_depth", file.complexity.max_nesting_depth},
                {"rating", file.complexity.to_string()}
            }},
            {"classes", nlohmann::json::array()},
            {"functions", nlohmann::json::array()},
            {"imports", nlohmann::json::array()}
        };
        
        // クラス詳細情報
        for (const auto& cls : file.classes) {
            nlohmann::json class_detail = {
                {"name", cls.name},
                {"start_line", cls.start_line},
                {"end_line", cls.end_line},
                {"parent_class", cls.parent_class},
                {"methods", nlohmann::json::array()},
                {"properties", cls.properties},
                {"member_variables", nlohmann::json::array()}
            };
            
            // メソッド詳細
            for (const auto& method : cls.methods) {
                class_detail["methods"].push_back({
                    {"name", method.name},
                    {"start_line", method.start_line},
                    {"end_line", method.end_line},
                    {"parameters", method.parameters},
                    {"complexity", {
                        {"cyclomatic_complexity", method.complexity.cyclomatic_complexity},
                        {"max_nesting_depth", method.complexity.max_nesting_depth},
                        {"rating", method.complexity.to_string()}
                    }},
                    {"is_async", method.is_async},
                    {"is_arrow_function", method.is_arrow_function}
                });
            }
            
            // メンバ変数詳細
            for (const auto& member : cls.member_variables) {
                class_detail["member_variables"].push_back({
                    {"name", member.name},
                    {"type", member.type},
                    {"declaration_line", member.declaration_line},
                    {"is_static", member.is_static},
                    {"is_const", member.is_const},
                    {"access_modifier", member.access_modifier}
                });
            }
            
            file_detail["classes"].push_back(class_detail);
        }
        
        // スタンドアロン関数詳細
        for (const auto& func : file.functions) {
            file_detail["functions"].push_back({
                {"name", func.name},
                {"start_line", func.start_line},
                {"end_line", func.end_line},
                {"parameters", func.parameters},
                {"complexity", {
                    {"cyclomatic_complexity", func.complexity.cyclomatic_complexity},
                    {"max_nesting_depth", func.complexity.max_nesting_depth},
                    {"rating", func.complexity.to_string()}
                }},
                {"is_async", func.is_async},
                {"is_arrow_function", func.is_arrow_function}
            });
        }
        
        // インポート詳細
        for (const auto& import : file.imports) {
            std::string import_type_str;
            switch (import.type) {
                case ImportType::ES6_IMPORT: import_type_str = "ES6_IMPORT"; break;
                case ImportType::COMMONJS_REQUIRE: import_type_str = "COMMONJS_REQUIRE"; break;
                case ImportType::DYNAMIC_IMPORT: import_type_str = "DYNAMIC_IMPORT"; break;
            }
            
            file_detail["imports"].push_back({
                {"type", import_type_str},
                {"module_path", import.module_path},
                {"imported_names", import.imported_names},
                {"alias", import.alias},
                {"line_number", import.line_number}
            });
        }
        
        // 統計情報
        file_detail["statistics"] = {
            {"class_count", file.classes.size()},
            {"function_count", file.functions.size()},
            {"import_count", file.imports.size()},
            {"total_methods", std::accumulate(file.classes.begin(), file.classes.end(), 0,
                [](int sum, const ClassInfo& cls) { return sum + cls.methods.size(); })},
            {"total_member_variables", std::accumulate(file.classes.begin(), file.classes.end(), 0,
                [](int sum, const ClassInfo& cls) { return sum + cls.member_variables.size(); })}
        };
        
        result["files"].push_back(file_detail);
    };
    
    if (!filename.empty()) {
        // 指定されたファイルのみ処理
        // 🔧 絶対パス vs 相対パス対応: ファイル名のみで比較
        std::string target_filename = std::filesystem::path(filename).filename().string();
        
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                std::string current_filename = std::filesystem::path(file.file_info.name).filename().string();
                if (current_filename.find(target_filename) != std::string::npos ||
                    current_filename == target_filename ||
                    file.file_info.name.find(filename) != std::string::npos ||
                    file.file_info.name == filename) {
                    process_file(file);
                    break;
                }
            }
        } else {
            std::string current_filename = std::filesystem::path(session.single_file_result.file_info.name).filename().string();
            if (current_filename.find(target_filename) != std::string::npos ||
                current_filename == target_filename ||
                session.single_file_result.file_info.name.find(filename) != std::string::npos ||
                session.single_file_result.file_info.name == filename) {
                process_file(session.single_file_result);
            }
        }
    } else {
        // 全ファイル処理
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                process_file(file);
            }
        } else {
            process_file(session.single_file_result);
        }
    }
    
    // 全体統計
    uint32_t total_classes = 0;
    uint32_t total_functions = 0;
    uint32_t total_methods = 0;
    uint32_t total_imports = 0;
    
    for (const auto& file : result["files"]) {
        total_classes += file["statistics"]["class_count"].get<uint32_t>();
        total_functions += file["statistics"]["function_count"].get<uint32_t>();
        total_methods += file["statistics"]["total_methods"].get<uint32_t>();
        total_imports += file["statistics"]["import_count"].get<uint32_t>();
    }
    
    result["summary_statistics"] = {
        {"total_files", result["files"].size()},
        {"total_classes", total_classes},
        {"total_functions", total_functions},
        {"total_methods", total_methods},
        {"total_imports", total_imports}
    };
    
    result["summary"] = "Detailed structure analysis of " + std::to_string(result["files"].size()) + " file(s)" +
                       (filename.empty() ? "" : " matching '" + filename + "'");
    
    return result;
}

nlohmann::json SessionCommands::cmd_complexity_methods(const SessionData& session, const std::string& filename) const {
    nlohmann::json result = {
        {"command", "complexity-methods"},
        {"methods", nlohmann::json::array()},
        {"total_methods", 0}
    };
    
    auto process_file = [&](const AnalysisResult& file) {
        // ファイル名フィルタ
        if (!filename.empty()) {
            std::string current_filename = std::filesystem::path(file.file_info.name).filename().string();
            std::string target_filename = std::filesystem::path(filename).filename().string();
            if (current_filename.find(target_filename) == std::string::npos &&
                current_filename != target_filename) {
                return;
            }
        }
        
        nlohmann::json file_methods = nlohmann::json::array();
        
        // クラスメソッドの複雑度
        for (const auto& cls : file.classes) {
            for (const auto& method : cls.methods) {
                nlohmann::json method_info;
                method_info["name"] = cls.name + "::" + method.name;
                method_info["class"] = cls.name;
                method_info["method"] = method.name;
                method_info["complexity"] = method.complexity.cyclomatic_complexity;
                method_info["line"] = method.start_line;
                method_info["file"] = file.file_info.name;
                method_info["type"] = "method";
                
                file_methods.push_back(method_info);
                result["total_methods"] = result["total_methods"].get<int>() + 1;
            }
        }
        
        // 単体関数の複雑度
        for (const auto& func : file.functions) {
            nlohmann::json func_info;
            func_info["name"] = func.name;
            func_info["class"] = "";
            func_info["method"] = func.name;
            func_info["complexity"] = func.complexity.cyclomatic_complexity;
            func_info["line"] = func.start_line;
            func_info["file"] = file.file_info.name;
            func_info["type"] = "function";
            
            file_methods.push_back(func_info);
            result["total_methods"] = result["total_methods"].get<int>() + 1;
        }
        
        // 複雑度でソート
        std::sort(file_methods.begin(), file_methods.end(), 
            [](const nlohmann::json& a, const nlohmann::json& b) {
                return a["complexity"].get<int>() > b["complexity"].get<int>();
            });
        
        for (const auto& method : file_methods) {
            result["methods"].push_back(method);
        }
    };
    
    if (session.is_directory) {
        for (const auto& file : session.directory_result.files) {
            process_file(file);
        }
    } else {
        process_file(session.single_file_result);
    }
    
    // 全メソッドを複雑度でソート
    std::sort(result["methods"].begin(), result["methods"].end(), 
        [](const nlohmann::json& a, const nlohmann::json& b) {
            return a["complexity"].get<int>() > b["complexity"].get<int>();
        });
    
    result["summary"] = "Found " + std::to_string(result["total_methods"].get<int>()) + " methods/functions" +
                       (filename.empty() ? "" : " in " + filename);
    
    return result;
}

nlohmann::json SessionCommands::cmd_calls_detailed(const SessionData& session, const std::string& function_name) const {
    return {
        {"command", "calls-detailed"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Calls detailed feature pending implementation"}
    };
}

} // namespace nekocode