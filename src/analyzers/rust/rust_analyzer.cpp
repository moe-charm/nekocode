//=============================================================================
// 🦀 Rust Language Analyzer Implementation
//=============================================================================

#include "nekocode/analyzers/rust_analyzer.hpp"
#include "nekocode/debug_logger.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🦀 メイン解析処理
//=============================================================================

AnalysisResult RustAnalyzer::analyze(const std::string& content, const std::string& filename) {
    using namespace nekocode::debug;
    NEKOCODE_PERF_TIMER("RustAnalyzer::analyze " + filename);
    
    NEKOCODE_LOG_INFO("RustAnalyzer", "Starting analysis of " + filename + " (" + std::to_string(content.size()) + " bytes)");
    
    AnalysisResult result;
    AnalysisStats stats;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::RUST;
    
    // 内部バッファクリア
    rust_functions_.clear();
    structs_.clear();
    enums_.clear();
    traits_.clear();
    impls_.clear();
    macros_.clear();
    
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Internal buffers cleared, starting element analysis");
    
    // 各要素を解析
    NEKOCODE_PERF_CHECKPOINT("functions");
    analyze_functions(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(rust_functions_.size()) + " functions");
    
    NEKOCODE_PERF_CHECKPOINT("structs");
    analyze_structs(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(structs_.size()) + " structs");
    
    NEKOCODE_PERF_CHECKPOINT("enums");
    analyze_enums(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(enums_.size()) + " enums");
    
    NEKOCODE_PERF_CHECKPOINT("traits");
    analyze_traits(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(traits_.size()) + " traits");
    
    NEKOCODE_PERF_CHECKPOINT("impls");
    analyze_impls(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(impls_.size()) + " impl blocks");
    
    NEKOCODE_PERF_CHECKPOINT("macros");
    analyze_macros(content);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(macros_.size()) + " macros");
    
    NEKOCODE_PERF_CHECKPOINT("modules");
    analyze_modules(content, result);
    
    NEKOCODE_PERF_CHECKPOINT("use_statements");
    analyze_use_statements(content, result);
    NEKOCODE_LOG_TRACE("RustAnalyzer", "Found " + std::to_string(result.imports.size()) + " imports");
    
    // 🔥 重要：RustFunctionInfoをAnalysisResult.functionsに変換！
    for (const auto& rust_func : rust_functions_) {
        FunctionInfo func_info;
        func_info.name = rust_func.name;
        func_info.start_line = rust_func.line_number;
        func_info.end_line = rust_func.end_line;      // 🎯 end_line設定追加
        func_info.complexity = rust_func.complexity;  // 🔧 複雑度を設定！
        
        // Rust特有の情報をメタデータに保存
        if (rust_func.is_async) {
            func_info.metadata["is_async"] = "true";
        }
        if (rust_func.is_unsafe) {
            func_info.metadata["is_unsafe"] = "true";
        }
        if (rust_func.is_pub) {
            func_info.metadata["is_pub"] = "true";
        }
        if (!rust_func.return_type.empty()) {
            func_info.metadata["return_type"] = rust_func.return_type;
        }
        
        result.functions.push_back(func_info);
        
        // 🚀 Phase 5: Universal Symbol直接生成
        add_test_function_symbol(rust_func.name, rust_func.line_number);
    }
    
    // 構造体をクラスとして扱う
    for (const auto& rust_struct : structs_) {
        ClassInfo class_info;
        class_info.name = rust_struct.name;
        class_info.start_line = rust_struct.line_number;
        // 🔥 end_line計算を追加（MoveClass対応）
        class_info.end_line = find_struct_end_line(content, rust_struct.line_number);
        if (rust_struct.is_pub) {
            class_info.metadata["is_pub"] = "true";
        }
        result.classes.push_back(class_info);
        
        // 🚀 Phase 5: Universal Symbol直接生成
        add_test_struct_symbol(rust_struct.name, rust_struct.line_number);
    }
    
    // 列挙型もクラスとして扱う
    for (const auto& rust_enum : enums_) {
        ClassInfo class_info;
        class_info.name = rust_enum.name;
        class_info.start_line = rust_enum.line_number;
        // 🔥 end_line計算を追加（MoveClass対応）
        class_info.end_line = find_struct_end_line(content, rust_enum.line_number);
        class_info.metadata["type"] = "enum";
        result.classes.push_back(class_info);
        
        // 🚀 Phase 5: Universal Symbol直接生成
        add_test_enum_symbol(rust_enum.name, rust_enum.line_number);
    }
    
    // 🎯 メンバ変数検出（新機能）
    NEKOCODE_PERF_CHECKPOINT("member_variables");
    detect_member_variables(result, content);
    // メンバ変数検出のログは detect_member_variables 内で出力

    // 🔧 基本統計情報を更新
    result.stats.function_count = result.functions.size();
    result.stats.class_count = result.classes.size();
    result.stats.import_count = result.imports.size();
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Updated stats - functions: " + std::to_string(result.stats.function_count) + 
                      ", classes: " + std::to_string(result.stats.class_count) + 
                      ", imports: " + std::to_string(result.stats.import_count));

    // 複雑度計算
    NEKOCODE_PERF_CHECKPOINT("complexity");
    result.complexity = calculate_rust_complexity(content);
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Calculated complexity: " + std::to_string(result.complexity.cyclomatic_complexity));
    
    // Rust特有の統計情報をメタデータに追加
    nlohmann::json rust_specific;
    rust_specific["trait_count"] = traits_.size();
    rust_specific["impl_count"] = impls_.size();
    rust_specific["macro_count"] = macros_.size();
    
    int unsafe_count = 0;
    int async_count = 0;
    for (const auto& func : rust_functions_) {
        if (func.is_unsafe) unsafe_count++;
        if (func.is_async) async_count++;
    }
    rust_specific["unsafe_function_count"] = unsafe_count;
    rust_specific["async_function_count"] = async_count;
    
    result.metadata["rust_specific"] = rust_specific.dump();
    
    // 行数カウント
    std::istringstream stream(content);
    std::string line;
    size_t total_lines = 0;
    size_t code_lines = 0;
    size_t comment_lines = 0;
    size_t empty_lines = 0;
    
    while (std::getline(stream, line)) {
        total_lines++;
        
        // 前後の空白を削除
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) {
            empty_lines++;
        } else if (line.find("//") == 0 || line.find("/*") == 0) {
            comment_lines++;
        } else {
            code_lines++;
        }
    }
    
    result.file_info.total_lines = total_lines;
    result.file_info.code_lines = code_lines;
    result.file_info.comment_lines = comment_lines;
    result.file_info.empty_lines = empty_lines;
    
    // 🆕 Phase 1: impl分類修正処理を追加
    NEKOCODE_PERF_CHECKPOINT("impl_classification");
    fix_impl_method_classification(result);
    
    // 🔥 重要：統計情報を更新！
    NEKOCODE_PERF_CHECKPOINT("statistics");
    result.update_statistics();
    
    // 🚀 Phase 5: Universal Symbol結果設定
    if (symbol_table_ && symbol_table_->get_all_symbols().size() > 0) {
        result.universal_symbols = symbol_table_;
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5] Rust analyzer generated " 
                  << symbol_table_->get_all_symbols().size() 
                  << " Universal Symbols" << std::endl;
#endif
    }
    
    // 統計情報をログに出力
    stats.total_lines = result.file_info.total_lines;
    stats.code_lines = result.file_info.code_lines;
    stats.functions_found = result.functions.size();
    stats.classes_found = result.classes.size();
    stats.imports_found = result.imports.size();
    stats.complexity_score = result.complexity.cyclomatic_complexity;
    stats.log_summary("Rust", filename);
    
    NEKOCODE_LOG_INFO("RustAnalyzer", "Analysis completed successfully for " + filename);
    
    return result;
}

//=============================================================================
// 🦀 関数解析
//=============================================================================

void RustAnalyzer::analyze_functions(const std::string& content) {
    using namespace nekocode::debug;
    NEKOCODE_PERF_TIMER("RustAnalyzer::analyze_functions");
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Starting function analysis");
    
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
    
    // 関数定義パターン: pub async unsafe const fn name<T>() -> Type
    std::regex fn_pattern(R"(^\s*(pub(?:\([^)]+\))?\s+)?(async\s+)?(unsafe\s+)?(const\s+)?fn\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, fn_pattern)) {
            RustFunctionInfo func_info;
            func_info.name = match[5].str();
            func_info.line_number = line_number;
            func_info.is_pub = !match[1].str().empty();
            func_info.is_async = !match[2].str().empty();
            func_info.is_unsafe = !match[3].str().empty();
            func_info.is_const = !match[4].str().empty();
            
            // ジェネリクスとライフタイムを抽出
            size_t name_end = line.find(func_info.name) + func_info.name.length();
            std::string generics_str = extract_generics(line, name_end);
            if (!generics_str.empty()) {
                // extract_lifetimesを使ってライフタイムと通常のジェネリクスを分離
                func_info.lifetimes = extract_lifetimes(generics_str);
                
                // ジェネリクスから個々の型パラメータを抽出
                size_t start = generics_str.find('<');
                size_t end = generics_str.find('>');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string inner = generics_str.substr(start + 1, end - start - 1);
                    std::istringstream stream(inner);
                    std::string item;
                    while (std::getline(stream, item, ',')) {
                        // 前後の空白を削除
                        item.erase(0, item.find_first_not_of(" \t"));
                        item.erase(item.find_last_not_of(" \t") + 1);
                        // ライフタイムでなければジェネリクスに追加
                        if (!item.empty() && item[0] != '\'') {
                            func_info.generics.push_back(item);
                        }
                    }
                }
            }
            
            // 戻り値型を抽出
            func_info.return_type = extract_return_type(line, name_end);
            
            // 🎯 end_line計算
            func_info.end_line = find_function_end_line(all_lines, func_info.line_number - 1);
            
            // 🔧 個別関数の複雑度計算
            std::string function_body = extract_function_body(content, func_info.line_number);
            if (!function_body.empty()) {
                func_info.complexity = calculate_function_complexity(function_body);
                NEKOCODE_LOG_TRACE("RustAnalyzer", "Calculated complexity for " + func_info.name + 
                                  ": " + std::to_string(func_info.complexity.cyclomatic_complexity));
            }
            
            rust_functions_.push_back(func_info);
            NEKOCODE_LOG_TRACE("RustAnalyzer", "Found function: " + func_info.name + 
                              (func_info.is_async ? " (async)" : "") + 
                              (func_info.is_unsafe ? " (unsafe)" : "") + 
                              (func_info.is_pub ? " (pub)" : "") +
                              " at line " + std::to_string(func_info.line_number) + 
                              " (complexity: " + std::to_string(func_info.complexity.cyclomatic_complexity) + ")");
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 構造体解析
//=============================================================================

void RustAnalyzer::analyze_structs(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // 構造体定義パターン: pub struct Name<T>
    std::regex struct_pattern(R"(^\s*(pub(?:\([^)]+\))?\s+)?struct\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, struct_pattern)) {
            StructInfo struct_info;
            struct_info.name = match[2].str();
            struct_info.line_number = line_number;
            struct_info.is_pub = !match[1].str().empty();
            
            // ジェネリクスを抽出
            size_t name_end = line.find(struct_info.name) + struct_info.name.length();
            std::string generics_str = extract_generics(line, name_end);
            // ジェネリクス文字列を個々の要素に分解
            if (!generics_str.empty()) {
                size_t start = generics_str.find('<');
                size_t end = generics_str.find('>');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string inner = generics_str.substr(start + 1, end - start - 1);
                    std::istringstream stream(inner);
                    std::string item;
                    while (std::getline(stream, item, ',')) {
                        item.erase(0, item.find_first_not_of(" \t"));
                        item.erase(item.find_last_not_of(" \t") + 1);
                        if (!item.empty()) {
                            struct_info.generics.push_back(item);
                        }
                    }
                }
            }
            
            structs_.push_back(struct_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 列挙型解析
//=============================================================================

void RustAnalyzer::analyze_enums(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // 列挙型定義パターン: pub enum Name<T>
    std::regex enum_pattern(R"(^\s*(pub(?:\([^)]+\))?\s+)?enum\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, enum_pattern)) {
            EnumInfo enum_info;
            enum_info.name = match[2].str();
            enum_info.line_number = line_number;
            enum_info.is_pub = !match[1].str().empty();
            
            // ジェネリクスを抽出
            size_t name_end = line.find(enum_info.name) + enum_info.name.length();
            std::string generics_str = extract_generics(line, name_end);
            // ジェネリクス文字列を個々の要素に分解
            if (!generics_str.empty()) {
                size_t start = generics_str.find('<');
                size_t end = generics_str.find('>');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string inner = generics_str.substr(start + 1, end - start - 1);
                    std::istringstream stream(inner);
                    std::string item;
                    while (std::getline(stream, item, ',')) {
                        item.erase(0, item.find_first_not_of(" \t"));
                        item.erase(item.find_last_not_of(" \t") + 1);
                        if (!item.empty()) {
                            enum_info.generics.push_back(item);
                        }
                    }
                }
            }
            
            enums_.push_back(enum_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 トレイト解析
//=============================================================================

void RustAnalyzer::analyze_traits(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // トレイト定義パターン: pub trait Name<T>
    std::regex trait_pattern(R"(^\s*(pub(?:\([^)]+\))?\s+)?trait\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, trait_pattern)) {
            TraitInfo trait_info;
            trait_info.name = match[2].str();
            trait_info.line_number = line_number;
            trait_info.is_pub = !match[1].str().empty();
            
            // ジェネリクスを抽出
            size_t name_end = line.find(trait_info.name) + trait_info.name.length();
            std::string generics_str = extract_generics(line, name_end);
            // ジェネリクス文字列を個々の要素に分解
            if (!generics_str.empty()) {
                size_t start = generics_str.find('<');
                size_t end = generics_str.find('>');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string inner = generics_str.substr(start + 1, end - start - 1);
                    std::istringstream stream(inner);
                    std::string item;
                    while (std::getline(stream, item, ',')) {
                        item.erase(0, item.find_first_not_of(" \t"));
                        item.erase(item.find_last_not_of(" \t") + 1);
                        if (!item.empty()) {
                            trait_info.generics.push_back(item);
                        }
                    }
                }
            }
            
            traits_.push_back(trait_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 impl解析
//=============================================================================

void RustAnalyzer::analyze_impls(const std::string& content) {
    using namespace nekocode::debug;
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Starting impl block analysis with method detection");
    
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // implパターン: impl<T> Trait for Struct または impl<T> Struct
    std::regex impl_pattern(R"(^\s*impl(?:<[^>]+>)?\s+(?:(\w+)\s+for\s+)?(\w+))");
    
    // 関数定義パターン（impl内メソッド用）
    std::regex fn_pattern(R"(^\s*(pub(?:\([^)]+\))?\s+)?(async\s+)?(unsafe\s+)?(const\s+)?fn\s+(\w+))");
    
    ImplInfo* current_impl = nullptr;
    int brace_level = 0;
    bool in_impl_block = false;
    
    while (std::getline(stream, line)) {
        // impl開始検出
        std::smatch impl_match;
        if (std::regex_search(line, impl_match, impl_pattern)) {
            ImplInfo impl_info;
            
            if (!impl_match[1].str().empty()) {
                // impl Trait for Struct パターン
                impl_info.trait_name = impl_match[1].str();
                impl_info.struct_name = impl_match[2].str();
            } else {
                // impl Struct パターン
                impl_info.struct_name = impl_match[2].str();
            }
            
            impl_info.line_number = line_number;
            impls_.push_back(impl_info);
            current_impl = &impls_.back();
            brace_level = 0;
            in_impl_block = true;
            
            NEKOCODE_LOG_TRACE("RustAnalyzer", "Found impl block for " + impl_info.struct_name + 
                              (impl_info.trait_name.empty() ? " (inherent)" : " (trait: " + impl_info.trait_name + ")") +
                              " at line " + std::to_string(line_number));
        }
        
        // ブレースレベル追跡
        for (char c : line) {
            if (c == '{') brace_level++;
            else if (c == '}') brace_level--;
        }
        
        // impl内関数検出
        if (in_impl_block && current_impl && brace_level > 0) {
            std::smatch fn_match;
            if (std::regex_search(line, fn_match, fn_pattern)) {
                std::string method_name = fn_match[5].str();
                current_impl->methods.push_back(method_name);
                
                NEKOCODE_LOG_TRACE("RustAnalyzer", "Found method '" + method_name + 
                                  "' in impl " + current_impl->struct_name + 
                                  " at line " + std::to_string(line_number));
            }
        }
        
        // impl終了検出
        if (in_impl_block && brace_level <= 0 && line.find('}') != std::string::npos) {
            in_impl_block = false;
            if (current_impl) {
                NEKOCODE_LOG_DEBUG("RustAnalyzer", "Completed impl block for " + current_impl->struct_name + 
                                  " with " + std::to_string(current_impl->methods.size()) + " methods");
            }
            current_impl = nullptr;
        }
        
        line_number++;
    }
    
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Impl analysis completed - found " + 
                      std::to_string(impls_.size()) + " impl blocks");
}

//=============================================================================
// 🦀 マクロ解析
//=============================================================================

void RustAnalyzer::analyze_macros(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // macro_rules! パターン
    std::regex macro_pattern(R"(^\s*macro_rules!\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, macro_pattern)) {
            MacroInfo macro_info;
            macro_info.name = match[1].str();
            macro_info.line_number = line_number;
            macro_info.is_declarative = true;
            
            macros_.push_back(macro_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 モジュール解析
//=============================================================================

void RustAnalyzer::analyze_modules(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // mod パターン: pub mod name
    std::regex mod_pattern(R"(^\s*(pub\s+)?mod\s+(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, mod_pattern)) {
            // モジュールを特殊なインポートとして扱う
            ImportInfo import_info;
            import_info.type = ImportType::ES6_IMPORT;  // 便宜上
            import_info.module_path = "mod::" + match[2].str();
            import_info.line_number = line_number;
            import_info.metadata["type"] = "module";
            if (!match[1].str().empty()) {
                import_info.metadata["is_pub"] = "true";
            }
            
            result.imports.push_back(import_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 use文解析
//=============================================================================

void RustAnalyzer::analyze_use_statements(const std::string& content, AnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // use パターン: use path::to::item;
    std::regex use_pattern(R"(^\s*use\s+([^;]+);)");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, use_pattern)) {
            ImportInfo import_info;
            import_info.type = ImportType::ES6_IMPORT;  // 便宜上
            import_info.module_path = match[1].str();
            import_info.line_number = line_number;
            
            // パスから最後の要素を抽出
            std::string path = match[1].str();
            size_t last_colon = path.rfind("::");
            if (last_colon != std::string::npos) {
                std::string item = path.substr(last_colon + 2);
                import_info.imported_names.push_back(item);
            }
            
            result.imports.push_back(import_info);
        }
        
        line_number++;
    }
}

//=============================================================================
// 🦀 複雑度計算
//=============================================================================

ComplexityInfo RustAnalyzer::calculate_rust_complexity(const std::string& content) {
    using namespace nekocode::debug;
    NEKOCODE_PERF_TIMER("RustAnalyzer::calculate_rust_complexity");
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Starting complexity calculation");
    
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1;
    
    // Rust固有の複雑度キーワード
    std::vector<std::string> complexity_keywords = {
        "if ", "else if", "else ", "match ", "for ", "while ", "loop ",
        "?", "unwrap(", "expect(", "panic!", "unreachable!",
        ".and_then(", ".or_else(", ".map(", ".filter("
    };
    
    for (const auto& keyword : complexity_keywords) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // match armの数も複雑度に加算
    std::regex match_arm_pattern(R"(=>\s*\{)");
    std::smatch match;
    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, match_arm_pattern)) {
        complexity.cyclomatic_complexity++;
        searchStart = match.suffix().first;
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
    
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Complexity calculation completed: " + 
                      std::to_string(complexity.cyclomatic_complexity) + 
                      " (" + complexity.to_string() + ")");
    
    return complexity;
}

//=============================================================================
// 🦀 ヘルパー関数
//=============================================================================

std::string RustAnalyzer::extract_generics(const std::string& line, size_t start_pos) {
    std::vector<std::string> generics;
    
    // <T, U> パターンを探す
    size_t open_bracket = line.find('<', start_pos);
    if (open_bracket == std::string::npos) {
        return "";
    }
    
    size_t close_bracket = line.find('>', open_bracket);
    if (close_bracket == std::string::npos) {
        return "";
    }
    
    std::string generic_str = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    
    // カンマで分割
    std::istringstream stream(generic_str);
    std::string item;
    
    while (std::getline(stream, item, ',')) {
        // 前後の空白を削除
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        
        if (!item.empty()) {
            generics.push_back(item);
        }
    }
    
    // ジェネリクスを結合して返す
    std::string result;
    for (size_t i = 0; i < generics.size(); ++i) {
        if (i > 0) result += ", ";
        result += generics[i];
    }
    
    return result.empty() ? "" : "<" + result + ">";
}

std::string RustAnalyzer::extract_return_type(const std::string& line, size_t fn_pos) {
    // -> Type パターンを探す
    size_t arrow_pos = line.find("->", fn_pos);
    if (arrow_pos == std::string::npos) {
        return "";
    }
    
    size_t type_start = arrow_pos + 2;
    while (type_start < line.length() && std::isspace(line[type_start])) {
        type_start++;
    }
    
    size_t type_end = type_start;
    while (type_end < line.length() && line[type_end] != '{' && line[type_end] != ';') {
        type_end++;
    }
    
    if (type_end > type_start) {
        std::string return_type = line.substr(type_start, type_end - type_start);
        // 前後の空白を削除
        return_type.erase(0, return_type.find_first_not_of(" \t"));
        return_type.erase(return_type.find_last_not_of(" \t") + 1);
        return return_type;
    }
    
    return "";
}

std::vector<std::string> RustAnalyzer::extract_lifetimes(const std::string& generics) {
    std::vector<std::string> lifetimes;
    
    // 'a, 'b パターンを探す
    std::regex lifetime_pattern(R"('(\w+))");
    std::smatch match;
    std::string::const_iterator searchStart(generics.cbegin());
    
    while (std::regex_search(searchStart, generics.cend(), match, lifetime_pattern)) {
        lifetimes.push_back(match[0].str());
        searchStart = match.suffix().first;
    }
    
    return lifetimes;
}

//=============================================================================
// 🎯 メンバ変数検出（新機能）
//=============================================================================

void RustAnalyzer::detect_member_variables(AnalysisResult& result, const std::string& content) {
    using namespace nekocode::debug;
    NEKOCODE_PERF_TIMER("RustAnalyzer::detect_member_variables");
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Starting member variable detection");
    
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // 構造体内のフィールド検出
    std::regex struct_field_pattern(R"(^\s*(?:(pub)(?:\([^)]*\))?\s+)?([a-zA-Z_]\w*)\s*:\s*([^,{}]+)(?:,|$))");
    
    // 列挙型のバリアント内フィールド検出
    std::regex enum_field_pattern(R"(^\s*([A-Z]\w*)\s*\{\s*([^}]+)\s*\})");
    
    // impl内での self.field パターン（参考用）
    std::regex self_field_pattern(R"(self\.([a-zA-Z_]\w*))");
    
    bool in_struct = false;
    bool in_enum = false;
    std::string current_struct_name;
    std::string current_enum_name;
    int brace_level = 0;
    
    while (std::getline(stream, line)) {
        // 構造体の開始検出
        std::smatch struct_match;
        std::regex struct_start_pattern(R"(^\s*(?:pub\s+)?struct\s+([a-zA-Z_]\w*))");
        if (std::regex_search(line, struct_match, struct_start_pattern)) {
            in_struct = true;
            current_struct_name = struct_match[1].str();
            brace_level = 0;
            NEKOCODE_LOG_TRACE("RustAnalyzer", "Found struct: " + current_struct_name + " at line " + std::to_string(line_number));
        }
        
        // 列挙型の開始検出
        std::smatch enum_match;
        std::regex enum_start_pattern(R"(^\s*(?:pub\s+)?enum\s+([a-zA-Z_]\w*))");
        if (std::regex_search(line, enum_match, enum_start_pattern)) {
            in_enum = true;
            current_enum_name = enum_match[1].str();
            brace_level = 0;
            NEKOCODE_LOG_TRACE("RustAnalyzer", "Found enum: " + current_enum_name + " at line " + std::to_string(line_number));
        }
        
        // ブレースレベル追跡
        for (char c : line) {
            if (c == '{') brace_level++;
            else if (c == '}') brace_level--;
        }
        
        // 構造体終了検出
        if (in_struct && brace_level <= 0 && line.find('}') != std::string::npos) {
            in_struct = false;
            current_struct_name.clear();
        }
        
        // 列挙型終了検出
        if (in_enum && brace_level <= 0 && line.find('}') != std::string::npos) {
            in_enum = false;
            current_enum_name.clear();
        }
        
        // 構造体フィールド検出
        if (in_struct && brace_level > 0) {
            std::smatch field_match;
            if (std::regex_search(line, field_match, struct_field_pattern)) {
                MemberVariable member_var;
                member_var.name = field_match[2].str();
                member_var.type = field_match[3].str();
                // current_struct_name は後で使用
                member_var.declaration_line = line_number;
                
                // アクセス修飾子
                if (!field_match[1].str().empty()) {
                    member_var.access_modifier = "pub";
                } else {
                    member_var.access_modifier = "private";  // Rustのデフォルト
                }
                
                // 型を整理
                member_var.type.erase(0, member_var.type.find_first_not_of(" \t"));
                member_var.type.erase(member_var.type.find_last_not_of(" \t,") + 1);
                
                // 対応するClassInfoを検索して追加
                for (auto& class_info : result.classes) {
                    if (class_info.name == current_struct_name) {
                        class_info.member_variables.push_back(member_var);
                        break;
                    }
                }
                NEKOCODE_LOG_TRACE("RustAnalyzer", "Found struct field: " + 
                                  member_var.access_modifier + " " + member_var.name + 
                                  ": " + member_var.type + " in " + current_struct_name);
            }
        }
        
        // 列挙型バリアント内フィールド検出
        if (in_enum && brace_level > 0) {
            std::smatch variant_match;
            if (std::regex_search(line, variant_match, enum_field_pattern)) {
                std::string variant_name = variant_match[1].str();
                std::string fields_str = variant_match[2].str();
                
                // フィールドを個別に解析
                std::istringstream fields_stream(fields_str);
                std::string field;
                while (std::getline(fields_stream, field, ',')) {
                    field.erase(0, field.find_first_not_of(" \t"));
                    field.erase(field.find_last_not_of(" \t") + 1);
                    
                    size_t colon_pos = field.find(':');
                    if (colon_pos != std::string::npos) {
                        MemberVariable member_var;
                        member_var.name = field.substr(0, colon_pos);
                        member_var.type = field.substr(colon_pos + 1);
                        // current_enum_name + variant_name は後で使用
                        member_var.declaration_line = line_number;
                        member_var.access_modifier = "pub";  // enum variant fields are pub
                        
                        // 型を整理
                        member_var.name.erase(0, member_var.name.find_first_not_of(" \t"));
                        member_var.name.erase(member_var.name.find_last_not_of(" \t") + 1);
                        member_var.type.erase(0, member_var.type.find_first_not_of(" \t"));
                        member_var.type.erase(member_var.type.find_last_not_of(" \t") + 1);
                        
                        // 対応するClassInfoを検索して追加
                        std::string full_enum_name = current_enum_name + "::" + variant_name;
                        for (auto& class_info : result.classes) {
                            if (class_info.name == current_enum_name) {
                                class_info.member_variables.push_back(member_var);
                                break;
                            }
                        }
                        NEKOCODE_LOG_TRACE("RustAnalyzer", "Found enum field: " + 
                                          member_var.name + ": " + member_var.type + 
                                          " in " + current_enum_name + "::" + variant_name);
                    }
                }
            }
        }
        
        line_number++;
    }
    
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Member variable detection completed");
}

//=============================================================================
// 🔧 個別関数複雑度計算（新実装）
//=============================================================================

ComplexityInfo RustAnalyzer::calculate_function_complexity(const std::string& function_body) {
    using namespace nekocode::debug;
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // 基本複雑度
    
    // Rust固有の複雑度キーワード
    std::vector<std::string> complexity_keywords = {
        "if ", "else if", "else ", "match ", "for ", "while ", "loop ",
        "?", ".unwrap(", ".expect(", ".and_then(", ".or_else(", 
        ".map(", ".filter(", "panic!", "unreachable!"
    };
    
    for (const auto& keyword : complexity_keywords) {
        size_t pos = 0;
        while ((pos = function_body.find(keyword, pos)) != std::string::npos) {
            complexity.cyclomatic_complexity++;
            pos += keyword.length();
        }
    }
    
    // match腕の数をカウント
    std::regex match_arm_pattern(R"(\s*[^=>\s]+\s*=>)");
    std::smatch match;
    std::string::const_iterator searchStart(function_body.cbegin());
    while (std::regex_search(searchStart, function_body.cend(), match, match_arm_pattern)) {
        complexity.cyclomatic_complexity++;
        searchStart = match.suffix().first;
    }
    
    // ネスト深度計算
    complexity.max_nesting_depth = 0;
    uint32_t current_depth = 0;
    
    for (char c : function_body) {
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

std::string RustAnalyzer::extract_function_body(const std::string& content, size_t fn_start_line) {
    std::istringstream stream(content);
    std::string line;
    size_t current_line = 1;
    
    // 開始行まで移動
    while (std::getline(stream, line) && current_line < fn_start_line) {
        current_line++;
    }
    
    if (current_line != fn_start_line) {
        return ""; // 関数が見つからない
    }
    
    std::string function_body;
    int brace_count = 0;
    bool found_opening_brace = false;
    
    // 関数定義行から開始
    do {
        // 中括弧のカウント
        for (char c : line) {
            if (c == '{') {
                brace_count++;
                found_opening_brace = true;
            } else if (c == '}' && found_opening_brace) {
                brace_count--;
            }
        }
        
        function_body += line + "\n";
        
        // 関数の終わりに到達
        if (found_opening_brace && brace_count == 0) {
            break;
        }
    } while (std::getline(stream, line));
    
    return function_body;
}

// 🎯 関数のend_lineを検出（Rust用ブレースマッチング）
LineNumber RustAnalyzer::find_function_end_line(const std::vector<std::string>& lines, size_t start_line) {
    if (start_line >= lines.size()) {
        return static_cast<LineNumber>(start_line + 1);
    }
    
    int brace_count = 0;
    bool in_function = false;
    
    for (size_t i = start_line; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        for (char c : line) {
            if (c == '{') {
                brace_count++;
                in_function = true;
            } else if (c == '}') {
                brace_count--;
                if (in_function && brace_count == 0) {
                    return static_cast<LineNumber>(i + 1);
                }
            }
        }
    }
    
    // 見つからない場合は開始行+10を返す
    return static_cast<LineNumber>(std::min(start_line + 10, lines.size()));
}

//=============================================================================
// 🆕 Phase 1: impl分類修正処理
//=============================================================================

void RustAnalyzer::fix_impl_method_classification(AnalysisResult& result) {
    using namespace nekocode::debug;
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Starting impl method classification fix");
    
    
    // implメソッドをfunctions[]からclasses[].methods[]に移動
    std::vector<FunctionInfo> remaining_functions;
    
    for (const auto& func : result.functions) {
        bool is_impl_method = false;
        
        // このfunctionがどのimplのメソッドか確認
        for (const auto& impl : impls_) {
            auto it = std::find(impl.methods.begin(), impl.methods.end(), func.name);
            if (it != impl.methods.end()) {
                // implメソッド発見！対応するstructのclassesに移動
                auto* target_struct = find_struct_in_classes(result.classes, impl.struct_name);
                if (target_struct) {
                    // FunctionInfo を ClassInfo.methods に追加
                    FunctionInfo method_info = func;
                    
                    // Phase 2用のメタデータ追加（先取り）
                    method_info.metadata["parent_struct"] = impl.struct_name;
                    method_info.metadata["impl_type"] = impl.trait_name.empty() ? "inherent" : "trait";
                    method_info.metadata["language"] = "rust";
                    if (!impl.trait_name.empty()) {
                        method_info.metadata["trait_name"] = impl.trait_name;
                    }
                    // 🆕 Phase 2: アクセス修飾子を統一形式で追加
                    if (method_info.metadata.count("is_pub")) {
                        method_info.metadata["access_modifier"] = "pub";
                    } else {
                        method_info.metadata["access_modifier"] = "private";  // Rustのデフォルトはprivate
                    }
                    
                    target_struct->methods.push_back(method_info);
                    is_impl_method = true;
                    
                    NEKOCODE_LOG_TRACE("RustAnalyzer", "Moved method '" + func.name + 
                                      "' from functions[] to " + impl.struct_name + ".methods[]");
                    break;
                }
            }
        }
        
        // implメソッドでなければfunctions[]に残す
        if (!is_impl_method) {
            remaining_functions.push_back(func);
        }
    }
    
    // functions[]を更新
    result.functions = remaining_functions;
    
    NEKOCODE_LOG_DEBUG("RustAnalyzer", "Impl method classification completed - " + 
                      std::to_string(result.functions.size()) + " standalone functions remaining");
}

//=============================================================================
// 🆕 ヘルパー関数
//=============================================================================

// 🎯 Rust構造体/列挙型の終了行を検出（ブレースベース）
LineNumber RustAnalyzer::find_struct_end_line(const std::string& content, LineNumber start_line) {
    std::istringstream stream(content);
    std::string line;
    LineNumber current_line = 1;
    int brace_level = 0;
    bool struct_started = false;
    
    while (std::getline(stream, line)) {
        if (current_line >= start_line) {
            // コメントを除外
            size_t comment_pos = line.find("//");
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            
            // ブロックコメントの簡易処理（/* */）
            size_t block_start = line.find("/*");
            size_t block_end = line.find("*/");
            if (block_start != std::string::npos && block_end != std::string::npos) {
                line = line.substr(0, block_start) + line.substr(block_end + 2);
            }
            
            // ブレースレベルをカウント
            for (char c : line) {
                if (c == '{') {
                    brace_level++;
                    struct_started = true;
                } else if (c == '}') {
                    brace_level--;
                    if (struct_started && brace_level == 0) {
                        // 構造体/列挙型の終了
                        return current_line;
                    }
                }
            }
            
            // タプル構造体やユニット構造体の場合（セミコロンで終わる）
            if (!struct_started && line.find(';') != std::string::npos) {
                return current_line;
            }
        }
        current_line++;
    }
    
    return start_line; // デフォルト
}

ClassInfo* RustAnalyzer::find_struct_in_classes(std::vector<ClassInfo>& classes, const std::string& struct_name) {
    for (auto& class_info : classes) {
        if (class_info.name == struct_name) {
            return &class_info;
        }
    }
    return nullptr;
}

//=============================================================================
// 🚀 Phase 5: Universal Symbol生成メソッド実装
//=============================================================================

void RustAnalyzer::initialize_symbol_table() {
    if (!symbol_table_) {
        symbol_table_ = std::make_shared<SymbolTable>();
        id_counters_.clear();
    }
}

std::string RustAnalyzer::generate_unique_id(const std::string& base) {
    id_counters_[base]++;
    return base + "_" + std::to_string(id_counters_[base] - 1);
}

void RustAnalyzer::add_test_struct_symbol(const std::string& struct_name, std::uint32_t start_line) {
    initialize_symbol_table();
    
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("struct_" + struct_name);
    symbol.symbol_type = SymbolType::CLASS;  // Rust structをclassとして扱う
    symbol.name = struct_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "rust";
    symbol.metadata["type"] = "struct";
    
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "[Phase 5 Test] Rust adding struct symbol: " << struct_name 
              << " with ID: " << symbol.symbol_id << std::endl;
#endif
    
    symbol_table_->add_symbol(std::move(symbol));
}

void RustAnalyzer::add_test_enum_symbol(const std::string& enum_name, std::uint32_t start_line) {
    initialize_symbol_table();
    
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("enum_" + enum_name);
    symbol.symbol_type = SymbolType::CLASS;  // Rust enumをclassとして扱う
    symbol.name = enum_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "rust";
    symbol.metadata["type"] = "enum";
    
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "[Phase 5 Test] Rust adding enum symbol: " << enum_name 
              << " with ID: " << symbol.symbol_id << std::endl;
#endif
    
    symbol_table_->add_symbol(std::move(symbol));
}

void RustAnalyzer::add_test_function_symbol(const std::string& function_name, std::uint32_t start_line) {
    initialize_symbol_table();
    
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("function_" + function_name);
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = function_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "rust";
    
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "[Phase 5 Test] Rust adding function symbol: " << function_name 
              << " with ID: " << symbol.symbol_id << std::endl;
#endif
    
    symbol_table_->add_symbol(std::move(symbol));
}

} // namespace nekocode