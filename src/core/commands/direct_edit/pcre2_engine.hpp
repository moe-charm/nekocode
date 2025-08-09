#pragma once

//=============================================================================
// 🐍 PCRE2 Python風エンジン - std::regex完全置き換え
//
// 革命的置換エンジン：
// ✅ Python re.sub() 互換API
// ✅ 安全なエラーハンドリング  
// ✅ 単語境界サポート (\b)
// ✅ C++ std::regex問題完全回避
//=============================================================================

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <string>
#include <vector>
#include <memory>

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 🐍 PCRE2 Python風置換エンジン
//=============================================================================

struct ReplaceResult {
    std::string new_content;
    std::vector<size_t> match_positions;
    std::vector<int> match_lines;
    int total_replacements = 0;
    bool success = true;
    std::string error_message;
    std::string pattern_used;
    std::string replacement_used;
};

class PCRE2Engine {
private:
    pcre2_code* compiled_pattern = nullptr;
    pcre2_match_data* match_data = nullptr;
    std::string last_pattern;
    
public:
    ~PCRE2Engine() {
        cleanup();
    }
    
    void cleanup() {
        if (compiled_pattern) {
            pcre2_code_free(compiled_pattern);
            compiled_pattern = nullptr;
        }
        if (match_data) {
            pcre2_match_data_free(match_data);
            match_data = nullptr;
        }
    }
    
    bool compile(const std::string& pattern) {
        // 同じパターンなら再コンパイル不要
        if (pattern == last_pattern && compiled_pattern) {
            return true;
        }
        
        cleanup();
        last_pattern = pattern;
        
        int error_number;
        PCRE2_SIZE error_offset;
        
        compiled_pattern = pcre2_compile(
            (PCRE2_SPTR)pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,  // options
            &error_number,
            &error_offset,
            nullptr  // compile context
        );
        
        if (!compiled_pattern) {
            return false;
        }
        
        match_data = pcre2_match_data_create_from_pattern(compiled_pattern, nullptr);
        return match_data != nullptr;
    }
    
    std::string get_compile_error(const std::string& pattern) {
        int error_number;
        PCRE2_SIZE error_offset;
        
        pcre2_code* temp_pattern = pcre2_compile(
            (PCRE2_SPTR)pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,
            &error_number,
            &error_offset,
            nullptr
        );
        
        if (!temp_pattern) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(error_number, buffer, sizeof(buffer));
            return std::string((char*)buffer) + " at position " + std::to_string(error_offset);
        }
        
        pcre2_code_free(temp_pattern);
        return "Unknown error";
    }
    
    // Python風 re.sub() - 全置換
    ReplaceResult substitute_all(const std::string& pattern,
                                const std::string& replacement, 
                                const std::string& text) {
        ReplaceResult result;
        result.pattern_used = pattern;
        result.replacement_used = replacement;
        result.new_content = text;
        
        if (!compile(pattern)) {
            result.success = false;
            result.error_message = "Pattern compilation failed: " + get_compile_error(pattern);
            return result;
        }
        
        // バッファサイズ計算（テキストの2倍 + 余裕）
        PCRE2_SIZE output_length = text.length() * 2 + 1024;
        std::vector<PCRE2_UCHAR> output_buffer(output_length);
        
        int rc = pcre2_substitute(
            compiled_pattern,
            (PCRE2_SPTR)text.c_str(),
            text.length(),
            0,                              // start offset
            PCRE2_SUBSTITUTE_GLOBAL,        // 全置換
            match_data,
            nullptr,                        // match context
            (PCRE2_SPTR)replacement.c_str(),
            PCRE2_ZERO_TERMINATED,
            output_buffer.data(),
            &output_length
        );
        
        if (rc < 0) {
            result.success = false;
            result.error_message = "Substitution failed with error code: " + std::to_string(rc);
            return result;
        }
        
        result.new_content = std::string((char*)output_buffer.data(), output_length);
        result.total_replacements = rc;  // 置換回数
        result.success = true;
        
        // マッチ位置を収集
        collect_match_positions(text, result);
        
        return result;
    }
    
private:
    void collect_match_positions(const std::string& text, ReplaceResult& result) {
        if (!compiled_pattern) return;
        
        PCRE2_SIZE start_offset = 0;
        
        while (start_offset < text.length()) {
            int rc = pcre2_match(
                compiled_pattern,
                (PCRE2_SPTR)text.c_str(),
                text.length(),
                start_offset,
                0,
                match_data,
                nullptr
            );
            
            if (rc <= 0) break;
            
            PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
            size_t match_start = ovector[0];
            size_t match_end = ovector[1];
            
            result.match_positions.push_back(match_start);
            
            // 行番号計算
            int line_number = 1 + std::count(text.begin(), text.begin() + match_start, '\n');
            result.match_lines.push_back(line_number);
            
            start_offset = match_end;
            
            // 空マッチの場合1文字進める
            if (match_start == match_end) {
                start_offset++;
            }
        }
    }
};

//=============================================================================
// 🚀 NekoCode統合用Python風API
//=============================================================================

// Python re.sub() 完全互換
inline ReplaceResult re_sub(const std::string& pattern, 
                           const std::string& replacement, 
                           const std::string& text) {
    static PCRE2Engine engine;  // 静的インスタンス（パフォーマンス向上）
    return engine.substitute_all(pattern, replacement, text);
}

// 安全なリテラル置換（フォールバック用）
inline ReplaceResult literal_replace(const std::string& search,
                                   const std::string& replacement,
                                   const std::string& text) {
    ReplaceResult result;
    result.pattern_used = search;
    result.replacement_used = replacement;
    result.new_content = text;
    result.success = true;
    
    size_t pos = 0;
    while ((pos = result.new_content.find(search, pos)) != std::string::npos) {
        result.match_positions.push_back(pos);
        
        // 行番号計算
        int line_number = 1 + std::count(text.begin(), text.begin() + pos, '\n');
        result.match_lines.push_back(line_number);
        
        result.new_content.replace(pos, search.length(), replacement);
        result.total_replacements++;
        
        pos += replacement.length();
    }
    
    return result;
}

// スマート置換（PCRE2失敗時リテラルフォールバック）
inline ReplaceResult smart_replace(const std::string& pattern,
                                  const std::string& replacement,
                                  const std::string& text) {
    // PCRE2で試行
    auto pcre_result = re_sub(pattern, replacement, text);
    
    if (pcre_result.success) {
        return pcre_result;
    }
    
    // 失敗時はリテラル置換
    auto literal_result = literal_replace(pattern, replacement, text);
    literal_result.error_message = "PCRE2 failed, used literal replacement: " + pcre_result.error_message;
    
    return literal_result;
}

} // namespace DirectEdit
} // namespace nekocode