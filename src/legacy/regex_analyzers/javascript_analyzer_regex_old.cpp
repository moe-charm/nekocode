//=============================================================================
// 🌟 JavaScript Language Analyzer - JavaScript専用解析エンジン
//
// JavaScriptコードの構造解析・複雑度計算
// ES6+対応、クラス、関数、import/export検出
//=============================================================================

#include "nekocode/analyzers/javascript_analyzer.hpp"
#include <sstream>
#include <algorithm>
#include <set>

namespace nekocode {

//=============================================================================
// 🌟 JavaScriptAnalyzer Implementation
//=============================================================================

JavaScriptAnalyzer::JavaScriptAnalyzer() {
    initialize_patterns();
}

void JavaScriptAnalyzer::initialize_patterns() {
    // ES6クラス: class ClassName [extends Parent] {
    es6_class_pattern_ = std::regex(R"(class\s+(\w+)(?:\s+extends\s+(\w+))?\s*\{)");
    
    // プロトタイプベースクラス（簡易版）
    prototype_pattern_ = std::regex(R"(function\s+(\w+)\s*\([^)]*\)\s*\{[^}]*\1\.prototype\.)");
    
    // 通常関数: function name(...) {
    function_pattern_ = std::regex(R"((?:async\s+)?function\s+(\w+)\s*\([^)]*\)\s*\{)");
    
    // アロー関数: const name = (...) => 
    arrow_function_pattern_ = std::regex(R"((?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>)");
    
    // メソッド（オブジェクト内）: name(...) { or name: function(...) {
    method_pattern_ = std::regex(R"((\w+)\s*(?::\s*(?:async\s+)?function)?\s*\([^)]*\)\s*\{)");
    
    // import文パターン
    import_patterns_ = {
        std::regex(R"(import\s+\{([^}]+)\}\s+from\s+['"](.*?)['"])"),      // import { a, b } from 'module'
        std::regex(R"(import\s+(\w+)\s+from\s+['"](.*?)['"])"),            // import name from 'module'
        std::regex(R"(import\s+\*\s+as\s+(\w+)\s+from\s+['"](.*?)['"])"),  // import * as name from 'module'
        std::regex(R"(import\s+['"](.*?)['"])"),                           // import 'module'
        std::regex(R"(const\s+(\w+)\s*=\s*require\s*\(\s*['"](.*?)['"]\s*\))"), // const name = require('module')
        std::regex(R"(import\s*\(\s*['"](.*?)['"]\s*\)")                   // dynamic import('module')
    };
    
    // export文パターン
    export_patterns_ = {
        std::regex(R"(export\s+default\s+)"),                              // export default
        std::regex(R"(export\s+\{([^}]+)\})"),                            // export { a, b }
        std::regex(R"(export\s+(?:const|let|var|function|class)\s+(\w+))"), // export const/let/var/function/class name
        std::regex(R"(module\.exports\s*=)"),                             // module.exports =
        std::regex(R"(exports\.(\w+)\s*=)")                               // exports.name =
    };
    
    // 関数呼び出しパターン
    function_call_pattern_ = std::regex(R"((\w+)\s*\()");
}

Language JavaScriptAnalyzer::get_language() const {
    return Language::JAVASCRIPT;
}

std::string JavaScriptAnalyzer::get_language_name() const {
    return "JavaScript";
}

std::vector<std::string> JavaScriptAnalyzer::get_supported_extensions() const {
    return {".js", ".mjs", ".jsx", ".cjs"};
}

AnalysisResult JavaScriptAnalyzer::analyze(const std::string& content, const std::string& filename) {
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::JAVASCRIPT;
    
    // 構造解析
    extract_classes(content, result);
    extract_functions(content, result);
    extract_imports(content, result);
    extract_exports(content, result);
    
    // 関数呼び出し解析
    if (!result.functions.empty()) {
        extract_function_calls(content, result);
    }
    
    // 複雑度計算
    result.complexity = calculate_javascript_complexity(content);
    
    // 統計更新
    result.update_statistics();
    
    return result;
}

void JavaScriptAnalyzer::extract_classes(const std::string& content, AnalysisResult& result) {
    // ES6クラス抽出
    std::sregex_iterator es6_iter(content.begin(), content.end(), es6_class_pattern_);
    std::sregex_iterator end;
    
    while (es6_iter != end) {
        ClassInfo class_info;
        class_info.name = (*es6_iter)[1].str();
        if (es6_iter->size() > 2 && (*es6_iter)[2].matched) {
            class_info.parent_class = (*es6_iter)[2].str();
        }
        class_info.start_line = calculate_line_number(content, es6_iter->position());
        result.classes.push_back(class_info);
        ++es6_iter;
    }
    
    // プロトタイプベースクラス抽出
    std::sregex_iterator proto_iter(content.begin(), content.end(), prototype_pattern_);
    while (proto_iter != end) {
        ClassInfo class_info;
        class_info.name = (*proto_iter)[1].str();
        class_info.start_line = calculate_line_number(content, proto_iter->position());
        
        // 既に追加されていないか確認
        auto it = std::find_if(result.classes.begin(), result.classes.end(),
            [&](const ClassInfo& c) { return c.name == class_info.name; });
        if (it == result.classes.end()) {
            result.classes.push_back(class_info);
        }
        ++proto_iter;
    }
}

void JavaScriptAnalyzer::extract_functions(const std::string& content, AnalysisResult& result) {
    // 通常関数抽出
    std::sregex_iterator func_iter(content.begin(), content.end(), function_pattern_);
    std::sregex_iterator end;
    
    while (func_iter != end) {
        FunctionInfo func_info;
        func_info.name = (*func_iter)[1].str();
        func_info.start_line = calculate_line_number(content, func_iter->position());
        func_info.is_async = (func_iter->str().find("async") != std::string::npos);
        result.functions.push_back(func_info);
        ++func_iter;
    }
    
    // アロー関数抽出
    std::sregex_iterator arrow_iter(content.begin(), content.end(), arrow_function_pattern_);
    while (arrow_iter != end) {
        FunctionInfo func_info;
        func_info.name = (*arrow_iter)[1].str();
        func_info.start_line = calculate_line_number(content, arrow_iter->position());
        func_info.is_arrow_function = true;
        func_info.is_async = (arrow_iter->str().find("async") != std::string::npos);
        result.functions.push_back(func_info);
        ++arrow_iter;
    }
}

void JavaScriptAnalyzer::extract_imports(const std::string& content, AnalysisResult& result) {
    for (size_t i = 0; i < import_patterns_.size(); ++i) {
        std::sregex_iterator iter(content.begin(), content.end(), import_patterns_[i]);
        std::sregex_iterator end;
        
        while (iter != end) {
            ImportInfo import_info;
            
            // パターンに応じて処理
            switch (i) {
                case 0: // import { a, b } from 'module'
                case 1: // import name from 'module'
                case 2: // import * as name from 'module'
                    if (iter->size() > 2) {
                        import_info.module_path = (*iter)[2].str();
                        std::string names = (*iter)[1].str();
                        // カンマ区切りの名前を分割
                        std::istringstream iss(names);
                        std::string name;
                        while (std::getline(iss, name, ',')) {
                            name.erase(0, name.find_first_not_of(" \t"));
                            name.erase(name.find_last_not_of(" \t") + 1);
                            if (!name.empty()) {
                                import_info.imported_names.push_back(name);
                            }
                        }
                    }
                    import_info.type = ImportType::ES6_IMPORT;
                    break;
                    
                case 3: // import 'module'
                case 5: // dynamic import('module')
                    import_info.module_path = (*iter)[1].str();
                    import_info.type = (i == 5) ? ImportType::DYNAMIC_IMPORT : ImportType::ES6_IMPORT;
                    break;
                    
                case 4: // const name = require('module')
                    if (iter->size() > 2) {
                        import_info.module_path = (*iter)[2].str();
                        import_info.imported_names.push_back((*iter)[1].str());
                    }
                    import_info.type = ImportType::COMMONJS_REQUIRE;
                    break;
            }
            
            import_info.line_number = calculate_line_number(content, iter->position());
            result.imports.push_back(import_info);
            ++iter;
        }
    }
}

void JavaScriptAnalyzer::extract_exports(const std::string& content, AnalysisResult& result) {
    for (size_t i = 0; i < export_patterns_.size(); ++i) {
        std::sregex_iterator iter(content.begin(), content.end(), export_patterns_[i]);
        std::sregex_iterator end;
        
        while (iter != end) {
            ExportInfo export_info;
            
            switch (i) {
                case 0: // export default
                    export_info.type = ExportType::ES6_DEFAULT;
                    export_info.is_default = true;
                    break;
                    
                case 1: // export { a, b }
                    export_info.type = ExportType::ES6_EXPORT;
                    if (iter->size() > 1) {
                        std::string names = (*iter)[1].str();
                        std::istringstream iss(names);
                        std::string name;
                        while (std::getline(iss, name, ',')) {
                            name.erase(0, name.find_first_not_of(" \t"));
                            name.erase(name.find_last_not_of(" \t") + 1);
                            if (!name.empty()) {
                                export_info.exported_names.push_back(name);
                            }
                        }
                    }
                    break;
                    
                case 2: // export const/let/var/function/class name
                    export_info.type = ExportType::ES6_EXPORT;
                    if (iter->size() > 1) {
                        export_info.exported_names.push_back((*iter)[1].str());
                    }
                    break;
                    
                case 3: // module.exports =
                case 4: // exports.name =
                    export_info.type = ExportType::COMMONJS_EXPORTS;
                    if (i == 4 && iter->size() > 1) {
                        export_info.exported_names.push_back((*iter)[1].str());
                    }
                    break;
            }
            
            export_info.line_number = calculate_line_number(content, iter->position());
            result.exports.push_back(export_info);
            ++iter;
        }
    }
}

void JavaScriptAnalyzer::extract_function_calls(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), function_call_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        std::string func_name = (*iter)[1].str();
        
        // 予約語や制御構造を除外
        static const std::set<std::string> keywords = {
            "if", "for", "while", "switch", "catch", "function", "class", "new", "return", "typeof", "instanceof"
        };
        
        if (keywords.find(func_name) == keywords.end()) {
            FunctionCall call;
            call.function_name = func_name;
            call.line_number = calculate_line_number(content, iter->position());
            
            result.function_calls.push_back(call);
            
            // 頻度カウント
            result.call_frequency[func_name]++;
        }
        
        ++iter;
    }
}

ComplexityInfo JavaScriptAnalyzer::calculate_javascript_complexity(const std::string& content) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1;
    
    // JavaScript固有の複雑度キーワード
    std::vector<std::string> complexity_keywords = {
        "if ", "else if", "else ", "for ", "while ", "do ",
        "switch ", "case ", "catch ", "&&", "||", "? ",
        ".then(", ".catch(", "async ", "await "
    };
    
    for (const auto& keyword : complexity_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // ネスト深度計算
    complexity.max_nesting_depth = 0;
    uint32_t current_depth = 0;
    
    for (char c : content) {
        if (c == '{') {
            current_depth++;
            if (current_depth > complexity.max_nesting_depth) {
                complexity.max_nesting_depth = current_depth;
            }
        } else if (c == '}' && current_depth > 0) {
            current_depth--;
        }
    }
    
    complexity.update_rating();
    return complexity;
}

//=============================================================================
// 🔷 TypeScriptAnalyzer Implementation
//=============================================================================

TypeScriptAnalyzer::TypeScriptAnalyzer() : JavaScriptAnalyzer() {
    // 基底クラスのコンストラクタが呼ばれた後、TypeScript固有のパターンを追加
    initialize_patterns();
}

void TypeScriptAnalyzer::initialize_patterns() {
    // 基底クラスのパターンはそのまま使用
    JavaScriptAnalyzer::initialize_patterns();
    
    // TypeScript固有パターン追加
    interface_pattern_ = std::regex(R"(interface\s+(\w+)(?:\s+extends\s+([^{]+))?\s*\{)");
    type_alias_pattern_ = std::regex(R"(type\s+(\w+)\s*=)");
    enum_pattern_ = std::regex(R"(enum\s+(\w+)\s*\{)");
}

Language TypeScriptAnalyzer::get_language() const {
    return Language::TYPESCRIPT;
}

std::string TypeScriptAnalyzer::get_language_name() const {
    return "TypeScript";
}

std::vector<std::string> TypeScriptAnalyzer::get_supported_extensions() const {
    return {".ts", ".tsx", ".mts", ".cts"};
}

} // namespace nekocode