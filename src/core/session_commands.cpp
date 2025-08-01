//=============================================================================
// 📋 Session Commands実装 - セッションコマンド実装
//
// SessionManagerから分離したコマンド実装群
// 責任: 各種解析コマンドの具体的実装
//=============================================================================

#include "nekocode/session_commands.hpp"
#include "nekocode/include_analyzer.hpp"
#include "nekocode/symbol_finder.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>

namespace nekocode {

//=============================================================================
// 🔍 基本統計コマンド実装
//=============================================================================

nlohmann::json SessionCommands::cmd_stats(const SessionData& session) const {
    return {
        {"command", "stats"},
        {"result", session.quick_stats},
        {"summary", session.is_directory ? 
            "Project: " + std::to_string(session.directory_result.summary.total_files) + " files" :
            "File: " + session.single_file_result.file_info.name}
    };
}

nlohmann::json SessionCommands::cmd_files(const SessionData& session) const {
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

nlohmann::json SessionCommands::cmd_complexity(const SessionData& session) const {
    std::cerr << "[DEBUG] cmd_complexity called! is_directory=" << session.is_directory << std::endl;
    if (!session.is_directory) {
        return {
            {"command", "complexity"},
            {"result", {{
                {"file", session.single_file_result.file_info.name},
                {"complexity", session.single_file_result.complexity.cyclomatic_complexity},
                {"rating", session.single_file_result.complexity.to_string()}
            }}},
            {"summary", "Single file complexity: " + 
                        std::to_string(session.single_file_result.complexity.cyclomatic_complexity)}
        };
    }
    
    // ディレクトリの場合は複雑度でソート
    std::vector<std::pair<std::string, ComplexityInfo>> complexity_list;
    for (const auto& file : session.directory_result.files) {
        complexity_list.emplace_back(file.file_info.name, file.complexity);
    }
    
    std::cerr << "[DEBUG] Before sort: files count = " << complexity_list.size() << std::endl;
    if (!complexity_list.empty()) {
        std::cerr << "[DEBUG] First file complexity: " << complexity_list[0].second.cyclomatic_complexity << std::endl;
    }
    
    // 複雑度で降順ソート
    std::sort(complexity_list.begin(), complexity_list.end(),
        [](const auto& a, const auto& b) {
            return a.second.cyclomatic_complexity > b.second.cyclomatic_complexity;
        });
    
    std::cerr << "[DEBUG] After sort: files count = " << complexity_list.size() << std::endl;
    if (!complexity_list.empty()) {
        std::cerr << "[DEBUG] First file complexity after sort: " << complexity_list[0].second.cyclomatic_complexity << std::endl;
    }
    
    nlohmann::json result = nlohmann::json::array();
    for (const auto& [filename, complexity] : complexity_list) {
        result.push_back({
            {"file", filename},
            {"complexity", complexity.cyclomatic_complexity},
            {"rating", complexity.to_string()}
        });
    }
    
    return {
        {"command", "complexity"},
        {"result", result},
        {"debug_info", {
            {"files_count", complexity_list.size()},
            {"first_file_complexity", complexity_list.empty() ? 0 : complexity_list[0].second.cyclomatic_complexity},
            {"function_called", "cmd_complexity"}
        }},
        {"summary", "Analyzed " + std::to_string(complexity_list.size()) + 
                    " files for complexity (sorted by complexity, highest first)"}
    };
}

nlohmann::json SessionCommands::cmd_help() const {
    return {
        {"command", "help"},
        {"result", {
            {"available_commands", {
                "stats - Project/file statistics",
                "files - File listing with details",
                "complexity - Complexity analysis by file",
                "complexity-ranking - Function complexity ranking",
                "structure - Class/function structure",
                "calls - Function call analysis",
                "find <term> - Search for symbols",
                "large-files [threshold] - Find large files",
                "duplicates - Find duplicate files",
                "todo - Find TODO/FIXME comments",
                "help - Show this help"
            }},
            {"cpp_specific", {
                "include-graph - Include dependency graph",
                "include-cycles - Circular dependency detection",
                "include-impact - Change impact analysis",
                "include-unused - Unused include detection",
                "include-optimize - Include optimization suggestions"
            }}
        }},
        {"summary", "Available session commands"}
    };
}

//=============================================================================
// 🛠️ 内部ヘルパー関数実装
//=============================================================================

uint32_t SessionCommands::calculate_total_complexity(const AnalysisResult& file) const {
    uint32_t total = file.complexity.cyclomatic_complexity;
    
    // クラス内の全メソッドの複雑度を加算
    for (const auto& cls : file.classes) {
        for (const auto& method : cls.methods) {
            total += method.complexity.cyclomatic_complexity;
        }
    }
    
    // スタンドアロン関数の複雑度を加算
    for (const auto& func : file.functions) {
        total += func.complexity.cyclomatic_complexity;
    }
    
    return total;
}

std::vector<std::pair<std::string, uint32_t>> SessionCommands::get_sorted_function_complexity(const AnalysisResult& file) const {
    std::vector<std::pair<std::string, uint32_t>> functions;
    
    // クラスメソッドを追加
    for (const auto& cls : file.classes) {
        for (const auto& method : cls.methods) {
            std::string full_name = cls.name + "::" + method.name;
            functions.emplace_back(full_name, method.complexity.cyclomatic_complexity);
        }
    }
    
    // スタンドアロン関数を追加
    for (const auto& func : file.functions) {
        functions.emplace_back(func.name, func.complexity.cyclomatic_complexity);
    }
    
    // 複雑度で降順ソート
    std::sort(functions.begin(), functions.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    return functions;
}

//=============================================================================
// 🔍 追加実装したコマンド群
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

//=============================================================================
// 🔍 残りのコマンド実装（スタブ）
//=============================================================================

nlohmann::json SessionCommands::cmd_find(const SessionData& session, const std::string& term) const {
    return {
        {"command", "find"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Find feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_analyze(const SessionData& session, const std::string& target, bool deep) const {
    return {
        {"command", "analyze"}, 
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Analyze feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_include_graph(const SessionData& session) const {
    try {
        // IncludeAnalyzerを作成
        IncludeAnalyzer analyzer;
        
        // 設定
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;  // システムヘッダーは除外
        config.detect_circular = true;
        config.detect_unused = true;
        analyzer.set_config(config);
        
        // ディレクトリ解析実行
        auto analysis_result = analyzer.analyze_directory(session.target_path);
        
        // グラフ情報をJSON形式で返却
        return analyzer.get_include_graph(analysis_result);
        
    } catch (const std::exception& e) {
        return {
            {"command", "include-graph"},
            {"error", e.what()},
            {"summary", "Include graph analysis failed"}
        };
    }
}

nlohmann::json SessionCommands::cmd_include_cycles(const SessionData& session) const {
    try {
        // IncludeAnalyzerを作成
        IncludeAnalyzer analyzer;
        
        // 設定
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;
        config.detect_circular = true;
        config.detect_unused = false;  // 循環依存のみ検出
        analyzer.set_config(config);
        
        // ディレクトリ解析実行
        auto analysis_result = analyzer.analyze_directory(session.target_path);
        
        // 循環依存情報をJSON形式で返却
        return analyzer.get_circular_dependencies(analysis_result);
        
    } catch (const std::exception& e) {
        return {
            {"command", "include-cycles"},
            {"error", e.what()},
            {"summary", "Circular dependency detection failed"}
        };
    }
}

nlohmann::json SessionCommands::cmd_include_impact(const SessionData& session) const {
    return {
        {"command", "include-impact"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Include impact feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_include_unused(const SessionData& session) const {
    try {
        // IncludeAnalyzerを作成
        IncludeAnalyzer analyzer;
        
        // 設定
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;
        config.detect_circular = false;
        config.detect_unused = true;  // 不要include検出
        analyzer.set_config(config);
        
        // ディレクトリ解析実行
        auto analysis_result = analyzer.analyze_directory(session.target_path);
        
        // 不要include情報をJSON形式で返却
        return analyzer.get_unused_includes(analysis_result);
        
    } catch (const std::exception& e) {
        return {
            {"command", "include-unused"},
            {"error", e.what()},
            {"summary", "Unused include detection failed"}
        };
    }
}

nlohmann::json SessionCommands::cmd_include_optimize(const SessionData& session) const {
    return {
        {"command", "include-optimize"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Include optimize feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_duplicates(const SessionData& session) const {
    return {
        {"command", "duplicates"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Duplicates feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_large_files(const SessionData& session, int threshold) const {
    return {
        {"command", "large-files"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Large files feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_todo(const SessionData& session) const {
    return {
        {"command", "todo"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "TODO feature pending implementation"}
    };
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
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                if (file.file_info.name.find(filename) != std::string::npos ||
                    file.file_info.name == filename) {
                    process_file(file);
                    break;
                }
            }
        } else {
            if (session.single_file_result.file_info.name.find(filename) != std::string::npos ||
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
        {"methods", nlohmann::json::array()}
    };
    
    // ファイル名が指定されている場合、該当ファイルのみ処理
    if (!filename.empty()) {
        if (session.is_directory) {
            // ディレクトリ内から指定ファイルを検索
            for (const auto& file : session.directory_result.files) {
                if (file.file_info.name.find(filename) != std::string::npos ||
                    file.file_info.name == filename) {
                    
                    // クラスメソッドの複雑度
                    for (const auto& cls : file.classes) {
                        for (const auto& method : cls.methods) {
                            result["methods"].push_back({
                                {"file", file.file_info.name},
                                {"class", cls.name},
                                {"method", method.name},
                                {"complexity", method.complexity.cyclomatic_complexity},
                                {"rating", method.complexity.to_string()},
                                {"start_line", method.start_line}
                            });
                        }
                    }
                    
                    // スタンドアロン関数の複雑度
                    for (const auto& func : file.functions) {
                        result["methods"].push_back({
                            {"file", file.file_info.name},
                            {"class", ""},
                            {"method", func.name},
                            {"complexity", func.complexity.cyclomatic_complexity},
                            {"rating", func.complexity.to_string()},
                            {"start_line", func.start_line}
                        });
                    }
                    break;
                }
            }
        } else {
            // 単一ファイルの場合
            const auto& file = session.single_file_result;
            if (file.file_info.name.find(filename) != std::string::npos ||
                file.file_info.name == filename) {
                
                // クラスメソッドの複雑度
                for (const auto& cls : file.classes) {
                    for (const auto& method : cls.methods) {
                        result["methods"].push_back({
                            {"file", file.file_info.name},
                            {"class", cls.name},
                            {"method", method.name},
                            {"complexity", method.complexity.cyclomatic_complexity},
                            {"rating", method.complexity.to_string()},
                            {"start_line", method.start_line}
                        });
                    }
                }
                
                // スタンドアロン関数の複雑度
                for (const auto& func : file.functions) {
                    result["methods"].push_back({
                        {"file", file.file_info.name},
                        {"class", ""},
                        {"method", func.name},
                        {"complexity", func.complexity.cyclomatic_complexity},
                        {"rating", func.complexity.to_string()},
                        {"start_line", func.start_line}
                    });
                }
            }
        }
    } else {
        // ファイル名未指定の場合、全ファイルの関数複雑度を表示
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                // クラスメソッドの複雑度
                for (const auto& cls : file.classes) {
                    for (const auto& method : cls.methods) {
                        result["methods"].push_back({
                            {"file", file.file_info.name},
                            {"class", cls.name},
                            {"method", method.name},
                            {"complexity", method.complexity.cyclomatic_complexity},
                            {"rating", method.complexity.to_string()},
                            {"start_line", method.start_line}
                        });
                    }
                }
                
                // スタンドアロン関数の複雑度
                for (const auto& func : file.functions) {
                    result["methods"].push_back({
                        {"file", file.file_info.name},
                        {"class", ""},
                        {"method", func.name},
                        {"complexity", func.complexity.cyclomatic_complexity},
                        {"rating", func.complexity.to_string()},
                        {"start_line", func.start_line}
                    });
                }
            }
        } else {
            const auto& file = session.single_file_result;
            
            // クラスメソッドの複雑度
            for (const auto& cls : file.classes) {
                for (const auto& method : cls.methods) {
                    result["methods"].push_back({
                        {"file", file.file_info.name},
                        {"class", cls.name},
                        {"method", method.name},
                        {"complexity", method.complexity.cyclomatic_complexity},
                        {"rating", method.complexity.to_string()},
                        {"start_line", method.start_line}
                    });
                }
            }
            
            // スタンドアロン関数の複雑度
            for (const auto& func : file.functions) {
                result["methods"].push_back({
                    {"file", file.file_info.name},
                    {"class", ""},
                    {"method", func.name},
                    {"complexity", func.complexity.cyclomatic_complexity},
                    {"rating", func.complexity.to_string()},
                    {"start_line", func.start_line}
                });
            }
        }
    }
    
    // 複雑度で降順ソート
    std::sort(result["methods"].begin(), result["methods"].end(),
        [](const nlohmann::json& a, const nlohmann::json& b) {
            return a["complexity"].get<uint32_t>() > b["complexity"].get<uint32_t>();
        });
    
    // 統計情報を追加
    size_t method_count = result["methods"].size();
    uint32_t total_complexity = 0;
    uint32_t max_complexity = 0;
    
    for (const auto& method : result["methods"]) {
        uint32_t complexity = method["complexity"].get<uint32_t>();
        total_complexity += complexity;
        if (complexity > max_complexity) {
            max_complexity = complexity;
        }
    }
    
    result["statistics"] = {
        {"total_methods", method_count},
        {"total_complexity", total_complexity},
        {"average_complexity", method_count > 0 ? static_cast<double>(total_complexity) / method_count : 0.0},
        {"max_complexity", max_complexity}
    };
    
    result["summary"] = "Found " + std::to_string(method_count) + " methods/functions" +
                       (filename.empty() ? "" : " in " + filename) +
                       " (sorted by complexity, highest first)";
    
    return result;
}

nlohmann::json SessionCommands::cmd_calls_detailed(const SessionData& session, const std::string& function_name) const {
    return {
        {"command", "calls-detailed"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Calls detailed feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_find_symbols(const SessionData& session, 
                                const std::string& symbol,
                                const std::vector<std::string>& options,
                                bool debug) const {
    return {
        {"command", "find-symbols"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Find symbols feature pending implementation"}
    };
}

} // namespace nekocode