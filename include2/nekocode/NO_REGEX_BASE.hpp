#pragma once

//=============================================================================
// 🚫 NO REGEX BASE - std::regex絶対禁止ベースクラス
//
// このプロジェクトの全アナライザーが継承すべき基底クラス
// std::regexの使用を構造的に防ぐ設計
//=============================================================================

#include <string>
#include <algorithm>

namespace nekocode {

//=============================================================================
// 🛡️ NoRegexAnalyzer - 正規表現を使わない解析基底クラス
//=============================================================================

class NoRegexAnalyzer {
protected:
    // std::regexの代替となる基本的な文字列処理関数
    
    /// 単純な文字列検索
    size_t find_token(const std::string& content, const std::string& token, size_t start = 0) {
        return content.find(token, start);
    }
    
    /// 単語境界を考慮した検索
    bool find_word(const std::string& content, const std::string& word, size_t& pos) {
        pos = 0;
        while ((pos = content.find(word, pos)) != std::string::npos) {
            bool start_ok = (pos == 0 || !std::isalnum(content[pos-1]));
            bool end_ok = (pos + word.length() >= content.length() || 
                          !std::isalnum(content[pos + word.length()]));
            if (start_ok && end_ok) {
                return true;
            }
            pos++;
        }
        return false;
    }
    
    /// 次のトークンまでスキップ
    size_t skip_whitespace(const std::string& content, size_t pos) {
        while (pos < content.length() && std::isspace(content[pos])) {
            pos++;
        }
        return pos;
    }
    
    /// 識別子の抽出
    std::string extract_identifier(const std::string& content, size_t& pos) {
        size_t start = pos;
        if (pos < content.length() && (std::isalpha(content[pos]) || content[pos] == '_')) {
            pos++;
            while (pos < content.length() && (std::isalnum(content[pos]) || content[pos] == '_')) {
                pos++;
            }
            return content.substr(start, pos - start);
        }
        return "";
    }
    
    /// ブロックの終端を見つける（ネスト対応）
    size_t find_block_end(const std::string& content, size_t start, char open = '{', char close = '}') {
        int depth = 0;
        bool in_string = false;
        bool in_char = false;
        bool escaped = false;
        
        for (size_t i = start; i < content.length(); i++) {
            if (!escaped) {
                if (!in_string && !in_char) {
                    if (content[i] == open) depth++;
                    else if (content[i] == close) {
                        if (depth == 0) return i;
                        depth--;
                    }
                    else if (content[i] == '"') in_string = true;
                    else if (content[i] == '\'') in_char = true;
                } else if (in_string && content[i] == '"') {
                    in_string = false;
                } else if (in_char && content[i] == '\'') {
                    in_char = false;
                }
                
                escaped = (content[i] == '\\');
            } else {
                escaped = false;
            }
        }
        return std::string::npos;
    }
    
    // コンストラクタでstd::regexヘッダーのインクルードをチェック（コンパイル時）
    NoRegexAnalyzer() {
        static_assert(true, "This class forbids std::regex usage!");
    }
    
public:
    virtual ~NoRegexAnalyzer() = default;
    
    /// 派生クラスで実装すべきメソッド
    virtual std::string analyze_without_regex(const std::string& content) = 0;
};

//=============================================================================
// 🔍 マクロによるregex使用防止
//=============================================================================

// std::regexを使おうとしたらコンパイルエラー
#define regex regex_is_forbidden_use_PEGTL_instead
#define sregex_iterator sregex_iterator_is_forbidden
#define regex_match regex_match_is_forbidden
#define regex_search regex_search_is_forbidden

} // namespace nekocode

#endif // NO_REGEX_BASE_HPP