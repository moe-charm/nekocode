#pragma once

//=============================================================================
// 🔍 Symbol Finder - 関数・変数検索機能
//
// シンプルで実用的な検索機能を提供
// Claude Code対応の出力制限機能付き
//=============================================================================

#include "types.hpp"
#include <string>
#include <vector>
#include <map>

namespace nekocode {

//=============================================================================
// 🎯 Symbol Finder - シンボル検索クラス
//=============================================================================

class SymbolFinder {
public:
    //=========================================================================
    // 📊 列挙型定義
    //=========================================================================
    
    enum class SymbolType {
        AUTO,      // 自動判定（デフォルト）
        FUNCTION,  // 関数のみ
        VARIABLE,  // 変数のみ
        ALL        // 両方強制
    };
    
    enum class UseType {
        DECLARATION,    // 宣言 (let x, function f)
        ASSIGNMENT,     // 代入 (x = 5)
        CALL,          // 呼び出し (f(), obj.method())
        REFERENCE      // 参照 (if(x), return x)
    };
    
    //=========================================================================
    // 📝 データ構造
    //=========================================================================
    
    struct SymbolLocation {
        std::string file_path;      // ファイルパス
        uint32_t line_number;       // 行番号
        std::string line_content;   // 該当行の内容
        UseType use_type;           // 使用タイプ
        SymbolType symbol_type;     // シンボルタイプ（関数/変数）
        
        // デバッグ用
        std::string toString() const;
    };
    
    struct FindOptions {
        SymbolType type;                      // 検索タイプ
        size_t display_limit;                 // 表示上限（Claude Code対応）
        std::string output_file;              // 出力ファイル（指定時）
        std::vector<std::string> search_paths; // 検索パス
        bool show_context;                    // 前後の行を表示
        size_t context_lines;                 // コンテキスト行数
        bool debug;                          // デバッグ出力有効化
        
        // デフォルトコンストラクタ
        FindOptions() 
            : type(SymbolType::AUTO)
            , display_limit(50)
            , show_context(false)
            , context_lines(2)
            , debug(false) {}
    };
    
    struct FindResults {
        std::vector<SymbolLocation> locations;  // 検索結果
        size_t total_count = 0;                 // 総件数
        
        // 統計情報
        size_t function_count = 0;              // 関数としての使用数
        size_t variable_count = 0;              // 変数としての使用数
        std::map<std::string, size_t> file_counts; // ファイル別件数
        std::map<UseType, size_t> use_type_counts; // 使用タイプ別件数
        
        // ヘルパー関数
        bool isEmpty() const { return locations.empty(); }
        void addLocation(const SymbolLocation& loc);
    };
    
    //=========================================================================
    // 🔍 メイン機能
    //=========================================================================
    
    /// シンボルを検索
    FindResults find(const std::string& symbol_name, 
                     const FindOptions& options = FindOptions());
    
    /// ファイル一覧を設定
    void setFiles(const std::vector<FileInfo>& files) { files_ = files; }
    
private:
    std::vector<FileInfo> files_;
    
    // 内部実装
    FindResults findInFiles(const std::string& symbol, 
                           const FindOptions& options);
    std::vector<SymbolLocation> findInFile(const std::string& filename,
                                          const std::string& content,
                                          const std::string& symbol,
                                          const FindOptions& options);
    
    // 解析ヘルパー
    bool isInComment(const std::string& line, size_t pos);
    bool isInString(const std::string& line, size_t pos);
    UseType detectUseType(const std::string& line, size_t pos, 
                         const std::string& symbol);
    SymbolType detectSymbolType(const std::string& line, size_t pos,
                               const std::string& symbol);
};

//=============================================================================
// 📋 出力マネージャー
//=============================================================================

class FindOutputManager {
public:
    FindOutputManager(bool is_ai_mode = false) : is_ai_mode_(is_ai_mode) {}
    
    /// 結果を表示
    void display(const SymbolFinder::FindResults& results,
                const SymbolFinder::FindOptions& options,
                const std::string& symbol_name);
                
private:
    bool is_ai_mode_;
    
    void displayToTerminal(const SymbolFinder::FindResults& results,
                          const SymbolFinder::FindOptions& options,
                          const std::string& symbol_name);
    void saveToFile(const SymbolFinder::FindResults& results,
                   const std::string& filename,
                   const std::string& symbol_name);
    void displayOmissionInfo(size_t displayed, size_t total,
                           const std::string& filename);
    std::string generateFilename(const std::string& symbol_name);
};

} // namespace nekocode