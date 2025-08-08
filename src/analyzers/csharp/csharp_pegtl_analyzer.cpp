//=============================================================================
// 🌟 C# PEGTL Analyzer Implementation - 革新的構文解析エンジン
//
// std::regexからの完全脱却
// PEGTLによる正確で高速なC#コード解析
//=============================================================================

#include "nekocode/analyzers/csharp_pegtl_analyzer.hpp"

namespace nekocode {

//=============================================================================
// 🎯 CSharpParseState 実装
//=============================================================================

CSharpParseState::CSharpParseState() {
    // 🚀 Phase 5: Universal Symbol初期化
    symbol_table = std::make_shared<SymbolTable>();
}

std::string CSharpParseState::generate_unique_id(const std::string& base) {
    id_counters[base]++;
    return base + "_" + std::to_string(id_counters[base] - 1);
}

void CSharpParseState::add_test_class_symbol(const std::string& class_name, std::uint32_t start_line) {
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("class_" + class_name);
    symbol.symbol_type = SymbolType::CLASS;
    symbol.name = class_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "csharp";
    
    std::cerr << "[Phase 5 Test] C# adding class symbol: " << class_name 
              << " with ID: " << symbol.symbol_id << std::endl;
    
    symbol_table->add_symbol(std::move(symbol));
}

void CSharpParseState::add_test_method_symbol(const std::string& method_name, std::uint32_t start_line) {
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("method_" + method_name);
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = method_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "csharp";
    
    std::cerr << "[Phase 5 Test] C# adding method symbol: " << method_name 
              << " with ID: " << symbol.symbol_id << std::endl;
    
    symbol_table->add_symbol(std::move(symbol));
}

void CSharpParseState::update_line(const char* from, const char* to) {
    while (from != to) {
        if (*from == '\n') current_line++;
        ++from;
    }
}

//=============================================================================
// 🎯 PEGTLアクション定義
//=============================================================================

namespace csharp_actions {

using namespace tao::pegtl;

// デフォルトアクション（何もしない）
template<typename Rule>
struct action : nothing<Rule> {};

// クラスヘッダーのアクション
template<>
struct action<csharp::minimal_grammar::class_header> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        ClassInfo class_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found class header: " << decl << std::endl;
        
        // "class"の後の識別子を抽出
        size_t class_pos = decl.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = decl.find_first_not_of(" \t", class_pos + 5);
            if (name_start != std::string::npos) {
                std::string class_name = decl.substr(name_start);
                // 空白で終わる場合は削除
                size_t name_end = class_name.find_first_of(" \t\n\r{");
                if (name_end != std::string::npos) {
                    class_name = class_name.substr(0, name_end);
                }
                class_info.name = class_name;
                class_info.start_line = state.current_line;
                state.current_classes.push_back(class_info);
                std::cerr << "DEBUG: Extracted class name: " << class_info.name << std::endl;
                
                // 🚀 Phase 5: Universal Symbol直接生成
                state.add_test_class_symbol(class_info.name, class_info.start_line);
            }
        }
    }
};

// 🚀 新文法対応: 通常メソッド検出
template<>
struct action<csharp::minimal_grammar::normal_method> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo method_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found normal method: " << decl << std::endl;
        
        // パラメータリストの前の識別子を探す（改良版）
        size_t paren_pos = decl.find('(');
        if (paren_pos != std::string::npos) {
            // 型名の後の識別子を探す（より精密）
            size_t name_end = paren_pos;
            while (name_end > 0 && std::isspace(decl[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(decl[name_start - 1]) || decl[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                method_info.name = decl.substr(name_start, name_end - name_start);
                method_info.start_line = state.current_line;
                state.current_methods.push_back(method_info);
                std::cerr << "DEBUG: Extracted normal method name: " << method_info.name << std::endl;
                
                // 🚀 Phase 5: Universal Symbol直接生成
                state.add_test_method_symbol(method_info.name, method_info.start_line);
            }
        }
    }
};

// 🚀 新文法対応: コンストラクタ検出
template<>
struct action<csharp::minimal_grammar::constructor> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo constructor_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found constructor: " << decl << std::endl;
        
        // コンストラクタ名を抽出（修飾子の後の最初の識別子）
        size_t paren_pos = decl.find('(');
        if (paren_pos != std::string::npos) {
            // 修飾子をスキップして識別子を探す
            std::string temp = decl.substr(0, paren_pos);
            
            // 右から左に最後の識別子を探す
            size_t name_end = temp.length();
            while (name_end > 0 && std::isspace(temp[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(temp[name_start - 1]) || temp[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                constructor_info.name = temp.substr(name_start, name_end - name_start) + "()"; // コンストラクタ明示
                constructor_info.start_line = state.current_line;
                state.current_methods.push_back(constructor_info);
                std::cerr << "DEBUG: Extracted constructor name: " << constructor_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: プロパティ（=>記法）検出
template<>
struct action<csharp::minimal_grammar::property_arrow> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo property_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found property (arrow): " << decl << std::endl;
        
        // =>の前の識別子を探す
        size_t arrow_pos = decl.find("=>");
        if (arrow_pos != std::string::npos) {
            std::string before_arrow = decl.substr(0, arrow_pos);
            
            // 型名の後の識別子を探す
            size_t name_end = before_arrow.length();
            while (name_end > 0 && std::isspace(before_arrow[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(before_arrow[name_start - 1]) || before_arrow[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                property_info.name = "property:" + before_arrow.substr(name_start, name_end - name_start);
                property_info.start_line = state.current_line;
                state.current_methods.push_back(property_info);
                std::cerr << "DEBUG: Extracted property (arrow) name: " << property_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: プロパティ（get/set記法）検出
template<>
struct action<csharp::minimal_grammar::property_getset> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo property_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found property (get/set): " << decl << std::endl;
        
        // {の前の識別子を探す
        size_t brace_pos = decl.find('{');
        if (brace_pos != std::string::npos) {
            std::string before_brace = decl.substr(0, brace_pos);
            
            // 型名の後の識別子を探す
            size_t name_end = before_brace.length();
            while (name_end > 0 && std::isspace(before_brace[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(before_brace[name_start - 1]) || before_brace[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                property_info.name = "property:" + before_brace.substr(name_start, name_end - name_start);
                property_info.start_line = state.current_line;
                state.current_methods.push_back(property_info);
                std::cerr << "DEBUG: Extracted property (get/set) name: " << property_info.name << std::endl;
            }
        }
    }
};

// 🔄 レガシー互換: 既存method_declも維持（後方互換性）
template<>
struct action<csharp::minimal_grammar::method_decl> {
    template<typename ParseInput>
    static void apply(const ParseInput& /*in*/, CSharpParseState& /*state*/) {
        // 新文法では個別のアクションが処理するため、ここは空でOK
        if (g_debug_mode) {
            std::cerr << "DEBUG: method_decl triggered (handled by specific actions)" << std::endl;
        }
    }
};

} // namespace csharp_actions

//=============================================================================
// 🚀 CSharpPEGTLAnalyzer 実装
//=============================================================================

CSharpPEGTLAnalyzer::CSharpPEGTLAnalyzer() {
    std::cerr << "DEBUG: CSharpPEGTLAnalyzer constructor called" << std::endl;
}

Language CSharpPEGTLAnalyzer::get_language() const {
    return Language::CSHARP;
}

std::string CSharpPEGTLAnalyzer::get_language_name() const {
    std::cerr << "DEBUG: CSharpPEGTLAnalyzer::get_language_name() called" << std::endl;
    return "C# (PEGTL)";
}

std::vector<std::string> CSharpPEGTLAnalyzer::get_supported_extensions() const {
    return {".cs", ".csx"};
}

AnalysisResult CSharpPEGTLAnalyzer::analyze(const std::string& content, const std::string& filename) {
    std::cerr << "DEBUG: CSharpPEGTLAnalyzer::analyze() called for " << filename << std::endl;
    
    // 🚀 デバッグファイル初期化（新しい解析開始）
    {
        std::ofstream debug_file("/tmp/csharp_regex_debug.txt", std::ios::trunc);  // trunc=上書き
        debug_file << "🚀 C# REGEX DEBUG SESSION STARTED 🚀\n";
        debug_file << "Analyzing file: " << filename << "\n";
        debug_file << "Content length: " << content.length() << " bytes\n";
    }
    
    CSharpParseState state;
    state.result.file_info.name = filename;
    state.result.file_info.size_bytes = content.size();
    state.result.language = Language::CSHARP;
    
    // 🎯 行数計算
    size_t total_lines = std::count(content.begin(), content.end(), '\n');
    if (!content.empty() && content.back() != '\n') {
        total_lines++;
    }
    state.result.file_info.total_lines = total_lines;
    
    // コード行数計算（空行とコメント行を除外）
    size_t code_lines = 0;
    std::istringstream line_stream(content);
    std::string line;
    while (std::getline(line_stream, line)) {
        // 空白を除去
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        // 空行やコメント行以外をカウント
        if (!line.empty() && line.find("//") != 0) {
            code_lines++;
        }
    }
    state.result.file_info.code_lines = code_lines;
    
    try {
        // PEGTL解析実行
        std::cerr << "DEBUG: Starting PEGTL parse for " << filename << std::endl;
        std::cerr << "DEBUG: Content length: " << content.length() << " bytes" << std::endl;
        tao::pegtl::string_input input(content, filename);
        bool parse_success = tao::pegtl::parse<csharp::minimal_grammar::csharp_minimal, csharp_actions::action>(input, state);
        std::cerr << "DEBUG: Parse result: " << (parse_success ? "SUCCESS" : "FAILED") << std::endl;
        
        // 結果を統合
        state.result.classes = std::move(state.current_classes);
        state.result.functions = std::move(state.current_methods);
        state.result.imports = std::move(state.imports);
        
    } catch (const tao::pegtl::parse_error& e) {
        // パースエラー処理（エラーログを出力して空の結果を返す）
        std::cerr << "PEGTL parse error: " << e.what() << std::endl;
        // 部分的な結果でも返す
    }
    
    // 複雑度計算（ハイブリッド戦略の前に実行）
    state.result.complexity = calculate_complexity(content);
    
    // 🚀 C#ハイブリッド戦略: JavaScript/TypeScript/C++成功パターン移植
    if (needs_csharp_line_based_fallback(state.result, content)) {
        apply_csharp_line_based_analysis(state.result, content, filename);
        std::cerr << "✅ C# Line-based analysis completed. Classes: " << state.result.classes.size() 
                  << ", Functions: " << state.result.functions.size() << std::endl;
        
        // 🚀 Phase 5: Line-basedフォールバック結果からUniversal Symbol生成
        if (!state.result.classes.empty() || !state.result.functions.empty()) {
            if (!state.symbol_table) {
                state.symbol_table = std::make_shared<SymbolTable>();
            }
            
            // クラスからUniversal Symbol生成
            for (const auto& class_info : state.result.classes) {
                state.add_test_class_symbol(class_info.name, class_info.start_line);
            }
            
            // 関数からUniversal Symbol生成  
            for (const auto& func_info : state.result.functions) {
                state.add_test_method_symbol(func_info.name, func_info.start_line);
            }
            
            std::cerr << "[Phase 5 Fallback] C# Line-based generated " 
                      << state.symbol_table->get_all_symbols().size() 
                      << " Universal Symbols" << std::endl;
        }
    } else {
        std::cerr << "⚠️  C# Hybrid Strategy NOT triggered" << std::endl;
    }
    
    // 統計情報更新
    state.result.update_statistics();
    
    // 🚀 Phase 5: Universal Symbol直接生成（CSharpParseStateから取得）
    if (state.symbol_table && state.symbol_table->get_all_symbols().size() > 0) {
        state.result.universal_symbols = state.symbol_table;
        std::cerr << "[Phase 5] C# analyzer generated " 
                  << state.symbol_table->get_all_symbols().size() 
                  << " Universal Symbols" << std::endl;
    }
    
    return state.result;
}

// プライベートメソッドの実装
bool CSharpPEGTLAnalyzer::needs_csharp_line_based_fallback(const AnalysisResult& result, const std::string& content) {
    // JavaScript戦略と同様: 複雑度 vs 検出数の妥当性検証
    uint32_t complexity = result.complexity.cyclomatic_complexity;
    size_t detected_classes = result.classes.size();
    size_t detected_functions = result.functions.size();
    
    bool has_class = content.find("class ") != std::string::npos;
    bool has_namespace = content.find("namespace ") != std::string::npos;
    bool has_interface = content.find("interface ") != std::string::npos;
    
    // C#特化閾値: C#は規則正しいので、C++より厳しい閾値
    if (complexity > 30 && detected_classes == 0 && detected_functions < 3) {
        std::cerr << "📊 Trigger reason: High complexity with no detection (C# specific)" << std::endl;
        return true;
    }
    
    // 複雑度100以上で関数検出0は絶対におかしい
    if (complexity > 100 && detected_functions == 0) {
        std::cerr << "📊 Trigger reason: Very high complexity with no functions" << std::endl;
        return true;
    }
    
    // C#特有パターンがあるのに検出できていない場合
    if ((has_class || has_namespace || has_interface) && detected_classes == 0) {
        std::cerr << "📊 Trigger reason: C# patterns found but no classes detected" << std::endl;
        return true;
    }
    
    std::cerr << "❌ No trigger conditions met" << std::endl;
    return false;
}

void CSharpPEGTLAnalyzer::apply_csharp_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename) {
    // 🎯 end_line計算用に全行を保存
    std::vector<std::string> all_lines;
    std::istringstream prestream(content);
    std::string preline;
    while (std::getline(prestream, preline)) {
        all_lines.push_back(preline);
    }
    
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // 既存の要素名を記録（重複検出を防ぐ - JavaScript成功パターン）
    std::set<std::string> existing_classes;
    std::set<std::string> existing_functions;
    
    for (const auto& cls : result.classes) {
        existing_classes.insert(cls.name);
    }
    for (const auto& func : result.functions) {
        existing_functions.insert(func.name);
    }
    
    // C#特化の行ベース解析
    while (std::getline(stream, line)) {
        extract_csharp_elements_from_line(line, line_number, result, existing_classes, existing_functions, all_lines);
        line_number++;
    }
}

void CSharpPEGTLAnalyzer::extract_csharp_elements_from_line(const std::string& line, size_t line_number,
                                       AnalysisResult& result, 
                                       std::set<std::string>& existing_classes,
                                       std::set<std::string>& existing_functions,
                                       const std::vector<std::string>& all_lines) {
    
    // 🚀 デバッグファイル出力（詳細マッチング調査用）
    static std::ofstream debug_file("/tmp/csharp_regex_debug.txt", std::ios::app);
    debug_file << "\n=== LINE " << line_number << " ===\n";
    debug_file << "Content: [" << line << "]\n";
    
    // パターン1: public class ClassName
    std::regex class_pattern(R"(^\s*(?:public|internal|private|protected)?\s*(?:static|sealed|abstract)?\s*class\s+(\w+))");
    std::smatch match;
    
    debug_file << "Testing class_pattern... ";
    if (std::regex_search(line, match, class_pattern)) {
        std::string class_name = match[1].str();
        debug_file << "MATCHED! class_name=[" << class_name << "]\n";
        if (existing_classes.find(class_name) == existing_classes.end()) {
            ClassInfo class_info;
            class_info.name = class_name;
            class_info.start_line = line_number;
            result.classes.push_back(class_info);
            existing_classes.insert(class_name);
            debug_file << "Added new class: " << class_name << "\n";
        } else {
            debug_file << "Class already exists, skipped\n";
        }
    } else {
        debug_file << "NO MATCH\n";
    }
    
    // デバッグファイルをflush（即座に書き込み）
    debug_file.flush();
}

uint32_t CSharpPEGTLAnalyzer::find_function_end_line(const std::vector<std::string>& lines, size_t start_line) {
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

// calculate_complexityメソッドの実装（BaseAnalyzerからオーバーライド）
ComplexityInfo CSharpPEGTLAnalyzer::calculate_complexity(const std::string& content) {
    ComplexityInfo complexity = BaseAnalyzer::calculate_complexity(content);
    
    // C#固有のキーワード追加
    std::vector<std::string> csharp_keywords = {
        "async", "await", "yield", "lock", "using", "foreach", "?.", "??", "?["
    };
    
    for (const auto& keyword : csharp_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    complexity.update_rating();
    return complexity;
}

} // namespace nekocode
