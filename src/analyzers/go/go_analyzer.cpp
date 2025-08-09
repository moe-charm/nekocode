//=============================================================================
// 🐹 Go Language Analyzer Implementation
//
// Go特化解析エンジン実装:
// - Goroutine並行処理の完全検出
// - Channel通信パターンの詳細解析
// - Select/Defer文の検出
// - 複数戻り値関数の解析
//=============================================================================

#include "nekocode/analyzers/go_analyzer.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🐹 Data Structure JSON Serialization
//=============================================================================

nlohmann::json GoroutineInfo::to_json() const {
    return nlohmann::json{
        {"function_name", function_name},
        {"line_number", line_number},
        {"is_anonymous", is_anonymous}
    };
}

nlohmann::json ChannelInfo::to_json() const {
    return nlohmann::json{
        {"variable_name", variable_name},
        {"type", type},
        {"declaration_line", declaration_line},
        {"is_buffered", is_buffered},
        {"buffer_size", buffer_size}
    };
}

nlohmann::json GoFunctionInfo::to_json() const {
    return nlohmann::json{
        {"name", name},
        {"line_number", line_number},
        {"parameters", parameters},
        {"return_types", return_types},
        {"has_receiver", has_receiver},
        {"receiver_type", receiver_type}
    };
}

nlohmann::json GoStructInfo::to_json() const {
    return nlohmann::json{
        {"name", name},
        {"line_number", line_number},
        {"fields", fields},
        {"methods", methods}
    };
}

//=============================================================================
// 🐹 GoAnalyzer Main Implementation
//=============================================================================

Language GoAnalyzer::get_language() const {
    return Language::GO;
}

std::string GoAnalyzer::get_language_name() const {
    return "Go";
}

std::vector<std::string> GoAnalyzer::get_supported_extensions() const {
    return {".go"};
}

void GoAnalyzer::reset_state() {
    goroutines_.clear();
    channels_.clear();
    go_functions_.clear();
    imports_.clear();
    package_name_.clear();
}

AnalysisResult GoAnalyzer::analyze(const std::string& content, const std::string& filename) {
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "🐹 Go Analyzer: Starting analysis..." << std::endl;
#endif
    
    reset_state();
    
    AnalysisResult result;
    result.language = Language::GO;
    
    try {
        // 基本統計
        std::istringstream iss(content);
        std::string line;
        LineNumber line_num = 0;
        LineNumber code_lines = 0;
        LineNumber comment_lines = 0;
        
        while (std::getline(iss, line)) {
            line_num++;
            
            // 空行をスキップ
            if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) {
                continue;
            }
            
            // コメント行検出
            if (line.find("//") != std::string::npos || 
                line.find("/*") != std::string::npos) {
                comment_lines++;
            } else {
                code_lines++;
            }
        }
        
        result.file_info.total_lines = line_num;
        result.file_info.code_lines = code_lines;
        result.file_info.comment_lines = comment_lines;
        
        // Package name extraction
        package_name_ = extract_package_name(content);
        
        // Import analysis
        auto imports = extract_imports(content);
        for (const auto& imp : imports) {
            imports_.insert(imp);
        }
        
        // Go-specific analysis
        goroutines_ = analyze_goroutines(content);
        channels_ = analyze_channels(content);
        go_functions_ = analyze_go_functions(content);
        auto go_structs = analyze_go_structs(content);  // 🔥 struct検出追加
        
        // Complexity calculation
        result.complexity.cyclomatic_complexity = calculate_go_complexity(content);
        
        // JSON出力用の詳細情報
        nlohmann::json go_details;
        go_details["package_name"] = package_name_;
        go_details["imports"] = std::vector<std::string>(imports_.begin(), imports_.end());
        go_details["goroutine_count"] = goroutines_.size();
        go_details["channel_count"] = channels_.size();
        go_details["function_count"] = go_functions_.size();
        
        // Goroutines詳細
        nlohmann::json goroutines_json = nlohmann::json::array();
        for (const auto& gr : goroutines_) {
            goroutines_json.push_back(gr.to_json());
        }
        go_details["goroutines"] = goroutines_json;
        
        // Channels詳細
        nlohmann::json channels_json = nlohmann::json::array();
        for (const auto& ch : channels_) {
            channels_json.push_back(ch.to_json());
        }
        go_details["channels"] = channels_json;
        
        // Functions詳細
        nlohmann::json functions_json = nlohmann::json::array();
        for (const auto& fn : go_functions_) {
            functions_json.push_back(fn.to_json());
        }
        go_details["functions"] = functions_json;
        
        result.metadata["go_specific"] = go_details.dump();
        
        // 🔥 重要：Go structsをAnalysisResult.classesに変換！
        for (const auto& go_struct : go_structs) {
            ClassInfo class_info;
            class_info.name = go_struct.name;
            class_info.start_line = go_struct.line_number;
            class_info.end_line = go_struct.line_number;  // 簡易実装
            class_info.metadata["type"] = "struct";  // metadataに型情報を格納
            class_info.metadata["has_methods"] = "false";  // メソッド関連付けは後で
            result.classes.push_back(class_info);
            
            // Universal Symbol生成
            add_test_struct_symbol(go_struct.name, go_struct.line_number);
        }
        
        // 🔥 重要：GoFunctionInfoをAnalysisResult.functionsに変換！
        for (const auto& go_func : go_functions_) {
            FunctionInfo func_info;
            func_info.name = go_func.name;
            func_info.start_line = go_func.line_number;
            func_info.end_line = go_func.line_number; // Goアナライザーでは行数のみ
            // Go特有の情報をmetadataに格納
            if (go_func.has_receiver) {
                func_info.metadata["receiver_type"] = go_func.receiver_type;
                func_info.metadata["is_method"] = "true";
            }
            if (!go_func.return_types.empty()) {
                func_info.metadata["return_count"] = std::to_string(go_func.return_types.size());
            }
            result.functions.push_back(func_info);
            
            // 🚀 Phase 5: Universal Symbol直接生成
            add_test_function_symbol(go_func.name, go_func.line_number);
        }
        
        // 🔥 重要：統計情報を更新！
        result.update_statistics();
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "🐹 Go Analysis Complete: " 
                  << goroutines_.size() << " goroutines, "
                  << channels_.size() << " channels, "
                  << go_functions_.size() << " functions detected" << std::endl;
#endif
        
        // 🚀 Phase 5: Universal Symbol結果設定
        if (symbol_table_ && symbol_table_->get_all_symbols().size() > 0) {
            result.universal_symbols = symbol_table_;
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[Phase 5] Go analyzer generated " 
                      << symbol_table_->get_all_symbols().size() 
                      << " Universal Symbols" << std::endl;
#endif
        }
        
    } catch (const std::exception& e) {
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "🚨 Go Analysis Error: " << e.what() << std::endl;
#endif
        result.complexity.cyclomatic_complexity = 1; // fallback
    }
    
    return result;
}

//=============================================================================
// 🐹 Go-Specific Analysis Methods
//=============================================================================

std::vector<GoroutineInfo> GoAnalyzer::analyze_goroutines(const std::string& content) {
    std::vector<GoroutineInfo> goroutines;
    
    std::istringstream iss(content);
    std::string line;
    LineNumber line_num = 0;
    
    while (std::getline(iss, line)) {
        line_num++;
        
        // Look for "go " keyword
        size_t go_pos = line.find("go ");
        if (go_pos == std::string::npos) continue;
        
        // Make sure it's a word boundary
        if (go_pos > 0 && std::isalnum(line[go_pos - 1])) continue;
        
        GoroutineInfo info;
        info.line_number = line_num;
        
        // Check if it's anonymous function: go func(
        size_t func_pos = line.find("func(", go_pos + 3);
        if (func_pos != std::string::npos) {
            info.function_name = "anonymous";
            info.is_anonymous = true;
        } else {
            // Try to extract function name: go functionName(
            size_t name_start = go_pos + 3;
            while (name_start < line.length() && std::isspace(line[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < line.length() && 
                   (std::isalnum(line[name_end]) || line[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                info.function_name = line.substr(name_start, name_end - name_start);
                info.is_anonymous = false;
            } else {
                info.function_name = "unknown";
                info.is_anonymous = false;
            }
        }
        
        goroutines.push_back(info);
    }
    
    return goroutines;
}

std::vector<ChannelInfo> GoAnalyzer::analyze_channels(const std::string& content) {
    std::vector<ChannelInfo> channels;
    
    std::istringstream iss(content);
    std::string line;
    LineNumber line_num = 0;
    
    while (std::getline(iss, line)) {
        line_num++;
        
        // Look for "make(chan" pattern
        size_t make_pos = line.find("make(");
        if (make_pos == std::string::npos) continue;
        
        size_t chan_pos = line.find("chan", make_pos + 5);
        if (chan_pos == std::string::npos) continue;
        
        ChannelInfo info;
        info.declaration_line = line_num;
        
        // Try to extract variable name (simple heuristic)
        // Look backwards from make_pos for variable assignment
        size_t assign_pos = line.rfind(":=", make_pos);
        if (assign_pos == std::string::npos) {
            assign_pos = line.rfind("=", make_pos);
        }
        
        if (assign_pos != std::string::npos) {
            // Extract variable name before assignment
            size_t var_end = assign_pos;
            while (var_end > 0 && std::isspace(line[var_end - 1])) {
                var_end--;
            }
            
            size_t var_start = var_end;
            while (var_start > 0 && 
                   (std::isalnum(line[var_start - 1]) || line[var_start - 1] == '_')) {
                var_start--;
            }
            
            if (var_end > var_start) {
                info.variable_name = line.substr(var_start, var_end - var_start);
            }
        }
        
        // Extract channel type after "chan "
        size_t type_start = chan_pos + 4;
        while (type_start < line.length() && std::isspace(line[type_start])) {
            type_start++;
        }
        
        size_t type_end = type_start;
        while (type_end < line.length() && 
               (std::isalnum(line[type_end]) || line[type_end] == '_')) {
            type_end++;
        }
        
        if (type_end > type_start) {
            info.type = line.substr(type_start, type_end - type_start);
        }
        
        // Check for buffer size
        size_t comma_pos = line.find(",", chan_pos);
        if (comma_pos != std::string::npos && comma_pos < line.find(")", chan_pos)) {
            info.is_buffered = true;
            // Try to extract buffer size (simplified)
            size_t size_start = comma_pos + 1;
            while (size_start < line.length() && std::isspace(line[size_start])) {
                size_start++;
            }
            
            size_t size_end = size_start;
            while (size_end < line.length() && std::isdigit(line[size_end])) {
                size_end++;
            }
            
            if (size_end > size_start) {
                std::string size_str = line.substr(size_start, size_end - size_start);
                try {
                    info.buffer_size = std::stoi(size_str);
                } catch (...) {
                    info.buffer_size = 0;
                }
            }
        } else {
            info.is_buffered = false;
            info.buffer_size = 0;
        }
        
        channels.push_back(info);
    }
    
    return channels;
}

std::vector<GoFunctionInfo> GoAnalyzer::analyze_go_functions(const std::string& content) {
    std::vector<GoFunctionInfo> functions;
    
    std::istringstream iss(content);
    std::string line;
    LineNumber line_num = 0;
    
    while (std::getline(iss, line)) {
        line_num++;
        
        // Look for "func " keyword
        size_t func_pos = line.find("func ");
        if (func_pos == std::string::npos) continue;
        
        // Make sure it's a word boundary
        if (func_pos > 0 && std::isalnum(line[func_pos - 1])) continue;
        
        GoFunctionInfo info;
        info.line_number = line_num;
        
        // Check for method receiver: func (receiver) funcName
        size_t receiver_start = func_pos + 5;
        while (receiver_start < line.length() && std::isspace(line[receiver_start])) {
            receiver_start++;
        }
        
        if (receiver_start < line.length() && line[receiver_start] == '(') {
            // Has receiver
            info.has_receiver = true;
            
            size_t receiver_end = line.find(')', receiver_start);
            if (receiver_end != std::string::npos) {
                std::string receiver = line.substr(receiver_start + 1, 
                                                 receiver_end - receiver_start - 1);
                
                // Extract receiver type (last word in receiver)
                size_t space_pos = receiver.find_last_of(" \t");
                if (space_pos != std::string::npos) {
                    info.receiver_type = receiver.substr(space_pos + 1);
                }
                
                // Function name starts after receiver
                size_t name_start = receiver_end + 1;
                while (name_start < line.length() && std::isspace(line[name_start])) {
                    name_start++;
                }
                
                size_t name_end = name_start;
                while (name_end < line.length() && 
                       (std::isalnum(line[name_end]) || line[name_end] == '_')) {
                    name_end++;
                }
                
                if (name_end > name_start) {
                    info.name = line.substr(name_start, name_end - name_start);
                }
            }
        } else {
            // No receiver, regular function
            info.has_receiver = false;
            
            size_t name_start = receiver_start;
            size_t name_end = name_start;
            while (name_end < line.length() && 
                   (std::isalnum(line[name_end]) || line[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                info.name = line.substr(name_start, name_end - name_start);
            }
        }
        
        if (!info.name.empty()) {
            functions.push_back(info);
        }
    }
    
    return functions;
}

std::vector<GoStructInfo> GoAnalyzer::analyze_go_structs(const std::string& content) {
    std::vector<GoStructInfo> structs;
    
    std::istringstream iss(content);
    std::string line;
    LineNumber line_num = 0;
    
    while (std::getline(iss, line)) {
        line_num++;
        
        // Look for "type " keyword
        size_t type_pos = line.find("type ");
        if (type_pos == std::string::npos) continue;
        
        // Make sure it's a word boundary
        if (type_pos > 0 && std::isalnum(line[type_pos - 1])) continue;
        
        // Check if it's a struct definition: type Name struct {
        size_t struct_pos = line.find("struct", type_pos + 5);
        if (struct_pos == std::string::npos) continue;
        
        GoStructInfo info;
        info.line_number = line_num;
        
        // Extract struct name between "type" and "struct"
        size_t name_start = type_pos + 5;
        while (name_start < line.length() && std::isspace(line[name_start])) {
            name_start++;
        }
        
        size_t name_end = name_start;
        while (name_end < struct_pos && 
               (std::isalnum(line[name_end]) || line[name_end] == '_')) {
            name_end++;
        }
        
        if (name_end > name_start) {
            info.name = line.substr(name_start, name_end - name_start);
            // Trim trailing spaces
            size_t last = info.name.find_last_not_of(" \t");
            if (last != std::string::npos) {
                info.name = info.name.substr(0, last + 1);
            }
        }
        
        if (!info.name.empty()) {
            structs.push_back(info);
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "🔥 Found Go struct: " << info.name 
                      << " at line " << line_num << std::endl;
#endif
        }
    }
    
    return structs;
}

//=============================================================================
// 🐹 Helper Methods
//=============================================================================

std::string GoAnalyzer::extract_package_name(const std::string& content) {
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        size_t package_pos = line.find("package ");
        if (package_pos == std::string::npos) continue;
        
        // Make sure it's at word boundary
        if (package_pos > 0 && std::isalnum(line[package_pos - 1])) continue;
        
        size_t name_start = package_pos + 8;
        while (name_start < line.length() && std::isspace(line[name_start])) {
            name_start++;
        }
        
        size_t name_end = name_start;
        while (name_end < line.length() && 
               (std::isalnum(line[name_end]) || line[name_end] == '_')) {
            name_end++;
        }
        
        if (name_end > name_start) {
            return line.substr(name_start, name_end - name_start);
        }
    }
    
    return "unknown";
}

std::vector<std::string> GoAnalyzer::extract_imports(const std::string& content) {
    std::vector<std::string> imports;
    
    std::istringstream iss(content);
    std::string line;
    bool in_import_block = false;
    
    while (std::getline(iss, line)) {
        // Single import: import "path"
        if (!in_import_block) {
            size_t import_pos = line.find("import ");
            if (import_pos != std::string::npos) {
                // Check if it's at word boundary
                if (import_pos > 0 && std::isalnum(line[import_pos - 1])) continue;
                
                // Check for single import
                size_t quote_start = line.find('"', import_pos + 7);
                if (quote_start != std::string::npos) {
                    size_t quote_end = line.find('"', quote_start + 1);
                    if (quote_end != std::string::npos) {
                        std::string import_path = line.substr(quote_start + 1, 
                                                            quote_end - quote_start - 1);
                        imports.push_back(import_path);
                    }
                }
                
                // Check for import block start
                if (line.find('(', import_pos + 7) != std::string::npos) {
                    in_import_block = true;
                }
            }
        } else {
            // Inside import block
            if (line.find(')') != std::string::npos) {
                in_import_block = false;
                continue;
            }
            
            // Extract import from block line
            size_t quote_start = line.find('"');
            if (quote_start != std::string::npos) {
                size_t quote_end = line.find('"', quote_start + 1);
                if (quote_end != std::string::npos) {
                    std::string import_path = line.substr(quote_start + 1, 
                                                        quote_end - quote_start - 1);
                    imports.push_back(import_path);
                }
            }
        }
    }
    
    return imports;
}

int GoAnalyzer::calculate_go_complexity(const std::string& content) {
    int complexity = 1; // base complexity
    
    // Standard complexity factors
    complexity += std::count(content.begin(), content.end(), '{'); // blocks
    
    // Go-specific complexity factors
    std::vector<std::string> complex_patterns = {
        "if ", "for ", "switch ", "select ", "case ", "range ",
        "go ", "defer ", "recover()", "panic("
    };
    
    for (const auto& pattern : complex_patterns) {
        size_t pos = 0;
        while ((pos = content.find(pattern, pos)) != std::string::npos) {
            complexity++;
            pos += pattern.length();
        }
    }
    
    // Channel operations add complexity
    complexity += channels_.size() * 2;  // Channel usage adds complexity
    complexity += goroutines_.size() * 3; // Goroutines add significant complexity
    
    return complexity;
}

//=============================================================================
// 🚀 Phase 5: Universal Symbol生成メソッド実装
//=============================================================================

void GoAnalyzer::initialize_symbol_table() {
    if (!symbol_table_) {
        symbol_table_ = std::make_shared<SymbolTable>();
        id_counters_.clear();
    }
}

std::string GoAnalyzer::generate_unique_id(const std::string& base) {
    id_counters_[base]++;
    return base + "_" + std::to_string(id_counters_[base] - 1);
}

void GoAnalyzer::add_test_struct_symbol(const std::string& struct_name, std::uint32_t start_line) {
    initialize_symbol_table();
    
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("struct_" + struct_name);
    symbol.symbol_type = SymbolType::CLASS;  // Go structをclassとして扱う
    symbol.name = struct_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "go";
    symbol.metadata["type"] = "struct";
    
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "[Phase 5 Test] Go adding struct symbol: " << struct_name 
              << " with ID: " << symbol.symbol_id << std::endl;
#endif
    
    symbol_table_->add_symbol(std::move(symbol));
}

void GoAnalyzer::add_test_function_symbol(const std::string& function_name, std::uint32_t start_line) {
    initialize_symbol_table();
    
    UniversalSymbolInfo symbol;
    symbol.symbol_id = generate_unique_id("function_" + function_name);
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = function_name;
    symbol.start_line = start_line;
    symbol.metadata["language"] = "go";
    
#ifdef NEKOCODE_DEBUG_SYMBOLS
    std::cerr << "[Phase 5 Test] Go adding function symbol: " << function_name 
              << " with ID: " << symbol.symbol_id << std::endl;
#endif
    
    symbol_table_->add_symbol(std::move(symbol));
}

} // namespace nekocode