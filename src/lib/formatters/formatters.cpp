//=============================================================================
// 🎨 NekoCode Formatters Implementation - 簡略版
//
// 実行ファイル２個大作戦: 出力フォーマッター実装（最小限）
//=============================================================================

#include "nekocode/formatters.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_debug_mode;

namespace nekocode {

//=============================================================================
// 🏭 FormatterFactory Implementation
//=============================================================================

std::unique_ptr<IReportFormatter> FormatterFactory::create_formatter(OutputFormat format) {
    switch (format) {
        case OutputFormat::AI_JSON:
            return std::make_unique<AIReportFormatter>();
        case OutputFormat::HUMAN_TEXT:
            return std::make_unique<HumanReportFormatter>();
        // case OutputFormat::COMPACT:
        //     return std::make_unique<AIReportFormatter>(); // Use AI formatter for compact
        default:
            return std::make_unique<AIReportFormatter>();
    }
}

//=============================================================================
// 🤖 AIReportFormatter Implementation - Claude Code最適化
//=============================================================================

AIReportFormatter::AIReportFormatter() {}

std::string AIReportFormatter::format_single_file(const AnalysisResult& result) {
    
    nlohmann::json json_result;
    
    json_result["analysis_type"] = "single_file";
    json_result["file_info"] = {
        {"name", result.file_info.name},
        {"total_lines", result.file_info.total_lines},
        {"code_lines", result.file_info.code_lines},
        {"size_bytes", result.file_info.size_bytes}
    };
    
    // 🔥 デバッグ：統計フィールド確認
    if (g_debug_mode) {
        // std::cerr << "🔥 Formatter debug: result.stats.class_count=" << result.stats.class_count
        //           << ", result.stats.function_count=" << result.stats.function_count << std::endl;
    }
    
    // std::cerr << "🔥 Formatter: result.stats.commented_lines_count=" << result.stats.commented_lines_count << std::endl;
    json_result["statistics"] = {
        {"total_classes", result.stats.class_count},      // 🔥 修正：正しい統計フィールドを使用
        {"total_functions", result.stats.function_count}, // 🔥 修正：正しい統計フィールドを使用
        {"total_imports", result.stats.import_count},     // 🔥 修正：正しい統計フィールドを使用
        {"total_exports", result.stats.export_count},     // 🔥 修正：正しい統計フィールドを使用
        {"commented_lines_count", result.stats.commented_lines_count}  // 🆕 コメントアウト行数
    };
    
    // 🎯 詳細なクラス情報
    if (!result.classes.empty()) {
        nlohmann::json classes_json = nlohmann::json::array();
        for (const auto& cls : result.classes) {
            nlohmann::json class_json = {
                {"name", cls.name},
                {"start_line", cls.start_line}
            };
            
            // メンバ変数情報を追加
            if (!cls.member_variables.empty()) {
                nlohmann::json member_vars_json = nlohmann::json::array();
                for (const auto& var : cls.member_variables) {
                    nlohmann::json var_json = {
                        {"name", var.name},
                        {"type", var.type},
                        {"line", var.declaration_line},
                        {"access", var.access_modifier}
                    };
                    if (var.is_static) var_json["static"] = true;
                    if (var.is_const) var_json["const"] = true;
                    member_vars_json.push_back(var_json);
                }
                class_json["member_variables"] = member_vars_json;
            }
            
            classes_json.push_back(class_json);
        }
        json_result["classes"] = classes_json;
    }
    
    // 🎯 詳細な関数情報
    if (!result.functions.empty()) {
        nlohmann::json functions_json = nlohmann::json::array();
        for (const auto& func : result.functions) {
            nlohmann::json func_json = {
                {"name", func.name},
                {"start_line", func.start_line}
            };
            if (func.is_async) {
                func_json["is_async"] = true;
            }
            if (func.is_arrow_function) {
                func_json["is_arrow_function"] = true;
            }
            functions_json.push_back(func_json);
        }
        json_result["functions"] = functions_json;
    }
    
    // 🎯 詳細なimport情報
    if (!result.imports.empty()) {
        nlohmann::json imports_json = nlohmann::json::array();
        for (const auto& imp : result.imports) {
            nlohmann::json import_json = {
                {"module_path", imp.module_path},
                {"line_number", imp.line_number},
                {"type", imp.type == ImportType::ES6_IMPORT ? "ES6_IMPORT" : "COMMONJS_REQUIRE"}
            };
            imports_json.push_back(import_json);
        }
        json_result["imports"] = imports_json;
    }
    
    // 🎯 詳細なexport情報
    if (!result.exports.empty()) {
        nlohmann::json exports_json = nlohmann::json::array();
        for (const auto& exp : result.exports) {
            nlohmann::json export_json = {
                {"line_number", exp.line_number},
                {"type", exp.type == ExportType::ES6_EXPORT ? "ES6_EXPORT" : "COMMONJS_EXPORT"}
            };
            if (!exp.exported_names.empty()) {
                export_json["exported_names"] = exp.exported_names;
            }
            exports_json.push_back(export_json);
        }
        json_result["exports"] = exports_json;
    }
    
    // 🆕 詳細なコメントアウト行情報
    // std::cerr << "🔥 Formatter: commented_lines.size()=" << result.commented_lines.size() << std::endl;
    if (!result.commented_lines.empty()) {
        // std::cerr << "🔥 Formatter: Processing commented_lines..." << std::endl;
        nlohmann::json commented_lines_json = nlohmann::json::array();
        for (const auto& comment : result.commented_lines) {
            nlohmann::json comment_json = {
                {"line_start", comment.line_start},
                {"line_end", comment.line_end},
                {"type", comment.type},
                {"content", comment.content},
                {"looks_like_code", comment.looks_like_code}
            };
            commented_lines_json.push_back(comment_json);
        }
        json_result["commented_lines"] = commented_lines_json;
    }
    
    if (result.complexity.cyclomatic_complexity > 0) {
        json_result["complexity"] = {
            {"cyclomatic_complexity", result.complexity.cyclomatic_complexity},
            {"cognitive_complexity", result.complexity.cognitive_complexity},
            {"max_nesting_depth", result.complexity.max_nesting_depth}
        };
    }
    
    // Template & Macro analysis (C++ only)
    if (result.language == Language::CPP) {
        const auto* cpp_result = dynamic_cast<const CppAnalysisResult*>(&result);
        if (cpp_result && (!cpp_result->template_analysis.templates.empty() || !cpp_result->template_analysis.macros.empty())) {
            nlohmann::json templates_json = nlohmann::json::array();
            for (const auto& tmpl : cpp_result->template_analysis.templates) {
                nlohmann::json tmpl_json = nlohmann::json::object();
                tmpl_json["name"] = tmpl.name;
                tmpl_json["type"] = tmpl.type;
                tmpl_json["parameters"] = tmpl.parameters;
                tmpl_json["is_variadic"] = tmpl.is_variadic;
                templates_json.push_back(tmpl_json);
            }
            
            nlohmann::json macros_json = nlohmann::json::array();
            for (const auto& macro : cpp_result->template_analysis.macros) {
                nlohmann::json macro_json = nlohmann::json::object();
                macro_json["name"] = macro.name;
                macro_json["definition"] = macro.definition;
                macro_json["parameters"] = macro.parameters;
                macro_json["is_function_like"] = macro.is_function_like;
                macros_json.push_back(macro_json);
            }
            
            json_result["template_analysis"] = nlohmann::json::object();
            json_result["template_analysis"]["templates"] = templates_json;
            json_result["template_analysis"]["macros"] = macros_json;
            json_result["template_analysis"]["template_count"] = cpp_result->template_analysis.templates.size();
            json_result["template_analysis"]["macro_count"] = cpp_result->template_analysis.macros.size();
        }
    }
    
    return json_result.dump(2);
}

std::string AIReportFormatter::format_directory(const DirectoryAnalysis& analysis) {
    nlohmann::json json_result;
    
    json_result["analysis_type"] = "directory";
    json_result["directory_path"] = analysis.directory_path.string();
    json_result["total_files"] = analysis.files.size();
    
    // ファイル統計
    uint32_t total_classes = 0;
    uint32_t total_functions = 0;
    uint32_t total_lines = 0;
    uint32_t total_commented_lines = 0;  // 🆕 コメントアウト行統計
    
    for (const auto& file : analysis.files) {
        total_classes += file.classes.size();
        total_functions += file.functions.size();
        total_lines += file.file_info.total_lines;
        total_commented_lines += file.stats.commented_lines_count;  // 🆕 コメントアウト行数を集計
    }
    
    json_result["summary"] = {
        {"total_classes", total_classes},
        {"total_functions", total_functions},
        {"total_commented_lines", total_commented_lines},  // 🆕 コメントアウト行数を追加
        {"total_lines", total_lines}
    };
    
    return json_result.dump(2);
}

std::string AIReportFormatter::format_summary(const DirectoryAnalysis::Summary& summary) {
    nlohmann::json json_result;
    
    json_result["analysis_type"] = "summary";
    json_result["summary"] = {
        {"total_files", summary.total_files},
        {"total_lines", summary.total_lines},
        {"total_classes", summary.total_classes},
        {"total_functions", summary.total_functions}
    };
    
    return json_result.dump(2);
}

//=============================================================================
// 👨‍💻 HumanReportFormatter Implementation - 美しいテキスト出力
//=============================================================================

HumanReportFormatter::HumanReportFormatter() {}

std::string HumanReportFormatter::format_single_file(const AnalysisResult& result) {
    std::stringstream ss;
    
    // ヘッダー
    ss << "\n+======================================================================+\n";
    ss << "|                     📄 File Analysis Report                        |\n";
    ss << "+======================================================================+\n\n";
    
    ss << "📁 File: " << result.file_info.name << "\n\n";
    
    // ファイル情報
    ss << "📊 File Information\n";
    ss << "---------------------\n";
    ss << "  📏 Total Lines: " << result.file_info.total_lines << "\n";
    ss << "  💻 Code Lines: " << result.file_info.code_lines << "\n";
    ss << "  💾 File Size: " << result.file_info.size_bytes << " bytes\n\n";
    
    // 統計
    ss << "📈 Code Statistics\n";
    ss << "-------------------\n";
    ss << "  🏗️ Classes: " << result.classes.size() << "\n";
    ss << "  ⚙️ Functions: " << result.functions.size() << "\n";
    ss << "  📥 Imports: " << result.imports.size() << "\n";
    ss << "  📤 Exports: " << result.exports.size() << "\n\n";
    
    // 複雑度
    if (result.complexity.cyclomatic_complexity > 0) {
        ss << "🧮 Complexity Analysis\n";
        ss << "-----------------------\n";
        ss << "  🔄 Cyclomatic Complexity: " << result.complexity.cyclomatic_complexity << "\n";
        ss << "  🧠 Cognitive Complexity: " << result.complexity.cognitive_complexity << "\n";
        ss << "  📊 Max Nesting Depth: " << result.complexity.max_nesting_depth << "\n";
        ss << "\n";
    }
    
    ss << "✨ Analysis completed by NekoCode C++ Engine ✨\n";
    
    return ss.str();
}

std::string HumanReportFormatter::format_directory(const DirectoryAnalysis& analysis) {
    std::stringstream ss;
    
    // ヘッダー
    ss << "\n+======================================================================+\n";
    ss << "|                   📁 Directory Analysis Report                     |\n";
    ss << "+======================================================================+\n\n";
    
    ss << "📁 Directory: " << analysis.directory_path.filename() << "\n";
    ss << "📊 Total Files: " << analysis.files.size() << "\n\n";
    
    // 統計
    uint32_t total_classes = 0;
    uint32_t total_functions = 0;
    uint32_t total_lines = 0;
    uint32_t total_commented_lines = 0;  // 🆕 コメントアウト行統計
    
    for (const auto& file : analysis.files) {
        total_classes += file.classes.size();
        total_functions += file.functions.size();
        total_lines += file.file_info.total_lines;
        total_commented_lines += file.stats.commented_lines_count;  // 🆕 コメントアウト行数を集計
    }
    
    ss << "📈 Project Summary\n";
    ss << "-------------------\n";
    ss << "  🏗️ Total Classes: " << total_classes << "\n";
    ss << "  📝 Total Commented Lines: " << total_commented_lines << "\n";  // 🆕 コメントアウト行数を表示
    ss << "  ⚙️ Total Functions: " << total_functions << "\n";
    ss << "  📏 Total Lines: " << total_lines << "\n\n";
    
    ss << "✨ Analysis completed by NekoCode C++ Engine ✨\n";
    
    return ss.str();
}

std::string HumanReportFormatter::format_summary(const DirectoryAnalysis::Summary& summary) {
    std::stringstream ss;
    
    ss << "\n+======================================================================+\n";
    ss << "|                      📊 Project Summary                            |\n";
    ss << "+======================================================================+\n\n";
    
    ss << "📁 Total Files: " << summary.total_files << "\n";
    ss << "📏 Total Lines: " << summary.total_lines << "\n";
    ss << "🏗️ Total Classes: " << summary.total_classes << "\n";
    ss << "⚙️ Total Functions: " << summary.total_functions << "\n\n";
    
    ss << "✨ Analysis completed by NekoCode C++ Engine ✨\n";
    
    return ss.str();
}

} // namespace nekocode