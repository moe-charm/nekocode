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
    // 正規表現問題のため一時的に無効化
    // if (config_.analyze_namespaces) {
    //     result.namespaces = analyze_namespaces(clean_content);
    // }
    
    // if (config_.analyze_classes) {
    //     result.cpp_classes = analyze_classes(clean_content);
    // }
    
    // if (config_.analyze_functions) {
    //     result.cpp_functions = analyze_functions(clean_content);
    // }
    
    // if (config_.analyze_includes) {
    //     result.includes = analyze_includes(content); // 元のコンテンツから
    // }
    
    // テンプレート・マクロ解析（新機能） - 一時的に無効化
    // if (config_.analyze_templates) {
    //     result.template_analysis = analyze_templates_and_macros(content);
    // }
    
    // 複雑度解析 - 正規表現問題のため一時的に無効化
    // if (config_.calculate_cyclomatic) {
    //     result.complexity = calculate_cpp_complexity(content);
    // }
    
    // 統計計算 - 正規表現問題のため一時的に無効化
    // calculate_cpp_statistics(result);
    
    // 🔧 シンプルな関数検出を追加（正規表現を使わない）
    result.cpp_functions = extract_functions_simple(clean_content);
    
    // 🔧 ファイル全体の複雑度も計算
    result.complexity = calculate_cpp_complexity(content);
    
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
    
    // クラス検出（テンプレート対応）
    std::regex class_regex(R"((class|struct|union)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*[^{]*)?\s*\{)");
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
    
    // 関数検出（戻り値型対応）
    std::regex func_regex(R"((?:[a-zA-Z_][a-zA-Z0-9_:]*\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?\s*[{;])");
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

// 🔧 シンプルな関数検出（正規表現を使わない文字列ベース）
std::vector<CppFunction> CppAnalyzer::extract_functions_simple(const std::string& content) {
    std::vector<CppFunction> functions;
    auto lines = utf8::split_lines_safe(content);
    
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        // 関数定義の基本パターン: "(" と ")" と "{" が同じ行または近くにある
        size_t paren_open = line.find('(');
        size_t paren_close = line.find(')', paren_open);
        
        if (paren_open != std::string::npos && paren_close != std::string::npos) {
            // 関数名を抽出
            size_t name_end = paren_open;
            while (name_end > 0 && std::isspace(line[name_end - 1])) {
                name_end--;
            }
            
            size_t name_start = name_end;
            while (name_start > 0 && (std::isalnum(line[name_start - 1]) || line[name_start - 1] == '_' || line[name_start - 1] == ':')) {
                name_start--;
            }
            
            if (name_start < name_end) {
                std::string func_name = line.substr(name_start, name_end - name_start);
                
                // C++キーワードを除外
                static const std::unordered_set<std::string> keywords = {
                    "if", "while", "for", "switch", "catch", "return", "sizeof", "typeof",
                    "static_cast", "dynamic_cast", "reinterpret_cast", "const_cast"
                };
                
                if (keywords.find(func_name) == keywords.end() && !func_name.empty()) {
                    // "{" を探す（同じ行または次の数行内）
                    bool has_brace = false;
                    size_t brace_line = i;
                    
                    // 現在の行をチェック
                    if (line.find('{', paren_close) != std::string::npos) {
                        has_brace = true;
                    } else {
                        // 次の数行をチェック
                        for (size_t j = i + 1; j < std::min(i + 5, lines.size()); ++j) {
                            if (lines[j].find('{') != std::string::npos) {
                                has_brace = true;
                                brace_line = j;
                                break;
                            }
                            // セミコロンがあったら宣言のみ
                            if (lines[j].find(';') != std::string::npos) {
                                break;
                            }
                        }
                    }
                    
                    if (has_brace) {
                        CppFunction func;
                        func.source_language = Language::CPP;
                        func.name = func_name;
                        func.start_line = static_cast<uint32_t>(i + 1);
                        
                        // パラメータを抽出
                        std::string params_str = line.substr(paren_open + 1, paren_close - paren_open - 1);
                        func.parameters = parse_function_parameters(params_str);
                        
                        // 関数の終了行を推定（簡易版）
                        func.end_line = find_function_end_line(lines, brace_line);
                        
                        // 関数本体の複雑度を計算
                        func.complexity = calculate_function_complexity(lines, func.start_line - 1, func.end_line - 1);
                        
                        functions.push_back(func);
                    }
                }
            }
        }
    }
    
    return functions;
}

// 関数の終了行を見つける（ブレースのバランスを追跡）
uint32_t CppAnalyzer::find_function_end_line(const std::vector<std::string>& lines, size_t start_line) {
    int brace_count = 0;
    bool in_function = false;
    
    for (size_t i = start_line; i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        for (char c : line) {
            if (c == '{') {
                brace_count++;
                in_function = true;
            } else if (c == '}') {
                brace_count--;
                if (in_function && brace_count == 0) {
                    return static_cast<uint32_t>(i + 1);
                }
            }
        }
    }
    
    // 見つからない場合は開始行+10を返す
    return static_cast<uint32_t>(std::min(start_line + 10, lines.size()));
}

// 関数の複雑度を計算
ComplexityInfo CppAnalyzer::calculate_function_complexity(const std::vector<std::string>& lines, size_t start_line, size_t end_line) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // 基本パス
    
    // 関数内の制御構造をカウント
    for (size_t i = start_line; i <= end_line && i < lines.size(); ++i) {
        const auto& line = lines[i];
        
        // 制御構造キーワードをチェック（文字列ベース、正規表現不使用）
        static const std::vector<std::string> control_keywords = {
            "if ", "else", "while ", "for ", "do ", "switch ", "case ", 
            "catch ", "&&", "||", "?", "return "
        };
        
        for (const auto& keyword : control_keywords) {
            size_t pos = 0;
            while ((pos = line.find(keyword, pos)) != std::string::npos) {
                // 単語境界チェック（簡易版）
                if (pos == 0 || !std::isalnum(line[pos - 1])) {
                    complexity.cyclomatic_complexity++;
                }
                pos += keyword.length();
            }
        }
    }
    
    // ネスト深度計算
    int current_depth = 0;
    complexity.max_nesting_depth = 0;
    
    for (size_t i = start_line; i <= end_line && i < lines.size(); ++i) {
        const auto& line = lines[i];
        for (char c : line) {
            if (c == '{') {
                current_depth++;
                if (current_depth > static_cast<int>(complexity.max_nesting_depth)) {
                    complexity.max_nesting_depth = current_depth;
                }
            } else if (c == '}') {
                current_depth = std::max(0, current_depth - 1);
            }
        }
    }
    
    complexity.update_rating();
    return complexity;
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
    
    // 🔧 正規表現を使わない単純な文字列検索に変更
    for (const auto& keyword : control_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            // 単語境界チェック（簡易版）
            bool valid = true;
            if (pos > 0 && std::isalnum(content[pos - 1])) {
                valid = false;
            }
            if (pos + keyword.length() < content.length() && 
                std::isalnum(content[pos + keyword.length()])) {
                valid = false;
            }
            
            if (valid) {
                complexity.cyclomatic_complexity++;
            }
            pos += keyword.length();
        }
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
    class_regex_ = std::regex(R"(class\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*[^{]*)?\s*\{)");
    struct_regex_ = std::regex(R"(struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?::\s*[^{]*)?\s*\{)");
    function_regex_ = std::regex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\))");
    include_regex_ = std::regex(R"(#include\s*([<"])[^>"]*[>"])");
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

//=============================================================================
// 🌟 TemplateParser - テンプレート解析専用クラス (リファクタリング)
//=============================================================================

class TemplateParser {
private:
    const std::string& content_;
    
public:
    explicit TemplateParser(const std::string& content) : content_(content) {}
    
    // ブラケットカウントでテンプレート範囲を特定
    struct TemplateRange {
        size_t start_pos;
        size_t end_pos;
        std::string parameters;
        bool valid = false;
    };
    
    TemplateRange find_template_range(size_t template_pos) {
        TemplateRange range;
        
        size_t start = content_.find("<", template_pos);
        if (start == std::string::npos) {
            return range;
        }
        
        size_t bracket_count = 1;
        size_t end = start + 1;
        while (end < content_.length() && bracket_count > 0) {
            if (content_[end] == '<') bracket_count++;
            else if (content_[end] == '>') bracket_count--;
            end++;
        }
        
        if (bracket_count == 0) {
            range.start_pos = start;
            range.end_pos = end;
            range.parameters = content_.substr(start + 1, end - start - 2);
            range.valid = true;
        }
        
        return range;
    }
    
    // クラステンプレート解析
    std::optional<CppTemplate> parse_class_template(size_t after_template, const std::string& params) {
        // class/structキーワードをチェック
        if (content_.substr(after_template, 5) != "class" && 
            content_.substr(after_template, 6) != "struct") {
            return std::nullopt;
        }
        
        // クラス名抽出
        size_t name_start = after_template + (content_.substr(after_template, 5) == "class" ? 5 : 6);
        while (name_start < content_.length() && std::isspace(content_[name_start])) {
            name_start++;
        }
        
        size_t name_end = name_start;
        while (name_end < content_.length() && 
               (std::isalnum(content_[name_end]) || content_[name_end] == '_')) {
            name_end++;
        }
        
        if (name_end <= name_start) {
            return std::nullopt;
        }
        
        CppTemplate tmpl;
        tmpl.type = "class";
        tmpl.name = content_.substr(name_start, name_end - name_start);
        tmpl.parameters.push_back(params);
        tmpl.is_variadic = (params.find("...") != std::string::npos);
        
        return tmpl;
    }
    
    // 関数テンプレート解析
    std::optional<CppTemplate> parse_function_template(size_t after_template, const std::string& params) {
        size_t paren_pos = content_.find("(", after_template);
        if (paren_pos == std::string::npos) {
            return std::nullopt;
        }
        
        std::string func_line = content_.substr(after_template, paren_pos - after_template);
        size_t last_space = func_line.find_last_of(" \t");
        if (last_space == std::string::npos) {
            return std::nullopt;
        }
        
        std::string func_name = func_line.substr(last_space + 1);
        func_name.erase(func_name.find_last_not_of(" \t") + 1);
        
        if (func_name.empty() || !std::isalpha(func_name[0])) {
            return std::nullopt;
        }
        
        CppTemplate tmpl;
        tmpl.type = "function";
        tmpl.name = func_name;
        tmpl.parameters.push_back(params);
        tmpl.is_variadic = (params.find("...") != std::string::npos);
        
        return tmpl;
    }
    
    // メインの解析メソッド
    std::vector<CppTemplate> parse_all_templates() {
        std::vector<CppTemplate> templates;
        
        size_t pos = 0;
        while ((pos = content_.find("template", pos)) != std::string::npos) {
            auto range = find_template_range(pos);
            if (!range.valid) {
                pos++;
                continue;
            }
            
            // class/struct/function を探す
            size_t after_template = range.end_pos;
            while (after_template < content_.length() && std::isspace(content_[after_template])) {
                after_template++;
            }
            
            // クラステンプレート試行
            if (auto class_tmpl = parse_class_template(after_template, range.parameters)) {
                templates.push_back(*class_tmpl);
            }
            // 関数テンプレート試行
            else if (auto func_tmpl = parse_function_template(after_template, range.parameters)) {
                templates.push_back(*func_tmpl);
            }
            
            pos = range.end_pos;
        }
        
        return templates;
    }
};

//=============================================================================
// 🌟 MacroParser - マクロ解析専用クラス (リファクタリング)
//=============================================================================

class MacroParser {
private:
    const std::string& content_;
    
public:
    explicit MacroParser(const std::string& content) : content_(content) {}
    
    // マクロ情報構造体
    struct MacroInfo {
        std::string name;
        std::string definition;
        size_t line_number = 0;
        bool valid = false;
    };
    
    // #defineの位置を探索
    std::optional<size_t> find_next_define(size_t start_pos) {
        size_t pos = content_.find("#define", start_pos);
        return (pos != std::string::npos) ? std::optional<size_t>(pos) : std::nullopt;
    }
    
    // マクロ名抽出
    std::optional<std::string> extract_macro_name(size_t after_define) {
        size_t start = after_define + 7; // "#define"の長さ
        
        // 空白をスキップ
        while (start < content_.length() && std::isspace(content_[start])) {
            start++;
        }
        
        // マクロ名の終端を見つける
        size_t name_end = start;
        while (name_end < content_.length() && 
               (std::isalnum(content_[name_end]) || content_[name_end] == '_')) {
            name_end++;
        }
        
        if (name_end <= start) {
            return std::nullopt;
        }
        
        return content_.substr(start, name_end - start);
    }
    
    // マクロ定義抽出
    std::string extract_macro_definition(size_t name_end) {
        size_t def_start = name_end;
        
        // 空白をスキップ
        while (def_start < content_.length() && std::isspace(content_[def_start])) {
            def_start++;
        }
        
        // 行末まで読み取り
        size_t line_end = content_.find('\n', def_start);
        if (line_end == std::string::npos) {
            line_end = content_.length();
        }
        
        std::string definition = content_.substr(def_start, line_end - def_start);
        
        // 末尾の空白を削除
        definition.erase(definition.find_last_not_of(" \t\r") + 1);
        
        return definition;
    }
    
    // メインの解析メソッド
    std::vector<std::pair<std::string, std::string>> parse_all_macros() {
        std::vector<std::pair<std::string, std::string>> macros;
        
        size_t pos = 0;
        while (auto define_pos = find_next_define(pos)) {
            if (auto macro_name = extract_macro_name(*define_pos)) {
                size_t name_end = *define_pos + 7; // "#define"の長さ
                
                // 名前の終端位置を正確に計算
                name_end += macro_name->length();
                while (name_end > *define_pos + 7 && std::isspace(content_[*define_pos + 7])) {
                    break; // 空白をスキップした位置から開始
                }
                name_end = *define_pos + 7;
                while (name_end < content_.length() && std::isspace(content_[name_end])) {
                    name_end++;
                }
                name_end += macro_name->length();
                
                std::string definition = extract_macro_definition(name_end);
                macros.emplace_back(*macro_name, definition);
                
                pos = name_end;
            } else {
                pos = *define_pos + 1;
            }
        }
        
        return macros;
    }
};

std::vector<CppTemplate> CppAnalyzer::analyze_templates(const std::string& content) {
    // 🌟 リファクタリング完了：TemplateParserクラスで責任分離！
    TemplateParser parser(content);
    return parser.parse_all_templates();
}

std::vector<std::pair<std::string, std::string>> CppAnalyzer::analyze_macros(const std::string& content) {
    // 🌟 リファクタリング完了：MacroParserクラスで責任分離！
    MacroParser parser(content);
    return parser.parse_all_macros();
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