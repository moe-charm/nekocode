//=============================================================================
// 📋 Session Commands実装 - セッションコマンド実装
//
// SessionManagerから分離したコマンド実装群
// 責任: 各種解析コマンドの具体的実装
//=============================================================================

#include "nekocode/session_commands.hpp"
#include "nekocode/include_analyzer.hpp"
#include "nekocode/symbol_finder.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <set>
#include <filesystem>

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
                "dependency-analyze [file] - Analyze dependencies",
                "help - Show this help"
            }},
            {"ast_revolution", {
                "ast-query <path> - Query AST nodes by path (e.g. MyClass::myMethod)",
                "scope-analysis <line> - Get scope information at specific line",
                "ast-dump [format] - Dump AST structure (tree/json/compact)",
                "ast-stats - AST-based statistics with structural analysis"
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
    nlohmann::json result = {
        {"command", "include-unused"},
        {"unused_includes", nlohmann::json::array()},
        {"total_unused", 0}
    };
    
    if (!session.is_directory) {
        result["summary"] = "Single file analysis - unused include detection not applicable";
        return result;
    }
    
    try {
        // 🔥 ハイブリッドアプローチ: IncludeAnalyzer + SessionData実解析結果
        
        // 1. IncludeAnalyzerで#include情報を取得
        IncludeAnalyzer analyzer;
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;
        analyzer.set_config(config);
        auto include_result = analyzer.analyze_directory(session.target_path);
        
        // 2. SessionDataから実際の提供シンボルを構築
        std::map<std::string, std::set<std::string>> provided_symbols; // ファイル→提供シンボル
        for (const auto& file : session.directory_result.files) {
            std::set<std::string> symbols;
            
            // クラス名を追加
            for (const auto& cls : file.classes) {
                symbols.insert(cls.name);
            }
            
            // 関数名を追加  
            for (const auto& func : file.functions) {
                symbols.insert(func.name);
            }
            
            // ファイル名でキー作成（フルパスから）
            std::string filename = std::filesystem::path(file.file_info.name).filename().string();
            provided_symbols[filename] = symbols;
        }
        
        // 3. 不要includeを検出
        nlohmann::json unused_array = nlohmann::json::array();
        int total_unused = 0;
        
        // include_resultから各ファイルのinclude情報を取得
        for (const auto& [file_path, node] : include_result.dependency_graph) {
            // .cppファイルのみチェック
            std::string ext = std::filesystem::path(file_path).extension().string();
            if (ext != ".cpp" && ext != ".cxx" && ext != ".cc") continue;
            
            // ファイル内容を一度だけ読み込み
            std::ifstream ifs(file_path);
            if (!ifs.is_open()) continue;
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            
            for (const auto& inc : node.include_statements) {
                // システムヘッダーはスキップ
                if (inc.is_system_header) continue;
                
                // included fileのファイル名を取得
                std::string included_filename = std::filesystem::path(inc.path).filename().string();
                
                // 提供シンボルを取得
                auto provided_it = provided_symbols.find(included_filename);
                if (provided_it == provided_symbols.end()) continue;
                
                // シンボル使用チェック (NO_REGEX準拠 + include文除外)
                bool is_used = false;
                for (const auto& symbol : provided_it->second) {
                    // include文の行を除外してチェック
                    std::istringstream iss(content);
                    std::string line;
                    size_t current_pos = 0;
                    
                    while (std::getline(iss, line)) {
                        // include文の行はスキップ
                        if (line.find("#include") != std::string::npos) {
                            current_pos += line.length() + 1; // +1 for newline
                            continue;
                        }
                        
                        // この行内でシンボル検索
                        size_t pos = line.find(symbol);
                        while (pos != std::string::npos) {
                            // 前の文字チェック（単語境界）
                            bool prev_ok = (pos == 0) || 
                                          (!std::isalnum(line[pos-1]) && line[pos-1] != '_');
                            // 後の文字チェック（単語境界）
                            bool next_ok = (pos + symbol.length() >= line.length()) || 
                                          (!std::isalnum(line[pos + symbol.length()]) && 
                                           line[pos + symbol.length()] != '_');
                            
                            if (prev_ok && next_ok) {
                                is_used = true;
                                break;
                            }
                            pos = line.find(symbol, pos + 1);
                        }
                        
                        if (is_used) break;
                        current_pos += line.length() + 1; // +1 for newline
                    }
                    if (is_used) break;
                }
                
                if (!is_used && !provided_it->second.empty()) {
                    unused_array.push_back({
                        {"file", std::filesystem::path(file_path).filename().string()},
                        {"unused_include", inc.path},
                        {"line", inc.line_number},
                        {"provided_symbols", std::vector<std::string>(provided_it->second.begin(), provided_it->second.end())},
                        {"reason", "None of the provided symbols are used in this file"}
                    });
                    total_unused++;
                }
            }
        }
        
        result["unused_includes"] = unused_array;
        result["total_unused"] = total_unused;
        result["summary"] = "Found " + std::to_string(total_unused) + " unused includes using hybrid analysis (IncludeAnalyzer + SessionData)";
        
        // 🎯 Production-ready output (debug info removed)
        
    } catch (const std::exception& e) {
        result["error"] = e.what();
        result["summary"] = "Unused include detection failed: " + std::string(e.what());
    }
    
    return result;
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

nlohmann::json SessionCommands::cmd_todo(const SessionData& session) const {
    nlohmann::json result = {
        {"command", "todo"},
        {"todos", nlohmann::json::array()},
        {"todo_patterns", {"TODO", "FIXME", "HACK", "BUG", "NOTE", "XXX"}}
    };
    
    std::vector<nlohmann::json> todos;
    int total_todos = 0;
    
    // TODO検索パターン
    std::vector<std::string> patterns = {"TODO", "FIXME", "HACK", "BUG", "NOTE", "XXX"};
    
    auto search_file = [&](const AnalysisResult& file) {
        std::ifstream input(file.file_info.path);
        if (!input.is_open()) return;
        
        std::string line;
        int line_number = 1;
        
        while (std::getline(input, line)) {
            for (const auto& pattern : patterns) {
                // パターンを検索（大文字小文字区別なし）
                std::string upper_line = line;
                std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(), ::toupper);
                
                size_t pos = upper_line.find(pattern);
                if (pos != std::string::npos) {
                    // コメント内かチェック
                    bool is_comment = false;
                    size_t comment_pos = line.find("//");
                    size_t multicomment_pos = line.find("/*");
                    size_t hash_pos = line.find("#");
                    
                    if ((comment_pos != std::string::npos && pos >= comment_pos) ||
                        (multicomment_pos != std::string::npos && pos >= multicomment_pos) ||
                        (hash_pos != std::string::npos && pos >= hash_pos)) {
                        is_comment = true;
                    }
                    
                    if (is_comment) {
                        total_todos++;
                        // TODOの内容を抽出
                        std::string todo_content = line.substr(pos);
                        // 先頭と末尾の空白を削除
                        todo_content.erase(0, todo_content.find_first_not_of(" \t"));
                        todo_content.erase(todo_content.find_last_not_of(" \t") + 1);
                        
                        todos.push_back({
                            {"file", file.file_info.path.string()},
                            {"line", line_number},
                            {"type", pattern},
                            {"content", todo_content},
                            {"full_line", line},
                            {"priority", pattern == "FIXME" || pattern == "BUG" ? "high" : 
                                       pattern == "TODO" ? "medium" : "low"}
                        });
                    }
                    break; // 一行につき一つのパターンのみ検出
                }
            }
            line_number++;
        }
    };
    
    if (session.is_directory) {
        // ディレクトリ解析の場合
        for (const auto& file : session.directory_result.files) {
            search_file(file);
        }
    } else {
        // 単一ファイル解析の場合
        search_file(session.single_file_result);
    }
    
    // 優先度とファイル名でソート
    std::sort(todos.begin(), todos.end(), 
        [](const nlohmann::json& a, const nlohmann::json& b) {
            std::string priority_a = a["priority"];
            std::string priority_b = b["priority"];
            if (priority_a != priority_b) {
                if (priority_a == "high") return true;
                if (priority_b == "high") return false;
                if (priority_a == "medium") return true;
                return false;
            }
            return a["file"].get<std::string>() < b["file"].get<std::string>();
        });
    
    result["todos"] = todos;
    result["summary"] = {
        {"total_todos", total_todos},
        {"high_priority", std::count_if(todos.begin(), todos.end(), 
            [](const nlohmann::json& todo) { return todo["priority"] == "high"; })},
        {"medium_priority", std::count_if(todos.begin(), todos.end(), 
            [](const nlohmann::json& todo) { return todo["priority"] == "medium"; })},
        {"files_with_todos", [&]() {
            std::set<std::string> unique_files;
            for (const auto& todo : todos) {
                unique_files.insert(todo["file"].get<std::string>());
            }
            return unique_files.size();
        }()}
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
        {"methods", nlohmann::json::array()}
    };
    
    // ファイル名が指定されている場合、該当ファイルのみ処理
    if (!filename.empty()) {
        // 🔧 絶対パス vs 相対パス対応: ファイル名のみで比較
        std::string target_filename = std::filesystem::path(filename).filename().string();
        
        if (session.is_directory) {
            // ディレクトリ内から指定ファイルを検索
            for (const auto& file : session.directory_result.files) {
                std::string current_filename = std::filesystem::path(file.file_info.name).filename().string();
                if (current_filename.find(target_filename) != std::string::npos ||
                    current_filename == target_filename ||
                    file.file_info.name.find(filename) != std::string::npos ||
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
            std::string current_filename = std::filesystem::path(file.file_info.name).filename().string();
            if (current_filename.find(target_filename) != std::string::npos ||
                current_filename == target_filename ||
                file.file_info.name.find(filename) != std::string::npos ||
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
    
    if (debug) {
        std::cerr << "[DEBUG] cmd_find_symbols called with symbol: " << symbol << std::endl;
        std::cerr << "[DEBUG] options count: " << options.size() << std::endl;
    }
    
    // SymbolFinderの設定
    SymbolFinder finder;
    SymbolFinder::FindOptions find_opts;
    find_opts.debug = debug;
    
    // オプション解析
    for (const auto& opt : options) {
        if (opt == "--debug") {
            find_opts.debug = true;
        } else if (opt == "--functions") {
            find_opts.type = SymbolFinder::SymbolType::FUNCTION;
        } else if (opt == "--variables") {
            find_opts.type = SymbolFinder::SymbolType::VARIABLE;
        }
    }
    
    // セッションデータからファイル情報を抽出
    std::vector<FileInfo> files;
    
    if (session.is_directory) {
        for (const auto& file : session.directory_result.files) {
            FileInfo file_info;
            file_info.path = file.file_info.path;
            files.push_back(file_info);
        }
    } else {
        FileInfo file_info;
        file_info.path = session.single_file_result.file_info.path;
        files.push_back(file_info);
    }
    
    finder.setFiles(files);
    
    // 検索実行
    auto results = finder.find(symbol, find_opts);
    
    if (debug) {
        std::cerr << "[DEBUG] Search completed. Found " << results.total_count << " matches" << std::endl;
    }
    
    // JSON結果を構築
    nlohmann::json json_results;
    json_results["command"] = "find-symbols";
    json_results["symbol"] = symbol;
    json_results["total_matches"] = results.total_count;
    json_results["function_matches"] = results.function_count;
    json_results["variable_matches"] = results.variable_count;
    json_results["files_affected"] = results.file_counts.size();
    
    // 結果詳細
    nlohmann::json matches = nlohmann::json::array();
    for (const auto& loc : results.locations) {
        nlohmann::json match;
        match["file"] = loc.file_path;
        match["line"] = loc.line_number;
        match["content"] = loc.line_content;
        match["symbol_type"] = (loc.symbol_type == SymbolFinder::SymbolType::FUNCTION) ? "function" : "variable";
        match["use_type"] = [&]() {
            switch(loc.use_type) {
                case SymbolFinder::UseType::DECLARATION: return "declaration";
                case SymbolFinder::UseType::ASSIGNMENT: return "assignment";
                case SymbolFinder::UseType::CALL: return "call";
                case SymbolFinder::UseType::REFERENCE: return "reference";
                default: return "unknown";
            }
        }();
        matches.push_back(match);
    }
    json_results["matches"] = matches;
    
    // サマリー
    json_results["summary"] = "Found " + std::to_string(results.total_count) + " matches for '" + symbol + "'";
    
    return json_results;
}

nlohmann::json SessionCommands::cmd_dependency_analyze(const SessionData& session, const std::string& filename) const {
    nlohmann::json result = {
        {"command", "dependency-analyze"},
        {"analysis", nlohmann::json::object()}
    };
    
    // C++ファイルのみ対応
    auto process_cpp_file = [&](const AnalysisResult& file) -> nlohmann::json {
        // C++/Cファイルかどうかを拡張子でチェック（一時的な対処）
        std::string ext = std::filesystem::path(file.file_info.name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext != ".cpp" && ext != ".cxx" && ext != ".cc" && ext != ".c" && 
            ext != ".hpp" && ext != ".hxx" && ext != ".h") {
            return nlohmann::json::object();
        }
        
        // CppAnalyzerを使用して詳細な依存関係を分析
        CppAnalyzer analyzer;
        std::string content; // 実際にはファイルを読み込む必要がある
        
        // ファイル内容を取得（セッションのターゲットパスを使用）
        std::filesystem::path full_path;
        if (session.is_directory) {
            // file.file_info.pathには余分なパスが含まれるので、ファイル名だけ使用
            std::filesystem::path file_path(file.file_info.path);
            // pathから最後のディレクトリ部分とファイル名を取得
            auto relative_path = file_path.filename();
            if (file_path.has_parent_path()) {
                auto parent = file_path.parent_path();
                // 親のディレクトリ名を取得（例：messages/）
                if (parent.filename() != "nyamesh-cpp") {
                    relative_path = parent.filename() / relative_path;
                }
            }
            full_path = session.target_path / relative_path;
        } else {
            // 単一ファイルの場合は、target_pathがそのファイルのパス
            full_path = session.target_path;
        }
        std::ifstream ifs(full_path);
        if (!ifs) {
            return {
                {"error", "Failed to read file: " + full_path.string()},
                {"file", file.file_info.name}
            };
        }
        content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        
        // 依存関係分析実行
        auto dep_result = analyzer.analyze_dependencies(content);
        
        nlohmann::json file_analysis = {
            {"filename", file.file_info.name},
            {"total_includes", dep_result.includes.size()},
            {"system_includes", 0},
            {"local_includes", 0},
            {"classes", nlohmann::json::array()}
        };
        
        // includeの分類
        for (const auto& inc : dep_result.includes) {
            if (inc.is_system_include) {
                file_analysis["system_includes"] = file_analysis["system_includes"].get<int>() + 1;
            } else {
                file_analysis["local_includes"] = file_analysis["local_includes"].get<int>() + 1;
            }
        }
        
        // クラスごとの依存関係
        for (const auto& [class_name, dep_info] : dep_result.class_dependencies) {
            nlohmann::json class_dep = {
                {"name", class_name},
                {"used_types", dep_info.used_types},
                {"required_includes", dep_info.required_includes},
                {"unused_includes", dep_info.unused_includes}
            };
            file_analysis["classes"].push_back(class_dep);
        }
        
        // 不要なincludeの総数
        std::set<std::string> all_unused;
        for (const auto& [_, dep_info] : dep_result.class_dependencies) {
            all_unused.insert(dep_info.unused_includes.begin(), dep_info.unused_includes.end());
        }
        file_analysis["total_unused_includes"] = all_unused.size();
        
        return file_analysis;
    };
    
    // 特定ファイルまたは全ファイルを処理
    nlohmann::json files_analysis = nlohmann::json::array();
    
    if (!filename.empty()) {
        // 特定のファイルのみ
        // 🔧 絶対パス vs 相対パス対応: ファイル名のみで比較
        std::string target_filename = std::filesystem::path(filename).filename().string();
        
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                std::string current_filename = std::filesystem::path(file.file_info.name).filename().string();
                if (current_filename.find(target_filename) != std::string::npos ||
                    current_filename == target_filename ||
                    file.file_info.name.find(filename) != std::string::npos) {
                    auto analysis = process_cpp_file(file);
                    if (!analysis.empty()) {
                        files_analysis.push_back(analysis);
                    }
                }
            }
        } else {
            std::string current_filename = std::filesystem::path(session.single_file_result.file_info.name).filename().string();
            if (current_filename.find(target_filename) != std::string::npos ||
                current_filename == target_filename ||
                session.single_file_result.file_info.name.find(filename) != std::string::npos) {
                auto analysis = process_cpp_file(session.single_file_result);
                if (!analysis.empty()) {
                    files_analysis.push_back(analysis);
                }
            }
        }
    } else {
        // 全C++/Cファイル（拡張子でフィルタ）
        if (session.is_directory) {
            for (const auto& file : session.directory_result.files) {
                auto analysis = process_cpp_file(file);
                if (!analysis.empty()) {
                    files_analysis.push_back(analysis);
                }
            }
        } else {
            auto analysis = process_cpp_file(session.single_file_result);
            if (!analysis.empty()) {
                files_analysis.push_back(analysis);
            }
        }
    }
    
    result["analysis"] = files_analysis;
    
    // サマリー統計
    int total_files = files_analysis.size();
    int total_includes = 0;
    int total_unused = 0;
    
    for (const auto& file_analysis : files_analysis) {
        if (file_analysis.contains("total_includes")) {
            total_includes += file_analysis["total_includes"].get<int>();
        }
        if (file_analysis.contains("total_unused_includes")) {
            total_unused += file_analysis["total_unused_includes"].get<int>();
        }
    }
    
    result["summary"] = {
        {"total_files_analyzed", total_files},
        {"total_includes", total_includes},
        {"total_unused_includes", total_unused},
        {"recommendation", total_unused > 0 ? 
            "Found " + std::to_string(total_unused) + " potentially unused includes" : 
            "No unused includes detected"}
    };
    
    return result;
}

//=============================================================================
// 🌳 AST革命: 新セッションコマンド群
//=============================================================================

nlohmann::json SessionCommands::cmd_ast_query(const SessionData& session, const std::string& query_path) const {
    nlohmann::json result = {
        {"command", "ast-query"},
        {"query_path", query_path},
        {"matches", nlohmann::json::array()}
    };
    
    // Helper function to process Enhanced Analysis Result with AST
    auto process_enhanced_file = [&](const EnhancedAnalysisResult* enhanced_file) {
        if (!enhanced_file || !enhanced_file->has_ast || !enhanced_file->ast_root) {
            return;
        }
        
        // AST Query実行
        auto nodes = enhanced_file->query_nodes(query_path);
        
        for (const ASTNode* node : nodes) {
            nlohmann::json match = {
                {"file", enhanced_file->file_info.name},
                {"node_type", node->type_to_string()},
                {"name", node->name},
                {"scope_path", node->scope_path},
                {"start_line", node->start_line},
                {"end_line", node->end_line},
                {"depth", node->depth},
                {"children_count", node->children.size()}
            };
            
            // 属性情報があれば追加
            if (!node->attributes.empty()) {
                match["attributes"] = node->attributes;
            }
            
            // 子ノード情報（名前のみ）
            if (!node->children.empty()) {
                nlohmann::json children_names = nlohmann::json::array();
                for (const auto& child : node->children) {
                    children_names.push_back({
                        {"name", child->name},
                        {"type", child->type_to_string()},
                        {"line", child->start_line}
                    });
                }
                match["children"] = children_names;
            }
            
            result["matches"].push_back(match);
        }
    };
    
    // ⚠️  现在的问题：SessionData中的结果是AnalysisResult，不是EnhancedAnalysisResult
    // 这里需要检查是否有AST数据可用
    
    // 暂时返回提示信息，说明需要AST支持
    if (session.is_directory) {
        // TODO: 检查directory_result中是否有AST数据
        result["error"] = "AST query requires Enhanced Analysis Results with AST data";
        result["note"] = "Current session contains basic AnalysisResult. Need to update session creation to use EnhancedAnalysisResult.";
    } else {
        // TODO: 检查single_file_result中是否有AST数据
        result["error"] = "AST query requires Enhanced Analysis Results with AST data";
        result["note"] = "Current session contains basic AnalysisResult. Need to update session creation to use EnhancedAnalysisResult.";
    }
    
    result["summary"] = "AST query for '" + query_path + "' - " + 
                       std::to_string(result["matches"].size()) + " matches found";
    
    return result;
}

nlohmann::json SessionCommands::cmd_scope_analysis(const SessionData& session, uint32_t line_number) const {
    nlohmann::json result = {
        {"command", "scope-analysis"},
        {"line_number", line_number},
        {"scopes", nlohmann::json::array()}
    };
    
    // Helper function to process Enhanced Analysis Result with AST
    auto process_enhanced_file = [&](const EnhancedAnalysisResult* enhanced_file) {
        if (!enhanced_file || !enhanced_file->has_ast || !enhanced_file->ast_root) {
            return;
        }
        
        // スコープ解析実行
        std::string scope_path = enhanced_file->get_scope_at_line(line_number);
        
        nlohmann::json scope_info = {
            {"file", enhanced_file->file_info.name},
            {"line", line_number},
            {"scope_path", scope_path}
        };
        
        // 詳細なスコープ階層情報を取得
        if (!scope_path.empty()) {
            // スコープパスを分解して階層情報を構築
            std::vector<std::string> scope_parts;
            std::istringstream iss(scope_path);
            std::string part;
            while (std::getline(iss, part, ':')) {
                if (!part.empty() && part != ":") {
                    scope_parts.push_back(part);
                }
            }
            
            scope_info["scope_hierarchy"] = scope_parts;
            scope_info["nesting_depth"] = scope_parts.size();
        } else {
            scope_info["scope_hierarchy"] = nlohmann::json::array();
            scope_info["nesting_depth"] = 0;
        }
        
        result["scopes"].push_back(scope_info);
    };
    
    // 現在の実装では基本的なAnalysisResultのみ利用可能
    result["error"] = "Scope analysis requires Enhanced Analysis Results with AST data";
    result["note"] = "Current session contains basic AnalysisResult. AST-based scope analysis is not available.";
    result["fallback_analysis"] = "Using basic structure analysis instead...";
    
    // フォールバック：基本的な構造情報から推測
    if (!session.is_directory) {
        const auto& file = session.single_file_result;
        
        nlohmann::json basic_scope = {
            {"file", file.file_info.name},
            {"line", line_number},
            {"estimated_scope", "unknown"}
        };
        
        // 関数・クラスの範囲から推測
        for (const auto& cls : file.classes) {
            if (line_number >= cls.start_line && line_number <= cls.end_line) {
                basic_scope["estimated_scope"] = "class:" + cls.name;
                
                // メソッド内かチェック
                for (const auto& method : cls.methods) {
                    if (line_number >= method.start_line && line_number <= method.end_line) {
                        basic_scope["estimated_scope"] = "class:" + cls.name + "::method:" + method.name;
                        break;
                    }
                }
                break;
            }
        }
        
        for (const auto& func : file.functions) {
            if (line_number >= func.start_line && line_number <= func.end_line) {
                basic_scope["estimated_scope"] = "function:" + func.name;
                break;
            }
        }
        
        result["scopes"].push_back(basic_scope);
    }
    
    result["summary"] = "Scope analysis for line " + std::to_string(line_number) + 
                       " (limited to basic structure analysis)";
    
    return result;
}

nlohmann::json SessionCommands::cmd_ast_dump(const SessionData& session, const std::string& format) const {
    nlohmann::json result = {
        {"command", "ast-dump"},
        {"format", format.empty() ? "tree" : format},
        {"ast_trees", nlohmann::json::array()}
    };
    
    // サポートされるフォーマット
    std::string dump_format = format.empty() ? "tree" : format;
    if (dump_format != "tree" && dump_format != "json" && dump_format != "compact") {
        result["error"] = "Unsupported format '" + format + "'. Use: tree, json, or compact";
        return result;
    }
    
    // Helper function to dump AST node recursively
    std::function<nlohmann::json(const ASTNode*, int)> dump_node_recursive = 
        [&](const ASTNode* node, int max_depth) -> nlohmann::json {
        if (!node || max_depth < 0) return nlohmann::json::object();
        
        nlohmann::json node_info = {
            {"name", node->name},
            {"type", node->type_to_string()},
            {"start_line", node->start_line},
            {"end_line", node->end_line},
            {"depth", node->depth}
        };
        
        if (dump_format == "json" || dump_format == "compact") {
            node_info["scope_path"] = node->scope_path;
            if (!node->attributes.empty()) {
                node_info["attributes"] = node->attributes;
            }
        }
        
        if (!node->children.empty() && max_depth > 0) {
            nlohmann::json children = nlohmann::json::array();
            for (const auto& child : node->children) {
                children.push_back(dump_node_recursive(child.get(), max_depth - 1));
            }
            node_info["children"] = children;
        } else if (!node->children.empty()) {
            node_info["children_count"] = node->children.size();
        }
        
        return node_info;
    };
    
    // Helper function to create tree format string
    std::function<std::string(const ASTNode*, int, std::string)> create_tree_string = 
        [&](const ASTNode* node, int depth, std::string prefix) -> std::string {
        if (!node) return "";
        
        std::string result_str;
        std::string indent = std::string(depth * 2, ' ');
        result_str += prefix + node->type_to_string() + ": " + node->name;
        if (node->start_line > 0) {
            result_str += " (line " + std::to_string(node->start_line) + ")";
        }
        result_str += "\n";
        
        for (size_t i = 0; i < node->children.size(); ++i) {
            bool is_last = (i == node->children.size() - 1);
            std::string child_prefix = prefix + (is_last ? "└── " : "├── ");
            std::string next_prefix = prefix + (is_last ? "    " : "│   ");
            result_str += create_tree_string(node->children[i].get(), depth + 1, child_prefix);
        }
        
        return result_str;
    };
    
    // 現在の実装制限
    result["error"] = "AST dump requires Enhanced Analysis Results with AST data";
    result["note"] = "Current session contains basic AnalysisResult. Full AST dump is not available.";
    
    // フォールバック：基本構造情報をツリー形式で表示
    if (!session.is_directory) {
        const auto& file = session.single_file_result;
        
        std::string basic_tree = "File: " + file.file_info.name + "\n";
        
        for (const auto& cls : file.classes) {
            basic_tree += "├── class: " + cls.name + " (line " + std::to_string(cls.start_line) + ")\n";
            for (size_t i = 0; i < cls.methods.size(); ++i) {
                const auto& method = cls.methods[i];
                bool is_last = (i == cls.methods.size() - 1);
                basic_tree += std::string("│   ") + (is_last ? "└── " : "├── ") + 
                             "method: " + method.name + " (line " + std::to_string(method.start_line) + ")\n";
            }
        }
        
        for (const auto& func : file.functions) {
            basic_tree += "├── function: " + func.name + " (line " + std::to_string(func.start_line) + ")\n";
        }
        
        result["fallback_tree"] = basic_tree;
    }
    
    result["summary"] = "AST dump in " + dump_format + " format (basic structure fallback)";
    
    return result;
}

nlohmann::json SessionCommands::cmd_ast_stats(const SessionData& session) const {
    nlohmann::json result = {
        {"command", "ast-stats"},
        {"files", nlohmann::json::array()}
    };
    
    // Helper function to process Enhanced Analysis Result with AST
    auto process_enhanced_file = [&](const EnhancedAnalysisResult* enhanced_file) {
        if (!enhanced_file || !enhanced_file->has_ast || !enhanced_file->ast_root) {
            return;
        }
        
        nlohmann::json file_stats = {
            {"filename", enhanced_file->file_info.name},
            {"has_ast", true},
            {"total_nodes", enhanced_file->ast_stats.total_nodes},
            {"max_depth", enhanced_file->ast_stats.max_depth},
            {"classes", enhanced_file->ast_stats.classes},
            {"functions", enhanced_file->ast_stats.functions},
            {"methods", enhanced_file->ast_stats.methods},
            {"variables", enhanced_file->ast_stats.variables},
            {"control_structures", enhanced_file->ast_stats.control_structures}
        };
        
        // ノードタイプ別統計
        nlohmann::json node_types = nlohmann::json::object();
        for (const auto& [node_type, count] : enhanced_file->ast_stats.node_type_counts) {
            // ASTNodeTypeをstringに変換（簡易版）
            std::string type_name = "unknown";
            // TODO: より詳細な型名変換が必要
            node_types[type_name] = count;
        }
        file_stats["node_type_counts"] = node_types;
        
        result["files"].push_back(file_stats);
    };
    
    // 現在の実装制限
    result["error"] = "AST statistics require Enhanced Analysis Results with AST data";
    result["note"] = "Current session contains basic AnalysisResult. Advanced AST statistics are not available.";
    
    // フォールバック：基本統計情報
    if (session.is_directory) {
        uint32_t total_classes = 0, total_functions = 0, total_imports = 0;
        
        for (const auto& file : session.directory_result.files) {
            total_classes += file.classes.size();
            total_functions += file.functions.size();
            total_imports += file.imports.size();
            
            nlohmann::json basic_stats = {
                {"filename", file.file_info.name},
                {"has_ast", false},
                {"classes", file.classes.size()},
                {"functions", file.functions.size()},
                {"imports", file.imports.size()},
                {"complexity", file.complexity.cyclomatic_complexity}
            };
            
            result["files"].push_back(basic_stats);
        }
        
        result["summary_statistics"] = {
            {"total_files", session.directory_result.files.size()},
            {"total_classes", total_classes},
            {"total_functions", total_functions},
            {"total_imports", total_imports}
        };
    } else {
        const auto& file = session.single_file_result;
        
        nlohmann::json basic_stats = {
            {"filename", file.file_info.name},
            {"has_ast", false},
            {"classes", file.classes.size()},
            {"functions", file.functions.size()},
            {"imports", file.imports.size()},
            {"complexity", file.complexity.cyclomatic_complexity}
        };
        
        result["files"].push_back(basic_stats);
        result["summary_statistics"] = basic_stats;
    }
    
    result["summary"] = "AST-based statistics (currently showing basic fallback statistics)";
    
    return result;
}

} // namespace nekocode