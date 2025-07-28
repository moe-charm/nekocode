//=============================================================================
// 📊 ProgressTracker実装 - リアルタイム進捗表示＆ファイル出力
//=============================================================================

#include "nekocode/progress_tracker.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>

namespace nekocode {

//=============================================================================
// 📊 ProgressTracker実装
//=============================================================================

ProgressTracker::ProgressTracker(const std::string& session_id, bool enable_stderr)
    : session_id_(session_id), enable_stderr_(enable_stderr), 
      total_files_(0), current_files_(0),
      success_count_(0), error_count_(0), skip_count_(0) {
}

ProgressTracker::~ProgressTracker() {
    if (progress_file_ && progress_file_->is_open()) {
        progress_file_->close();
    }
}

void ProgressTracker::start(size_t total_files, const std::string& target_path) {
    total_files_ = total_files;
    current_files_ = 0;
    target_path_ = target_path;
    start_time_ = std::chrono::steady_clock::now();
    last_update_ = start_time_;
    
    success_count_ = 0;
    error_count_ = 0;
    skip_count_ = 0;
    
    // プログレスファイル作成
    if (!session_id_.empty()) {
        progress_file_path_ = std::filesystem::path("sessions") / (session_id_ + "_progress.txt");
        
        // sessionsディレクトリ作成
        std::filesystem::create_directories("sessions");
        
        progress_file_ = std::make_unique<std::ofstream>(progress_file_path_);
        if (progress_file_->is_open()) {
            write_to_file("[" + get_timestamp() + "] START: " + 
                         std::to_string(total_files) + " files | Target: " + target_path);
        }
    }
    
    // stderr出力
    if (enable_stderr_) {
        write_to_stderr("🚀 Starting analysis: " + std::to_string(total_files) + " files in " + target_path);
    }
}

void ProgressTracker::update(size_t current_file, const std::string& current_filename, 
                           size_t file_size_bytes, const std::string& status) {
    current_files_ = current_file;
    last_update_ = std::chrono::steady_clock::now();
    
    if (status == "OK") {
        success_count_++;
    }
    
    // ファイル出力
    if (progress_file_ && progress_file_->is_open()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            last_update_ - start_time_).count() / 1000.0;
            
        std::ostringstream oss;
        oss << "[" << get_timestamp() << "] PROCESSING: " 
            << current_file << "/" << total_files_ 
            << " (" << std::fixed << std::setprecision(1) << (100.0 * current_file / total_files_) << "%) | "
            << current_filename << " (" << format_size(file_size_bytes) << ") | "
            << status << " | " << std::fixed << std::setprecision(1) << elapsed << "s";
            
        write_to_file(oss.str());
    }
    
    // stderr出力（頻度制限：10ファイルに1回）
    if (enable_stderr_ && (current_file % 10 == 0 || current_file == total_files_)) {
        double progress_percent = 100.0 * current_file / total_files_;
        double rate = get_files_per_second();
        std::string eta = get_eta_string();
        
        std::ostringstream oss;
        oss << "Processing " << current_file << "/" << total_files_ 
            << " (" << std::fixed << std::setprecision(1) << progress_percent << "%) | "
            << "Rate: " << std::fixed << std::setprecision(1) << rate << "/sec | "
            << "ETA: " << eta;
            
        write_to_stderr(oss.str());
        
        // 現在ファイル詳細（長いファイル名は省略）
        std::string display_filename = current_filename;
        if (display_filename.length() > 50) {
            display_filename = "..." + display_filename.substr(display_filename.length() - 47);
        }
        write_to_stderr("Current: " + display_filename + " (" + format_size(file_size_bytes) + ")");
    }
}

void ProgressTracker::error(size_t current_file, const std::string& current_filename, 
                          const std::string& error_message) {
    current_files_ = current_file;
    error_count_++;
    
    if (progress_file_ && progress_file_->is_open()) {
        std::ostringstream oss;
        oss << "[" << get_timestamp() << "] ERROR: " 
            << current_file << "/" << total_files_ 
            << " (" << std::fixed << std::setprecision(1) << (100.0 * current_file / total_files_) << "%) | "
            << current_filename << " | " << error_message;
            
        write_to_file(oss.str());
    }
    
    if (enable_stderr_) {
        write_to_stderr("❌ ERROR: " + current_filename + " - " + error_message);
    }
}

void ProgressTracker::skip(size_t current_file, const std::string& current_filename, 
                          const std::string& skip_reason) {
    current_files_ = current_file;
    skip_count_++;
    
    if (progress_file_ && progress_file_->is_open()) {
        std::ostringstream oss;
        oss << "[" << get_timestamp() << "] SKIP: " 
            << current_file << "/" << total_files_ 
            << " (" << std::fixed << std::setprecision(1) << (100.0 * current_file / total_files_) << "%) | "
            << current_filename << " | " << skip_reason;
            
        write_to_file(oss.str());
    }
    
    if (enable_stderr_) {
        write_to_stderr("⏭️ SKIP: " + current_filename + " - " + skip_reason);
    }
}

void ProgressTracker::complete(size_t success_count, size_t error_count, size_t skip_count) {
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time_);
    
    success_count_ = success_count;
    error_count_ = error_count;
    skip_count_ = skip_count;
    
    if (progress_file_ && progress_file_->is_open()) {
        std::ostringstream oss;
        oss << "[" << get_timestamp() << "] COMPLETE: " 
            << total_files_ << "/" << total_files_ << " (100%) | "
            << "Total: " << format_duration(total_duration) << " | "
            << "Success: " << success_count << " | "
            << "Errors: " << error_count << " | "
            << "Skipped: " << skip_count;
            
        write_to_file(oss.str());
        progress_file_->close();
    }
    
    if (enable_stderr_) {
        write_to_stderr("🎉 Analysis complete! " + 
                       std::to_string(success_count) + " success, " +
                       std::to_string(error_count) + " errors, " +
                       std::to_string(skip_count) + " skipped in " +
                       format_duration(total_duration));
    }
}

std::string ProgressTracker::get_progress_file_path() const {
    return progress_file_path_.string();
}

double ProgressTracker::get_files_per_second() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count() / 1000.0;
    
    if (elapsed < 0.1) return 0.0;
    return static_cast<double>(current_files_) / elapsed;
}

std::string ProgressTracker::get_eta_string() const {
    if (current_files_ == 0) return "∞";
    
    double rate = get_files_per_second();
    if (rate < 0.1) return "∞";
    
    size_t remaining_files = total_files_ - current_files_;
    double eta_seconds = remaining_files / rate;
    
    return format_duration(std::chrono::seconds(static_cast<long>(eta_seconds)));
}

std::string ProgressTracker::get_elapsed_time_string() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return format_duration(elapsed);
}

// 🔧 内部関数実装

void ProgressTracker::write_to_file(const std::string& message) {
    if (progress_file_ && progress_file_->is_open()) {
        *progress_file_ << message << std::endl;
        progress_file_->flush();
    }
}

void ProgressTracker::write_to_stderr(const std::string& message) {
    std::cerr << message << std::endl;
    std::cerr.flush();
}

std::string ProgressTracker::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string ProgressTracker::format_size(size_t bytes) const {
    const char* suffixes[] = {"B", "KB", "MB", "GB"};
    double size = static_cast<double>(bytes);
    int suffix_index = 0;
    
    while (size >= 1024.0 && suffix_index < 3) {
        size /= 1024.0;
        suffix_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << suffixes[suffix_index];
    return oss.str();
}

std::string ProgressTracker::format_duration(std::chrono::seconds duration) const {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds = duration % std::chrono::minutes(1);
    
    std::ostringstream oss;
    if (hours.count() > 0) {
        oss << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
    } else if (minutes.count() > 0) {
        oss << minutes.count() << "m " << seconds.count() << "s";
    } else {
        oss << seconds.count() << "s";
    }
    
    return oss.str();
}

//=============================================================================
// 🎯 SessionProgressTracker実装
//=============================================================================

SessionProgressTracker::SessionProgressTracker(const std::string& session_id, bool enable_progress)
    : current_file_index_(0), success_count_(0), error_count_(0), skip_count_(0) {
    
    if (enable_progress) {
        tracker_ = std::make_unique<ProgressTracker>(session_id, true);
    }
}

void SessionProgressTracker::start_directory_analysis(const std::filesystem::path& target_path, size_t file_count) {
    if (tracker_) {
        tracker_->start(file_count, target_path.string());
    }
}

void SessionProgressTracker::update_file_analysis(const std::string& filename, size_t file_size, 
                                                 bool success, const std::string& error) {
    current_file_index_++;
    
    if (tracker_) {
        if (success) {
            tracker_->update(current_file_index_, filename, file_size, "OK");
            success_count_++;
        } else {
            tracker_->error(current_file_index_, filename, error);
            error_count_++;
        }
    } else {
        // プログレス無効時も統計は取る
        if (success) {
            success_count_++;
        } else {
            error_count_++;
        }
    }
}

void SessionProgressTracker::complete_analysis() {
    if (tracker_) {
        tracker_->complete(success_count_, error_count_, skip_count_);
    }
}

std::string SessionProgressTracker::get_progress_file_path() const {
    if (tracker_) {
        return tracker_->get_progress_file_path();
    }
    return "";
}

} // namespace nekocode