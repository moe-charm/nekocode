//=============================================================================
// 🌍 UTF-8 Safe String Utilities Implementation
//
// UTF8-CPP + 最小限自前実装戦略の実装
//=============================================================================

#include "nekocode/utf8_utils.hpp"
#include <utf8.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace nekocode {
namespace utf8 {

//=============================================================================
// 🎯 Basic UTF-8 Operations
//=============================================================================

std::string remove_bom(const std::string& content) {
    if (content.size() >= 3 && 
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }
    return content;
}

bool is_valid_utf8(const std::string& content) {
    try {
        return ::utf8::is_valid(content.begin(), content.end());
    } catch (...) {
        return false;
    }
}

size_t utf8_length(const std::string& str) {
    try {
        return ::utf8::distance(str.begin(), str.end());
    } catch (...) {
        return str.length(); // fallback to byte length
    }
}

std::vector<std::string> split_lines_safe(const std::string& content) {
    std::vector<std::string> lines;
    
    if (!is_valid_utf8(content)) {
        // Invalid UTF-8, use simple byte-based splitting
        std::stringstream ss(content);
        std::string line;
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
        return lines;
    }
    
    // UTF-8 safe line splitting
    auto it = content.begin();
    auto line_start = it;
    
    while (it != content.end()) {
        char32_t codepoint = ::utf8::next(it, content.end());
        
        if (codepoint == '\n') {
            // Found newline, extract line
            std::string line(line_start, it - 1); // -1 to exclude \n
            lines.push_back(line);
            line_start = it;
        } else if (codepoint == '\r') {
            // Handle \r\n or standalone \r
            auto next_it = it;
            if (next_it != content.end()) {
                char32_t next_char = ::utf8::peek_next(next_it, content.end());
                if (next_char == '\n') {
                    ::utf8::next(it, content.end()); // consume \n
                }
            }
            std::string line(line_start, it - (next_it != it ? 2 : 1));
            lines.push_back(line);
            line_start = it;
        }
    }
    
    // Add final line if exists
    if (line_start != content.end()) {
        std::string line(line_start, content.end());
        lines.push_back(line);
    }
    
    return lines;
}

std::string substr_safe(const std::string& str, size_t start, size_t length) {
    if (!is_valid_utf8(str)) {
        // Fallback to byte-based substring
        return str.substr(start, length);
    }
    
    try {
        auto it = str.begin();
        auto start_it = str.begin();
        auto end_it = str.end();
        
        // Advance to start position
        ::utf8::advance(start_it, start, str.end());
        
        if (length != std::string::npos) {
            end_it = start_it;
            ::utf8::advance(end_it, length, str.end());
        }
        
        return std::string(start_it, end_it);
    } catch (...) {
        return str.substr(start, length);
    }
}

//=============================================================================
// 🎯 Unicode Character Classification
//=============================================================================

bool is_js_identifier_char(char32_t codepoint) {
    // JavaScript identifier rules (simplified)
    if ((codepoint >= 'a' && codepoint <= 'z') ||
        (codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= '0' && codepoint <= '9') ||
        codepoint == '_' || codepoint == '$') {
        return true;
    }
    
    // Unicode letters (basic check)
    // For full compliance, would need Unicode character database
    return (codepoint >= 0x00C0 && codepoint <= 0x1FFF) ||  // Latin Extended, Greek, etc.
           (codepoint >= 0x3040 && codepoint <= 0x309F) ||  // Hiragana
           (codepoint >= 0x30A0 && codepoint <= 0x30FF) ||  // Katakana
           (codepoint >= 0x4E00 && codepoint <= 0x9FFF);    // CJK Unified Ideographs
}

bool is_cpp_identifier_char(char32_t codepoint) {
    // C++ identifier rules (more restrictive than JS)
    if ((codepoint >= 'a' && codepoint <= 'z') ||
        (codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= '0' && codepoint <= '9') ||
        codepoint == '_') {
        return true;
    }
    
    // C++11+ allows Unicode identifier characters
    return (codepoint >= 0x00C0 && codepoint <= 0x1FFF) ||
           (codepoint >= 0x3040 && codepoint <= 0x309F) ||
           (codepoint >= 0x30A0 && codepoint <= 0x30FF) ||
           (codepoint >= 0x4E00 && codepoint <= 0x9FFF);
}

bool is_ascii(char32_t codepoint) {
    return codepoint <= 0x7F;
}

UnicodeCategory classify_unicode_char(char32_t codepoint) {
    if (codepoint >= 'A' && codepoint <= 'Z' ||
        codepoint >= 'a' && codepoint <= 'z') {
        return UnicodeCategory::ASCII_LETTER;
    }
    
    if (codepoint >= '0' && codepoint <= '9') {
        return UnicodeCategory::ASCII_DIGIT;
    }
    
    if (codepoint == '_') {
        return UnicodeCategory::ASCII_UNDERSCORE;
    }
    
    if (codepoint == ' ' || codepoint == '\t' || 
        codepoint == '\n' || codepoint == '\r') {
        return UnicodeCategory::WHITESPACE;
    }
    
    // 非ASCII文字の簡易分類
    if (codepoint > 127) {
        // 日本語範囲
        if ((codepoint >= 0x3040 && codepoint <= 0x309F)) {
            return UnicodeCategory::HIRAGANA;
        }
        if ((codepoint >= 0x30A0 && codepoint <= 0x30FF)) {
            return UnicodeCategory::KATAKANA;
        }
        if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF)) {
            return UnicodeCategory::KANJI;
        }
        // 絵文字範囲（主要のみ）
        if ((codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||
            (codepoint >= 0x1F300 && codepoint <= 0x1F5FF)) {
            return UnicodeCategory::EMOJI;
        }
    }
    
    // Basic Unicode ranges
    if ((codepoint >= 0x00C0 && codepoint <= 0x1FFF)) {
        return UnicodeCategory::UNICODE_LETTER;
    }
    
    // Unicode digits (basic ranges)
    if ((codepoint >= 0xFF10 && codepoint <= 0xFF19)) { // Full-width digits
        return UnicodeCategory::UNICODE_DIGIT;
    }
    
    return UnicodeCategory::OTHER;
}

//=============================================================================
// 🎯 Simple ASCII Code Quality Check
//=============================================================================

bool is_ascii_only(const std::string& text) {
    for (char c : text) {
        if (static_cast<unsigned char>(c) > 127) {
            return false;
        }
    }
    return true;
}

std::vector<std::pair<size_t, char32_t>> find_non_ascii_chars(const std::string& text) {
    std::vector<std::pair<size_t, char32_t>> result;
    
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c > 127) {
            // UTF-8の簡易デコード（第1バイトのみ）
            char32_t codepoint = c;
            result.emplace_back(i, codepoint);
        }
    }
    
    return result;
}

//=============================================================================
// 🔧 UTF-8 Conversion Utilities
//=============================================================================

std::u32string utf8_to_utf32(const std::string& utf8_str) {
    std::u32string result;
    
    try {
        auto it = utf8_str.begin();
        while (it != utf8_str.end()) {
            char32_t codepoint = ::utf8::next(it, utf8_str.end());
            result.push_back(codepoint);
        }
    } catch (...) {
        // Conversion failed, return empty
        result.clear();
    }
    
    return result;
}

std::string utf32_to_utf8(const std::u32string& utf32_str) {
    std::string result;
    
    try {
        for (char32_t codepoint : utf32_str) {
            ::utf8::append(codepoint, std::back_inserter(result));
        }
    } catch (...) {
        // Conversion failed, return empty
        result.clear();
    }
    
    return result;
}

//=============================================================================
// 🎯 UTF8Iterator Implementation
//=============================================================================

UTF8Iterator::UTF8Iterator(const std::string& str, size_t pos) 
    : str_(str), pos_(pos) {}

char32_t UTF8Iterator::operator*() const {
    if (at_end()) return 0;
    
    try {
        auto it = str_.begin() + pos_;
        return ::utf8::peek_next(it, str_.end());
    } catch (...) {
        return 0;
    }
}

UTF8Iterator& UTF8Iterator::operator++() {
    if (!at_end()) {
        try {
            auto it = str_.begin() + pos_;
            ::utf8::next(it, str_.end());
            pos_ = it - str_.begin();
        } catch (...) {
            pos_ = str_.length(); // Move to end on error
        }
    }
    return *this;
}

UTF8Iterator UTF8Iterator::operator++(int) {
    UTF8Iterator tmp = *this;
    ++(*this);
    return tmp;
}

bool UTF8Iterator::operator==(const UTF8Iterator& other) const {
    return &str_ == &other.str_ && pos_ == other.pos_;
}

bool UTF8Iterator::operator!=(const UTF8Iterator& other) const {
    return !(*this == other);
}

bool UTF8Iterator::at_end() const {
    return pos_ >= str_.length();
}

//=============================================================================
// 🎯 Language-Specific String Processing
//=============================================================================

namespace javascript {

bool is_valid_identifier(const std::string& name) {
    if (name.empty()) return false;
    
    auto utf32_name = utf8_to_utf32(name);
    if (utf32_name.empty()) return false;
    
    // First character: letter, underscore, or dollar sign
    char32_t first = utf32_name[0];
    if (!is_js_identifier_char(first) && first != '$' && first != '_') {
        return false;
    }
    
    // Remaining characters: letters, digits, underscore, dollar sign
    for (size_t i = 1; i < utf32_name.size(); ++i) {
        if (!is_js_identifier_char(utf32_name[i]) && utf32_name[i] != '$') {
            return false;
        }
    }
    
    return true;
}

std::string remove_string_literals(const std::string& content) {
    std::string result = content;
    
    // Remove double-quoted strings
    std::regex double_quote_regex(R"("(?:[^"\\]|\\.)*")");
    result = std::regex_replace(result, double_quote_regex, "\"\"");
    
    // Remove single-quoted strings
    std::regex single_quote_regex(R"('(?:[^'\\]|\\.)*')");
    result = std::regex_replace(result, single_quote_regex, "''");
    
    // Remove template literals
    std::regex template_regex(R"(`(?:[^`\\]|\\.)*`)");
    result = std::regex_replace(result, template_regex, "``");
    
    return result;
}

std::string remove_regex_literals(const std::string& content) {
    // Simple regex literal removal (basic implementation)
    std::regex regex_literal(R"(/(?:[^/\\]|\\.)+/[gimsuvy]*)");
    return std::regex_replace(content, regex_literal, "//");
}

} // namespace javascript

namespace cpp {

bool is_valid_identifier(const std::string& name) {
    if (name.empty()) return false;
    
    auto utf32_name = utf8_to_utf32(name);
    if (utf32_name.empty()) return false;
    
    // First character: letter or underscore
    char32_t first = utf32_name[0];
    if (!is_cpp_identifier_char(first) && first != '_') {
        return false;
    }
    
    // Remaining characters: letters, digits, underscore
    for (size_t i = 1; i < utf32_name.size(); ++i) {
        if (!is_cpp_identifier_char(utf32_name[i])) {
            return false;
        }
    }
    
    return true;
}

std::string remove_string_literals(const std::string& content) {
    std::string result = content;
    
    // Remove raw string literals R"(...)"
    std::regex raw_string_regex(R"(R\"[^(]*\(.*?\)[^"]*\")");
    result = std::regex_replace(result, raw_string_regex, "R\"\"");
    
    // Remove regular string literals
    std::regex double_quote_regex(R"("(?:[^"\\]|\\.)*")");
    result = std::regex_replace(result, double_quote_regex, "\"\"");
    
    // Remove character literals
    std::regex char_regex(R"('(?:[^'\\]|\\.)*')");
    result = std::regex_replace(result, char_regex, "''");
    
    return result;
}

std::string remove_comments(const std::string& content) {
    std::string result = content;
    
    // Remove single-line comments
    std::regex single_comment_regex(R"(//.*$)", std::regex_constants::multiline);
    result = std::regex_replace(result, single_comment_regex, "");
    
    // Remove multi-line comments
    std::regex multi_comment_regex(R"(/\*[\s\S]*?\*/)");
    result = std::regex_replace(result, multi_comment_regex, "");
    
    return result;
}

std::string remove_preprocessor(const std::string& content) {
    std::regex preprocessor_regex(R"(^\s*#.*$)", std::regex_constants::multiline);
    return std::regex_replace(content, preprocessor_regex, "");
}

} // namespace cpp

//=============================================================================
// 🚨 Encoding Detection
//=============================================================================

Encoding detect_encoding(const std::string& content) {
    if (content.empty()) return Encoding::ASCII;
    
    // Check for BOM
    if (content.size() >= 3 && 
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return Encoding::UTF8_BOM;
    }
    
    if (content.size() >= 2) {
        if (static_cast<unsigned char>(content[0]) == 0xFF &&
            static_cast<unsigned char>(content[1]) == 0xFE) {
            return Encoding::UTF16_LE;
        }
        if (static_cast<unsigned char>(content[0]) == 0xFE &&
            static_cast<unsigned char>(content[1]) == 0xFF) {
            return Encoding::UTF16_BE;
        }
    }
    
    // Check if valid UTF-8
    if (is_valid_utf8(content)) {
        // Check if pure ASCII
        bool is_pure_ascii = true;
        for (unsigned char c : content) {
            if (c > 0x7F) {
                is_pure_ascii = false;
                break;
            }
        }
        return is_pure_ascii ? Encoding::ASCII : Encoding::UTF8;
    }
    
    // Basic Japanese encoding detection (simplified)
    // In practice, would use more sophisticated detection
    return Encoding::UNKNOWN;
}

std::string encoding_to_string(Encoding encoding) {
    switch (encoding) {
        case Encoding::UTF8: return "UTF-8";
        case Encoding::UTF8_BOM: return "UTF-8 with BOM";
        case Encoding::UTF16_LE: return "UTF-16 Little Endian";
        case Encoding::UTF16_BE: return "UTF-16 Big Endian";
        case Encoding::SHIFT_JIS: return "Shift_JIS";
        case Encoding::EUC_JP: return "EUC-JP";
        case Encoding::ASCII: return "ASCII";
        case Encoding::UNKNOWN: return "Unknown";
    }
    return "Unknown";
}

//=============================================================================
// 🎯 Safe File Reading with Encoding Handling
//=============================================================================

SafeFileContent read_file_safe_utf8(const std::string& file_path) {
    SafeFileContent result;
    
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            result.conversion_success = false;
            result.error_message = "Cannot open file: " + file_path;
            return result;
        }
        
        // Read file content
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::string raw_content(size, '\0');
        file.read(&raw_content[0], size);
        
        // Detect encoding
        result.detected_encoding = detect_encoding(raw_content);
        
        // Convert to UTF-8 if needed
        switch (result.detected_encoding) {
            case Encoding::UTF8_BOM:
                result.content = remove_bom(raw_content);
                result.conversion_success = true;
                break;
                
            case Encoding::UTF8:
            case Encoding::ASCII:
                result.content = raw_content;
                result.conversion_success = true;
                break;
                
            case Encoding::UTF16_LE:
            case Encoding::UTF16_BE:
            case Encoding::SHIFT_JIS:
            case Encoding::EUC_JP:
                // For now, just copy as-is (would need iconv or similar for proper conversion)
                result.content = raw_content;
                result.conversion_success = false;
                result.error_message = "Encoding conversion not implemented: " + encoding_to_string(result.detected_encoding);
                break;
                
            case Encoding::UNKNOWN:
                result.content = raw_content;
                result.conversion_success = false;
                result.error_message = "Unknown encoding detected";
                break;
        }
        
    } catch (const std::exception& e) {
        result.conversion_success = false;
        result.error_message = "File reading error: " + std::string(e.what());
    }
    
    return result;
}

} // namespace utf8
} // namespace nekocode