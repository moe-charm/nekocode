//=============================================================================
// 🌳 AST Commands実装 - AST Revolution 高級解析コマンド
//=============================================================================

#include "nekocode/session_commands.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <sstream>
#include <set>
#include <functional>
#include <filesystem>

namespace nekocode {

//=============================================================================
// 🌳 AST Revolution コマンド実装
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