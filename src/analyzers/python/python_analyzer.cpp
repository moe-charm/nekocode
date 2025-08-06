//=============================================================================
// 🐍 Python Language Analyzer - Python専用解析エンジン (std::regex完全除去版)
//
// 文字列ベース解析 + ハイブリッド戦略
// Unity analyzer の成功パターンをPythonに適用
//=============================================================================

#include "nekocode/analyzers/python_analyzer.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

// 🚫 std::regex 完全除去 - 文字列ベース解析に移行

namespace nekocode {

//=============================================================================
// 🐍 PythonAnalyzer Implementation (String-based)
//=============================================================================

PythonAnalyzer::PythonAnalyzer() {
    // 🐍 Python analyzer (std::regex完全除去版)
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
    // Analyzing Python file
    
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::PYTHON;
    
    // 文字列ベース解析実行
    extract_classes(content, result);
    extract_functions(content, result);
    extract_imports(content, result);
    
    // 複雑度計算（Python特化版）
    result.complexity = calculate_python_complexity(content);
    
    // 🎯 ハイブリッド戦略: 統計整合性チェック
    if (needs_python_line_based_fallback(result, content)) {
        // Python line-based fallback triggered
        apply_python_line_based_analysis(result, content);
    }
    
    // 統計更新
    result.update_statistics();
    
    std::cerr << "✅ Python analysis completed. Classes: " << result.classes.size() 
              << ", Functions: " << result.functions.size() << std::endl;
    
    return result;
}

//=============================================================================
// 🔍 Python 文字列ベース解析実装
//=============================================================================

void PythonAnalyzer::extract_classes(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_python_class_line(line)) {
            // "class ClassName:" または "class ClassName(Base):" パターン
            size_t class_pos = line.find("class ");
            if (class_pos != std::string::npos) {
                size_t name_start = class_pos + 6; // "class " の長さ
                size_t name_end = line.find_first_of(" (:", name_start);
                
                if (name_end != std::string::npos) {
                    std::string class_name = line.substr(name_start, name_end - name_start);
                    
                    ClassInfo class_info;
                    class_info.name = class_name;
                    class_info.start_line = line_number;
                    
                    // 継承関係検出
                    size_t paren_start = line.find('(', name_end);
                    size_t paren_end = line.find(')', paren_start);
                    if (paren_start != std::string::npos && paren_end != std::string::npos) {
                        std::string parent = line.substr(paren_start + 1, paren_end - paren_start - 1);
                        // 簡単な清理 (空白削除)
                        parent.erase(std::remove_if(parent.begin(), parent.end(), ::isspace), parent.end());
                        if (!parent.empty()) {
                            class_info.parent_class = parent;
                        }
                    }
                    
                    result.classes.push_back(class_info);
                }
            }
        }
        line_number++;
    }
}

void PythonAnalyzer::extract_functions(const std::string& content, AnalysisResult& result) {
    // 🎯 end_line計算用に全行を保存
    std::vector<std::string> all_lines;
    std::istringstream prestream(content);
    std::string preline;
    while (std::getline(prestream, preline)) {
        all_lines.push_back(preline);
    }
    
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_python_function_line(line)) {
            // インデントレベルでトップレベル関数を識別
            int indent = calculate_indentation_depth(line);
            std::cerr << "DEBUG: Line " << line_number << ": found function line: " << line << " (indent=" << indent << ")" << std::endl;
            
            // トップレベル関数のみ抽出（クラス内メソッドは除外）
            // TODO: クラス内メソッドも検出するオプションを追加
            if (indent == 0) {
                size_t def_pos = line.find("def ");
                if (def_pos != std::string::npos) {
                    size_t name_start = def_pos + 4; // "def " の長さ
                    size_t name_end = line.find('(', name_start);
                    
                    if (name_end != std::string::npos) {
                        std::string func_name = line.substr(name_start, name_end - name_start);
                        
                        FunctionInfo func_info;
                        func_info.name = func_name;
                        func_info.start_line = line_number;
                        
                        func_info.end_line = find_function_end_line(all_lines, line_number - 1, indent);
                        
                        // パラメータ抽出
                        func_info.parameters = extract_parameters(line);
                        
                        // async関数チェック
                        if (line.find("async def") != std::string::npos) {
                            func_info.is_async = true;
                        }
                        
                        result.functions.push_back(func_info);
                    }
                }
            }
        }
        line_number++;
    }
}

void PythonAnalyzer::extract_imports(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        if (is_python_import_line(line)) {
            ImportInfo import_info;
            import_info.line_number = line_number;
            
            if (line.find("from ") == 0) {
                // "from module import name" パターン
                size_t from_pos = 5; // "from " の長さ
                size_t import_pos = line.find(" import ");
                
                if (import_pos != std::string::npos) {
                    std::string module = line.substr(from_pos, import_pos - from_pos);
                    std::string names = line.substr(import_pos + 8); // " import " の長さ
                    
                    import_info.type = ImportType::ES6_IMPORT; // Pythonでは適切な型がないので暫定
                    import_info.module_path = module;
                    
                    // 複数名前のサポート ("name1, name2")
                    std::stringstream names_stream(names);
                    std::string name;
                    while (std::getline(names_stream, name, ',')) {
                        // 空白除去
                        name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
                        if (!name.empty()) {
                            import_info.imported_names.push_back(name);
                        }
                    }
                }
            } else if (line.find("import ") == 0) {
                // "import module" パターン
                size_t import_pos = 7; // "import " の長さ
                std::string modules = line.substr(import_pos);
                
                import_info.type = ImportType::COMMONJS_REQUIRE; // 暫定
                import_info.module_path = modules;
            }
            
            if (!import_info.module_path.empty()) {
                result.imports.push_back(import_info);
            }
        }
        line_number++;
    }
}

//=============================================================================
// 🧮 Python 特化複雑度計算 + ユーティリティ
//=============================================================================

ComplexityInfo PythonAnalyzer::calculate_python_complexity(const std::string& content) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // ベーススコア
    
    // Python特有のキーワードで複雑度計算
    std::vector<std::string> complexity_keywords = {
        "if ", "elif ", "else:", "for ", "while ", "try:", "except", "finally:", 
        "with ", "match ", "case "  // Python 3.10+ match-case
    };
    
    for (const auto& keyword : complexity_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // 最大ネスト深度計算（インデントベース）
    std::istringstream stream(content);
    std::string line;
    int max_depth = 0;
    
    while (std::getline(stream, line)) {
        int depth = calculate_indentation_depth(line);
        if (depth > max_depth) {
            max_depth = depth;
        }
    }
    
    complexity.max_nesting_depth = static_cast<uint32_t>(max_depth / 4); // 4スペース = 1レベル
    complexity.update_rating();
    
    return complexity;
}

int PythonAnalyzer::calculate_indentation_depth(const std::string& line) {
    int spaces = 0;
    for (char c : line) {
        if (c == ' ') {
            spaces++;
        } else if (c == '\t') {
            spaces += 4; // タブは4スペース相当
        } else {
            break;
        }
    }
    return spaces;
}

// 🎯 Python関数の終了行を検出（インデントベース）
uint32_t PythonAnalyzer::find_function_end_line(const std::vector<std::string>& lines, size_t start_line, int base_indent) {
    // start_lineが0-indexedで、結果は1-indexedにする必要がある
    if (start_line >= lines.size()) {
        return static_cast<uint32_t>(start_line + 1);
    }
    
    // 関数の本体は次の行から始まる
    size_t current_line = start_line + 1;
    uint32_t last_non_empty_line = static_cast<uint32_t>(start_line + 1);
    
    while (current_line < lines.size()) {
        const std::string& line = lines[current_line];
        
        // 空行の判定
        std::string trimmed = line;
        size_t first_non_space = trimmed.find_first_not_of(" \t\r\n");
        
        if (first_non_space != std::string::npos) {
            trimmed = trimmed.substr(first_non_space);
            
            // コメント行でない場合
            if (trimmed[0] != '#') {
                int indent = calculate_indentation_depth(line);
                
                // 同じまたはそれより浅いインデントレベルの非空白行を見つけたら終了
                if (indent <= base_indent && !trimmed.empty()) {
                    // 前の行が関数の最終行
                    return last_non_empty_line;
                }
                
                // 関数内のコードなので、最後の非空白行を更新
                last_non_empty_line = static_cast<uint32_t>(current_line + 1);
            } else {
                // コメント行も関数の一部とみなす
                last_non_empty_line = static_cast<uint32_t>(current_line + 1);
            }
        }
        
        current_line++;
    }
    
    // ファイルの終わりまで到達した場合
    return last_non_empty_line;
}

std::vector<std::string> PythonAnalyzer::extract_parameters(const std::string& func_line) {
    std::vector<std::string> parameters;
    
    size_t paren_start = func_line.find('(');
    size_t paren_end = func_line.find(')', paren_start);
    
    if (paren_start != std::string::npos && paren_end != std::string::npos) {
        std::string params_str = func_line.substr(paren_start + 1, paren_end - paren_start - 1);
        
        if (!params_str.empty()) {
            std::stringstream params_stream(params_str);
            std::string param;
            
            while (std::getline(params_stream, param, ',')) {
                // 空白・型ヒント除去の簡易版
                size_t colon_pos = param.find(':');
                if (colon_pos != std::string::npos) {
                    param = param.substr(0, colon_pos);
                }
                
                // 空白除去
                param.erase(std::remove_if(param.begin(), param.end(), ::isspace), param.end());
                
                if (!param.empty() && param != "self") {
                    parameters.push_back(param);
                }
            }
        }
    }
    
    return parameters;
}

//=============================================================================
// 🔍 Python キーワード検出
//=============================================================================

bool PythonAnalyzer::is_python_function_line(const std::string& line) {
    std::string trimmed = line;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
    
    return line.find("def ") != std::string::npos && line.find(':') != std::string::npos;
}

bool PythonAnalyzer::is_python_class_line(const std::string& line) {
    std::string trimmed = line;
    size_t first_non_space = line.find_first_not_of(" \t");
    
    if (first_non_space != std::string::npos) {
        std::string content = line.substr(first_non_space);
        return content.find("class ") == 0 && content.find(':') != std::string::npos;
    }
    
    return false;
}

bool PythonAnalyzer::is_python_import_line(const std::string& line) {
    std::string trimmed = line;
    size_t first_non_space = line.find_first_not_of(" \t");
    
    if (first_non_space != std::string::npos) {
        std::string content = line.substr(first_non_space);
        return content.find("import ") == 0 || content.find("from ") == 0;
    }
    
    return false;
}

//=============================================================================
// 🎯 Python ハイブリッド戦略
//=============================================================================

bool PythonAnalyzer::needs_python_line_based_fallback(const AnalysisResult& result, const std::string& content) {
    // Python特化の統計整合性チェック
    
    // 1. def キーワードがあるのに関数が検出されていない
    if (content.find("def ") != std::string::npos && result.functions.empty()) {
        return true;
    }
    
    // 2. class キーワードがあるのにクラスが検出されていない
    if (content.find("class ") != std::string::npos && result.classes.empty()) {
        return true;
    }
    
    // 3. インデント複雑度 vs 検出数の不整合
    int def_count = 0;
    size_t pos = 0;
    while ((pos = content.find("def ", pos)) != std::string::npos) {
        def_count++;
        pos += 4;
    }
    
    if (def_count > 0 && result.functions.size() < static_cast<size_t>(def_count / 2)) {
        return true; // 半分以下しか検出できていない
    }
    
    return false;
}

void PythonAnalyzer::apply_python_line_based_analysis(AnalysisResult& result, const std::string& content) {
    // 🔧 フォールバック: より単純な行ベース解析
    
    // 🎯 end_line計算用に全行を保存
    std::vector<std::string> all_lines;
    std::istringstream prestream(content);
    std::string preline;
    while (std::getline(prestream, preline)) {
        all_lines.push_back(preline);
    }
    
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 1;
    
    while (std::getline(stream, line)) {
        // より緩い条件での関数検出
        if (line.find("def ") != std::string::npos) {
            size_t def_pos = line.find("def ");
            size_t name_start = def_pos + 4;
            size_t paren_pos = line.find('(', name_start);
            
            if (paren_pos != std::string::npos) {
                std::string func_name = line.substr(name_start, paren_pos - name_start);
                
                // 空白除去
                func_name.erase(std::remove_if(func_name.begin(), func_name.end(), ::isspace), func_name.end());
                
                if (!func_name.empty()) {
                    // 重複チェック
                    bool already_exists = false;
                    for (const auto& existing : result.functions) {
                        if (existing.name == func_name) {
                            already_exists = true;
                            break;
                        }
                    }
                    
                    if (!already_exists) {
                        int indent = calculate_indentation_depth(line);
                        FunctionInfo func_info;
                        func_info.name = func_name;
                        func_info.start_line = line_number;
                        func_info.end_line = find_function_end_line(all_lines, line_number - 1, indent);
                        result.functions.push_back(func_info);
                    }
                }
            }
        }
        
        // より緩い条件でのクラス検出
        if (line.find("class ") != std::string::npos) {
            size_t class_pos = line.find("class ");
            size_t name_start = class_pos + 6;
            size_t name_end = line.find_first_of(" (:", name_start);
            
            if (name_end != std::string::npos) {
                std::string class_name = line.substr(name_start, name_end - name_start);
                
                if (!class_name.empty()) {
                    // 重複チェック
                    bool already_exists = false;
                    for (const auto& existing : result.classes) {
                        if (existing.name == class_name) {
                            already_exists = true;
                            break;
                        }
                    }
                    
                    if (!already_exists) {
                        ClassInfo class_info;
                        class_info.name = class_name;
                        class_info.start_line = line_number;
                        result.classes.push_back(class_info);
                    }
                }
            }
        }
        
        line_number++;
    }
}

} // namespace nekocode