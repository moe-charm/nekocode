//=============================================================================
// 🔧 NekoCode Config Manager - Implementation
//=============================================================================

#include "config_manager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <limits.h>

namespace nekocode {

//=============================================================================
// 🎯 シングルトン実装
//=============================================================================

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    set_defaults();
    load_from_file();  // 自動的に設定ファイルを読み込み
}

//=============================================================================
// 📖 設定取得・変更
//=============================================================================

ConfigManager::MemoryConfig ConfigManager::get_memory_config() const {
    return memory_config_;
}

ConfigManager::PerformanceConfig ConfigManager::get_performance_config() const {
    return performance_config_;
}

void ConfigManager::set_memory_config(const MemoryConfig& config) {
    memory_config_ = config;
}

void ConfigManager::set_performance_config(const PerformanceConfig& config) {
    performance_config_ = config;
}

bool ConfigManager::set_value(const std::string& key, const std::string& value) {
    try {
        // メモリ設定
        if (key == "memory.edit_history.max_size_mb") {
            memory_config_.history_max_mb = std::stoul(value);
            return true;
        }
        if (key == "memory.edit_history.min_files_keep") {
            memory_config_.history_min_files = std::stoul(value);
            return true;
        }
        if (key == "memory.edit_previews.max_size_mb") {
            memory_config_.preview_max_mb = std::stoul(value);
            return true;
        }
        
        // パフォーマンス設定
        if (key == "performance.default_io_threads") {
            performance_config_.default_io_threads = std::stoi(value);
            return true;
        }
        if (key == "performance.storage_type") {
            if (value == "ssd" || value == "hdd" || value == "auto") {
                performance_config_.storage_type = value;
                return true;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ 設定値エラー: " << key << " = " << value 
                  << " (" << e.what() << ")" << std::endl;
    }
    
    return false;
}

std::string ConfigManager::get_value(const std::string& key) const {
    // メモリ設定
    if (key == "memory.edit_history.max_size_mb") {
        return std::to_string(memory_config_.history_max_mb);
    }
    if (key == "memory.edit_history.min_files_keep") {
        return std::to_string(memory_config_.history_min_files);
    }
    if (key == "memory.edit_previews.max_size_mb") {
        return std::to_string(memory_config_.preview_max_mb);
    }
    
    // パフォーマンス設定
    if (key == "performance.default_io_threads") {
        return std::to_string(performance_config_.default_io_threads);
    }
    if (key == "performance.storage_type") {
        return performance_config_.storage_type;
    }
    
    return "";
}

//=============================================================================
// 💾 ファイル操作
//=============================================================================

void ConfigManager::load_from_file() {
    auto config_path = get_config_path();
    
    if (!std::filesystem::exists(config_path)) {
        // 設定ファイルがない場合はデフォルト値を使用
        configured_ = false;
        return;
    }
    
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            std::cerr << "⚠️ 設定ファイルを開けません: " << config_path << std::endl;
            return;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        from_json(j);
        configured_ = true;
        
    } catch (const std::exception& e) {
        std::cerr << "⚠️ 設定ファイル読み込みエラー: " << e.what() << std::endl;
        configured_ = false;
    }
}

void ConfigManager::save_to_file() {
    auto config_path = get_config_path();
    
    try {
        // ディレクトリが存在することを確認
        auto parent_dir = config_path.parent_path();
        if (!std::filesystem::exists(parent_dir)) {
            std::filesystem::create_directories(parent_dir);
        }
        
        nlohmann::json j = config_to_json();
        
        std::ofstream file(config_path);
        if (!file.is_open()) {
            std::cerr << "❌ 設定ファイルに書き込めません: " << config_path << std::endl;
            return;
        }
        
        file << j.dump(2);  // 2スペースインデント
        file.close();
        
        configured_ = true;
        std::cout << "✅ 設定を保存しました: " << config_path << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 設定ファイル保存エラー: " << e.what() << std::endl;
    }
}

bool ConfigManager::config_exists() const {
    return std::filesystem::exists(get_config_path());
}

std::filesystem::path ConfigManager::get_config_path() const {
    return get_executable_dir() / CONFIG_FILENAME;
}

//=============================================================================
// 📊 設定情報取得
//=============================================================================

nlohmann::json ConfigManager::to_json() const {
    return config_to_json();
}

std::string ConfigManager::to_string() const {
    std::stringstream ss;
    
    ss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    ss << "📋 Current Configuration\n";
    ss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    ss << "Config location: " << get_config_path() << "\n";
    ss << "Status: " << (configured_ ? "Configured" : "Default") << "\n\n";
    
    ss << "Edit History:\n";
    ss << "  Max Size:     " << memory_config_.history_max_mb << " MB\n";
    ss << "  Min Files:    " << memory_config_.history_min_files << "\n\n";
    
    ss << "Preview Files:\n";
    ss << "  Max Size:     " << memory_config_.preview_max_mb << " MB\n\n";
    
    ss << "Performance:\n";
    ss << "  IO Threads:   " << performance_config_.default_io_threads << "\n";
    ss << "  Storage Type: " << performance_config_.storage_type << "\n";
    ss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    return ss.str();
}

//=============================================================================
// 🔒 プライベート実装
//=============================================================================

std::filesystem::path ConfigManager::get_executable_dir() const {
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    
    // フォールバック: カレントディレクトリのbin
    return std::filesystem::current_path() / "bin";
}

void ConfigManager::set_defaults() {
    // デフォルト値はヘッダーの初期化子で設定済み
    memory_config_ = MemoryConfig();
    performance_config_ = PerformanceConfig();
    configured_ = false;
}

void ConfigManager::from_json(const nlohmann::json& j) {
    // バージョンチェック（将来の互換性のため）
    // std::string version = j.value("version", "1.0");
    
    // メモリ設定
    if (j.contains("memory")) {
        auto mem = j["memory"];
        if (mem.contains("edit_history")) {
            memory_config_.history_max_mb = mem["edit_history"].value("max_size_mb", 10);
            memory_config_.history_min_files = mem["edit_history"].value("min_files_keep", 10);
        }
        if (mem.contains("edit_previews")) {
            memory_config_.preview_max_mb = mem["edit_previews"].value("max_size_mb", 5);
        }
    }
    
    // パフォーマンス設定
    if (j.contains("performance")) {
        auto perf = j["performance"];
        performance_config_.default_io_threads = perf.value("default_io_threads", 8);
        performance_config_.storage_type = perf.value("storage_type", "auto");
    }
}

nlohmann::json ConfigManager::config_to_json() const {
    return {
        {"version", CONFIG_VERSION},
        {"memory", {
            {"edit_history", {
                {"max_size_mb", memory_config_.history_max_mb},
                {"min_files_keep", memory_config_.history_min_files}
            }},
            {"edit_previews", {
                {"max_size_mb", memory_config_.preview_max_mb}
            }}
        }},
        {"performance", {
            {"default_io_threads", performance_config_.default_io_threads},
            {"storage_type", performance_config_.storage_type}
        }}
    };
}

} // namespace nekocode