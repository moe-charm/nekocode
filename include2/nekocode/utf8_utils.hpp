#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nekocode {
namespace utf8 {

//=============================================================================
// 🌍 UTF-8 Safe String Utilities
//
// UTF8-CPP + 最小限自前実装戦略:
// - BOM除去
// - UTF-8妥当性チェック  
// - 文字境界安全な処理
// - Unicode文字分類
//=============================================================================

/// BOM (Byte Order Mark) 除去
std::string remove_bom(const std::string& content);

/// UTF-8妥当性チェック（UTF8-CPP使用）
bool is_valid_utf8(const std::string& content);

/// UTF-8文字数カウント（バイト数ではなく文字数）
size_t utf8_length(const std::string& str);

/// 文字境界安全な行分割
std::vector<std::string> split_lines_safe(const std::string& content);

/// 文字境界安全な部分文字列抽出
std::string substr_safe(const std::string& str, size_t start, size_t length = std::string::npos);

//=============================================================================
// 🎯 Unicode Character Classification
//=============================================================================

/// Unicode文字がJavaScript識別子文字かチェック
bool is_js_identifier_char(char32_t codepoint);

/// Unicode文字がC++識別子文字かチェック  
bool is_cpp_identifier_char(char32_t codepoint);

/// ASCII文字かチェック
bool is_ascii(char32_t codepoint);

/// Unicode文字種別
enum class UnicodeCategory {
    ASCII_LETTER,       // A-Z, a-z
    ASCII_DIGIT,        // 0-9
    ASCII_UNDERSCORE,   // _
    UNICODE_LETTER,     // 他言語文字
    UNICODE_DIGIT,      // 他言語数字
    SYMBOL,            // 記号
    WHITESPACE,        // 空白
    EMOJI,             // 絵文字 🎯
    HIRAGANA,          // ひらがな あいうえお
    KATAKANA,          // カタカナ アイウエオ
    KANJI,             // 漢字 日本語漢字
    OTHER              // その他
};

/// Unicode文字分類
UnicodeCategory classify_unicode_char(char32_t codepoint);

/// ASCIIのみかチェック（コード品質チェック）
bool is_ascii_only(const std::string& text);

/// 非ASCII文字の位置と文字をリスト取得
std::vector<std::pair<size_t, char32_t>> find_non_ascii_chars(const std::string& text);

//=============================================================================
// 🔧 UTF-8 Conversion Utilities
//=============================================================================

/// UTF-8 → UTF-32変換
std::u32string utf8_to_utf32(const std::string& utf8_str);

/// UTF-32 → UTF-8変換
std::string utf32_to_utf8(const std::u32string& utf32_str);

/// UTF-8文字列から1文字ずつ取得するイテレータ
class UTF8Iterator {
private:
    const std::string& str_;
    size_t pos_;
    
public:
    explicit UTF8Iterator(const std::string& str, size_t pos = 0);
    
    char32_t operator*() const;
    UTF8Iterator& operator++();
    UTF8Iterator operator++(int);
    
    bool operator==(const UTF8Iterator& other) const;
    bool operator!=(const UTF8Iterator& other) const;
    
    size_t position() const { return pos_; }
    bool at_end() const;
};

//=============================================================================
// 🎯 Language-Specific String Processing
//=============================================================================

/// JavaScript用文字列処理
namespace javascript {
    /// JavaScript識別子として有効かチェック
    bool is_valid_identifier(const std::string& name);
    
    /// JavaScript文字列リテラル除去（Unicode考慮）
    std::string remove_string_literals(const std::string& content);
    
    /// JavaScript正規表現リテラル除去
    std::string remove_regex_literals(const std::string& content);
}

/// C++用文字列処理
namespace cpp {
    /// C++識別子として有効かチェック
    bool is_valid_identifier(const std::string& name);
    
    /// C++文字列リテラル除去（Raw文字列含む）
    std::string remove_string_literals(const std::string& content);
    
    /// C++コメント除去（// と /* */ の両方）
    std::string remove_comments(const std::string& content);
    
    /// プリプロセッサディレクティブ除去
    std::string remove_preprocessor(const std::string& content);
}

//=============================================================================
// 🚨 Encoding Detection
//=============================================================================

/// ファイルエンコーディング検出結果
enum class Encoding {
    UTF8,           // UTF-8
    UTF8_BOM,       // UTF-8 with BOM
    UTF16_LE,       // UTF-16 Little Endian
    UTF16_BE,       // UTF-16 Big Endian  
    SHIFT_JIS,      // Shift_JIS (日本語)
    EUC_JP,         // EUC-JP (日本語)
    ASCII,          // 純粋ASCII
    UNKNOWN         // 不明
};

/// 簡易エンコーディング検出
Encoding detect_encoding(const std::string& content);

/// エンコーディング名を文字列で取得
std::string encoding_to_string(Encoding encoding);

//=============================================================================
// 🎯 Safe File Reading with Encoding Handling
//=============================================================================

/// UTF-8として安全にファイル読み込み
struct SafeFileContent {
    std::string content;
    Encoding detected_encoding;
    bool conversion_success;
    std::string error_message;
};

/// ファイルを安全に読み込み、UTF-8に変換
SafeFileContent read_file_safe_utf8(const std::string& file_path);

} // namespace utf8
} // namespace nekocode