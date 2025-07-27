//=============================================================================
// 🔥 C++ Code Analyzer Implementation - 地獄のnyamesh_v22対応版
//
// 大規模C++プロジェクト対応:
// - EditorCore_v22.cpp (1,366行)
// - MillionPeerP2PTransport.cpp (864行)
// - 複雑なクラス階層・テンプレート・名前空間
//=============================================================================

#include "nekocode/cpp_analyzer.hpp"
#include "nekocode/utf8_utils.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace nekocode {

//=============================================================================
// 🏗️ CppAnalyzer Construction
//=============================================================================

CppAnalyzer::CppAnalyzer() 
    : cpp_standard_("C++17")
    , comment_style_(CommentStyle::ALL) {
    initialize_patterns();
    initialize_cpp_keywords();
}

CppAnalyzer::~CppAnalyzer() = default;

//=============================================================================
// 🎯 Main Analysis Interface
//=============================================================================

CppAnalysisResult CppAnalyzer::analyze_cpp_file(const std::string& content, const std::string& filename) {
    CppAnalysisResult result;
    result.file_info.name = filename;
    result.language = Language::CPP;
    
    if (content.empty()) {
        return result;
    }
    
    // UTF-8 safe preprocessing
    std::string clean_content = remove_cpp_comments(content, true);
    clean_content = remove_cpp_literals(clean_content);
    
    // 基本ファイル情報
    auto lines = utf8::split_lines_safe(content);
    result.file_info.total_lines = static_cast<uint32_t>(lines.size());
    result.file_info.size_bytes = content.size();
    
    // C++構造解析
    if (config_.analyze_namespaces) {
        result.namespaces = analyze_namespaces(clean_content);
    }
    
    if (config_.analyze_classes) {
        result.classes = analyze_classes(clean_content);
    }
    
    if (config_.analyze_functions) {
        result.functions = analyze_functions(clean_content);
    }
    
    if (config_.analyze_includes) {
        result.includes = analyze_includes(content); // 元のコンテンツから
    }
    
    // テンプレート・マクロ解析（新機能）
    if (config_.analyze_templates) {
        result.template_analysis = analyze_templates_and_macros(content);
    }
    
    // 複雑度解析
    if (config_.calculate_cyclomatic) {
        result.complexity = calculate_cpp_complexity(content);
    }
    
    // 統計計算
    calculate_cpp_statistics(result);
    
    return result;
}

CppAnalysisResult CppAnalyzer::analyze_cpp_stats_only(const std::string& content, const std::string& filename) {
    CppAnalysisResult result;
    result.file_info.name = filename;
    result.language = Language::CPP;
    
    if (content.empty()) {
        return result;
    }
    
    // 高速統計のみ
    auto lines = utf8::split_lines_safe(content);
    result.file_info.total_lines = static_cast<uint32_t>(lines.size());
    result.file_info.size_bytes = content.size();
    
    // 簡易カウント
    result.cpp_stats.class_count = std::count_if(lines.begin(), lines.end(), 
        [](const std::string& line) {
            return line.find("class ") != std::string::npos;
        });
    
    result.cpp_stats.function_count = std::count_if(lines.begin(), lines.end(),
        [](const std::string& line) {
            return line.find("(") != std::string::npos && line.find(")") != std::string::npos;
        });
    
    result.cpp_stats.include_count = std::count_if(lines.begin(), lines.end(),
        [](const std::string& line) {
            return line.find("#include") != std::string::npos;
        });
    
    // 簡易複雑度
    result.complexity.cyclomatic_complexity = std::count_if(lines.begin(), lines.end(),
        [](const std::string& line) {
            return line.find("if") != std::string::npos || 
                   line.find("for") != std::string::npos ||
                   line.find("while") != std::string::npos;
        });
    
    result.complexity.update_rating();
    
    return result;
}

//=============================================================================
// 🏗️ Structure Analysis Implementation
//=============================================================================

std::vector<CppNamespace> CppAnalyzer::analyze_namespaces(const std::string& content) {
    std::vector<CppNamespace> namespaces;
    
    std::regex ns_regex(R"(namespace\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
    std::sregex_iterator iter(content.begin(), content.end(), ns_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        CppNamespace ns;
        ns.name = (*iter)[1].str();
        ns.source_language = Language::CPP;
        
        // 行番号計算（簡易）
        size_t pos = iter->position();
        ns.start_line = static_cast<uint32_t>(
            std::count(content.begin(), content.begin() + pos, '\n') + 1
        );
        
        namespaces.push_back(ns);
    }
    
    // 匿名名前空間検出
    std::regex anon_ns_regex(R"(namespace\s*\{)");
    std::sregex_iterator anon_iter(content.begin(), content.end(), anon_ns_regex);
    
    for (; anon_iter != end; ++anon_iter) {
        CppNamespace ns;
        ns.name = "(anonymous)";
        ns.is_anonymous = true;
        ns.source_language = Language::CPP;
        
        size_t pos = anon_iter->position();
        ns.start_line = static_cast<uint32_t>(
            std::count(content.begin(), content.begin() + pos, '\n') + 1
        );
        
        namespaces.push_back(ns);
    }
    
    return namespaces;
}

std::vector<CppClass> CppAnalyzer::analyze_classes(const std::string& content) {
    std::vector<CppClass> classes;
    
    // クラス検出
    std::regex class_regex(R"((class|struct|union)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*([^{]+))?\s*\{)");
    std::sregex_iterator iter(content.begin(), content.end(), class_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        CppClass cls;
        cls.source_language = Language::CPP;
        
        std::string type_str = (*iter)[1].str();
        if (type_str == "class") {
            cls.class_type = CppClass::CLASS;
        } else if (type_str == "struct") {
            cls.class_type = CppClass::STRUCT;
        } else if (type_str == "union") {
            cls.class_type = CppClass::UNION;
        }
        
        cls.name = (*iter)[2].str();
        
        // 継承関係解析
        if ((*iter)[3].matched) {
            std::string inheritance_str = (*iter)[3].str();
            cls.base_classes = parse_base_classes(inheritance_str);
        }
        
        // 行番号計算
        size_t pos = iter->position();
        cls.start_line = static_cast<uint32_t>(
            std::count(content.begin(), content.begin() + pos, '\n') + 1
        );
        
        classes.push_back(cls);
    }
    
    return classes;
}

std::vector<CppFunction> CppAnalyzer::analyze_functions(const std::string& content) {
    std::vector<CppFunction> functions;
    
    // 関数検出（簡易実装）
    std::regex func_regex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\s*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?\s*[{;])");
    std::sregex_iterator iter(content.begin(), content.end(), func_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        CppFunction func;
        func.source_language = Language::CPP;
        func.name = (*iter)[1].str();
        
        // パラメータ解析
        std::string params_str = (*iter)[2].str();
        func.parameters = parse_function_parameters(params_str);
        
        // 行番号計算
        size_t pos = iter->position();
        func.start_line = static_cast<uint32_t>(
            std::count(content.begin(), content.begin() + pos, '\n') + 1
        );
        
        // 簡易修飾子検出
        std::string full_match = iter->str();
        func.is_const = full_match.find("const") != std::string::npos;
        func.is_virtual = full_match.find("virtual") != std::string::npos;
        
        functions.push_back(func);
    }
    
    return functions;
}

std::vector<CppInclude> CppAnalyzer::analyze_includes(const std::string& content) {
    std::vector<CppInclude> includes;
    
    std::regex include_regex(R"(#include\s*([<"])([^>"]+)[>"])");
    std::sregex_iterator iter(content.begin(), content.end(), include_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        CppInclude inc;
        inc.path = (*iter)[2].str();
        inc.is_system_include = ((*iter)[1].str() == "<");
        
        // 行番号計算
        size_t pos = iter->position();
        inc.line_number = static_cast<uint32_t>(
            std::count(content.begin(), content.begin() + pos, '\n') + 1
        );
        
        includes.push_back(inc);
    }
    
    return includes;
}

//=============================================================================
// 🧮 C++ Complexity Analysis
//=============================================================================

ComplexityInfo CppAnalyzer::calculate_cpp_complexity(const std::string& content) {
    ComplexityInfo complexity;
    
    // C++ 制御構造キーワード
    std::vector<std::string> control_keywords = {
        "if", "else", "while", "for", "do", "switch", "case", "catch", "try",
        "&&", "||", "?", ":", "return", "break", "continue"
    };
    
    // サイクロマチック複雑度計算
    complexity.cyclomatic_complexity = 1; // 基本パス
    
    for (const auto& keyword : control_keywords) {
        std::regex keyword_regex("\\b" + keyword + "\\b");
        std::sregex_iterator iter(content.begin(), content.end(), keyword_regex);
        std::sregex_iterator end;
        
        complexity.cyclomatic_complexity += static_cast<uint32_t>(std::distance(iter, end));
    }
    
    // 最大ネスト深度計算
    uint32_t current_depth = 0;
    complexity.max_nesting_depth = 0;
    
    for (char c : content) {
        if (c == '{') {
            current_depth++;
            complexity.max_nesting_depth = std::max(complexity.max_nesting_depth, current_depth);
        } else if (c == '}' && current_depth > 0) {
            current_depth--;
        }
    }
    
    // 認知複雑度（簡易実装）
    complexity.cognitive_complexity = complexity.cyclomatic_complexity + 
                                     (complexity.max_nesting_depth * 2);
    
    complexity.update_rating();
    
    return complexity;
}

uint32_t CppAnalyzer::calculate_template_complexity(const std::string& content) {
    std::regex template_regex(R"(template\s*<[^>]*>)");
    std::sregex_iterator iter(content.begin(), content.end(), template_regex);
    std::sregex_iterator end;
    
    return static_cast<uint32_t>(std::distance(iter, end));
}

uint32_t CppAnalyzer::calculate_inheritance_complexity(const std::vector<CppClass>& classes) {
    uint32_t complexity = 0;
    
    for (const auto& cls : classes) {
        // 継承階層の深さを加算
        complexity += static_cast<uint32_t>(cls.base_classes.size());
        
        // 仮想関数の数を加算（簡易推測）
        complexity += cls.methods.size() / 4; // 約25%が仮想関数と仮定
    }
    
    return complexity;
}

//=============================================================================
// ⚙️ Configuration
//=============================================================================

void CppAnalyzer::set_analysis_config(const LanguageAnalysisConfig& config) {
    config_ = config;
}

void CppAnalyzer::set_cpp_standard(const std::string& standard) {
    cpp_standard_ = standard;
}

void CppAnalyzer::set_comment_style(CommentStyle style) {
    comment_style_ = style;
}

//=============================================================================
// 🔧 Internal Implementation
//=============================================================================

void CppAnalyzer::initialize_patterns() {
    // 基本パターン初期化
    namespace_regex_ = std::regex(R"(namespace\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
    class_regex_ = std::regex(R"(class\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*([^{]+))?\s*\{)");
    struct_regex_ = std::regex(R"(struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*([^{]+))?\s*\{)");
    function_regex_ = std::regex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\s*\))");
    include_regex_ = std::regex(R"(#include\s*([<"])([^>"]+)[>"])");
    template_regex_ = std::regex(R"(template\s*<[^>]*>)");
    macro_regex_ = std::regex(R"(#define\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(.*))");
    enum_regex_ = std::regex(R"(enum\s+(?:class\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
}

void CppAnalyzer::initialize_cpp_keywords() {
    cpp_keywords_ = {
        // C++ キーワード
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
        "compl", "concept", "const", "constexpr", "const_cast", "continue",
        "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
        "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
        "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
        "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
        "private", "protected", "public", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "template", "this", "thread_local",
        "throw", "true", "try", "typedef", "typeid", "typename", "union",
        "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
        "xor", "xor_eq", "override", "final"
    };
    
    access_specifiers_ = {"public", "private", "protected"};
    storage_specifiers_ = {"static", "extern", "mutable", "thread_local", "register"};
}

//=============================================================================
// 🎯 Parsing Helpers
//=============================================================================

std::string CppAnalyzer::remove_cpp_comments(const std::string& content, bool preserve_doxygen) {
    std::string result = content;
    
    if (!preserve_doxygen || comment_style_ == CommentStyle::STANDARD_ONLY) {
        // 全コメント除去
        // 単行コメント
        std::regex single_comment_regex(R"(//.*$)", std::regex_constants::multiline);
        result = std::regex_replace(result, single_comment_regex, "");
        
        // 複数行コメント
        std::regex multi_comment_regex(R"(/\*[\s\S]*?\*/)");
        result = std::regex_replace(result, multi_comment_regex, "");
    } else {
        // Doxygen コメント保持
        // 通常の単行コメントのみ除去（/// は保持）
        std::regex normal_single_regex(R"(//(?!/)[^/].*$)", std::regex_constants::multiline);
        result = std::regex_replace(result, normal_single_regex, "");
        
        // 通常の複数行コメントのみ除去（/** は保持）
        std::regex normal_multi_regex(R"(/\*(?!\*)[\s\S]*?\*/)");
        result = std::regex_replace(result, normal_multi_regex, "");
    }
    
    return result;
}

std::string CppAnalyzer::remove_cpp_literals(const std::string& content) {
    std::string result = content;
    
    // Raw 文字列リテラル R"(...)"
    std::regex raw_string_regex(R"(R\"[^(]*\(.*?\)[^\"]*\")");
    result = std::regex_replace(result, raw_string_regex, "R\"\"");
    
    // 通常の文字列リテラル
    std::regex string_regex(R"("(?:[^"\\]|\\.)*")");
    result = std::regex_replace(result, string_regex, "\"\"");
    
    // 文字リテラル
    std::regex char_regex(R"('(?:[^'\\]|\\.)*')");
    result = std::regex_replace(result, char_regex, "''");
    
    return result;
}

std::vector<std::string> CppAnalyzer::parse_function_parameters(const std::string& params_str) {
    std::vector<std::string> parameters;
    
    if (params_str.empty() || params_str.find_first_not_of(" \t") == std::string::npos) {
        return parameters;
    }
    
    // 簡易パラメータ分割
    std::stringstream ss(params_str);
    std::string param;
    
    while (std::getline(ss, param, ',')) {
        // トリミング
        param.erase(0, param.find_first_not_of(" \t"));
        param.erase(param.find_last_not_of(" \t") + 1);
        
        if (!param.empty()) {
            parameters.push_back(param);
        }
    }
    
    return parameters;
}

std::vector<std::string> CppAnalyzer::parse_base_classes(const std::string& inheritance_str) {
    std::vector<std::string> base_classes;
    
    // アクセス指定子を除去して基底クラス名を抽出
    std::regex base_regex(R"((?:public|private|protected)\s+([a-zA-Z_][a-zA-Z0-9_:]*))");
    std::sregex_iterator iter(inheritance_str.begin(), inheritance_str.end(), base_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        base_classes.push_back((*iter)[1].str());
    }
    
    return base_classes;
}

//=============================================================================
// 📊 Statistics Calculation
//=============================================================================

void CppAnalyzer::calculate_cpp_statistics(CppAnalysisResult& result) {
    result.update_statistics();
    
    // 追加統計計算
    result.file_info.code_lines = result.file_info.total_lines; // 簡易実装
    result.file_info.code_ratio = 1.0; // 簡易実装
}

//=============================================================================
// 🎯 C++ Language Features Detection
//=============================================================================

CppFeatureDetector::CppFeatures CppFeatureDetector::detect_features(const std::string& content) {
    CppFeatures features;
    
    // C++11 features
    features.has_auto_keyword = content.find("auto ") != std::string::npos;
    features.has_range_based_for = content.find("for (") != std::string::npos && 
                                   content.find(" : ") != std::string::npos;
    features.has_lambda = content.find("[]") != std::string::npos;
    features.has_smart_pointers = content.find("std::unique_ptr") != std::string::npos ||
                                  content.find("std::shared_ptr") != std::string::npos;
    features.has_nullptr = content.find("nullptr") != std::string::npos;
    
    // C++14 features
    features.has_constexpr = content.find("constexpr") != std::string::npos;
    
    // C++17 features
    features.has_move_semantics = content.find("std::move") != std::string::npos;
    
    // C++20 features  
    features.has_concepts = content.find("concept") != std::string::npos;
    features.has_modules = content.find("import ") != std::string::npos;
    features.has_coroutines = content.find("co_await") != std::string::npos ||
                              content.find("co_yield") != std::string::npos;
    
    features.estimated_standard = estimate_cpp_standard(features);
    
    return features;
}

std::string CppFeatureDetector::estimate_cpp_standard(const CppFeatures& features) {
    if (features.has_concepts || features.has_modules || features.has_coroutines) {
        return "C++20";
    }
    if (features.has_move_semantics) {
        return "C++17";
    }
    if (features.has_constexpr) {
        return "C++14";
    }
    if (features.has_auto_keyword || features.has_lambda || features.has_nullptr) {
        return "C++11";
    }
    return "C++98";
}

//=============================================================================
// 🎯 Advanced C++ Features Implementation
//=============================================================================

std::vector<CppTemplate> CppAnalyzer::analyze_templates(const std::string& content) {
    std::vector<CppTemplate> templates;
    
    std::regex template_regex(R"(template\s*<([^>]*)>\s*(?:class|struct|typename)\s+(\w+))");
    std::sregex_iterator iter(content.begin(), content.end(), template_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        CppTemplate tmpl;
        std::string params = iter->str(1);
        
        // パラメータ解析
        if (!params.empty()) {
            std::stringstream ss(params);
            std::string param;
            while (std::getline(ss, param, ',')) {
                param.erase(0, param.find_first_not_of(" \t"));
                param.erase(param.find_last_not_of(" \t") + 1);
                if (!param.empty()) {
                    tmpl.parameters.push_back(param);
                }
            }
        }
        
        // 可変長テンプレートチェック
        if (params.find("...") != std::string::npos) {
            tmpl.is_variadic = true;
        }
        
        templates.push_back(tmpl);
    }
    
    return templates;
}

std::vector<std::pair<std::string, std::string>> CppAnalyzer::analyze_macros(const std::string& content) {
    std::vector<std::pair<std::string, std::string>> macros;
    
    std::regex macro_regex(R"(#define\s+(\w+)(?:\([^)]*\))?\s*(.*)(?:\n|$))");
    std::sregex_iterator iter(content.begin(), content.end(), macro_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string name = iter->str(1);
        std::string definition = iter->str(2);
        
        // 複数行マクロの簡単な処理
        definition.erase(definition.find_last_not_of(" \t\r\n") + 1);
        
        macros.emplace_back(name, definition);
    }
    
    return macros;
}

//=============================================================================
// 🔥 Template & Macro Analysis Implementation - 新機能
//=============================================================================

TemplateAnalysisResult CppAnalyzer::analyze_templates_and_macros(const std::string& content) {
    TemplateAnalysisResult result;
    
    // 1. テンプレート解析（既存機能を活用）
    result.templates = analyze_templates(content);
    
    // 2. マクロ解析（既存機能を拡張）
    auto macro_pairs = analyze_macros(content);
    for (const auto& pair : macro_pairs) {
        CppMacro macro;
        macro.name = pair.first;
        macro.definition = pair.second;
        
        // 関数マクロの検出
        if (macro.definition.find('(') != std::string::npos) {
            macro.is_function_like = true;
            // パラメータ抽出の簡単な実装
            size_t start = macro.definition.find('(');
            size_t end = macro.definition.find(')');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string params = macro.definition.substr(start + 1, end - start - 1);
                if (!params.empty()) {
                    std::stringstream ss(params);
                    std::string param;
                    while (std::getline(ss, param, ',')) {
                        param.erase(0, param.find_first_not_of(" \t"));
                        param.erase(param.find_last_not_of(" \t") + 1);
                        if (!param.empty()) {
                            macro.parameters.push_back(param);
                        }
                    }
                }
            }
        }
        
        result.macros.push_back(macro);
    }
    
    // 3. テンプレート特殊化検出
    std::regex specialization_regex(R"(template\s*<\s*>\s*\w+\s*<[^>]+>)");
    std::sregex_iterator iter(content.begin(), content.end(), specialization_regex);
    std::sregex_iterator end;
    for (; iter != end; ++iter) {
        result.template_specializations.push_back(iter->str());
    }
    
    // 4. 可変長テンプレート検出
    std::regex variadic_regex(R"(template\s*<[^>]*\.\.\.[^>]*>)");
    std::sregex_iterator variadic_iter(content.begin(), content.end(), variadic_regex);
    for (; variadic_iter != end; ++variadic_iter) {
        result.variadic_templates.push_back(variadic_iter->str());
    }
    
    // 5. 統計カウント
    result.template_instantiation_count = static_cast<uint32_t>(result.templates.size());
    result.macro_expansion_count = static_cast<uint32_t>(result.macros.size());
    
    return result;
}

} // namespace nekocode