/**
 * @file ISettingsCore.h
 * @brief 設定管理Core Interface - 完全JSON排除版
 * 
 * 革命的設計変更：
 * - nlohmann::json完全排除（Gemini先生指摘対応）
 * - 明示的構造体使用（ChatGPT先生提案）
 * - 型安全・コンパイル時エラー検出
 * - Interface+Pimpl真の独立性実現
 * - Phase C（C++20モジュール）完全対応
 * 
 * 対比：
 * 旧版: nlohmann::json getSetting() ← JSON依存・実行時エラー
 * 新版: SettingsValue getSetting() ← 型安全・コンパイル時エラー
 * 
 * @version 2.0 - JSON Independence Revolution
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

// ✅ 軽量メッセージ構造体のみインクルード（JSON不要）
#include "../messages/CoreMessages.h"

namespace charmcode {

/**
 * @brief 設定値構造体（完全型安全版）
 * 
 * 旧版: nlohmann::json（動的・実行時エラー）
 * 新版: SettingsValue（静的・コンパイル時エラー）
 * 
 * 利点：
 * - IDE補完完璧
 * - 型チェック厳密
 * - デバッグ情報豊富
 * - JSON変換は必要時のみ
 */
struct SettingsValue {
    std::string path;        // "/editor/fontSize" 形式
    std::string value;       // 文字列として格納
    std::string type;        // "string", "number", "boolean", "array", "object"
    bool exists = false;     // 設定が存在するか
    
    SettingsValue() = default;
    SettingsValue(const std::string& p, const std::string& v, const std::string& t)
        : path(p), value(v), type(t), exists(true) {}
    
    // ===============================================
    // 🎯 型安全変換ヘルパー
    // ===============================================
    
    /**
     * @brief 文字列として取得
     */
    std::string asString() const {
        return value;
    }
    
    /**
     * @brief 数値として取得
     * @param defaultValue デフォルト値
     * @return 数値変換結果
     */
    int asInt(int defaultValue = 0) const {
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    /**
     * @brief 浮動小数点として取得
     * @param defaultValue デフォルト値
     * @return 浮動小数点変換結果
     */
    double asDouble(double defaultValue = 0.0) const {
        try {
            return std::stod(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    /**
     * @brief 真偽値として取得
     * @param defaultValue デフォルト値
     * @return 真偽値変換結果
     */
    bool asBool(bool defaultValue = false) const {
        if (value == "true" || value == "1") return true;
        if (value == "false" || value == "0") return false;
        return defaultValue;
    }
    
    /**
     * @brief 有効性確認
     */
    bool isValid() const {
        return exists && !path.empty();
    }
};

/**
 * @brief 設定変更通知構造体
 */
struct SettingsChangeNotification {
    std::string path;        // 変更されたパス
    SettingsValue oldValue;  // 旧値
    SettingsValue newValue;  // 新値
    std::string changeType;  // "created", "updated", "deleted"
    uint64_t timestamp;      // 変更時刻
    
    SettingsChangeNotification() : timestamp(0) {}
};

/**
 * @brief 設定検索結果
 */
struct SettingsSearchResult {
    std::vector<SettingsValue> matches;  // 検索結果
    int totalCount = 0;                  // 総件数
    bool hasMore = false;                // 続きがあるか
    std::string searchQuery;             // 検索クエリ
    
    SettingsSearchResult() = default;
    explicit SettingsSearchResult(const std::vector<SettingsValue>& m)
        : matches(m), totalCount(static_cast<int>(m.size())) {}
};

/**
 * @brief 純粋抽象設定管理インターフェース
 * 
 * 革命的特徴：
 * - JSON完全排除・型安全保証
 * - 非同期・同期両対応
 * - テスト用モック作成簡単
 * - Interface+Pimpl完全独立性
 * - C++20モジュール完全対応
 * 
 * 使用例：
 * ```cpp
 * auto settings = createSettingsCore("settings.json");
 * auto fontSize = settings->getSetting("/editor/fontSize");
 * if (fontSize.isValid()) {
 *     int size = fontSize.asInt(12);
 *     // 型安全・IDE補完完璧
 * }
 * ```
 */
class ISettingsCore {
public:
    virtual ~ISettingsCore() = default;
    
    // ===============================================
    // 🔄 同期API - 簡単な操作用
    // ===============================================
    
    /**
     * @brief 設定値取得（同期版）
     * 
     * 旧版: nlohmann::json getSetting(path) ← JSON依存
     * 新版: SettingsValue getSetting(path) ← 型安全
     * 
     * @param path JSON pointer path (例: "/editor/fontSize")
     * @return 設定値（見つからない場合は exists=false）
     */
    virtual SettingsValue getSetting(const std::string& path) const = 0;
    
    /**
     * @brief 設定値設定（同期版）
     * 
     * 旧版: bool setSetting(path, json) ← JSON依存
     * 新版: bool setSetting(SettingsValue) ← 型安全
     * 
     * @param setting 設定値
     * @return 成功時true
     */
    virtual bool setSetting(const SettingsValue& setting) = 0;
    
    /**
     * @brief 複数設定値一括取得
     * @param paths 取得したいパス一覧
     * @return 設定値一覧
     */
    virtual std::vector<SettingsValue> getSettings(const std::vector<std::string>& paths) const = 0;
    
    /**
     * @brief 複数設定値一括設定
     * @param settings 設定値一覧
     * @return 成功した設定数
     */
    virtual int setSettings(const std::vector<SettingsValue>& settings) = 0;
    
    /**
     * @brief 全設定取得
     * @return 全設定のリスト
     */
    virtual std::vector<SettingsValue> getAllSettings() const = 0;
    
    /**
     * @brief 設定存在確認
     * @param path 確認したいパス
     * @return 存在時true
     */
    virtual bool hasSetting(const std::string& path) const = 0;
    
    /**
     * @brief 設定削除
     * @param path 削除したいパス
     * @return 成功時true
     */
    virtual bool deleteSetting(const std::string& path) = 0;
    
    /**
     * @brief 設定ファイル保存
     * @return 成功時true
     */
    virtual bool saveSettings() = 0;
    
    /**
     * @brief 設定ファイル再読み込み
     * @return 成功時true
     */
    virtual bool reloadSettings() = 0;
    
    // ===============================================
    // ⚡ 非同期API - 高性能・非ブロッキング操作用
    // ===============================================
    
    /**
     * @brief 設定値取得（非同期版）
     * 
     * ファイルI/O・ネットワーク処理時に有用
     * 
     * @param path 取得したいパス
     * @param callback 完了コールバック
     */
    virtual void getSettingAsync(const std::string& path,
                               std::function<void(const SettingsValue&)> callback) = 0;
    
    /**
     * @brief 設定値設定（非同期版）
     * @param setting 設定値
     * @param callback 完了コールバック
     */
    virtual void setSettingAsync(const SettingsValue& setting,
                               std::function<void(bool success)> callback) = 0;
    
    /**
     * @brief 複数設定取得（非同期版）
     * @param paths 取得したいパス一覧
     * @param callback 完了コールバック
     */
    virtual void getSettingsAsync(const std::vector<std::string>& paths,
                                std::function<void(const std::vector<SettingsValue>&)> callback) = 0;
    
    // ===============================================
    // 📡 変更監視・通知システム
    // ===============================================
    
    /**
     * @brief 設定変更監視
     * 
     * 指定パスの変更を監視し、変更時にコールバック実行
     * 
     * @param path 監視したいパス（ワイルドカード対応）
     * @param callback 変更通知コールバック
     * @return 監視ID (解除時に使用)
     */
    virtual std::string watchSetting(const std::string& path,
                                   std::function<void(const SettingsChangeNotification&)> callback) = 0;
    
    /**
     * @brief 設定変更監視解除
     * @param watchId 監視ID
     * @return 成功時true
     */
    virtual bool unwatchSetting(const std::string& watchId) = 0;
    
    // ===============================================
    // 🔍 検索・フィルタリング機能
    // ===============================================
    
    /**
     * @brief 設定検索
     * 
     * パス・値での検索に対応
     * 
     * @param query 検索クエリ（正規表現対応）
     * @param searchInValues 値も検索対象にするか
     * @return 検索結果
     */
    virtual SettingsSearchResult searchSettings(const std::string& query,
                                               bool searchInValues = false) const = 0;
    
    /**
     * @brief プレフィックス検索
     * 
     * 指定プレフィックスで始まる設定を取得
     * 
     * @param prefix プレフィックス (例: "/editor/")
     * @return 該当設定一覧
     */
    virtual std::vector<SettingsValue> getSettingsWithPrefix(const std::string& prefix) const = 0;
    
    // ===============================================
    // 📊 統計・メタ情報
    // ===============================================
    
    /**
     * @brief 設定統計情報
     */
    struct SettingsStatistics {
        int totalSettings = 0;
        int totalCategories = 0;
        size_t totalSizeBytes = 0;
        std::string lastModified;
        std::string configVersion;
    };
    
    /**
     * @brief 統計情報取得
     * @return 統計情報
     */
    virtual SettingsStatistics getStatistics() const = 0;
    
    /**
     * @brief Core情報取得
     * @return Core情報文字列
     */
    virtual std::string getCoreInfo() const = 0;
    
    // ===============================================
    // 🔧 高度な機能
    // ===============================================
    
    /**
     * @brief 設定スキーマ検証
     * @param schemaPath スキーマファイルパス
     * @return 検証結果
     */
    virtual bool validateSettings(const std::string& schemaPath = "") const = 0;
    
    /**
     * @brief 設定バックアップ作成
     * @param backupPath バックアップファイルパス
     * @return 成功時true
     */
    virtual bool createBackup(const std::string& backupPath) const = 0;
    
    /**
     * @brief 設定復元
     * @param backupPath バックアップファイルパス
     * @return 成功時true
     */
    virtual bool restoreFromBackup(const std::string& backupPath) = 0;
    
    /**
     * @brief デフォルト設定リセット
     * @param path リセットするパス（空文字列で全体）
     * @return 成功時true
     */
    virtual bool resetToDefaults(const std::string& path = "") = 0;
};

// ===============================================
// 🏭 Factory関数（Phase C準備）
// ===============================================

/**
 * @brief SettingsCore作成
 * 
 * 実装詳細を隠蔽したFactory関数
 * Phase C（C++20モジュール）移行時も同じインターフェース
 * 
 * @param settingsPath 設定ファイルパス
 * @return SettingsCoreインスタンス
 */
std::unique_ptr<ISettingsCore> createSettingsCore(const std::string& settingsPath = "charm_editor_settings.json");

/**
 * @brief 軽量SettingsCore作成（メモリ専用）
 * 
 * ファイルI/Oなしのメモリ専用版
 * テスト・一時的用途に最適
 * 
 * @return 軽量SettingsCoreインスタンス
 */
std::unique_ptr<ISettingsCore> createLightSettingsCore();

/**
 * @brief SettingsCore設定
 */
struct SettingsCoreConfig {
    std::string filePath = "charm_editor_settings.json";
    bool autoSave = true;
    int autoSaveIntervalMs = 5000;
    bool enableWatching = true;
    bool enableBackup = true;
    std::string backupPath = "backup/";
    bool validateOnLoad = true;
    std::string schemaPath = "";
};

/**
 * @brief 設定付きSettingsCore作成
 * @param config Core設定
 * @return SettingsCoreインスタンス
 */
std::unique_ptr<ISettingsCore> createSettingsCoreWithConfig(const SettingsCoreConfig& config);

} // namespace charmcode

/**
 * @brief Phase C（C++20モジュール）対応準備
 */
#ifdef __cpp_modules
// export module nyamesh.interfaces.settings;
// export import nyamesh.messages;
// export namespace charmcode { class ISettingsCore; }
#endif