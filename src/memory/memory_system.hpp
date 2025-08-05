//=============================================================================
// 🧠 NekoCode Memory System - 時間軸Memory革命
//=============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace nekocode {
namespace memory {

//=============================================================================
// 🎯 MemoryType - 4種類の明示的分類
//=============================================================================

enum class MemoryType {
    Auto,       // 自動生成 (解析結果)
    Manual,     // 手動作成 (メモ)  
    API,        // 外部連携
    Cache       // 一時保存・わからないやつ
};

std::string memory_type_to_string(MemoryType type);
MemoryType string_to_memory_type(const std::string& str);

//=============================================================================
// 📊 MemoryEntry - Serena差別化の時間軸管理
//=============================================================================

struct MemoryEntry {
    std::string id;                                           // メモリーID
    MemoryType type;                                          // 種類
    
    // 🕐 時間軸管理 (Serenaにない独自機能)
    std::chrono::system_clock::time_point created_at;         // 作成時刻
    std::chrono::system_clock::time_point updated_at;         // 更新時刻  
    std::chrono::system_clock::time_point accessed_at;        // 最終アクセス
    
    nlohmann::json content;                                   // 内容 (JSON形式)
    
    // コンストラクタ
    MemoryEntry() = default;
    MemoryEntry(const std::string& memory_id, MemoryType memory_type, const nlohmann::json& data);
    
    // ユーティリティ
    std::string get_filename() const;                         // ファイル名生成
    std::string get_timestamp_string() const;                // タイムスタンプ文字列
    bool is_expired(std::chrono::hours max_age) const;       // 期限切れ判定
};

//=============================================================================
// 🔍 MemoryQuery - 高度な検索・フィルタリング
//=============================================================================

struct MemoryQuery {
    std::string text_search;                                  // テキスト検索
    std::vector<MemoryType> types;                           // 種類フィルター
    
    // 時間フィルター
    std::chrono::system_clock::time_point after;
    std::chrono::system_clock::time_point before;
    
    // NekoCode特化検索
    std::string project_path_contains;                        // プロジェクトパスフィルター
    int min_complexity = -1;                                 // 最小複雑度
    int max_complexity = -1;                                 // 最大複雑度
    
    MemoryQuery() = default;
    static MemoryQuery for_type(MemoryType type);            // 種類特化クエリ
    static MemoryQuery recent(int days);                     // 最近のもの
    static MemoryQuery search_text(const std::string& text); // テキスト検索
};

//=============================================================================
// 🚀 MemoryTransport Interface - nyamesh抽象化パターン
//=============================================================================

class MemoryTransport {
public:
    virtual ~MemoryTransport() = default;
    
    // 基本CRUD操作
    virtual std::future<bool> store(const MemoryEntry& entry) = 0;
    virtual std::future<MemoryEntry> load(const std::string& id) = 0;
    virtual std::future<bool> remove(const std::string& id) = 0;
    virtual std::future<std::vector<std::string>> list(MemoryType type = MemoryType::Auto) = 0;
    
    // 高度な検索・分析
    virtual std::future<std::vector<std::string>> search(const MemoryQuery& query) = 0;
    virtual std::future<nlohmann::json> get_statistics() = 0;
    
    // Transport情報
    virtual std::string get_transport_name() const = 0;
    virtual bool is_available() const = 0;
    virtual std::future<void> initialize() = 0;
    virtual std::future<void> cleanup() = 0;
    
    // NekoCode特化: 自動保存機能
    virtual std::future<bool> auto_save_analysis(const nlohmann::json& analysis_result,
                                                 const std::string& project_path = "") = 0;
};

//=============================================================================
// 📁 FileSystemMemoryTransport - ローカルファイル保存
//=============================================================================

class FileSystemMemoryTransport : public MemoryTransport {
private:
    std::filesystem::path memory_dir_;
    
public:
    explicit FileSystemMemoryTransport(
        const std::filesystem::path& memory_dir = ".nekocode_memories");
    
    // MemoryTransport interface
    std::future<bool> store(const MemoryEntry& entry) override;
    std::future<MemoryEntry> load(const std::string& id) override;
    std::future<bool> remove(const std::string& id) override;
    std::future<std::vector<std::string>> list(MemoryType type = MemoryType::Auto) override;
    std::future<std::vector<std::string>> search(const MemoryQuery& query) override;
    std::future<nlohmann::json> get_statistics() override;
    
    // Transport info
    std::string get_transport_name() const override { return "FileSystem"; }
    bool is_available() const override;
    std::future<void> initialize() override;
    std::future<void> cleanup() override;
    
    // NekoCode特化
    std::future<bool> auto_save_analysis(const nlohmann::json& analysis_result,
                                        const std::string& project_path = "") override;

private:
    std::filesystem::path get_memory_file_path(const std::string& id, MemoryType type) const;
    std::string generate_auto_memory_name(const std::string& project_path) const;
    std::vector<std::filesystem::path> find_memory_files(MemoryType type) const;
    bool matches_query(const MemoryEntry& entry, const MemoryQuery& query) const;
};

//=============================================================================
// 🎮 MemoryManager - 統合Memory管理システム
//=============================================================================

class MemoryManager {
private:
    std::unique_ptr<MemoryTransport> transport_;
    
public:
    explicit MemoryManager(std::unique_ptr<MemoryTransport> transport);
    ~MemoryManager() = default;
    
    // シンプルなAPI - CLIから直接呼ばれる
    std::future<bool> save(MemoryType type, const std::string& name, const nlohmann::json& content);
    std::future<nlohmann::json> load(MemoryType type, const std::string& name);
    std::future<std::vector<std::string>> list(MemoryType type);
    std::future<std::vector<std::string>> search(const std::string& text);
    std::future<bool> remove(MemoryType type, const std::string& name);
    
    // 時系列機能
    std::future<std::vector<std::string>> timeline(MemoryType type, int days = 30);
    std::future<bool> cleanup_old(MemoryType type, int days = 30);
    
    // 統計・分析
    std::future<nlohmann::json> get_stats();
    std::future<nlohmann::json> complexity_timeline(int days = 30);
    std::future<nlohmann::json> performance_history(int days = 30);
    
    // 自動機能
    std::future<bool> auto_save_current_analysis();
    
    // Transport管理
    MemoryTransport* get_transport() { return transport_.get(); }
    bool is_ready() const { return transport_ && transport_->is_available(); }
};

//=============================================================================
// 🏭 MemorySystem Factory
//=============================================================================

class MemorySystem {
public:
    enum class TransportType {
        FileSystem,     // ローカルファイル
        // 将来拡張: SQLite, P2P
    };
    
    static std::unique_ptr<MemoryManager> create(
        TransportType type = TransportType::FileSystem,
        const nlohmann::json& config = {});
        
    static std::unique_ptr<MemoryManager> create_default();
};

} // namespace memory
} // namespace nekocode

//=============================================================================
// 使用例 - シンプルで人間フレンドリー
//=============================================================================

/*
// 基本的な使用方法
auto memory = MemorySystem::create_default();

// 解析結果の保存 (Auto型)
nlohmann::json analysis = {{"functions", 127}, {"complexity", 435}};
await memory->save(MemoryType::Auto, "project_analysis_jan15", analysis);

// メモの保存 (Manual型)  
nlohmann::json memo = {{"content", "リファクタリング計画"}};
await memory->save(MemoryType::Manual, "refactor_plan", memo);

// 読み込み
auto data = await memory->load(MemoryType::Auto, "project_analysis_jan15");

// 一覧表示
auto auto_memories = await memory->list(MemoryType::Auto);
auto all_memories = await memory->list(MemoryType::Cache);  // わからないやつも

// 検索
auto search_results = await memory->search("complexity");

// 時系列機能
auto recent = await memory->timeline(MemoryType::Auto, 7);  // 過去7日間
auto cleaned = await memory->cleanup_old(MemoryType::Cache, 30);  // 30日以上のCache削除

// 統計情報
auto stats = await memory->get_stats();
auto complexity_trend = await memory->complexity_timeline(30);
*/