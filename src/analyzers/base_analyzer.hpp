#pragma once

//=============================================================================
// 🏗️ Base Language Analyzer - 言語解析基底クラス
//
// 各言語固有のアナライザーが継承する基底クラス
// 一言語一ファイルの原則を守るための設計
//=============================================================================

// ⚠️⚠️⚠️ 超重要警告 ⚠️⚠️⚠️
// このプロジェクトではstd::regexは使用禁止です！
// 代わりにPEGTLまたは単純な文字列処理を使用してください。
// 
// ❌ 絶対にやってはいけないこと:
// #include <regex>
// std::regex pattern("...");
// 
// ✅ 代わりに使うべきもの:
// #include <tao/pegtl.hpp>  // PEGTLで文法定義
// content.find("...")       // 単純な文字列検索
// 
// 理由: std::regexは遅い、ネスト構造を扱えない、保守性が低い
// Claudeへ: また無意識にstd::regex使おうとしてるなら深呼吸してPEGTL使うにゃ！

#include "nekocode/types.hpp"
#include "nekocode/symbol_table.hpp"
#include "nekocode/universal_symbol.hpp"
#include <string>
#include <memory>

// 🚫 std::regexの使用を防ぐマクロ定義（基盤処理core.cppは例外）
#if defined(NEKOCODE_PREVENT_REGEX) && !defined(NEKOCODE_FOUNDATION_CORE_CPP)
    #define regex COMPILE_ERROR_DO_NOT_USE_REGEX_USE_PEGTL_INSTEAD
    #define sregex_iterator COMPILE_ERROR_NO_REGEX_ALLOWED
    #define regex_match BANNED_USE_PEGTL_PARSE_INSTEAD
    #define regex_search FORBIDDEN_USE_STRING_FIND_INSTEAD
#endif

namespace nekocode {

//=============================================================================
// 🎯 BaseAnalyzer - 言語解析基底クラス
//=============================================================================

class BaseAnalyzer {
public:
    virtual ~BaseAnalyzer() = default;
    
    //=========================================================================
    // 🔍 純粋仮想関数 - 各言語で実装必須
    //=========================================================================
    
    /// 言語タイプを返す
    virtual Language get_language() const = 0;
    
    /// メイン解析関数
    virtual AnalysisResult analyze(const std::string& content, const std::string& filename) = 0;
    
    /// 言語名を返す（表示用）
    virtual std::string get_language_name() const = 0;
    
    /// サポートする拡張子を返す
    virtual std::vector<std::string> get_supported_extensions() const = 0;
    
    //=========================================================================
    // 🛠️ 共通ユーティリティ関数（std::regex不使用）
    //=========================================================================
    
protected:
    /// 行番号計算（共通処理）
    uint32_t calculate_line_number(const std::string& content, size_t position) {
        if (position >= content.length()) {
            return 1;
        }
        
        uint32_t line_count = 1;
        for (size_t i = 0; i < position; ++i) {
            if (content[i] == '\n') {
                line_count++;
            }
        }
        return line_count;
    }
    
    /// 基本的な複雑度計算（std::regex不使用版）
    virtual ComplexityInfo calculate_complexity(const std::string& content) {
        ComplexityInfo complexity;
        complexity.cyclomatic_complexity = 1; // ベーススコア
        
        // 言語共通のキーワード
        std::vector<std::string> complexity_keywords = {
            "if", "else", "for", "while", "switch", "case", "catch"
        };
        
        // ⚠️ 注意: std::regexは使わない！単純な文字列検索で実装
        for (const auto& keyword : complexity_keywords) {
            size_t pos = 0;
            while ((pos = content.find(keyword, pos)) != std::string::npos) {
                // 単語境界チェック（簡易版）
                if ((pos == 0 || !std::isalnum(content[pos-1])) &&
                    (pos + keyword.length() >= content.length() || 
                     !std::isalnum(content[pos + keyword.length()]))) {
                    complexity.cyclomatic_complexity++;
                }
                pos += keyword.length();
            }
        }
        
        complexity.update_rating();
        return complexity;
    }
    
    // 🛡️ 単純な文字列処理ヘルパー（std::regexの代替）
    
    /// 次の単語を抽出
    std::string extract_next_word(const std::string& content, size_t& pos) {
        // 空白をスキップ
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }
        
        size_t start = pos;
        while (pos < content.length() && (std::isalnum(content[pos]) || content[pos] == '_')) {
            pos++;
        }
        
        return content.substr(start, pos - start);
    }
    
    /// 特定の文字までスキップ
    void skip_until(const std::string& content, size_t& pos, char target) {
        while (pos < content.length() && content[pos] != target) {
            pos++;
        }
    }
    
    /// 文字列リテラルをスキップ
    void skip_string_literal(const std::string& content, size_t& pos, char quote) {
        if (pos < content.length() && content[pos] == quote) {
            pos++; // 開始クォート
            while (pos < content.length()) {
                if (content[pos] == '\\' && pos + 1 < content.length()) {
                    pos += 2; // エスケープシーケンス
                } else if (content[pos] == quote) {
                    pos++; // 終了クォート
                    break;
                } else {
                    pos++;
                }
            }
        }
    }
    
    //=========================================================================
    // 🚀 Universal Symbols共通生成エンジン
    //=========================================================================
    
    /// Universal Symbols生成（全言語共通）
    void generate_universal_symbols(AnalysisResult& result, const std::string& language_name = "unknown") {
        // Phase 5: Universal Symbols生成
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] Starting Universal Symbol generation for " << language_name << std::endl;
        std::cerr << "[DEBUG] Classes: " << result.classes.size() 
                  << ", Functions: " << result.functions.size() << std::endl;
#endif
        
        auto symbol_table = std::make_shared<SymbolTable>();
        int class_counter = 0;
        int method_counter = 0;
        int function_counter = 0;
        
        // クラス情報からUniversal Symbol生成
        for (const auto& class_info : result.classes) {
            UniversalSymbolInfo class_symbol;
            class_symbol.symbol_id = "class_" + class_info.name + "_" + std::to_string(class_counter++);
            class_symbol.symbol_type = SymbolType::CLASS;
            class_symbol.name = class_info.name;
            class_symbol.start_line = class_info.start_line;
            class_symbol.metadata["language"] = language_name;
            
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Adding class symbol: " << class_info.name 
                      << " with ID: " << class_symbol.symbol_id << std::endl;
#endif
            
            symbol_table->add_symbol(std::move(class_symbol));
            
            // クラスメソッドのシンボル生成
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Class " << class_info.name << " has " << class_info.methods.size() << " methods" << std::endl;
#endif
            for (const auto& method : class_info.methods) {
                UniversalSymbolInfo method_symbol;
                method_symbol.symbol_id = "method_" + method.name + "_" + std::to_string(method_counter++);
                method_symbol.symbol_type = SymbolType::FUNCTION;
                method_symbol.name = method.name;
                method_symbol.start_line = method.start_line;
                method_symbol.metadata["language"] = language_name;
                method_symbol.metadata["class"] = class_info.name;
                
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Adding method symbol: " << method.name 
                          << " from class " << class_info.name 
                          << " with ID: " << method_symbol.symbol_id << std::endl;
#endif
                
                symbol_table->add_symbol(std::move(method_symbol));
            }
        }
        
        // 独立関数からUniversal Symbol生成
        for (const auto& func_info : result.functions) {
            UniversalSymbolInfo function_symbol;
            function_symbol.symbol_id = "function_" + func_info.name + "_" + std::to_string(function_counter++);
            function_symbol.symbol_type = SymbolType::FUNCTION;
            function_symbol.name = func_info.name;
            function_symbol.start_line = func_info.start_line;
            function_symbol.metadata["language"] = language_name;
            
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Adding function symbol: " << func_info.name 
                      << " with ID: " << function_symbol.symbol_id << std::endl;
#endif
            
            symbol_table->add_symbol(std::move(function_symbol));
        }
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] Universal Symbol generation completed. Total symbols: " 
                  << symbol_table->get_all_symbols().size() << std::endl;
#endif
        
        // AnalysisResultにUniversal Symbolsを設定
        result.universal_symbols = symbol_table;
    }
};

//=============================================================================
// 🏭 AnalyzerFactory - アナライザーファクトリー
//=============================================================================

class AnalyzerFactory {
public:
    /// 言語に応じたアナライザーを生成
    static std::unique_ptr<BaseAnalyzer> create_analyzer(Language language);
    
    /// ファイル拡張子からアナライザーを生成
    static std::unique_ptr<BaseAnalyzer> create_analyzer_from_extension(const std::string& extension);
    
    //=========================================================================
    // 🎮 Unity 特化ファクトリー関数
    //=========================================================================
    
    /// Unity analyzer を直接生成
    static std::unique_ptr<BaseAnalyzer> create_unity_analyzer();
    
    /// ファイル内容からUnity analyzer を自動選択生成
    static std::unique_ptr<BaseAnalyzer> create_unity_analyzer_from_file(
        const std::string& filename, 
        const std::string& content_preview
    );
    
private:
    /// ファイル拡張子を取得
    static std::string get_extension(const std::string& filename);
};

} // namespace nekocode