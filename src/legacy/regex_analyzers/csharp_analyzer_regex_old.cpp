//=============================================================================
// 🎯 C# Language Analyzer - C#専用解析エンジン実装
//
// C#コードの構造解析・複雑度計算
// .NET/.NET Core/Unity対応、実用的な企業開発向け解析
//=============================================================================

#include "nekocode/analyzers/csharp_analyzer.hpp"
#include <sstream>
#include <algorithm>
#include <stack>

namespace nekocode {

//=============================================================================
// 🎯 CSharpAnalyzer Implementation
//=============================================================================

CSharpAnalyzer::CSharpAnalyzer() {
    initialize_patterns();
}

void CSharpAnalyzer::initialize_patterns() {
    // namespace パターン
    namespace_pattern_ = std::regex(R"(namespace\s+([\w\.]+)(?:\s*\{|\s*;))");
    
    // クラス・インターフェース・構造体パターン
    class_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal|abstract|sealed|static|partial)\s+)*)"
        R"(class\s+(\w+)(?:\s*<[^>]*>)?(?:\s*:\s*([^{]+))?\s*\{)"
    );
    
    interface_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal)\s+)*)"
        R"(interface\s+(\w+)(?:\s*<[^>]*>)?(?:\s*:\s*([^{]+))?\s*\{)"
    );
    
    struct_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal|readonly)\s+)*)"
        R"(struct\s+(\w+)(?:\s*<[^>]*>)?(?:\s*:\s*([^{]+))?\s*\{)"
    );
    
    enum_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal)\s+)*)"
        R"(enum\s+(\w+)(?:\s*:\s*\w+)?\s*\{)"
    );
    
    record_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal)\s+)*)"
        R"(record\s+(\w+)(?:\s*<[^>]*>)?(?:\s*\([^)]*\))?(?:\s*:\s*([^{]+))?\s*[{;])"
    );
    
    // using文パターン
    using_patterns_ = {
        std::regex(R"(using\s+static\s+([\w\.]+)\s*;)"),           // using static
        std::regex(R"(using\s+(\w+)\s*=\s*([\w\.<>]+)\s*;)"),     // using alias
        std::regex(R"(using\s+([\w\.]+)\s*;)")                    // using namespace
    };
    
    // メソッドパターン
    method_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal|static|virtual|override|abstract|async)\s+)*)"
        R"((?:[\w\.<>]+\??)\s+)"          // 戻り値型（null許容型対応）
        R"((\w+)\s*)"
        R"(\([^)]*\)\s*)"
        R"((?:\{|=>))"                   // メソッド本体開始
    );
    
    // プロパティパターン
    property_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal|static|virtual|override|abstract)\s+)*)"
        R"((?:[\w\.<>]+\??)\s+)"          // プロパティ型
        R"((\w+)\s*)"
        R"(\{\s*(?:get|set))"            // get/set
    );
    
    auto_property_pattern_ = std::regex(
        R"((?:(?:public|private|protected|internal|static)\s+)*)"
        R"((?:[\w\.<>]+\??)\s+)"          // プロパティ型
        R"((\w+)\s*)"
        R"(\{\s*get\s*;\s*set\s*;\s*\})" // 自動プロパティ
    );
    
    // 属性パターン
    attribute_pattern_ = std::regex(R"(\[([^\]]+)\])");
    attribute_multiline_pattern_ = std::regex(R"(\[([^\]]*(?:\][^\[])*[^\]]*)\])", std::regex::multiline);
    
    // LINQ・async/awaitパターン
    linq_pattern_ = std::regex(R"(\b(?:from|where|select|join|group|orderby|let)\b)");
    async_pattern_ = std::regex(R"(\basync\s+)");
    await_pattern_ = std::regex(R"(\bawait\s+)");
    
    // 複雑度キーワード初期化
    complexity_keywords_ = {
        "if", "else", "for", "foreach", "while", "do", "switch", "case", 
        "catch", "finally", "&&", "||", "?", "?.", "??", "?[", "=>",
        "try", "using", "lock", "yield"
    };
    
    linq_keywords_ = {
        "from", "where", "select", "join", "group", "orderby", "let",
        "into", "on", "equals", "by", "ascending", "descending"
    };
}

AnalysisResult CSharpAnalyzer::analyze(const std::string& content, const std::string& filename) {
    AnalysisResult result;
    
    // ファイル情報設定
    result.file_info.name = filename;
    result.file_info.size_bytes = content.size();
    result.language = Language::CSHARP;
    
    // 詳細解析実行
    auto detailed_result = analyze_csharp_detailed(content, filename);
    
    // 基本解析結果に変換
    for (const auto& cs_class : detailed_result.classes) {
        ClassInfo class_info;
        class_info.name = cs_class.name;
        class_info.start_line = cs_class.start_line;
        class_info.end_line = cs_class.end_line;
        
        // ベースクラス（最初の1つのみ）
        if (!cs_class.base_classes.empty()) {
            class_info.parent_class = cs_class.base_classes[0];
        }
        
        // メソッドを変換
        for (const auto& cs_method : cs_class.methods) {
            FunctionInfo func_info;
            func_info.name = cs_method.name;
            func_info.start_line = cs_method.start_line;
            func_info.end_line = cs_method.end_line;
            func_info.is_async = cs_method.is_async;
            func_info.parameters = cs_method.parameters;
            class_info.methods.push_back(func_info);
        }
        
        // プロパティ名をプロパティリストに追加
        for (const auto& prop : cs_class.properties) {
            class_info.properties.push_back(prop.name);
        }
        
        result.classes.push_back(class_info);
    }
    
    // using文をimport情報に変換
    for (const auto& using_stmt : detailed_result.using_statements) {
        ImportInfo import_info;
        import_info.module_path = using_stmt.namespace_or_type;
        import_info.type = using_stmt.is_static ? ImportType::ES6_IMPORT : ImportType::COMMONJS_REQUIRE;
        import_info.line_number = using_stmt.line_number;
        
        if (using_stmt.is_alias) {
            import_info.alias = using_stmt.alias_name;
        }
        
        result.imports.push_back(import_info);
    }
    
    // 複雑度計算
    result.complexity = calculate_csharp_complexity(content);
    
    // 統計更新
    result.update_statistics();
    
    return result;
}

CSharpAnalyzer::CSharpAnalysisResult CSharpAnalyzer::analyze_csharp_detailed(
    const std::string& content, const std::string& filename) {
    
    CSharpAnalysisResult result;
    result.base_result.file_info.name = filename;
    result.base_result.language = Language::CSHARP;
    
    // namespace抽出
    extract_namespaces(content, result.namespaces);
    
    // using文抽出
    extract_using_statements(content, result.using_statements);
    
    // クラス・構造体・インターフェース抽出
    extract_classes(content, result.classes);
    
    return result;
}

void CSharpAnalyzer::extract_namespaces(const std::string& content, std::vector<std::string>& namespaces) {
    std::sregex_iterator iter(content.begin(), content.end(), namespace_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        std::string namespace_name = (*iter)[1].str();
        namespaces.push_back(namespace_name);
        ++iter;
    }
}

void CSharpAnalyzer::extract_using_statements(const std::string& content, std::vector<CSharpUsing>& usings) {
    for (size_t i = 0; i < using_patterns_.size(); ++i) {
        std::sregex_iterator iter(content.begin(), content.end(), using_patterns_[i]);
        std::sregex_iterator end;
        
        while (iter != end) {
            CSharpUsing using_info;
            using_info.line_number = calculate_line_number(content, iter->position());
            
            switch (i) {
                case 0: // using static
                    using_info.namespace_or_type = (*iter)[1].str();
                    using_info.is_static = true;
                    break;
                    
                case 1: // using alias
                    using_info.alias_name = (*iter)[1].str();
                    using_info.namespace_or_type = (*iter)[2].str();
                    using_info.is_alias = true;
                    break;
                    
                case 2: // using namespace
                    using_info.namespace_or_type = (*iter)[1].str();
                    break;
            }
            
            usings.push_back(using_info);
            ++iter;
        }
    }
}

void CSharpAnalyzer::extract_classes(const std::string& content, std::vector<CSharpClass>& classes) {
    // クラス・インターフェース・構造体・enum・recordを検出
    std::vector<std::pair<std::regex*, std::string>> patterns = {
        {&class_pattern_, "class"},
        {&interface_pattern_, "interface"},
        {&struct_pattern_, "struct"},
        {&enum_pattern_, "enum"},
        {&record_pattern_, "record"}
    };
    
    for (const auto& pattern_pair : patterns) {
        std::sregex_iterator iter(content.begin(), content.end(), *pattern_pair.first);
        std::sregex_iterator end;
        
        while (iter != end) {
            CSharpClass class_info;
            class_info.name = (*iter)[1].str();
            class_info.class_type = pattern_pair.second;
            class_info.start_line = calculate_line_number(content, iter->position());
            
            // アクセス修飾子抽出
            std::string full_declaration = iter->str();
            class_info.access_modifier = extract_access_modifier(full_declaration);
            
            // 修飾子チェック
            class_info.is_static = (full_declaration.find("static") != std::string::npos);
            class_info.is_abstract = (full_declaration.find("abstract") != std::string::npos);
            class_info.is_sealed = (full_declaration.find("sealed") != std::string::npos);
            class_info.is_partial = (full_declaration.find("partial") != std::string::npos);
            
            // 継承・インターフェース情報（グループ2があれば）
            if (iter->size() > 2 && (*iter)[2].matched) {
                std::string inheritance = (*iter)[2].str();
                std::istringstream iss(inheritance);
                std::string item;
                while (std::getline(iss, item, ',')) {
                    item.erase(0, item.find_first_not_of(" \t"));
                    item.erase(item.find_last_not_of(" \t") + 1);
                    if (!item.empty()) {
                        if (class_info.base_classes.empty() && class_info.class_type == "class") {
                            class_info.base_classes.push_back(item);
                        } else {
                            class_info.interfaces.push_back(item);
                        }
                    }
                }
            }
            
            // クラス内容の終端を検出
            size_t class_start = iter->position();
            size_t class_end = find_class_end(content, class_start);
            class_info.end_line = calculate_line_number(content, class_end);
            
            if (class_end > class_start) {
                std::string class_content = content.substr(class_start, class_end - class_start);
                
                // 属性抽出（クラスの直前）
                if (class_start > 0) {
                    class_info.attributes = extract_attributes(content, class_start - 200);
                }
                
                // メソッド・プロパティ抽出
                extract_methods(class_content, class_info, class_info.start_line);
                extract_properties(class_content, class_info, class_info.start_line);
            }
            
            classes.push_back(class_info);
            ++iter;
        }
    }
}

void CSharpAnalyzer::extract_methods(const std::string& class_content, CSharpClass& class_info, uint32_t base_line) {
    std::sregex_iterator iter(class_content.begin(), class_content.end(), method_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        CSharpMethod method_info;
        method_info.name = (*iter)[1].str();
        method_info.start_line = base_line + calculate_line_number(class_content, iter->position()) - 1;
        
        std::string full_declaration = iter->str();
        
        // 修飾子解析
        method_info.is_async = (full_declaration.find("async") != std::string::npos);
        method_info.is_static = (full_declaration.find("static") != std::string::npos);
        method_info.is_virtual = (full_declaration.find("virtual") != std::string::npos);
        method_info.is_override = (full_declaration.find("override") != std::string::npos);
        method_info.is_abstract = (full_declaration.find("abstract") != std::string::npos);
        method_info.access_modifier = extract_access_modifier(full_declaration);
        
        // 属性抽出（メソッドの直前）
        size_t method_pos = iter->position();
        if (method_pos > 0) {
            method_info.attributes = extract_attributes(class_content, method_pos - 100);
        }
        
        class_info.methods.push_back(method_info);
        ++iter;
    }
}

void CSharpAnalyzer::extract_properties(const std::string& class_content, CSharpClass& class_info, uint32_t base_line) {
    // 通常プロパティ
    std::sregex_iterator iter(class_content.begin(), class_content.end(), property_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        CSharpProperty prop_info;
        prop_info.name = (*iter)[1].str();
        prop_info.line_number = base_line + calculate_line_number(class_content, iter->position()) - 1;
        
        std::string prop_content = iter->str();
        prop_info.has_getter = (prop_content.find("get") != std::string::npos);
        prop_info.has_setter = (prop_content.find("set") != std::string::npos);
        
        class_info.properties.push_back(prop_info);
        ++iter;
    }
    
    // 自動プロパティ
    std::sregex_iterator auto_iter(class_content.begin(), class_content.end(), auto_property_pattern_);
    while (auto_iter != end) {
        CSharpProperty prop_info;
        prop_info.name = (*auto_iter)[1].str();
        prop_info.line_number = base_line + calculate_line_number(class_content, auto_iter->position()) - 1;
        prop_info.is_auto_property = true;
        prop_info.has_getter = true;
        prop_info.has_setter = true;
        
        class_info.properties.push_back(prop_info);
        ++auto_iter;
    }
}

std::vector<CSharpAttribute> CSharpAnalyzer::extract_attributes(const std::string& content, size_t start_pos) {
    std::vector<CSharpAttribute> attributes;
    
    // start_posから少し前の範囲で属性を検索
    size_t search_start = (start_pos > 200) ? start_pos - 200 : 0;
    size_t search_end = std::min(start_pos + 50, content.length());
    
    if (search_end <= search_start) return attributes;
    
    std::string search_content = content.substr(search_start, search_end - search_start);
    
    std::sregex_iterator iter(search_content.begin(), search_content.end(), attribute_pattern_);
    std::sregex_iterator end;
    
    while (iter != end) {
        CSharpAttribute attr;
        attr.full_expression = iter->str();
        attr.name = (*iter)[1].str();
        attr.line_number = calculate_line_number(content, search_start + iter->position());
        
        // 属性名のみ抽出（パラメータがある場合は除去）
        size_t paren_pos = attr.name.find('(');
        if (paren_pos != std::string::npos) {
            attr.name = attr.name.substr(0, paren_pos);
        }
        
        attributes.push_back(attr);
        ++iter;
    }
    
    return attributes;
}

ComplexityInfo CSharpAnalyzer::calculate_csharp_complexity(const std::string& content) {
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // ベーススコア
    
    // 基本的な制御構造
    for (const auto& keyword : complexity_keywords_) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            // 単語境界チェック
            bool is_word_boundary = 
                (pos == 0 || !std::isalnum(content[pos - 1])) &&
                (pos + keyword.length() >= content.length() || 
                 !std::isalnum(content[pos + keyword.length()]));
            
            if (is_word_boundary) {
                complexity.cyclomatic_complexity++;
            }
            pos += keyword.length();
        }
    }
    
    // LINQ複雑度
    complexity.cyclomatic_complexity += calculate_linq_complexity(content);
    
    // async/await複雑度
    complexity.cyclomatic_complexity += calculate_async_complexity(content);
    
    // ネスト深度計算（ブレースベース）
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

uint32_t CSharpAnalyzer::calculate_linq_complexity(const std::string& content) {
    uint32_t linq_complexity = 0;
    
    for (const auto& keyword : linq_keywords_) {
        size_t pos = 0;
        while ((pos = content.find(keyword, pos)) != std::string::npos) {
            // LINQ構文内での使用かチェック
            bool is_word_boundary = 
                (pos == 0 || !std::isalnum(content[pos - 1])) &&
                (pos + keyword.length() >= content.length() || 
                 !std::isalnum(content[pos + keyword.length()]));
            
            if (is_word_boundary) {
                linq_complexity++;
            }
            pos += keyword.length();
        }
    }
    
    // メソッドチェーン形式のLINQ
    std::regex method_chain_pattern(R"(\.\s*(?:Where|Select|OrderBy|GroupBy|Join|FirstOrDefault|Any|All|Count)\s*\()");
    std::sregex_iterator iter(content.begin(), content.end(), method_chain_pattern);
    std::sregex_iterator end;
    
    while (iter != end) {
        linq_complexity++;
        ++iter;
    }
    
    return linq_complexity;
}

uint32_t CSharpAnalyzer::calculate_async_complexity(const std::string& content) {
    uint32_t async_complexity = 0;
    
    // async/await ペアをカウント
    std::sregex_iterator async_iter(content.begin(), content.end(), async_pattern_);
    std::sregex_iterator await_iter(content.begin(), content.end(), await_pattern_);
    std::sregex_iterator end;
    
    while (async_iter != end) {
        async_complexity++;
        ++async_iter;
    }
    
    while (await_iter != end) {
        async_complexity++;
        ++await_iter;
    }
    
    return async_complexity;
}

size_t CSharpAnalyzer::find_class_end(const std::string& content, size_t class_start) {
    std::stack<char> brace_stack;
    size_t pos = content.find('{', class_start);
    
    if (pos == std::string::npos) {
        return content.length();
    }
    
    brace_stack.push('{');
    pos++;
    
    while (pos < content.length() && !brace_stack.empty()) {
        char c = content[pos];
        
        if (c == '{') {
            brace_stack.push('{');
        } else if (c == '}') {
            brace_stack.pop();
        }
        // 文字列リテラル内のブレースを無視する簡易実装
        else if (c == '"') {
            pos++;
            while (pos < content.length() && content[pos] != '"') {
                if (content[pos] == '\\') pos++; // エスケープ文字をスキップ
                pos++;
            }
        }
        
        pos++;
    }
    
    return pos;
}

std::string CSharpAnalyzer::extract_access_modifier(const std::string& declaration) {
    if (declaration.find("public") != std::string::npos) return "public";
    if (declaration.find("private") != std::string::npos) return "private";
    if (declaration.find("protected") != std::string::npos) return "protected";
    if (declaration.find("internal") != std::string::npos) return "internal";
    return "private"; // C#のデフォルト
}

} // namespace nekocode