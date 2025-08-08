//=============================================================================
// 🐹 Go Symbol Converter Implementation
//
// Phase 4.5: Go専用変換レイヤーの実装
// C#実装をテンプレートとして使用
//=============================================================================

#include "go_symbol_converter.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace nekocode {

//=============================================================================
// 主要変換メソッド
//=============================================================================

SymbolTable GoSymbolConverter::convert_from_analysis_result(const AnalysisResult& result) {
    SymbolTable table;
    
    // Go固有のコンテキスト設定
    GoContext context;
    context.language = result.language;
    
    // Go特有: package階層を解析
    auto packages = parse_packages(result);
    context.has_interfaces = false;
    context.has_goroutines = false;
    context.has_channels = false;
    
    // Go特有フラグの検出
    for (const auto& cls : result.classes) {
        if (is_interface_class(cls)) {
            context.has_interfaces = true;
        }
    }
    
    // マッピング用（class名 → symbol_id）
    std::unordered_map<std::string, std::string> class_name_to_id;
    std::unordered_map<std::string, std::string> package_name_to_id;
    std::unordered_map<std::string, std::string> interface_name_to_id;
    
    // ========== 1. packageを変換 ==========
    for (const auto& pkg : packages) {
        UniversalSymbolInfo pkg_sym = convert_package_to_symbol(pkg, context);
        pkg_sym.symbol_id = generate_unique_id("package_" + pkg.name);
        pkg_sym.start_line = pkg.start_line;
        
        // packageのメタデータ設定
        pkg_sym.metadata = build_package_metadata(pkg, context);
        
        // IDマッピングを保存
        package_name_to_id[pkg.name] = pkg_sym.symbol_id;
        
        table.add_symbol(pkg_sym);
    }
    
    // ========== 2. interfaceを変換 ==========
    for (const auto& cls : result.classes) {
        if (!is_interface_class(cls)) {
            continue; // interfaceでない場合はスキップ
        }
        
        UniversalSymbolInfo interface_sym = convert_interface_to_symbol(cls, context);
        interface_sym.symbol_id = generate_unique_id("interface_" + extract_interface_name(cls.name));
        interface_sym.start_line = cls.start_line;
        interface_sym.end_line = cls.end_line;
        
        // interfaceのメタデータ設定
        interface_sym.metadata = build_interface_metadata(cls, context);
        
        // IDマッピングを保存
        interface_name_to_id[cls.name] = interface_sym.symbol_id;
        
        table.add_symbol(interface_sym);
    }
    
    // ========== 3. 通常のstruct/typeを変換 ==========
    for (const auto& cls : result.classes) {
        if (is_package_class(cls) || is_interface_class(cls)) {
            continue; // package/interfaceとして既に処理済み
        }
        
        UniversalSymbolInfo struct_sym = convert_struct_to_symbol(cls, context);
        struct_sym.symbol_id = generate_unique_id("struct_" + cls.name);
        struct_sym.start_line = cls.start_line;
        struct_sym.end_line = cls.end_line;
        
        // structのメタデータ設定
        struct_sym.metadata = build_struct_metadata(cls, context);
        
        // IDマッピングを保存
        class_name_to_id[cls.name] = struct_sym.symbol_id;
        
        // ========== 4. メンバ変数を子要素として追加 ==========
        for (const auto& member : cls.member_variables) {
            UniversalSymbolInfo member_sym = convert_member_to_symbol(member, struct_sym.symbol_id, context);
            member_sym.symbol_id = generate_unique_id("field_" + cls.name + "_" + member.name);
            member_sym.parent_id = struct_sym.symbol_id;
            
            // Go固有のメンバ変数メタデータ
            member_sym.metadata["access_modifier"] = member.access_modifier;
            member_sym.metadata["type"] = member.type;
            if (member.is_static) {
                member_sym.metadata["is_static"] = "true";
            }
            if (member.is_const) {
                member_sym.metadata["is_const"] = "true";
            }
            
            // 子要素として追加
            struct_sym.child_ids.push_back(member_sym.symbol_id);
            table.add_symbol(member_sym);
        }
        
        // ========== 5. メソッド（レシーバー付き関数）を子要素として追加 ==========
        for (const auto& method : cls.methods) {
            UniversalSymbolInfo method_sym = convert_method_to_symbol(method, struct_sym.symbol_id, context);
            method_sym.symbol_id = generate_unique_id("method_" + cls.name + "_" + method.name);
            method_sym.parent_id = struct_sym.symbol_id;
            
            // qualified_name設定（Go風）
            method_sym.qualified_name = cls.name + "." + method.name;
            
            // メソッドのメタデータ
            method_sym.metadata = build_method_metadata(method, context);
            method_sym.metadata["parent_struct"] = cls.name;
            method_sym.metadata["language"] = "go";
            
            // 複雑度情報設定
            method_sym.complexity.cyclomatic_complexity = method.complexity.cyclomatic_complexity;
            method_sym.complexity.cognitive_complexity = method.complexity.cognitive_complexity;
            method_sym.complexity.max_nesting_depth = method.complexity.max_nesting_depth;
            
            // 子要素として追加
            struct_sym.child_ids.push_back(method_sym.symbol_id);
            table.add_symbol(method_sym);
        }
        
        table.add_symbol(struct_sym);
    }
    
    // ========== 6. スタンドアロン関数を変換 ==========
    for (const auto& func : result.functions) {
        // レシーバー付きメソッドかどうかの簡易判定（既にメソッドとして処理済みの場合はスキップ）
        bool is_method = false;
        for (const auto& cls : result.classes) {
            if (is_package_class(cls) || is_interface_class(cls)) continue;
            
            for (const auto& method : cls.methods) {
                if (method.name == func.name) {
                    is_method = true;
                    break;
                }
            }
            if (is_method) break;
        }
        
        if (is_method) continue; // 既にメソッドとして処理済み
        
        UniversalSymbolInfo func_sym = convert_function_to_symbol(func, context);
        func_sym.symbol_id = generate_unique_id("function_" + func.name);
        
        // 関数のメタデータ
        func_sym.metadata = build_function_metadata(func, context);
        func_sym.metadata["language"] = "go";
        
        // 複雑度情報設定
        func_sym.complexity.cyclomatic_complexity = func.complexity.cyclomatic_complexity;
        func_sym.complexity.cognitive_complexity = func.complexity.cognitive_complexity;
        func_sym.complexity.max_nesting_depth = func.complexity.max_nesting_depth;
        
        table.add_symbol(func_sym);
    }
    
    return table;
}

//=============================================================================
// Go特有の解析メソッド
//=============================================================================

std::vector<GoSymbolConverter::PackageInfo> GoSymbolConverter::parse_packages(const AnalysisResult& result) {
    std::vector<PackageInfo> packages;
    
    for (const auto& cls : result.classes) {
        if (is_package_class(cls)) {
            PackageInfo pkg;
            pkg.name = extract_package_name(cls.name);
            pkg.full_name = cls.name; // "package:main" 形式
            pkg.start_line = cls.start_line;
            packages.push_back(pkg);
        }
    }
    
    return packages;
}

bool GoSymbolConverter::is_package_class(const ClassInfo& class_info) {
    return class_info.name.starts_with("package:");
}

bool GoSymbolConverter::is_interface_class(const ClassInfo& class_info) {
    return class_info.name.starts_with("interface:");
}

bool GoSymbolConverter::is_struct_class(const ClassInfo& class_info) {
    // Go structは通常のクラスとして検出される（prefixなし）
    return !is_package_class(class_info) && !is_interface_class(class_info);
}

bool GoSymbolConverter::is_receiver_method(const FunctionInfo& func_info) {
    // レシーバー付きメソッドの簡易判定
    // Go analyzerがmetadataに"receiver_type"を設定している場合
    return func_info.metadata.find("receiver_type") != func_info.metadata.end();
}

std::string GoSymbolConverter::extract_package_name(const std::string& class_name) {
    if (class_name.starts_with("package:")) {
        return class_name.substr(8); // "package:"を除去
    }
    return class_name;
}

std::string GoSymbolConverter::extract_interface_name(const std::string& class_name) {
    if (class_name.starts_with("interface:")) {
        return class_name.substr(10); // "interface:"を除去
    }
    return class_name;
}

std::string GoSymbolConverter::extract_receiver_type(const FunctionInfo& func_info) {
    auto it = func_info.metadata.find("receiver_type");
    if (it != func_info.metadata.end()) {
        return it->second;
    }
    return "";
}

//=============================================================================
// 個別シンボル変換メソッド
//=============================================================================

UniversalSymbolInfo GoSymbolConverter::convert_package_to_symbol(const PackageInfo& pkg_info, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::NAMESPACE; // PackageはNamespaceとして扱う
    symbol.name = pkg_info.name;
    symbol.start_line = pkg_info.start_line;
    symbol.qualified_name = pkg_info.name;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo GoSymbolConverter::convert_interface_to_symbol(const ClassInfo& class_info, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::INTERFACE;
    symbol.name = extract_interface_name(class_info.name);
    symbol.start_line = class_info.start_line;
    symbol.end_line = class_info.end_line;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo GoSymbolConverter::convert_struct_to_symbol(const ClassInfo& class_info, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::CLASS; // StructはClassとして扱う
    symbol.name = class_info.name;
    symbol.start_line = class_info.start_line;
    symbol.end_line = class_info.end_line;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo GoSymbolConverter::convert_function_to_symbol(const FunctionInfo& func_info, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = func_info.name;
    symbol.start_line = func_info.start_line;
    symbol.end_line = func_info.end_line;
    symbol.parameters = func_info.parameters;
    
    return symbol;
}

UniversalSymbolInfo GoSymbolConverter::convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::METHOD;
    symbol.name = method.name;
    symbol.start_line = method.start_line;
    symbol.end_line = method.end_line;
    symbol.parameters = method.parameters;
    symbol.parent_id = parent_id;
    
    return symbol;
}

UniversalSymbolInfo GoSymbolConverter::convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, GoContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::MEMBER_VAR;
    symbol.name = member.name;
    symbol.start_line = member.declaration_line;
    symbol.end_line = member.declaration_line;
    symbol.parent_id = parent_id;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

//=============================================================================
// メタデータ構築メソッド
//=============================================================================

std::unordered_map<std::string, std::string> GoSymbolConverter::build_package_metadata(const PackageInfo& pkg_info, GoContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "package";
    metadata["language"] = "go";
    metadata["full_name"] = pkg_info.full_name;
    
    return metadata;
}

std::unordered_map<std::string, std::string> GoSymbolConverter::build_interface_metadata(const ClassInfo& class_info, GoContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "interface";
    metadata["language"] = "go";
    
    return metadata;
}

std::unordered_map<std::string, std::string> GoSymbolConverter::build_struct_metadata(const ClassInfo& class_info, GoContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "struct";
    metadata["language"] = "go";
    
    if (!class_info.parent_class.empty()) {
        metadata["embedded_struct"] = class_info.parent_class;
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> GoSymbolConverter::build_function_metadata(const FunctionInfo& func_info, GoContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    // Go特有の関数判定
    if (func_info.name == "main") {
        metadata["is_main"] = "true";
    }
    if (func_info.name == "init") {
        metadata["is_init"] = "true";
    }
    
    // variadic関数判定（...を含む）
    for (const auto& param : func_info.parameters) {
        if (param.find("...") != std::string::npos) {
            metadata["is_variadic"] = "true";
            break;
        }
    }
    
    // Go固有のfunction metadata
    for (const auto& [key, value] : func_info.metadata) {
        metadata[key] = value;
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> GoSymbolConverter::build_method_metadata(const FunctionInfo& method, GoContext& context) {
    auto metadata = build_function_metadata(method, context);
    
    // Go特有のメソッド判定
    auto receiver_it = method.metadata.find("receiver_type");
    if (receiver_it != method.metadata.end()) {
        metadata["receiver_type"] = receiver_it->second;
        
        // ポインタレシーバーの判定
        if (receiver_it->second.find("*") != std::string::npos) {
            metadata["receiver_kind"] = "pointer";
        } else {
            metadata["receiver_kind"] = "value";
        }
    }
    
    return metadata;
}

//=============================================================================
// ユーティリティメソッド
//=============================================================================

std::string GoSymbolConverter::generate_unique_id(const std::string& base) {
    std::string candidate = base;
    
    if (used_ids_.count(candidate)) {
        int counter = ++id_counters_[base];
        candidate = base + "_" + std::to_string(counter);
        
        while (used_ids_.count(candidate)) {
            counter = ++id_counters_[base];
            candidate = base + "_" + std::to_string(counter);
        }
    }
    
    used_ids_.insert(candidate);
    return candidate;
}

//=============================================================================
// 将来実装: SymbolTable → AnalysisResult 変換
//=============================================================================

AnalysisResult GoSymbolConverter::convert_to_analysis_result(const SymbolTable& table) {
    // 将来実装予定
    AnalysisResult result;
    result.language = Language::GO;
    return result;
}

} // namespace nekocode