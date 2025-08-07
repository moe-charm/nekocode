//=============================================================================
// 📊 Basic Commands実装 - 基本統計・ヘルプコマンド
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
        if (file.file_info.total_lines > 500) {
            large_count++;
        }
        
        files_json[file.file_info.name] = {
            {"lines", file.file_info.total_lines},
            {"size", file.file_info.size_bytes},
            {"language", static_cast<int>(file.language)},
            {"complexity", file.complexity.cyclomatic_complexity}
        };
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
                "replace-preview <file> <pattern> <replacement> - Preview replacement",
                "replace-confirm <preview_id> - Confirm replacement",
                "edit-history - Show edit history",
                "edit-show <id> - Show edit details",
                "insert-preview <file> <position> <content> - Preview insertion",
                "insert-confirm <preview_id> - Confirm insertion",
                "movelines-preview <srcfile> <start_line> <line_count> <dstfile> <insert_line> - Preview line move",
                "movelines-confirm <preview_id> - Confirm line move",
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

} // namespace nekocode