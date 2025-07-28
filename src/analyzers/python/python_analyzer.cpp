//=============================================================================
// 🐍 Python Language Analyzer - Python専用解析エンジン
//
// Pythonコードの構造解析・複雑度計算
// 一言語一ファイルの原則に従った実装
//=============================================================================

#include "nekocode/analyzers/python_analyzer.hpp"
#include <regex>
#include <sstream>

namespace nekocode {

//=============================================================================
// 🐍 PythonAnalyzer Implementation
//=============================================================================

PythonAnalyzer::PythonAnalyzer() {
    // Python固有のパターンを初期化
    initialize_patterns();
}

void PythonAnalyzer::initialize_patterns() {
    // クラス定義パターン
    class_pattern_ = std::regex(R"(^class\s+(\w+)(?:\s*\([^)]*\))?\s*:)", 
                                std::regex::multiline);
    
    // 関数定義パターン（通常関数とasync関数）
    function_pattern_ = std::regex(R"(^(?:async\s+)?def\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[^:]+)?\s*:)", 
                                   std::regex::multiline);
    
    // メソッド定義パターン（インデントされた関数）
    method_pattern_ = std::regex(R"(^\s+(?:async\s+)?def\s+(\w+)\s*\([^)]*\)\s*(?:->\s*[^:]+)?\s*:)", 
                                 std::regex::multiline);
    
    // import文パターン
    import_patterns_ = {
        std::regex(R"(^from\s+([\w.]+)\s+import\s+(.+))", std::regex::multiline),
        std::regex(R"(^import\s+([\w.,\s]+))", std::regex::multiline)
    };
    
    // デコレータパターン
    decorator_pattern_ = std::regex(R"(^@(\w+)(?:\([^)]*\))?)", std::regex::multiline);
}

Language PythonAnalyzer::get_language() const {
    return Language::PYTHON;
}

std::string PythonAnalyzer::get_language_name() const {
    return "Python";
}

std::vector<std::string> PythonAnalyzer::get_supported_extensions() const {
    return {".py", ".pyw", ".pyi"};
}

AnalysisResult PythonAnalyzer::analyze(const std::string& content, const std::string& filename) {
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::PYTHON;
    
    // クラス解析
    extract_classes(content, result);
    
    // 関数解析
    extract_functions(content, result);
    
    // import解析
    extract_imports(content, result);
    
    // 複雑度計算（Python特化版）
    result.complexity = calculate_python_complexity(content);
    
    // 統計更新
    result.update_statistics();
    
    return result;
}

void PythonAnalyzer::extract_classes(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), class_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        ClassInfo class_info;
        class_info.name = (*iter)[1].str();
        class_info.start_line = calculate_line_number(content, iter->position());
        
        // クラス内のメソッドを検出
        size_t class_start = iter->position();
        size_t class_end = find_class_end(content, class_start);
        
        if (class_end > class_start) {
            std::string class_content = content.substr(class_start, class_end - class_start);
            extract_methods(class_content, class_info, class_info.start_line);
        }
        
        result.classes.push_back(class_info);
        ++iter;
    }
}

void PythonAnalyzer::extract_functions(const std::string& content, AnalysisResult& result) {
    std::sregex_iterator iter(content.begin(), content.end(), function_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        FunctionInfo func_info;
        func_info.name = (*iter)[1].str();
        func_info.start_line = calculate_line_number(content, iter->position());
        func_info.is_async = (iter->str().find("async") != std::string::npos);
        
        // パラメータ抽出（簡易版）
        size_t paren_start = iter->str().find('(');
        size_t paren_end = iter->str().find(')');
        if (paren_start != std::string::npos && paren_end != std::string::npos) {
            std::string params = iter->str().substr(paren_start + 1, paren_end - paren_start - 1);
            extract_parameters(params, func_info.parameters);
        }
        
        result.functions.push_back(func_info);
        ++iter;
    }
}

void PythonAnalyzer::extract_methods(const std::string& class_content, 
                                    ClassInfo& class_info, 
                                    uint32_t base_line) {
    std::sregex_iterator iter(class_content.begin(), class_content.end(), method_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        FunctionInfo method_info;
        method_info.name = (*iter)[1].str();
        method_info.start_line = base_line + calculate_line_number(class_content, iter->position()) - 1;
        method_info.is_async = (iter->str().find("async") != std::string::npos);
        
        class_info.methods.push_back(method_info);
        ++iter;
    }
}

void PythonAnalyzer::extract_imports(const std::string& content, AnalysisResult& result) {
    for (const auto& pattern : import_patterns_) {
        std::sregex_iterator iter(content.begin(), content.end(), pattern);
        std::sregex_iterator end;
        
        while (iter != end) {
            ImportInfo import_info;
            import_info.module_path = (*iter)[1].str();
            import_info.type = ImportType::ES6_IMPORT; // Python用の型を将来追加
            import_info.line_number = calculate_line_number(content, iter->position());
            
            // from ... import の場合、インポート名も抽出
            if (iter->size() > 2) {
                std::string import_names = (*iter)[2].str();
                std::istringstream iss(import_names);
                std::string name;
                while (std::getline(iss, name, ',')) {
                    // トリミング
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_last_not_of(" \t") + 1);
                    if (!name.empty()) {
                        import_info.imported_names.push_back(name);
                    }
                }
            }
            
            result.imports.push_back(import_info);
            ++iter;
        }
    }
}

void PythonAnalyzer::extract_parameters(const std::string& params_str, 
                                       std::vector<std::string>& parameters) {
    std::istringstream iss(params_str);
    std::string param;
    
    while (std::getline(iss, param, ',')) {
        // トリミング
        param.erase(0, param.find_first_not_of(" \t"));
        param.erase(param.find_last_not_of(" \t") + 1);
        
        // デフォルト値や型注釈を除去（簡易版）
        size_t pos = param.find('=');
        if (pos == std::string::npos) {
            pos = param.find(':');
        }
        if (pos != std::string::npos) {
            param = param.substr(0, pos);
            param.erase(param.find_last_not_of(" \t") + 1);
        }
        
        if (!param.empty() && param != "self" && param != "cls") {
            parameters.push_back(param);
        }
    }
}

size_t PythonAnalyzer::find_class_end(const std::string& content, size_t class_start) {
    // インデントレベルを追跡してクラスの終了位置を検出
    size_t pos = class_start;
    size_t line_start = content.find('\n', pos);
    if (line_start == std::string::npos) {
        return content.length();
    }
    
    // クラス定義行のインデントレベルを取得
    size_t class_indent = 0;
    size_t line_begin = content.rfind('\n', class_start);
    if (line_begin != std::string::npos) {
        line_begin++;
        while (line_begin < class_start && std::isspace(content[line_begin])) {
            class_indent++;
            line_begin++;
        }
    }
    
    // 次の同じインデントレベルまたはファイル終端を探す
    pos = line_start + 1;
    while (pos < content.length()) {
        size_t line_end = content.find('\n', pos);
        if (line_end == std::string::npos) {
            line_end = content.length();
        }
        
        // 空行はスキップ
        bool is_empty = true;
        for (size_t i = pos; i < line_end && is_empty; ++i) {
            if (!std::isspace(content[i])) {
                is_empty = false;
            }
        }
        
        if (!is_empty) {
            // インデントレベルチェック
            size_t indent = 0;
            size_t i = pos;
            while (i < line_end && std::isspace(content[i])) {
                indent++;
                i++;
            }
            
            if (indent <= class_indent) {
                return pos;
            }
        }
        
        pos = line_end + 1;
    }
    
    return content.length();
}

ComplexityInfo PythonAnalyzer::calculate_python_complexity(const std::string& content) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1;
    
    // Python固有の複雑度キーワード
    std::vector<std::string> complexity_keywords = {
        "if ", "elif ", "else:", "for ", "while ", 
        "except", "except:", "and ", "or ", 
        "try:", "with ", "lambda ", "yield "
    };
    
    for (const auto& keyword : complexity_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // ネスト深度計算（インデントベース）
    calculate_nesting_depth(content, complexity);
    
    complexity.update_rating();
    return complexity;
}

void PythonAnalyzer::calculate_nesting_depth(const std::string& content, 
                                            ComplexityInfo& complexity) {
    std::istringstream stream(content);
    std::string line;
    uint32_t max_indent = 0;
    
    while (std::getline(stream, line)) {
        uint32_t indent = 0;
        for (char c : line) {
            if (c == ' ') {
                indent++;
            } else if (c == '\t') {
                indent += 4; // タブは4スペースとして扱う
            } else {
                break;
            }
        }
        
        // インデントレベルをネスト深度に変換（4スペース = 1レベル）
        uint32_t nesting = indent / 4;
        if (nesting > max_indent) {
            max_indent = nesting;
        }
    }
    
    complexity.max_nesting_depth = max_indent;
}

} // namespace nekocode