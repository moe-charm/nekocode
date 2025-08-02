//=============================================================================
// 🔍 Find Command - find コマンドの実装
//=============================================================================

#include "nekocode/symbol_finder.hpp"
#include "nekocode/session_manager.hpp"
#include <iostream>
#include <algorithm>

namespace nekocode {

class FindCommand {
public:
    FindCommand(SessionManager& session, const std::string& session_id, bool is_ai_mode = false) 
        : session_(session), session_id_(session_id), is_ai_mode_(is_ai_mode) {}
    
    int execute(const std::vector<std::string>& args) {
        // 引数チェック
        if (args.size() < 2) {
            showUsage();
            return 1;
        }
        
        std::string symbol_name = args[1];
        SymbolFinder::FindOptions options = parseOptions(args);
        
        // Claude Code対応
        if (is_ai_mode_) {
            options.display_limit = 50;
        }
        
        // セッションからファイル情報を取得
        auto files = session_.getProjectFiles(session_id_);
        if (files.empty()) {
            std::cerr << "❌ プロジェクトファイルが見つかりません。\n";
            std::cerr << "   session-create でセッションを作成してください。\n";
            return 1;
        }
        
        // 検索実行
        SymbolFinder finder;
        finder.setFiles(files);
        
        auto results = finder.find(symbol_name, options);
        
        // 結果表示
        FindOutputManager output(is_ai_mode_);
        output.display(results, options, symbol_name);
        
        return 0;
    }
    
private:
    SessionManager& session_;
    std::string session_id_;
    bool is_ai_mode_;
    
    SymbolFinder::FindOptions parseOptions(const std::vector<std::string>& args) {
        SymbolFinder::FindOptions options;
        
        for (size_t i = 2; i < args.size(); ++i) {
            const auto& arg = args[i];
            
            // -f / --function: 関数のみ
            if (arg == "-f" || arg == "--function") {
                options.type = SymbolFinder::SymbolType::FUNCTION;
            }
            // -v / --variable: 変数のみ
            else if (arg == "-v" || arg == "--variable") {
                options.type = SymbolFinder::SymbolType::VARIABLE;
            }
            // -o / --output: ファイル出力
            else if ((arg == "-o" || arg == "--output") && i + 1 < args.size()) {
                options.output_file = args[++i];
            }
            // --limit: 表示上限（= 記法と スペース記法の両方対応）
            else if (arg.find("--limit=") == 0) {
                options.display_limit = std::stoul(arg.substr(8));
            }
            else if (arg == "--limit" && i + 1 < args.size()) {
                options.display_limit = std::stoul(args[++i]);
            }
            // --context: コンテキスト表示
            else if (arg == "--context" && i + 1 < args.size()) {
                options.show_context = true;
                options.context_lines = std::stoul(args[++i]);
            }
            // --debug: デバッグ出力有効化
            else if (arg == "--debug") {
                options.debug = true;
            }
            // パス指定（オプションでないもの、ただし数字のみは除外）
            else if (!arg.empty() && arg[0] != '-') {
                // 数字のみの引数は除外（--limitやオプションの値の可能性）
                bool is_only_digits = !arg.empty() && std::all_of(arg.begin(), arg.end(), ::isdigit);
                if (!is_only_digits) {
                    options.search_paths.push_back(arg);
                }
            }
        }
        
        return options;
    }
    
    void showUsage() {
        std::cerr << "\n使用法: find <シンボル名> [オプション] [パス...]\n\n";
        std::cerr << "オプション:\n";
        std::cerr << "  -f, --function    関数のみを検索\n";
        std::cerr << "  -v, --variable    変数のみを検索\n";
        std::cerr << "  -o, --output FILE 結果をファイルに出力\n";
        std::cerr << "  --limit N         表示上限を設定（デフォルト: 50）\n";
        std::cerr << "  --context N       前後N行を表示\n";
        std::cerr << "  --debug           詳細なデバッグ情報を表示\n\n";
        std::cerr << "例:\n";
        std::cerr << "  find handleClick              # handleClick を検索\n";
        std::cerr << "  find data -v                  # data 変数のみ検索\n";
        std::cerr << "  find processData src/         # src/ 内で検索\n";
        std::cerr << "  find test -o results.txt      # 結果をファイルに出力\n";
        std::cerr << "  find class --debug            # デバッグ情報付きで検索\n\n";
    }
};

// グローバル関数として公開
int executeFindCommand(SessionManager& session,
                      const std::string& session_id,
                      const std::vector<std::string>& args,
                      bool is_ai_mode) {
    FindCommand cmd(session, session_id, is_ai_mode);
    return cmd.execute(args);
}

} // namespace nekocode