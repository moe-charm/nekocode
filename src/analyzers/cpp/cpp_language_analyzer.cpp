//=============================================================================
// 🔥 C++ Language Analyzer - C++専用解析エンジン
//
// 既存のCppAnalyzerをBaseAnalyzerインターフェースに適合させるアダプター
//=============================================================================

#include "nekocode/analyzers/cpp_language_analyzer.hpp"

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
    initialize_patterns();
}

void CLanguageAnalyzer::initialize_patterns() {
    // C言語の関数パターン
    // 戻り値型 関数名(パラメータ) {
    function_pattern_ = std::regex(
        R"(^(?:static\s+|inline\s+|extern\s+)*)"
        R"((?:const\s+|volatile\s+)*)"
        R"((?:unsigned\s+|signed\s+|long\s+|short\s+)*)"
        R"((?:\w+(?:\s*\*)*\s+))"
        R"((\w+)\s*\([^)]*\)\s*\{)",
        std::regex::multiline
    );
    
    // 構造体定義
    struct_pattern_ = std::regex(R"((?:typedef\s+)?struct\s+(\w*)\s*\{)");
    
    // include文
    include_pattern_ = std::regex(R"(#include\s*[<"]([^>"]+)[>"])");
    
    // typedef
    typedef_pattern_ = std::regex(R"(typedef\s+.+\s+(\w+)\s*;)");
}

AnalysisResult CLanguageAnalyzer::analyze(const std::string& content, const std::string& filename) {
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::C;
    
    // 構造解析
    extract_functions(content, result);
    extract_structs(content, result);
    extract_includes(content, result);
    
    // 複雑度計算
    result.complexity = calculate_complexity(content);
    
    // 統計更新
    result.update_statistics();
    
    return result;
}

void CLanguageAnalyzer::extract_functions(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), function_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        FunctionInfo func_info;
        func_info.name = (*iter)[1].str();
        func_info.start_line = calculate_line_number(content, iter->position());
        
        // main関数の特別扱い
        if (func_info.name == "main") {
            func_info.parameters = {"argc", "argv"};
        }
        
        result.functions.push_back(func_info);
        ++iter;
    }
}

void CLanguageAnalyzer::extract_structs(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), struct_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        ClassInfo struct_info;
        struct_info.name = (*iter)[1].str();
        if (struct_info.name.empty()) {
            struct_info.name = "anonymous_struct";
        }
        struct_info.start_line = calculate_line_number(content, iter->position());
        result.classes.push_back(struct_info);
        ++iter;
    }
}

void CLanguageAnalyzer::extract_includes(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), include_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        ImportInfo include_info;
        include_info.module_path = (*iter)[1].str();
        include_info.type = ImportType::ES6_IMPORT; // C言語用の型を将来追加
        include_info.line_number = calculate_line_number(content, iter->position());
        result.imports.push_back(include_info);
        ++iter;
    }
}

} // namespace nekocode