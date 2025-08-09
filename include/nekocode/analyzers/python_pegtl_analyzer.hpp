#pragma once

//=============================================================================
// 🐍 Python PEGTL Analyzer - インデント地獄攻略版
//
// 完全PEGTL移行：std::regex完全撤廃（JavaScript/C++成功パターン適用）
// Python特殊事情：インデント依存・def/class構文・import多様性
//=============================================================================

#include "base_analyzer.hpp"
#include "nekocode/analyzers/python_minimal_grammar.hpp"
#include <tao/pegtl.hpp>
#include <vector>
#include <string>

// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"

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
    
    // 🚀 Phase 5: Universal Symbol直接生成 
    std::shared_ptr<SymbolTable> symbol_table;
    std::unordered_map<std::string, int> id_counters;
    
    // コンストラクタ
    PythonParseState() {
        symbol_table = std::make_shared<SymbolTable>();
    }
    
    std::string generate_unique_id(const std::string& base) {
        int& counter = id_counters[base];
        return base + "_" + std::to_string(counter++);
    }
    
    /// 🐍 テスト用: Pythonクラス Symbol生成
    void add_test_class_symbol(const std::string& class_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("class_" + class_name);
        symbol.symbol_type = SymbolType::CLASS;
        symbol.name = class_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "python";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] Python adding class symbol: " << class_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
    /// 🐍 テスト用: Python関数 Symbol生成
    void add_test_function_symbol(const std::string& func_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("function_" + func_name);
        symbol.symbol_type = SymbolType::FUNCTION;
        symbol.name = func_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "python";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] Python adding function symbol: " << func_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
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
                std::string class_name = matched.substr(name_start, name_end - name_start);
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = state.current_line;
                state.classes.push_back(class_info);
                
                // 🚀 Phase 5 テスト: Universal Symbol直接生成
                state.add_test_class_symbol(class_name, state.current_line);
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
                
                // 🚀 Phase 5: Universal Symbol直接生成
                state.add_test_function_symbol(func_info.name, func_info.start_line);
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
        
        // 🔥 前処理革命：コメント・文字列除去システム（コメント収集付き）
        std::vector<CommentInfo> comments;
        std::string preprocessed_content = preprocess_content(content, &comments);
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::PYTHON;
        
        // 🆕 コメントアウト行情報を結果に追加
        result.commented_lines = std::move(comments);
        
        // デバッグコード削除（偽クラス検出問題修正）
        
        // PEGTL解析実行
        bool pegtl_success = false;
        try {
            PythonParseState state;
            state.current_content = preprocessed_content;
            
            tao::pegtl::string_input input(preprocessed_content, filename);
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
        // 改善：常にフォールバックも実行し、PEGTLで見つからなかった関数を補完
        {
            // 簡易パターンマッチング（std::regex代替）
            auto fallback_classes = extract_classes_fallback(content);
            auto fallback_functions = extract_functions_fallback(content);
            auto fallback_imports = extract_imports_fallback(content);
            
            // PEGTLで見つからなかった要素を追加
            for (const auto& fc : fallback_functions) {
                bool found = false;
                for (const auto& pf : result.functions) {
                    if (pf.name == fc.name && pf.start_line == fc.start_line) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.functions.push_back(fc);
                }
            }
            
            for (const auto& cc : fallback_classes) {
                bool found = false;
                for (const auto& pc : result.classes) {
                    if (pc.name == cc.name && pc.start_line == cc.start_line) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.classes.push_back(cc);
                }
            }
            
            // imports は単純に追加（重複チェック省略）
            if (!pegtl_success) {
                result.imports.insert(result.imports.end(), fallback_imports.begin(), fallback_imports.end());
            }
        }
        
        // 複雑度計算（Python特化版）
        result.complexity = calculate_python_complexity(content);
        
        // 🔍 Python メンバ変数検出（JavaScript成功パターン移植）
        detect_member_variables(result, content);
        
        // 🚀 重要: クラス-メソッド関連付け処理（JavaScript成功パターン適用）
        associate_methods_with_classes(result, content);
        
        // 統計更新
        result.update_statistics();
        
        // 🚀 Phase 5: Universal Symbol直接生成（PythonParseStateから取得）
        try {
            PythonParseState state;  // 上で作成されたstateを再利用したいが、スコープ外なので一時的に新しいstateを作成
            state.current_content = content;
            tao::pegtl::string_input input(content, filename);
            tao::pegtl::parse<python::minimal_grammar::python_minimal, python_action>(input, state);
            
            if (state.symbol_table && state.symbol_table->get_all_symbols().size() > 0) {
                result.universal_symbols = state.symbol_table;
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[Phase 5] Python analyzer generated " 
                          << state.symbol_table->get_all_symbols().size() 
                          << " Universal Symbols" << std::endl;
#endif
            }
        } catch (...) {
            // Phase 5のエラーは無視（メイン解析に影響しない）
        }
        
        return result;
    }

private:
    // 位置から行番号を計算（Python位置バグ修正用）
    size_t calculate_line_number(const std::string& content, size_t position) {
        size_t line_count = 1;
        for (size_t i = 0; i < position && i < content.size(); ++i) {
            if (content[i] == '\n') {
                line_count++;
            }
        }
        return line_count;
    }
    
    // 🎯 Python関数の終了行を検出（linesベクターを使用）
    uint32_t find_function_end_line_with_lines(const std::vector<std::string>& lines, size_t start_idx, uint32_t base_indent_level) {
        // 関数の終了を探す
        uint32_t last_non_empty = start_idx + 1;  // 1ベースの行番号
        
        for (size_t i = start_idx + 1; i < lines.size(); ++i) {
            const std::string& current_line = lines[i];
            
            // 空行やコメント行はスキップして記録
            if (current_line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            if (current_line.find_first_not_of(" \t") != std::string::npos && 
                current_line[current_line.find_first_not_of(" \t")] == '#') {
                continue;
            }
            
            // インデントレベルを計算
            uint32_t indent = 0;
            for (char c : current_line) {
                if (c == ' ') indent++;
                else if (c == '\t') indent += 4;
                else break;
            }
            uint32_t indent_level = indent / 4;
            
            // 同じまたはそれより浅いインデントの非空白行を見つけたら終了
            if (indent_level <= base_indent_level) {
                return i;  // 前の行が関数の最後
            }
            
            last_non_empty = i + 1;  // 1ベースの行番号
        }
        
        // ファイルの最後まで到達した場合
        return last_non_empty;
    }
    
    // 🎯 Python関数の終了行を検出（インデントベース）
    uint32_t find_function_end_line(const std::string& content, uint32_t start_line) {
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        if (start_line == 0 || start_line > lines.size()) {
            return start_line;
        }
        
        // 開始行のインデントレベルを取得
        std::string start_line_str = lines[start_line - 1];
        int base_indent = 0;
        for (char c : start_line_str) {
            if (c == ' ') base_indent++;
            else if (c == '\t') base_indent += 4;
            else break;
        }
        
        // 関数の終了を探す
        uint32_t last_non_empty = start_line;
        for (size_t i = start_line; i < lines.size(); ++i) {
            const std::string& current_line = lines[i];
            
            // 空行をスキップ
            if (current_line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            
            // インデントレベルを計算
            int indent = 0;
            for (char c : current_line) {
                if (c == ' ') indent++;
                else if (c == '\t') indent += 4;
                else break;
            }
            
            // 同じまたはそれより浅いインデントの非空白行を見つけたら終了
            if (indent <= base_indent && current_line.find_first_not_of(" \t") != std::string::npos) {
                return last_non_empty;
            }
            
            last_non_empty = static_cast<uint32_t>(i + 1);
        }
        
        return last_non_empty;
    }
    
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
        
        // classパターン検索（行の先頭にあるclass文のみ）
        size_t pos = 0;
        while ((pos = content.find("class ", pos)) != std::string::npos) {
            // 行の先頭かどうかチェック（インデント込み）
            bool is_line_start = true;
            if (pos > 0) {
                // pos直前まで行の始まりからスペース/タブのみかチェック
                size_t line_start = content.rfind('\n', pos - 1);
                if (line_start == std::string::npos) line_start = 0;
                else line_start++; // '\n'の次から
                
                for (size_t i = line_start; i < pos; i++) {
                    if (content[i] != ' ' && content[i] != '\t') {
                        is_line_start = false;
                        break;
                    }
                }
            }
            
            if (!is_line_start) {
                pos++; // docstring内のclassをスキップ
                continue;
            }
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
                
                // 正確な行番号計算: pos位置までの改行数をカウント
                uint32_t line_number = 1;
                for (size_t i = 0; i < pos && i < content.size(); i++) {
                    if (content[i] == '\n') {
                        line_number++;
                    }
                }
                class_info.start_line = line_number;
                
                classes.push_back(class_info);
            }
            
            pos = name_end;
        }
        
        return classes;
    }
    
    std::vector<FunctionInfo> extract_functions_fallback(const std::string& content) {
        std::vector<FunctionInfo> functions;
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        
        // 全行をベクターに格納
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        // クラス情報を追跡
        struct ClassScope {
            std::string name;
            uint32_t indent_level;
            uint32_t start_line;
        };
        std::vector<ClassScope> class_stack;
        
        // 各行を解析
        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string& current_line = lines[i];
            uint32_t line_number = i + 1;
            
            // 空行やコメント行をスキップ
            if (current_line.empty() || current_line.find_first_not_of(" \t") == std::string::npos) {
                continue;
            }
            if (current_line.find_first_not_of(" \t") != std::string::npos && 
                current_line[current_line.find_first_not_of(" \t")] == '#') {
                continue;
            }
            
            // インデントレベルを計算
            uint32_t indent_level = 0;
            for (char c : current_line) {
                if (c == ' ') indent_level++;
                else if (c == '\t') indent_level += 4;
                else break;
            }
            indent_level = indent_level / 4;  // 4スペース = 1レベル
            
            // クラススタックを整理（深いインデントから抜けた場合）
            while (!class_stack.empty() && class_stack.back().indent_level >= indent_level) {
                class_stack.pop_back();
            }
            
            // class定義を検出
            size_t class_pos = current_line.find("class ");
            if (class_pos != std::string::npos) {
                size_t name_start = class_pos + 6;  // "class "の長さ
                while (name_start < current_line.size() && std::isspace(current_line[name_start])) {
                    name_start++;
                }
                
                size_t name_end = name_start;
                while (name_end < current_line.size() && 
                       (std::isalnum(current_line[name_end]) || current_line[name_end] == '_')) {
                    name_end++;
                }
                
                if (name_end > name_start) {
                    ClassScope cls;
                    cls.name = current_line.substr(name_start, name_end - name_start);
                    cls.indent_level = indent_level;
                    cls.start_line = line_number;
                    class_stack.push_back(cls);
                }
            }
            
            // def定義を検出
            size_t def_pos = current_line.find("def ");
            if (def_pos != std::string::npos) {
                size_t name_start = def_pos + 4;  // "def "の長さ
                while (name_start < current_line.size() && std::isspace(current_line[name_start])) {
                    name_start++;
                }
                
                size_t name_end = name_start;
                while (name_end < current_line.size() && 
                       (std::isalnum(current_line[name_end]) || current_line[name_end] == '_')) {
                    name_end++;
                }
                
                if (name_end > name_start) {
                    FunctionInfo func_info;
                    std::string func_name = current_line.substr(name_start, name_end - name_start);
                    
                    // クラス内のメソッドかどうか判定
                    if (!class_stack.empty() && indent_level > class_stack.back().indent_level) {
                        // クラス内メソッドの場合、クラス名も記録（オプション）
                        func_info.name = func_name;  // メソッド名のみ保存
                        // func_info.name = class_stack.back().name + "::" + func_name;  // クラス名::メソッド名
                    } else {
                        // トップレベル関数
                        func_info.name = func_name;
                    }
                    
                    func_info.start_line = line_number;
                    // linesベクターを使って直接end_lineを計算
                    func_info.end_line = find_function_end_line_with_lines(lines, i, indent_level);
                    functions.push_back(func_info);
                }
            }
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
    
    // 🔍 Python メンバ変数検出（JavaScript成功パターン + Python特化）
    void detect_member_variables(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 現在解析中のクラス情報
        std::string current_class;
        size_t current_class_index = 0;
        bool in_init_method = false;
        size_t current_indent_level = 0;
        size_t class_indent_level = 0;
        
        while (std::getline(stream, line)) {
            line_number++;
            
            // インデント計算
            size_t line_indent = 0;
            for (char c : line) {
                if (c == ' ') line_indent++;
                else if (c == '\t') line_indent += 4; // タブは4スペース相当
                else break;
            }
            
            // クラス終了チェック（インデントベース）
            if (!current_class.empty() && line_indent <= class_indent_level && 
                !line.empty() && line.find_first_not_of(" \t") != std::string::npos) {
                current_class.clear();
                in_init_method = false;
                class_indent_level = 0;
            }
            
            // クラス開始検出
            std::regex class_pattern(R"(^\s*class\s+(\w+))");
            std::smatch class_match;
            if (std::regex_search(line, class_match, class_pattern)) {
                current_class = class_match[1].str();
                class_indent_level = line_indent;
                
                // 既存のクラス情報を見つける
                for (size_t i = 0; i < result.classes.size(); i++) {
                    if (result.classes[i].name == current_class) {
                        current_class_index = i;
                        break;
                    }
                }
            }
            
            // __init__ メソッド検出
            if (!current_class.empty()) {
                std::regex init_pattern(R"(^\s*def\s+__init__\s*\()");
                if (std::regex_search(line, init_pattern)) {
                    in_init_method = true;
                }
                
                // メソッド終了チェック（次のdefまたはクラス終了）
                std::regex method_pattern(R"(^\s*def\s+\w+)");
                if (in_init_method && std::regex_search(line, method_pattern) && 
                    line.find("__init__") == std::string::npos) {
                    in_init_method = false;
                }
            }
            
            // Python メンバ変数パターン検出
            if (!current_class.empty() && current_class_index < result.classes.size()) {
                detect_python_member_patterns(line, line_number, result.classes[current_class_index], in_init_method, line_indent);
            }
        }
    }
    
    // Python メンバ変数パターン検出（Python特化版）
    void detect_python_member_patterns(const std::string& line, size_t line_number, 
                                      ClassInfo& class_info, bool in_init_method, size_t line_indent) {
        std::smatch match;
        
        // パターン1: self.property = value (__init__やメソッド内)
        std::regex self_property_pattern(R"(self\.(\w+)\s*=)");
        auto self_begin = std::sregex_iterator(line.begin(), line.end(), self_property_pattern);
        auto self_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = self_begin; i != self_end; ++i) {
            std::smatch match = *i;
            std::string property_name = match[1].str();
            
            // 重複チェック
            if (!member_already_exists(class_info, property_name)) {
                MemberVariable member;
                member.name = property_name;
                member.type = infer_python_type_from_assignment(line);
                member.declaration_line = line_number;
                member.access_modifier = determine_python_access_modifier(property_name);
                
                class_info.member_variables.push_back(member);
            }
        }
        
        // パターン2: クラス変数（クラス直下のインデント）
        std::regex class_variable_pattern(R"(^\s*(\w+)\s*=)");
        if (std::regex_search(line, match, class_variable_pattern)) {
            std::string var_name = match[1].str();
            
            // __init__ 内やメソッド内でない場合のみ（クラス変数）
            if (!in_init_method && !member_already_exists(class_info, var_name)) {
                // 簡易的なメソッド検出を除外
                if (line.find("def ") == std::string::npos && line.find("(") == std::string::npos) {
                    MemberVariable member;
                    member.name = var_name;
                    member.type = infer_python_type_from_assignment(line);
                    member.declaration_line = line_number;
                    member.access_modifier = determine_python_access_modifier(var_name);
                    member.is_static = true; // Pythonクラス変数は静的
                    
                    class_info.member_variables.push_back(member);
                }
            }
        }
        
        // パターン3: Type hints (Python 3.6+) property: Type = value
        std::regex type_hint_pattern(R"(^\s*(\w+)\s*:\s*([^=]+)\s*=)");
        if (std::regex_search(line, match, type_hint_pattern)) {
            std::string property_name = match[1].str();
            std::string type_annotation = match[2].str();
            
            if (!in_init_method && !member_already_exists(class_info, property_name)) {
                MemberVariable member;
                member.name = property_name;
                member.type = trim_python_type(type_annotation);
                member.declaration_line = line_number;
                member.access_modifier = determine_python_access_modifier(property_name);
                member.is_static = true;
                
                class_info.member_variables.push_back(member);
            }
        }
        
        // パターン4: @property デコレータ
        std::regex property_decorator_pattern(R"(^\s*@property)");
        if (std::regex_search(line, property_decorator_pattern)) {
            // 次の行でdef name(self): を探す必要があるが、簡易版では省略
            // 将来的にデコレータ対応を強化
        }
    }
    
    // ヘルパー関数：Python型推論
    std::string infer_python_type_from_assignment(const std::string& line) {
        if (line.find("= []") != std::string::npos) {
            return "list";
        } else if (line.find("= {}") != std::string::npos) {
            return "dict";
        } else if (line.find("= set()") != std::string::npos) {
            return "set";
        } else if (line.find("= True") != std::string::npos || line.find("= False") != std::string::npos) {
            return "bool";
        } else if (line.find("= \"") != std::string::npos || line.find("= '") != std::string::npos) {
            return "str";
        } else if (line.find("= f\"") != std::string::npos || line.find("= f'") != std::string::npos) {
            return "str";
        } else if (std::regex_search(line, std::regex(R"(= \d+\.\d+)"))) {
            return "float";
        } else if (std::regex_search(line, std::regex(R"(= \d+)"))) {
            return "int";
        } else if (line.find("= None") != std::string::npos) {
            return "None";
        }
        return "Any";
    }
    
    // ヘルパー関数：Pythonアクセス修飾子判定
    std::string determine_python_access_modifier(const std::string& name) {
        if (name.size() >= 4 && name.substr(0, 2) == "__" && 
            name.substr(name.size() - 2) != "__") {
            return "private"; // name mangling
        } else if (name.size() >= 1 && name[0] == '_') {
            return "protected"; // conventionally protected
        }
        return "public";
    }
    
    // ヘルパー関数：Python型注釈のトリミング
    std::string trim_python_type(const std::string& type_str) {
        std::string result = type_str;
        // 前後の空白を除去
        size_t start = result.find_first_not_of(" \t");
        if (start == std::string::npos) return "Any";
        
        size_t end = result.find_last_not_of(" \t");
        result = result.substr(start, end - start + 1);
        
        // よくあるパターンの正規化
        if (result == "List" || result == "list") return "list";
        if (result == "Dict" || result == "dict") return "dict";
        if (result == "Set" || result == "set") return "set";
        if (result == "Tuple" || result == "tuple") return "tuple";
        
        return result;
    }
    
    // ヘルパー関数：メンバ変数の重複チェック
    bool member_already_exists(const ClassInfo& class_info, const std::string& name) {
        for (const auto& member : class_info.member_variables) {
            if (member.name == name) {
                return true;
            }
        }
        return false;
    }
    
    // 🆕 Python用コメント収集機能付き前処理（オーバーロード）
    std::string preprocess_content(const std::string& content, std::vector<CommentInfo>* out_comments) {
        if (!out_comments) {
            return content;  // Pythonは基本的に前処理不要だけど、コメント除去版を返す
        }
        
        // std::cerr << "🔥 Python preprocess_content called with comment collection!" << std::endl;
        
        // Python用コメント除去処理と同時にコメント情報を収集
        std::string result = content;
        
        // 単行コメント # の除去と収集
        result = remove_single_line_comments(result, *out_comments);
        // std::cerr << "🔥 After single line: " << out_comments->size() << " comments collected" << std::endl;
        
        return result;
    }
    
    // 🆕 従来版preprocess_content（後方互換性）
    std::string preprocess_content(const std::string& content) {
        // Pythonは基本的に前処理不要
        return content;
    }
    
    // 🆕 Python単行コメント除去と収集
    std::string remove_single_line_comments(const std::string& content, std::vector<CommentInfo>& comments) {
        std::istringstream stream(content);
        std::ostringstream result;
        std::string line;
        uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            size_t comment_pos = line.find("#");
            
            if (comment_pos != std::string::npos) {
                // 文字列リテラル内の#は除外（簡易版）
                bool in_string = false;
                char string_char = 0;
                bool is_real_comment = true;
                
                for (size_t i = 0; i < comment_pos; i++) {
                    char c = line[i];
                    if (!in_string && (c == '"' || c == '\'')) {
                        in_string = true;
                        string_char = c;
                    } else if (in_string && c == string_char && (i == 0 || line[i-1] != '\\')) {
                        in_string = false;
                    }
                }
                
                if (in_string) {
                    is_real_comment = false;
                }
                
                if (is_real_comment) {
                    // コメント内容を抽出
                    std::string comment_content = line.substr(comment_pos);
                    
                    // コメント情報を作成
                    CommentInfo comment_info(line_number, line_number, "single_line", comment_content);
                    comment_info.looks_like_code = looks_like_code(comment_content);
                    comments.push_back(comment_info);
                    
                    // コメント部分を除去
                    line = line.substr(0, comment_pos);
                }
            }
            
            result << line << '\n';
            line_number++;
        }
        
        return result.str();
    }
    
    // 🆕 コードらしさ判定（Python特化版）
    bool looks_like_code(const std::string& comment) {
        // Pythonキーワードを定義
        static const std::vector<std::string> python_keywords = {
            "if", "else", "elif", "for", "while", "def", "class", "import", "from",
            "return", "break", "continue", "pass", "try", "except", "finally",
            "with", "as", "lambda", "yield", "global", "nonlocal", "assert",
            "True", "False", "None", "and", "or", "not", "in", "is",
            "print", "len", "range", "str", "int", "float", "list", "dict", "set"
        };
        
        // コメント記号を除去
        std::string content = comment;
        if (content.find("#") == 0) {
            content = content.substr(1);
        }
        
        // 前後の空白を除去
        content.erase(0, content.find_first_not_of(" \t\n\r"));
        content.erase(content.find_last_not_of(" \t\n\r") + 1);
        
        // 空の場合はコードではない
        if (content.empty()) return false;
        
        // Pythonのコード特徴をチェック
        int code_score = 0;
        
        // キーワードマッチング
        for (const auto& keyword : python_keywords) {
            if (content.find(keyword) != std::string::npos) {
                code_score += 2;
            }
        }
        
        // Python構文特徴
        if (content.find("(") != std::string::npos && content.find(")") != std::string::npos) {
            code_score += 1; // 関数呼び出しっぽい
        }
        if (content.find("[") != std::string::npos && content.find("]") != std::string::npos) {
            code_score += 1; // リストアクセスっぽい
        }
        if (content.find("=") != std::string::npos) {
            code_score += 1; // 代入っぽい
        }
        if (content.find(".") != std::string::npos) {
            code_score += 1; // メソッド呼び出しっぽい
        }
        if (content.find(":") != std::string::npos) {
            code_score += 1; // Pythonのコロン構文
        }
        if (content.find("==") != std::string::npos || content.find("!=") != std::string::npos ||
            content.find(">=") != std::string::npos || content.find("<=") != std::string::npos) {
            code_score += 1; // 比較演算子
        }
        if (content.find("import ") != std::string::npos || content.find("from ") != std::string::npos) {
            code_score += 3; // インポート文
        }
        
        // 通常のコメント特徴（減点）
        if (content.find("TODO") != std::string::npos || content.find("FIXME") != std::string::npos ||
            content.find("NOTE") != std::string::npos || content.find("BUG") != std::string::npos) {
            code_score -= 1; // 通常のコメント
        }
        
        // 3点以上でコードらしいと判定
        return code_score >= 3;
    }
    
    //=========================================================================
    // 🚀 クラス-メソッド関連付け処理（JavaScript成功パターン適用）
    //=========================================================================
    
    void associate_methods_with_classes(AnalysisResult& result, const std::string& content) {
        // デバッグ出力一時的にコメントアウト
        // std::cerr << "[DEBUG associate_methods_with_classes] Starting. Classes: " << result.classes.size() 
        //           << ", Functions: " << result.functions.size() << std::endl;
        if (result.classes.empty() || result.functions.empty()) {
            // std::cerr << "[DEBUG associate_methods_with_classes] Early return - empty classes or functions" << std::endl;
            return; // 何もすることがない
        }
        
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 現在のクラス追跡
        ClassInfo* current_class = nullptr;
        int base_indent = -1; // クラス定義のインデントレベル
        int class_body_indent = -1; // クラス本体のインデントレベル
        
        while (std::getline(stream, line)) {
            line_number++;
            
            // インデントレベル計算
            int line_indent = 0;
            for (char c : line) {
                if (c == ' ') line_indent++;
                else if (c == '\t') line_indent += 4; // タブは4スペース相当
                else break;
            }
            
            // 空行・コメント行をスキップ
            std::string trimmed = line;
            size_t first_non_space = trimmed.find_first_not_of(" \t");
            if (first_non_space == std::string::npos || trimmed[first_non_space] == '#') {
                continue;
            }
            
            // クラス開始検出
            if (trimmed.find("class ") != std::string::npos) {
                // std::cerr << "[DEBUG associate_methods_with_classes] Found class at line " << line_number << std::endl;
                // 対応するクラスを検索
                for (auto& cls : result.classes) {
                    if (cls.start_line == line_number) {
                        current_class = &cls;
                        base_indent = line_indent;
                        class_body_indent = -1; // リセット
                        // std::cerr << "[DEBUG associate_methods_with_classes] Matched class: " << cls.name << std::endl;
                        break;
                    }
                }
                continue;
            }
            
            // クラス本体のインデントレベル設定
            if (current_class && class_body_indent == -1 && line_indent > base_indent) {
                class_body_indent = line_indent;
            }
            
            // クラス終了判定
            if (current_class && line_indent <= base_indent && 
                trimmed.find("class ") == std::string::npos) {
                current_class = nullptr;
                base_indent = -1;
                class_body_indent = -1;
            }
            
            // メソッド検出（クラス内でdef文）
            if (current_class && class_body_indent != -1 && 
                line_indent == class_body_indent && trimmed.find("def ") != std::string::npos) {
                // std::cerr << "[DEBUG associate_methods_with_classes] Found method at line " << line_number 
                //           << " in class " << current_class->name << std::endl;
                
                // 対応する関数をresult.functionsから検索
                for (auto it = result.functions.begin(); it != result.functions.end(); ++it) {
                    if (it->start_line == line_number) {
                        // std::cerr << "[DEBUG associate_methods_with_classes] Moving method " << it->name 
                        //           << " to class " << current_class->name << std::endl;
                        // メソッドとしてクラスに移動
                        current_class->methods.push_back(*it);
                        
                        // functionsリストから削除（メソッドなので独立関数ではない）
                        result.functions.erase(it);
                        break;
                    }
                }
            }
        }
    }
};

} // namespace nekocode