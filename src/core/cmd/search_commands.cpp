//=============================================================================
// 🔍 Search Commands実装 - 検索・解析コマンド
//=============================================================================

#include "nekocode/session_commands.hpp"
#include "nekocode/symbol_finder.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <filesystem>
#include <cctype>
#include <cstdio>

namespace nekocode {

//=============================================================================
// 🔍 検索・解析コマンド実装  
//=============================================================================

// Helper function to parse class::method from content
static std::pair<std::string, std::string> parse_symbol_name(const std::string& content) {
    // Look for patterns like "ClassName::methodName" or "::methodName"
    size_t pos = content.find("::");
    if (pos != std::string::npos) {
        // Find the start of the class name (backwards from ::)
        size_t class_start = pos;
        while (class_start > 0 && (std::isalnum(content[class_start - 1]) || content[class_start - 1] == '_')) {
            class_start--;
        }
        
        // Find the end of the method name (forwards from ::)
        size_t method_start = pos + 2;
        size_t method_end = method_start;
        while (method_end < content.length() && (std::isalnum(content[method_end]) || content[method_end] == '_')) {
            method_end++;
        }
        
        std::string class_name = content.substr(class_start, pos - class_start);
        std::string method_name = content.substr(method_start, method_end - method_start);
        return {class_name, method_name};
    }
    
    // No class::method pattern, try to extract just the method name
    // Look for patterns like "analyze_something" or "analyze"
    size_t start = 0;
    while (start < content.length() && !std::isalpha(content[start])) {
        start++;
    }
    size_t end = start;
    while (end < content.length() && (std::isalnum(content[end]) || content[end] == '_')) {
        end++;
    }
    
    if (start < end) {
        return {"", content.substr(start, end - start)};
    }
    
    return {"", ""};
}

// Helper function to detect language from method name pattern
static std::string detect_language_from_pattern(const std::string& method_name) {
    for (const auto& [pattern, lang] : LANGUAGE_PATTERNS) {
        if (method_name.find(pattern) != std::string::npos) {
            return lang;
        }
    }
    return "Unknown";
}

// Helper function to check if method is universal
static bool is_universal_method(const std::string& method_name) {
    // Check exact match
    if (UNIVERSAL_METHODS.count(method_name) > 0) {
        return true;
    }
    
    // Check if it starts with a universal method name
    for (const auto& universal : UNIVERSAL_METHODS) {
        if (method_name.find(universal) == 0) {
            return true;
        }
    }
    
    return false;
}

// Create hierarchical structure from flat results
static nlohmann::json create_hierarchical_structure(const nlohmann::json& matches) {
    nlohmann::json universal = nlohmann::json::object();
    nlohmann::json language_specific = nlohmann::json::object();
    
    universal["classes"] = nlohmann::json::object();
    universal["functions"] = nlohmann::json::object();
    
    for (const auto& match : matches) {
        std::string content = match["content"];
        std::string file = match["file"];
        int line = match["line"];
        std::string symbol_type = match["symbol_type"];
        
        auto [class_name, method_name] = parse_symbol_name(content);
        
        // Skip empty results
        if (method_name.empty()) continue;
        
        if (is_universal_method(method_name)) {
            // Add to universal section
            if (!class_name.empty()) {
                universal["classes"][class_name][method_name] = {
                    {"line", line},
                    {"file", file},
                    {"type", symbol_type}
                };
            } else {
                universal["functions"][method_name] = {
                    {"line", line},
                    {"file", file},
                    {"type", symbol_type}
                };
            }
        } else {
            // Add to language-specific section
            std::string lang = detect_language_from_pattern(method_name);
            
            // Get category if available
            std::string category = "other";
            for (const auto& [pattern, cat] : FEATURE_CATEGORIES) {
                if (method_name.find(pattern) != std::string::npos) {
                    category = cat;
                    break;
                }
            }
            
            if (!class_name.empty()) {
                language_specific[lang][category][class_name][method_name] = {
                    {"line", line},
                    {"file", file}
                };
            } else {
                language_specific[lang][category][method_name] = {
                    {"line", line},
                    {"file", file}
                };
            }
        }
    }
    
    return {
        {"classes", universal["classes"]},
        {"functions", universal["functions"]},
        {"language_specific", language_specific}
    };
}

nlohmann::json SessionCommands::cmd_find(const SessionData& session, const std::string& term) const {
    return {
        {"command", "find"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Find feature pending implementation"}
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
    
    // Add hierarchical structure (new feature!)
    auto hierarchical = create_hierarchical_structure(matches);
    json_results["classes"] = hierarchical["classes"];
    json_results["functions"] = hierarchical["functions"];  
    json_results["language_specific"] = hierarchical["language_specific"];
    
    // サマリー
    json_results["summary"] = "Found " + std::to_string(results.total_count) + " matches for '" + symbol + "'";
    
    return json_results;
}

nlohmann::json SessionCommands::cmd_analyze(const SessionData& session, const std::string& target, bool deep, bool complete) const {
    nlohmann::json result;
    result["command"] = "analyze";
    
    // 基本解析情報を設定
    if (session.is_directory) {
        result["target"] = session.target_path.string();
        result["total_files"] = session.directory_result.summary.total_files;
        result["total_lines"] = session.directory_result.summary.total_lines;
        result["total_functions"] = session.directory_result.summary.total_functions;
        result["total_classes"] = session.directory_result.summary.total_classes;
    } else {
        result["target"] = session.single_file_result.file_info.path.string();
        result["functions"] = session.single_file_result.stats.function_count;
        result["classes"] = session.single_file_result.stats.class_count;
        result["lines"] = session.single_file_result.file_info.total_lines;
    }
    
    // 完全解析が要求された場合
    if (complete) {
        try {
            // Pythonスクリプトを呼び出してデッドコード検出
            std::string python_script = "universal_deadcode_analyzer.py";
            std::string cmd = "python3 " + python_script + " \"" + session.target_path.string() + "\" --complete";
            
            // コマンド実行
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char buffer[128];
                std::string output;
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int status = pclose(pipe);
                
                // JSON出力をパース
                try {
                    size_t json_start = output.find("{");
                    if (json_start != std::string::npos) {
                        std::string json_str = output.substr(json_start);
                        nlohmann::json dead_code_result = nlohmann::json::parse(json_str);
                        
                        // dead_code_infoを設定
                        if (dead_code_result.contains("dead_code")) {
                            result["dead_code"] = dead_code_result["dead_code"];
                            
                            // SessionDataにも保存（mutableにする必要があるため、const_castを使用）
                            const_cast<SessionData&>(session).dead_code_info = dead_code_result["dead_code"];
                        }
                    }
                } catch (const std::exception& e) {
                    result["dead_code"] = {
                        {"status", "error"},
                        {"message", std::string("Failed to parse Python output: ") + e.what()}
                    };
                }
            } else {
                result["dead_code"] = {
                    {"status", "error"},
                    {"message", "Failed to execute Python script"}
                };
            }
        } catch (const std::exception& e) {
            result["dead_code"] = {
                {"status", "error"},
                {"message", std::string("Dead code analysis failed: ") + e.what()}
            };
        }
    }
    
    result["deep"] = deep;
    result["complete"] = complete;
    result["summary"] = complete ? "Complete analysis with dead code detection" : "Basic structure analysis";
    
    return result;
}

nlohmann::json SessionCommands::cmd_duplicates(const SessionData& session) const {
    return {
        {"command", "duplicates"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Duplicates feature pending implementation"}
    };
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

} // namespace nekocode