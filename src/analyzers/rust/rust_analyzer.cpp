//=============================================================================
// 🦀 Rust Language Analyzer Implementation
//=============================================================================

#include "nekocode/analyzers/rust_analyzer.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🦀 メイン解析処理
//=============================================================================

AnalysisResult RustAnalyzer::analyze(const std::string& content, const std::string& filename) {
    AnalysisResult result;
    
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
    
    // 各要素を解析
    analyze_functions(content);
    analyze_structs(content);
    analyze_enums(content);
    analyze_traits(content);
    analyze_impls(content);
    analyze_macros(content);
    analyze_modules(content, result);
    analyze_use_statements(content, result);
    
    // 🔥 重要：RustFunctionInfoをAnalysisResult.functionsに変換！
    for (const auto& rust_func : rust_functions_) {
        FunctionInfo func_info;
        func_info.name = rust_func.name;
        func_info.start_line = rust_func.line_number;
        
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
    }
    
    // 構造体をクラスとして扱う
    for (const auto& rust_struct : structs_) {
        ClassInfo class_info;
        class_info.name = rust_struct.name;
        class_info.start_line = rust_struct.line_number;
        if (rust_struct.is_pub) {
            class_info.metadata["is_pub"] = "true";
        }
        result.classes.push_back(class_info);
    }
    
    // 列挙型もクラスとして扱う
    for (const auto& rust_enum : enums_) {
        ClassInfo class_info;
        class_info.name = rust_enum.name;
        class_info.start_line = rust_enum.line_number;
        class_info.metadata["type"] = "enum";
        result.classes.push_back(class_info);
    }
    
    // 複雑度計算
    result.complexity = calculate_rust_complexity(content);
    
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
    
    // 🔥 重要：統計情報を更新！
    result.update_statistics();
    
    return result;
}

//=============================================================================
// 🦀 関数解析
//=============================================================================

void RustAnalyzer::analyze_functions(const std::string& content) {
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
            
            rust_functions_.push_back(func_info);
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
    std::istringstream stream(content);
    std::string line;
    size_t line_number = 1;
    
    // implパターン: impl<T> Trait for Struct または impl<T> Struct
    std::regex impl_pattern(R"(^\s*impl(?:<[^>]+>)?\s+(?:(\w+)\s+for\s+)?(\w+))");
    
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, impl_pattern)) {
            ImplInfo impl_info;
            
            if (!match[1].str().empty()) {
                // impl Trait for Struct パターン
                impl_info.trait_name = match[1].str();
                impl_info.struct_name = match[2].str();
            } else {
                // impl Struct パターン
                impl_info.struct_name = match[2].str();
            }
            
            impl_info.line_number = line_number;
            impls_.push_back(impl_info);
        }
        
        line_number++;
    }
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

} // namespace nekocode