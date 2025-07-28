//=============================================================================
// 🎨 NekoCode Formatters Implementation - 簡略版
//
// 実行ファイル２個大作戦: 出力フォーマッター実装（最小限）
//=============================================================================

#include "nekocode/formatters.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include <sstream>
#include <iomanip>

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
    
    json_result["statistics"] = {
        {"total_classes", result.classes.size()},
        {"total_functions", result.functions.size()},
        {"total_imports", result.imports.size()},
        {"total_exports", result.exports.size()}
    };
    
    // 🎯 詳細なクラス情報
    if (!result.classes.empty()) {
        nlohmann::json classes_json = nlohmann::json::array();
        for (const auto& cls : result.classes) {
            nlohmann::json class_json = {
                {"name", cls.name},
                {"start_line", cls.start_line}
            };
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
    
    for (const auto& file : analysis.files) {
        total_classes += file.classes.size();
        total_functions += file.functions.size();
        total_lines += file.file_info.total_lines;
    }
    
    json_result["summary"] = {
        {"total_classes", total_classes},
        {"total_functions", total_functions},
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
    
    for (const auto& file : analysis.files) {
        total_classes += file.classes.size();
        total_functions += file.functions.size();
        total_lines += file.file_info.total_lines;
    }
    
    ss << "📈 Project Summary\n";
    ss << "-------------------\n";
    ss << "  🏗️ Total Classes: " << total_classes << "\n";
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