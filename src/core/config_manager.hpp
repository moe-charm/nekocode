//=============================================================================
// 🔧 NekoCode Config Manager - 設定管理システム
//=============================================================================

#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace nekocode {

//=============================================================================
// 📋 ConfigManager - 設定の読み込み・保存・管理
//=============================================================================

class ConfigManager {
public:
    //=========================================================================
    // 📊 設定構造体
    //=========================================================================
    
    struct MemoryConfig {
        // 編集履歴設定
        size_t history_max_mb = 10;        // デフォルト10MB
        size_t history_min_files = 10;     // 最低10ファイル保持
        
        // プレビューファイル設定
        size_t preview_max_mb = 5;          // デフォルト5MB
        
        // バイト変換ヘルパー
        size_t get_history_max_bytes() const { 
            return history_max_mb * 1024 * 1024; 
        }
        size_t get_preview_max_bytes() const { 
            return preview_max_mb * 1024 * 1024; 
        }
    };
    
    struct PerformanceConfig {
        int default_io_threads = 8;         // デフォルトスレッド数
        std::string storage_type = "auto";  // "ssd", "hdd", "auto"
    };
    
    //=========================================================================
    // 🎯 シングルトンインターフェース
    //=========================================================================
    
    static ConfigManager& instance();
    
    // コピー/ムーブ禁止
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;
    
    //=========================================================================
    // 📖 設定取得・変更
    //=========================================================================
    
    MemoryConfig get_memory_config() const;
    PerformanceConfig get_performance_config() const;
    
    void set_memory_config(const MemoryConfig& config);
    void set_performance_config(const PerformanceConfig& config);
    
    // 個別設定値の更新
    bool set_value(const std::string& key, const std::string& value);
    std::string get_value(const std::string& key) const;
    
    //=========================================================================
    // 💾 ファイル操作
    //=========================================================================
    
    void load_from_file();
    void save_to_file();
    bool config_exists() const;
    
    std::filesystem::path get_config_path() const;
    
    //=========================================================================
    // 📊 設定情報取得
    //=========================================================================
    
    nlohmann::json to_json() const;
    std::string to_string() const;
    
    // 設定ソース（default/configured）
    bool is_configured() const { return configured_; }
    
private:
    //=========================================================================
    // 🔒 プライベート実装
    //=========================================================================
    
    ConfigManager();
    ~ConfigManager() = default;
    
    // 実行ファイルのディレクトリ取得
    std::filesystem::path get_executable_dir() const;
    
    // デフォルト値設定
    void set_defaults();
    
    // JSON変換
    void from_json(const nlohmann::json& j);
    nlohmann::json config_to_json() const;
    
    //=========================================================================
    // 📁 メンバ変数
    //=========================================================================
    
    MemoryConfig memory_config_;
    PerformanceConfig performance_config_;
    bool configured_ = false;  // 設定ファイルから読み込み済みか
    
    static constexpr const char* CONFIG_FILENAME = "nekocode_config.json";
    static constexpr const char* CONFIG_VERSION = "1.0";
};

//=============================================================================
// 🔧 ヘルパー関数
//=============================================================================

// グローバルアクセス用ヘルパー
inline ConfigManager& config() {
    return ConfigManager::instance();
}

} // namespace nekocode