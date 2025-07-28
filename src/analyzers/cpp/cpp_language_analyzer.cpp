//=============================================================================
// 🔥 C++ Language Analyzer - C++専用解析エンジン
//
// 既存のCppAnalyzerをBaseAnalyzerインターフェースに適合させるアダプター
//=============================================================================

#include "nekocode/analyzers/cpp_language_analyzer.hpp"
#include <iostream>
#include <algorithm>

namespace nekocode {

//=============================================================================
// 🔥 CppLanguageAnalyzer Implementation
//=============================================================================

CppLanguageAnalyzer::CppLanguageAnalyzer() 
    : cpp_analyzer_(std::make_unique<CppAnalyzer>()) {
}

AnalysisResult CppLanguageAnalyzer::analyze(const std::string& content, const std::string& filename) {
    // 既存のCppAnalyzerを使用
    CppAnalysisResult cpp_result = cpp_analyzer_->analyze_cpp_file(content, filename);
    
    // 結果を変換
    return convert_result(cpp_result);
}

AnalysisResult CppLanguageAnalyzer::convert_result(const CppAnalysisResult& cpp_result) {
    AnalysisResult result;
    
    // 基本情報コピー
    result.file_info = cpp_result.file_info;
    result.language = cpp_result.language;
    result.complexity = cpp_result.complexity;
    
    // C++クラスを汎用クラス情報に変換
    for (const auto& cpp_class : cpp_result.cpp_classes) {
        ClassInfo class_info;
        class_info.name = cpp_class.name;
        class_info.parent_class = cpp_class.base_classes.empty() ? "" : cpp_class.base_classes[0];
        class_info.start_line = cpp_class.start_line;
        class_info.end_line = cpp_class.end_line;
        
        // メソッドをコピー
        for (const auto& method : cpp_class.methods) {
            FunctionInfo func_info;
            func_info.name = method.name;
            func_info.start_line = method.start_line;
            func_info.end_line = method.end_line;
            func_info.parameters = method.parameters;
            class_info.methods.push_back(func_info);
        }
        
        result.classes.push_back(class_info);
    }
    
    // C++関数を汎用関数情報に変換
    for (const auto& cpp_func : cpp_result.cpp_functions) {
        FunctionInfo func_info;
        func_info.name = cpp_func.name;
        func_info.start_line = cpp_func.start_line;
        func_info.end_line = cpp_func.end_line;
        func_info.parameters = cpp_func.parameters;
        result.functions.push_back(func_info);
    }
    
    // include情報（import扱い）
    for (const auto& include : cpp_result.includes) {
        ImportInfo import_info;
        import_info.module_path = include.path;
        import_info.type = include.is_system_include ? ImportType::ES6_IMPORT : ImportType::COMMONJS_REQUIRE;
        import_info.line_number = include.line_number;
        result.imports.push_back(import_info);
    }
    
    // 統計更新
    result.update_statistics();
    
    return result;
}

//=============================================================================
// 🎯 CLanguageAnalyzer Implementation
//=============================================================================

CLanguageAnalyzer::CLanguageAnalyzer() {
    // 🔧 C言語 analyzer (std::regex完全除去版)
    std::cout << "🔧 CLanguageAnalyzer (String-based) initialized" << std::endl;
}

AnalysisResult CLanguageAnalyzer::analyze(const std::string& content, const std::string& filename) {
    std::cout << "🔧 CLanguageAnalyzer analyzing: " << filename << std::endl;
    
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::C;
    
    // 行数計算
    calculate_line_info(content, result.file_info);
    
    // 文字列ベース解析実行
    extract_functions(content, result);
    extract_structs(content, result);
    extract_includes(content, result);
    
    // 複雑度計算（C言語特化版）
    result.complexity = calculate_c_complexity(content);
    
    // 🎯 ハイブリッド戦略: 統計整合性チェック
    if (needs_c_line_based_fallback(result, content)) {
        std::cout << "🔧 C line-based fallback triggered" << std::endl;
        apply_c_line_based_analysis(result, content);
    }
    
    // 統計更新
    result.update_statistics();
    
    std::cout << "✅ C analysis completed. Structs: " << result.classes.size() 
              << ", Functions: " << result.functions.size() << std::endl;
    
    return result;
}

//=============================================================================
// 🔧 C言語 構造化解析実装（C++成功パターン参考）
//=============================================================================

void CLanguageAnalyzer::extract_functions(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_c_function_line(line)) {
            // 🎯 より精密な関数名抽出（C++パターン参考）
            FunctionInfo func_info = parse_c_function_declaration(line, line_number);
            if (!func_info.name.empty()) {
                // 重複チェック
                if (!is_function_already_detected(result.functions, func_info.name)) {
                    result.functions.push_back(func_info);
                }
            }
        }
        line_number++;
    }
}

//=============================================================================
// 🎯 C言語 関数解析ヘルパー（構造化・モジュール化）
//=============================================================================

FunctionInfo CLanguageAnalyzer::parse_c_function_declaration(const std::string& line, uint32_t line_number) {
    FunctionInfo func_info;
    
    size_t paren_pos = line.find('(');
    if (paren_pos == std::string::npos) {
        return func_info; // 空のFunctionInfoを返す
    }
    
    // 🔍 関数名抽出の改良版
    std::string func_name = extract_function_name_from_line(line, paren_pos);
    if (func_name.empty()) {
        return func_info;
    }
    
    func_info.name = func_name;
    func_info.start_line = line_number;
    
    // 🎯 パラメータ抽出（C++パターン参考）
    func_info.parameters = extract_c_function_parameters(line, paren_pos);
    
    // 🔧 C言語特有の処理
    enhance_c_function_info(func_info, line);
    
    return func_info;
}

std::string CLanguageAnalyzer::extract_function_name_from_line(const std::string& line, size_t paren_pos) {
    // 逆方向に関数名を探す（より精密版）
    size_t name_end = paren_pos;
    while (name_end > 0 && (std::isalnum(line[name_end-1]) || line[name_end-1] == '_')) {
        name_end--;
    }
    
    if (name_end >= paren_pos) {
        return "";
    }
    
    std::string func_name = line.substr(name_end, paren_pos - name_end);
    
    // 🚫 キーワード除外
    if (is_c_keyword(func_name)) {
        return "";
    }
    
    return func_name;
}

std::vector<std::string> CLanguageAnalyzer::extract_c_function_parameters(const std::string& line, size_t paren_start) {
    std::vector<std::string> parameters;
    
    size_t paren_end = line.find(')', paren_start);
    if (paren_end == std::string::npos) {
        return parameters;
    }
    
    std::string params_str = line.substr(paren_start + 1, paren_end - paren_start - 1);
    
    // 空白・型情報除去の簡易版
    if (!params_str.empty() && params_str != "void") {
        // "int argc, char* argv[]" → ["argc", "argv"]
        std::stringstream params_stream(params_str);
        std::string param;
        
        while (std::getline(params_stream, param, ',')) {
            std::string clean_param = extract_parameter_name(param);
            if (!clean_param.empty()) {
                parameters.push_back(clean_param);
            }
        }
    }
    
    return parameters;
}

void CLanguageAnalyzer::enhance_c_function_info(FunctionInfo& func_info, const std::string& line) {
    // C言語特有の情報をmetadataに保存
    if (line.find("static") != std::string::npos) {
        func_info.metadata["storage_class"] = "static";
    }
    
    if (line.find("inline") != std::string::npos) {
        func_info.metadata["specifier"] = "inline";
    }
    
    // main関数の特別扱い
    if (func_info.name == "main") {
        func_info.metadata["function_type"] = "entry_point";
        if (func_info.parameters.empty()) {
            func_info.parameters = {"argc", "argv"}; // 標準的なmain引数
        }
    }
}

std::string CLanguageAnalyzer::extract_parameter_name(const std::string& param) {
    // "int argc" → "argc"
    // "char* argv[]" → "argv"
    std::string trimmed = param;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
    
    if (trimmed.empty()) {
        return "";
    }
    
    // 最後の識別子を抽出
    size_t last_space = trimmed.find_last_of(" *&[]");
    if (last_space != std::string::npos) {
        return trimmed.substr(last_space + 1);
    }
    
    // スペースがない場合は型名だけの可能性があるのでスキップ
    return "";
}

bool CLanguageAnalyzer::is_function_already_detected(const std::vector<FunctionInfo>& functions, const std::string& name) {
    for (const auto& func : functions) {
        if (func.name == name) {
            return true;
        }
    }
    return false;
}

bool CLanguageAnalyzer::is_c_keyword(const std::string& word) {
    // C言語キーワードのチェック
    static const std::vector<std::string> c_keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    
    return std::find(c_keywords.begin(), c_keywords.end(), word) != c_keywords.end();
}

void CLanguageAnalyzer::extract_structs(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_c_struct_line(line)) {
            // 🎯 構造化された構造体解析（C++パターン参考）
            ClassInfo struct_info = parse_c_struct_declaration(line, line_number);
            if (!struct_info.name.empty()) {
                // 重複チェック
                if (!is_struct_already_detected(result.classes, struct_info.name)) {
                    result.classes.push_back(struct_info);
                }
            }
        }
        line_number++;
    }
}

//=============================================================================
// 🎯 C言語 構造体解析ヘルパー（構造化・モジュール化）
//=============================================================================

ClassInfo CLanguageAnalyzer::parse_c_struct_declaration(const std::string& line, uint32_t line_number) {
    ClassInfo struct_info;
    
    size_t struct_pos = line.find("struct");
    if (struct_pos == std::string::npos) {
        return struct_info; // 空のClassInfoを返す
    }
    
    size_t name_start = struct_pos + 6; // "struct" の長さ
    size_t brace_pos = line.find('{', name_start);
    
    if (brace_pos == std::string::npos) {
        return struct_info; // 宣言のみで定義なし
    }
    
    // 🔍 構造体名抽出
    std::string struct_name = extract_struct_name(line, name_start, brace_pos);
    if (struct_name.empty()) {
        struct_name = generate_anonymous_struct_name(line_number);
    }
    
    struct_info.name = struct_name;
    struct_info.start_line = line_number;
    
    // 🔧 C言語特有の構造体情報をmetadataに保存
    enhance_c_struct_info(struct_info, line);
    
    return struct_info;
}

std::string CLanguageAnalyzer::extract_struct_name(const std::string& line, size_t name_start, size_t brace_pos) {
    std::string between = line.substr(name_start, brace_pos - name_start);
    
    // 空白除去
    between.erase(std::remove_if(between.begin(), between.end(), ::isspace), between.end());
    
    // typedef structの場合を考慮
    if (between.empty() || between == "typedef") {
        return ""; // 無名構造体または特殊ケース
    }
    
    return between;
}

std::string CLanguageAnalyzer::generate_anonymous_struct_name(uint32_t line_number) {
    return "anonymous_struct_" + std::to_string(line_number);
}

void CLanguageAnalyzer::enhance_c_struct_info(ClassInfo& struct_info, const std::string& line) {
    // C言語特有の構造体情報をmetadataに保存
    struct_info.metadata["type"] = "c_struct";
    
    if (line.find("typedef") != std::string::npos) {
        struct_info.metadata["definition_type"] = "typedef_struct";
    } else {
        struct_info.metadata["definition_type"] = "struct";
    }
    
    // packedやalignedなどの属性検出
    if (line.find("__packed") != std::string::npos || line.find("__attribute__((packed))") != std::string::npos) {
        struct_info.metadata["attributes"] = "packed";
    }
}

bool CLanguageAnalyzer::is_struct_already_detected(const std::vector<ClassInfo>& classes, const std::string& name) {
    for (const auto& cls : classes) {
        if (cls.name == name) {
            return true;
        }
    }
    return false;
}

void CLanguageAnalyzer::extract_includes(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_c_include_line(line)) {
            // 🎯 構造化されたinclude解析（C++パターン参考）
            ImportInfo include_info = parse_c_include_directive(line, line_number);
            if (!include_info.module_path.empty()) {
                // 重複チェック
                if (!is_include_already_detected(result.imports, include_info.module_path)) {
                    result.imports.push_back(include_info);
                }
            }
        }
        line_number++;
    }
}

//=============================================================================
// 🎯 C言語 include解析ヘルパー（構造化・モジュール化）
//=============================================================================

ImportInfo CLanguageAnalyzer::parse_c_include_directive(const std::string& line, uint32_t line_number) {
    ImportInfo include_info;
    
    size_t include_pos = line.find("#include");
    if (include_pos == std::string::npos) {
        return include_info; // 空のImportInfoを返す
    }
    
    // 🔍 ヘッダー名とタイプを抽出
    std::pair<std::string, bool> header_info = extract_header_info(line, include_pos);
    
    if (header_info.first.empty()) {
        return include_info;
    }
    
    include_info.module_path = header_info.first;
    include_info.line_number = line_number;
    
    // 🔧 C言語特有のinclude情報をmetadataに保存
    enhance_c_include_info(include_info, line, header_info.second);
    
    // ImportTypeの設定（C言語用の適切なマッピング）
    include_info.type = header_info.second ? ImportType::ES6_IMPORT : ImportType::COMMONJS_REQUIRE;
    
    return include_info;
}

std::pair<std::string, bool> CLanguageAnalyzer::extract_header_info(const std::string& line, size_t include_pos) {
    // <header> パターンチェック
    size_t bracket_start = line.find('<', include_pos);
    size_t bracket_end = line.find('>', bracket_start);
    
    if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
        std::string header_name = line.substr(bracket_start + 1, bracket_end - bracket_start - 1);
        return std::make_pair(header_name, true); // システムヘッダー
    }
    
    // "header" パターンチェック
    size_t quote_start = line.find('"', include_pos);
    size_t quote_end = line.rfind('"');
    
    if (quote_start != std::string::npos && quote_end != std::string::npos && quote_end > quote_start) {
        std::string header_name = line.substr(quote_start + 1, quote_end - quote_start - 1);
        return std::make_pair(header_name, false); // ローカルヘッダー
    }
    
    return std::make_pair("", false);
}

void CLanguageAnalyzer::enhance_c_include_info(ImportInfo& include_info, const std::string& line, bool is_system_header) {
    // C言語特有のinclude情報をmetadataに保存
    include_info.metadata["language"] = "c";
    include_info.metadata["header_type"] = is_system_header ? "system" : "local";
    
    // 標準Cライブラリヘッダーの検出
    if (is_system_header) {
        static const std::vector<std::string> standard_headers = {
            "stdio.h", "stdlib.h", "string.h", "math.h", "time.h", "ctype.h",
            "assert.h", "errno.h", "float.h", "limits.h", "stdarg.h", "stddef.h"
        };
        
        if (std::find(standard_headers.begin(), standard_headers.end(), include_info.module_path) != standard_headers.end()) {
            include_info.metadata["category"] = "standard_c_library";
        }
    }
    
    // 条件付きincludeの検出
    if (line.find("#ifdef") != std::string::npos || line.find("#ifndef") != std::string::npos || line.find("#if") != std::string::npos) {
        include_info.metadata["conditional"] = "true";
    }
}

bool CLanguageAnalyzer::is_include_already_detected(const std::vector<ImportInfo>& imports, const std::string& module_path) {
    for (const auto& import : imports) {
        if (import.module_path == module_path) {
            return true;
        }
    }
    return false;
}

//=============================================================================
// 🧮 C言語 複雑度計算（構造化）
//=============================================================================

ComplexityInfo CLanguageAnalyzer::calculate_c_complexity(const std::string& content) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // ベーススコア
    
    // C言語特有のキーワードで複雑度計算
    std::vector<std::string> c_complexity_keywords = {
        "if ", "else", "for ", "while ", "do ", "switch ", "case ", 
        "goto ", "break", "continue", "return"
    };
    
    for (const auto& keyword : c_complexity_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // 最大ネスト深度計算（ブレースベース）
    complexity.max_nesting_depth = calculate_c_nesting_depth(content);
    
    // C言語特有の複雑度要素
    calculate_c_specific_complexity(complexity, content);
    
    complexity.update_rating();
    return complexity;
}

uint32_t CLanguageAnalyzer::calculate_c_nesting_depth(const std::string& content) {
    uint32_t max_depth = 0;
    uint32_t current_depth = 0;
    
    for (char c : content) {
        if (c == '{') {
            current_depth++;
            if (current_depth > max_depth) {
                max_depth = current_depth;
            }
        } else if (c == '}') {
            if (current_depth > 0) {
                current_depth--;
            }
        }
    }
    
    return max_depth;
}

void CLanguageAnalyzer::calculate_c_specific_complexity(ComplexityInfo& complexity, const std::string& content) {
    // ポインタ使用による複雑度増加
    size_t pointer_count = 0;
    size_t pos = 0;
    while ((pos = content.find('*', pos)) != std::string::npos) {
        pointer_count++;
        pos++;
    }
    
    // マクロ使用による複雑度
    size_t macro_count = 0;
    pos = 0;
    while ((pos = content.find("#define", pos)) != std::string::npos) {
        macro_count++;
        pos += 7;
    }
    
    // 複雑度調整（ポインタとマクロは複雑さを増す）
    complexity.cyclomatic_complexity += static_cast<uint32_t>(pointer_count / 10); // 10個のポインタで+1
    complexity.cyclomatic_complexity += static_cast<uint32_t>(macro_count); // マクロは直接加算
}

//=============================================================================
// 🔍 C言語 ファイル情報計算
//=============================================================================

void CLanguageAnalyzer::calculate_line_info(const std::string& content, FileInfo& file_info) {
    std::istringstream stream(content);
    std::string line;
    uint32_t total_lines = 0;
    uint32_t code_lines = 0;
    
    while (std::getline(stream, line)) {
        total_lines++;
        
        // 空行・コメント行でない場合はコード行としてカウント
        std::string trimmed = line;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        
        if (!trimmed.empty() && 
            trimmed.substr(0, 2) != "//" && 
            trimmed.substr(0, 2) != "/*" && 
            !(trimmed.length() >= 1 && trimmed[0] == '*' && trimmed != "*")) {
            code_lines++;
        }
    }
    
    file_info.total_lines = total_lines;
    file_info.code_lines = code_lines;
}

//=============================================================================
// 🔍 C言語 キーワード検出
//=============================================================================

bool CLanguageAnalyzer::is_c_function_line(const std::string& line) {
    // 関数定義の検出: "型 関数名(" + "{"
    return line.find('(') != std::string::npos && 
           line.find('{') != std::string::npos &&
           line.find('#') == std::string::npos && // プリプロセッサ除外
           line.find("//") != line.find_first_not_of(" \t"); // コメント行除外
}

bool CLanguageAnalyzer::is_c_struct_line(const std::string& line) {
    return line.find("struct") != std::string::npos && 
           line.find('{') != std::string::npos;
}

bool CLanguageAnalyzer::is_c_include_line(const std::string& line) {
    size_t first_non_space = line.find_first_not_of(" \t");
    if (first_non_space != std::string::npos) {
        return line.substr(first_non_space).find("#include") == 0;
    }
    return false;
}

//=============================================================================
// 🎯 C言語 ハイブリッド戦略
//=============================================================================

bool CLanguageAnalyzer::needs_c_line_based_fallback(const AnalysisResult& result, const std::string& content) {
    // C言語特化の統計整合性チェック
    
    // 1. 関数っぽいパターンがあるのに検出されていない
    int func_patterns = 0;
    size_t pos = 0;
    while ((pos = content.find("(", pos)) != std::string::npos) {
        // ")" も近くにあるかチェック
        size_t close_pos = content.find(")", pos);
        if (close_pos != std::string::npos && close_pos - pos < 100) { // 妥当な長さ
            func_patterns++;
        }
        pos++;
    }
    
    if (func_patterns > 3 && result.functions.empty()) { // 3つ以上の関数パターン
        return true;
    }
    
    // 2. struct キーワードがあるのに構造体が検出されていない
    if (content.find("struct") != std::string::npos && result.classes.empty()) {
        return true;
    }
    
    // 3. #include があるのに import が検出されていない
    if (content.find("#include") != std::string::npos && result.imports.empty()) {
        return true;
    }
    
    return false;
}

void CLanguageAnalyzer::apply_c_line_based_analysis(AnalysisResult& result, const std::string& content) {
    // 🔧 フォールバック: より単純な行ベース解析
    
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        // より緩い条件での関数検出
        if (line.find('(') != std::string::npos && line.find(')') != std::string::npos) {
            // 可能性のある関数名を抽出
            size_t paren_pos = line.find('(');
            if (paren_pos > 0) {
                // 単語境界を探す
                size_t name_start = paren_pos;
                while (name_start > 0 && (std::isalnum(line[name_start-1]) || line[name_start-1] == '_')) {
                    name_start--;
                }
                
                if (name_start < paren_pos) {
                    std::string potential_func = line.substr(name_start, paren_pos - name_start);
                    
                    // C言語の関数名っぽいかチェック（簡易版）
                    if (!potential_func.empty() && std::isalpha(potential_func[0])) {
                        // 重複チェック
                        bool already_exists = false;
                        for (const auto& existing : result.functions) {
                            if (existing.name == potential_func) {
                                already_exists = true;
                                break;
                            }
                        }
                        
                        if (!already_exists) {
                            FunctionInfo func_info;
                            func_info.name = potential_func;
                            func_info.start_line = line_number;
                            result.functions.push_back(func_info);
                        }
                    }
                }
            }
        }
        
        line_number++;
    }
}

} // namespace nekocode