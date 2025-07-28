#pragma once

//=============================================================================
// 🐍 Python PEGTL Analyzer - インデント地獄攻略版
//
// 完全PEGTL移行：std::regex完全撤廃（JavaScript/C++成功パターン適用）
// Python特殊事情：インデント依存・def/class構文・import多様性
//=============================================================================

#include "nekocode/analyzers/base_analyzer.hpp"
#include "nekocode/analyzers/python_minimal_grammar.hpp"
#include <tao/pegtl.hpp>
#include <vector>
#include <string>

namespace nekocode {

//=============================================================================
// 🐍 Python解析状態（JavaScript成功パターン準拠）
//=============================================================================

struct PythonParseState {
    std::vector<ClassInfo> classes;
    std::vector<FunctionInfo> functions;
    std::vector<ImportInfo> imports;
    
    // Python特有：インデントレベル管理
    std::vector<int> indent_stack;
    int current_indent = 0;
    
    // 現在の解析位置情報
    size_t current_line = 1;
    std::string current_content;
    
    void update_line_from_content(const std::string& matched_text) {
        // 改行数をカウントして行番号更新
        for (char c : matched_text) {
            if (c == '\n') {
                current_line++;
            }
        }
    }
};

//=============================================================================
// 🎮 PEGTL Action System - Python特化版
//=============================================================================

template<typename Rule>
struct python_action : tao::pegtl::nothing<Rule> {};

// 🐍 class検出
template<>
struct python_action<python::minimal_grammar::python_class> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, PythonParseState& state) {
        std::string matched = in.string();
        state.update_line_from_content(matched);
        
        // class Name: から名前抽出
        size_t class_pos = matched.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = class_pos + 5; // "class"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = matched.substr(name_start, name_end - name_start);
                class_info.start_line = state.current_line;
                state.classes.push_back(class_info);
            }
        }
    }
};

// 🎯 def検出（関数・メソッド共通）
template<>
struct python_action<python::minimal_grammar::python_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, PythonParseState& state) {
        std::string matched = in.string();
        state.update_line_from_content(matched);
        
        // def name(): から名前抽出
        size_t def_pos = matched.find("def");
        if (def_pos != std::string::npos) {
            size_t name_start = def_pos + 3; // "def"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                func_info.start_line = state.current_line;
                state.functions.push_back(func_info);
            }
        }
    }
};

// 📦 import検出
template<>
struct python_action<python::minimal_grammar::python_import> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, PythonParseState& state) {
        std::string matched = in.string();
        state.update_line_from_content(matched);
        
        ImportInfo import_info;
        import_info.line_number = state.current_line;
        
        // import文の種類判定
        if (matched.find("from ") != std::string::npos) {
            // from module import name
            size_t from_pos = matched.find("from ");
            size_t import_pos = matched.find(" import ");
            
            if (from_pos != std::string::npos && import_pos != std::string::npos) {
                size_t module_start = from_pos + 5;
                std::string module_name = matched.substr(module_start, import_pos - module_start);
                
                // 空白削除
                module_name.erase(0, module_name.find_first_not_of(" \t"));
                module_name.erase(module_name.find_last_not_of(" \t") + 1);
                
                import_info.module_path = module_name;
                import_info.type = ImportType::ES6_IMPORT; // Python from_import
            }
        } else {
            // import module
            size_t import_pos = matched.find("import ");
            if (import_pos != std::string::npos) {
                size_t module_start = import_pos + 7;
                size_t module_end = matched.find(" as ", module_start);
                if (module_end == std::string::npos) {
                    module_end = matched.size();
                }
                
                std::string module_name = matched.substr(module_start, module_end - module_start);
                
                // 空白削除
                module_name.erase(0, module_name.find_first_not_of(" \t"));
                module_name.erase(module_name.find_last_not_of(" \t") + 1);
                
                import_info.module_path = module_name;
                import_info.type = ImportType::ES6_IMPORT; // Python import
            }
        }
        
        if (!import_info.module_path.empty()) {
            state.imports.push_back(import_info);
        }
    }
};

//=============================================================================
// 🐍 Python PEGTL Analyzer 本体
//=============================================================================

class PythonPEGTLAnalyzer : public BaseAnalyzer {
public:
    PythonPEGTLAnalyzer() = default;
    ~PythonPEGTLAnalyzer() = default;
    
    Language get_language() const override {
        return Language::PYTHON;
    }
    
    std::string get_language_name() const override {
        return "Python (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".py", ".pyw", ".pyi"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        AnalysisResult result;
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::PYTHON;
        
        // 強制デバッグ: Python PEGTL analyzer が呼ばれたことを確認
        ClassInfo debug_class;
        debug_class.name = "PYTHON_PEGTL_ANALYZER_CALLED";
        debug_class.start_line = 1;
        result.classes.push_back(debug_class);
        
        // PEGTL解析実行
        bool pegtl_success = false;
        try {
            PythonParseState state;
            state.current_content = content;
            
            tao::pegtl::string_input input(content, filename);
            bool success = tao::pegtl::parse<python::minimal_grammar::python_minimal, 
                                          python_action>(input, state);
            
            if (success && (!state.classes.empty() || !state.functions.empty() || !state.imports.empty())) {
                // 解析結果をAnalysisResultに移動
                result.classes.insert(result.classes.end(), state.classes.begin(), state.classes.end());
                result.functions = std::move(state.functions);
                result.imports = std::move(state.imports);
                
                pegtl_success = true;
            }
            
        } catch (const tao::pegtl::parse_error& e) {
            // パースエラーは警告として記録（完全失敗ではない）
            pegtl_success = false;
        }
        
        // 🚨 PEGTL失敗時のフォールバック戦略
        if (!pegtl_success) {
            // 簡易パターンマッチング（std::regex代替）
            auto fallback_classes = extract_classes_fallback(content);
            auto fallback_functions = extract_functions_fallback(content);
            auto fallback_imports = extract_imports_fallback(content);
            
            // デバッグクラスを保持しつつフォールバック結果を追加
            result.classes.insert(result.classes.end(), fallback_classes.begin(), fallback_classes.end());
            result.functions.insert(result.functions.end(), fallback_functions.begin(), fallback_functions.end());
            result.imports.insert(result.imports.end(), fallback_imports.begin(), fallback_imports.end());
        }
        
        // 複雑度計算（Python特化版）
        result.complexity = calculate_python_complexity(content);
        
        // 統計更新
        result.update_statistics();
        
        return result;
    }

private:
    // 複雑度計算（Python特化版）
    ComplexityInfo calculate_python_complexity(const std::string& content) {
        ComplexityInfo complexity;
        complexity.cyclomatic_complexity = 1;
        
        // Python固有の複雑度キーワード
        std::vector<std::string> complexity_keywords = {
            "if ", "elif ", "else:", "for ", "while ", "try:", "except:", 
            "finally:", "with ", "and ", "or ", "lambda:", "assert ", 
            "yield ", "return ", "break ", "continue "
        };
        
        for (const auto& keyword : complexity_keywords) {
            size_t pos = 0;
            while ((pos = content.find(keyword, pos)) != std::string::npos) {
                complexity.cyclomatic_complexity++;
                pos += keyword.length();
            }
        }
        
        // インデントベースネスト深度計算
        complexity.max_nesting_depth = calculate_indent_depth(content);
        
        complexity.update_rating();
        return complexity;
    }
    
    // インデント深度計算（Python特有）
    uint32_t calculate_indent_depth(const std::string& content) {
        uint32_t max_depth = 0;
        uint32_t current_depth = 0;
        
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (line.empty() || line[0] == '#') {
                continue; // 空行・コメント行はスキップ
            }
            
            // 行頭空白をカウント
            uint32_t indent_count = 0;
            for (char c : line) {
                if (c == ' ') {
                    indent_count++;
                } else if (c == '\t') {
                    indent_count += 4; // タブ = 4スペース換算
                } else {
                    break;
                }
            }
            
            // インデントレベル計算（4スペース = 1レベル）
            uint32_t indent_level = indent_count / 4;
            current_depth = indent_level;
            
            if (current_depth > max_depth) {
                max_depth = current_depth;
            }
        }
        
        return max_depth;
    }
    
    // 🚨 フォールバック戦略（std::regex不使用版）
    std::vector<ClassInfo> extract_classes_fallback(const std::string& content) {
        std::vector<ClassInfo> classes;
        
        // classパターン検索
        size_t pos = 0;
        while ((pos = content.find("class ", pos)) != std::string::npos) {
            size_t name_start = pos + 6; // "class "の長さ
            while (name_start < content.size() && std::isspace(content[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < content.size() && 
                   (std::isalnum(content[name_end]) || content[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = content.substr(name_start, name_end - name_start);
                class_info.start_line = 1; // 簡易版
                classes.push_back(class_info);
            }
            
            pos = name_end;
        }
        
        return classes;
    }
    
    std::vector<FunctionInfo> extract_functions_fallback(const std::string& content) {
        std::vector<FunctionInfo> functions;
        
        // defパターン検索
        size_t pos = 0;
        while ((pos = content.find("def ", pos)) != std::string::npos) {
            size_t name_start = pos + 4; // "def "の長さ
            while (name_start < content.size() && std::isspace(content[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < content.size() && 
                   (std::isalnum(content[name_end]) || content[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = content.substr(name_start, name_end - name_start);
                func_info.start_line = 1; // 簡易版
                functions.push_back(func_info);
            }
            
            pos = name_end;
        }
        
        return functions;
    }
    
    std::vector<ImportInfo> extract_imports_fallback(const std::string& content) {
        std::vector<ImportInfo> imports;
        
        // importパターン検索
        size_t pos = 0;
        while ((pos = content.find("import ", pos)) != std::string::npos) {
            // 行の開始位置を探す
            size_t line_start = pos;
            while (line_start > 0 && content[line_start - 1] != '\n') {
                line_start--;
            }
            
            // 行末を探す
            size_t line_end = pos;
            while (line_end < content.size() && content[line_end] != '\n') {
                line_end++;
            }
            
            if (line_end > line_start) {
                std::string import_line = content.substr(line_start, line_end - line_start);
                
                ImportInfo import_info;
                import_info.module_path = import_line;
                import_info.type = ImportType::ES6_IMPORT;
                import_info.line_number = 1; // 簡易版
                imports.push_back(import_info);
            }
            
            pos = line_end;
        }
        
        return imports;
    }
};

} // namespace nekocode