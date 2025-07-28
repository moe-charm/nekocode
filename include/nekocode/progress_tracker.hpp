//=============================================================================
// 📊 ProgressTracker - リアルタイム進捗表示＆ファイル出力
//=============================================================================

#pragma once

#include <string>
#include <chrono>
#include <fstream>
#include <memory>
#include <filesystem>

namespace nekocode {

//=============================================================================
// 📊 ProgressTracker - Claude Code完全対応進捗管理
//=============================================================================

class ProgressTracker {
public:
    explicit ProgressTracker(const std::string& session_id = "", bool enable_stderr = false);
    ~ProgressTracker();

    // 🚀 進捗開始
    void start(size_t total_files, const std::string& target_path);
    
    // 📝 進捗更新
    void update(size_t current_file, const std::string& current_filename, 
                size_t file_size_bytes, const std::string& status = "OK");
    
    // ⚠️ エラー記録
    void error(size_t current_file, const std::string& current_filename, 
               const std::string& error_message);
    
    // ⏭️ スキップ記録
    void skip(size_t current_file, const std::string& current_filename, 
              const std::string& skip_reason);
    
    // 🎉 完了
    void complete(size_t success_count, size_t error_count, size_t skip_count);
    
    // 📄 進捗ファイルパス取得
    std::string get_progress_file_path() const;
    
    // ⏱️ 処理速度・ETA計算
    double get_files_per_second() const;
    std::string get_eta_string() const;
    std::string get_elapsed_time_string() const;

private:
    // 設定
    std::string session_id_;
    bool enable_stderr_;
    
    // 進捗状態
    size_t total_files_;
    size_t current_files_;
    std::string target_path_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_update_;
    
    // ファイル出力
    std::unique_ptr<std::ofstream> progress_file_;
    std::filesystem::path progress_file_path_;
    
    // 統計
    size_t success_count_;
    size_t error_count_;
    size_t skip_count_;
    
    // 🔧 内部関数
    void write_to_file(const std::string& message);
    void write_to_stderr(const std::string& message);
    std::string get_timestamp() const;
    std::string format_size(size_t bytes) const;
    std::string format_duration(std::chrono::seconds duration) const;
};

//=============================================================================
// 🎯 便利な進捗表示ヘルパー
//=============================================================================

// セッション作成時の進捗表示用ラッパー
class SessionProgressTracker {
public:
    SessionProgressTracker(const std::string& session_id, bool enable_progress);
    
    void start_directory_analysis(const std::filesystem::path& target_path, size_t file_count);
    void update_file_analysis(const std::string& filename, size_t file_size, bool success, const std::string& error = "");
    void complete_analysis();
    
    std::string get_progress_file_path() const;

private:
    std::unique_ptr<ProgressTracker> tracker_;
    size_t current_file_index_;
    size_t success_count_;
    size_t error_count_;
    size_t skip_count_;
};

} // namespace nekocode