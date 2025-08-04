#pragma once

//=============================================================================
// 🎯 File Size Reporter - 大ファイル処理時の進捗表示
//
// Claude Code向け：大きいファイルでも処理を続行することを明確に示す
//=============================================================================

#include <string>
#include <cstdint>
#include <iostream>
#include <iomanip>

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_quiet_mode;

namespace nekocode {

class FileSizeReporter {
public:
    // ファイルサイズをMB単位でフォーマット
    static std::string format_size(size_t size_bytes) {
        const double mb = size_bytes / (1024.0 * 1024.0);
        
        if (mb >= 1.0) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << mb << "MB";
            return oss.str();
        } else {
            const double kb = size_bytes / 1024.0;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << kb << "KB";
            return oss.str();
        }
    }
    
    // 推定処理時間を計算（非常に単純な推定）
    static double estimate_processing_time(size_t size_bytes) {
        // 仮定: 1MBあたり1秒（実際はもっと速い）
        const double mb = size_bytes / (1024.0 * 1024.0);
        return std::max(0.1, mb * 0.5); // 最低0.1秒
    }
    
    // 大ファイル処理開始メッセージ
    static void report_large_file_start(const std::string& filename, size_t size_bytes) {
        const std::string size_str = format_size(size_bytes);
        const double estimated_time = estimate_processing_time(size_bytes);
        
        // Claude Code向け：処理継続中であることを明確に示す
        if (!g_quiet_mode) {
            std::cerr << "📄 Processing large file: " << filename 
                      << " (" << size_str << ")" << std::endl;
            std::cerr << "⏱️  Estimated time: ~" << std::fixed << std::setprecision(1) 
                      << estimated_time << " seconds. Processing..." << std::endl;
        }
    }
    
    // 大ファイル処理完了メッセージ
    static void report_large_file_complete(const std::string& filename) {
        if (!g_quiet_mode) {
            std::cerr << "✅ Large file processed: " << filename << std::endl;
        }
    }
    
    // 大ファイルかどうかの判定（500KB以上）
    static bool is_large_file(size_t size_bytes) {
        return size_bytes >= (500 * 1024); // 500KB
    }
};

} // namespace nekocode