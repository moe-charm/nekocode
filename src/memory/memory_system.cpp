//=============================================================================
// 🧠 NekoCode Memory System Implementation
//=============================================================================

#include "memory_system.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <regex>
#include <iostream>

namespace nekocode {
namespace memory {

//=============================================================================
// 🎯 MemoryType utilities
//=============================================================================

std::string memory_type_to_string(MemoryType type) {
    switch (type) {
        case MemoryType::Auto:   return "auto";
        case MemoryType::Manual: return "memo";
        case MemoryType::API:    return "api";
        case MemoryType::Cache:  return "cache";
        default: return "unknown";
    }
}

MemoryType string_to_memory_type(const std::string& str) {
    if (str == "auto") return MemoryType::Auto;
    if (str == "memo") return MemoryType::Manual;
    if (str == "api") return MemoryType::API;
    if (str == "cache") return MemoryType::Cache;
    return MemoryType::Cache;  // デフォルトは「わからないやつはCache」
}

//=============================================================================
// 📊 MemoryEntry implementation
//=============================================================================

MemoryEntry::MemoryEntry(const std::string& memory_id, MemoryType memory_type, const nlohmann::json& data)
    : id(memory_id), type(memory_type), content(data) {
    auto now = std::chrono::system_clock::now();
    created_at = now;
    updated_at = now;
    accessed_at = now;
}

std::string MemoryEntry::get_filename() const {
    auto time_t = std::chrono::system_clock::to_time_t(created_at);
    std::tm* tm = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << id << "_" 
        << memory_type_to_string(type) << "_"
        << std::put_time(tm, "%Y_%m_%d_%H_%M") 
        << ".json";
    
    return oss.str();
}

std::string MemoryEntry::get_timestamp_string() const {
    auto time_t = std::chrono::system_clock::to_time_t(created_at);
    std::tm* tm = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool MemoryEntry::is_expired(std::chrono::hours max_age) const {
    auto now = std::chrono::system_clock::now();
    return (now - created_at) > max_age;
}

//=============================================================================
// 🔍 MemoryQuery implementation
//=============================================================================

MemoryQuery MemoryQuery::for_type(MemoryType type) {
    MemoryQuery query;
    query.types = {type};
    return query;
}

MemoryQuery MemoryQuery::recent(int days) {
    MemoryQuery query;
    auto now = std::chrono::system_clock::now();
    query.after = now - std::chrono::hours(24 * days);
    return query;
}

MemoryQuery MemoryQuery::search_text(const std::string& text) {
    MemoryQuery query;
    query.text_search = text;
    return query;
}

//=============================================================================
// 📁 FileSystemMemoryTransport implementation
//=============================================================================

FileSystemMemoryTransport::FileSystemMemoryTransport(const std::filesystem::path& memory_dir)
    : memory_dir_(memory_dir) {
}

std::future<bool> FileSystemMemoryTransport::store(const MemoryEntry& entry) {
    return std::async(std::launch::async, [this, entry]() -> bool {
        try {
            // ディレクトリ作成
            std::filesystem::create_directories(memory_dir_);
            
            // ファイルパス生成
            auto file_path = get_memory_file_path(entry.id, entry.type);
            
            // タイプ別ディレクトリ作成（重要：親ディレクトリを確実に作成）
            std::filesystem::create_directories(file_path.parent_path());
            
            // JSON構造作成
            nlohmann::json json_data = {
                {"id", entry.id},
                {"type", memory_type_to_string(entry.type)},
                {"created_at", std::chrono::duration_cast<std::chrono::seconds>(
                    entry.created_at.time_since_epoch()).count()},
                {"updated_at", std::chrono::duration_cast<std::chrono::seconds>(
                    entry.updated_at.time_since_epoch()).count()},
                {"accessed_at", std::chrono::duration_cast<std::chrono::seconds>(
                    entry.accessed_at.time_since_epoch()).count()},
                {"content", entry.content}
            };
            
            // ファイル書き込み
            std::ofstream file(file_path);
            if (!file.is_open()) {
                return false;
            }
            file << json_data.dump(2);
            file.close();
            
            return true;
            
        } catch (const std::exception& e) {
            return false;
        }
    });
}

std::future<MemoryEntry> FileSystemMemoryTransport::load(const std::string& id) {
    return std::async(std::launch::async, [this, id]() -> MemoryEntry {
        // 全タイプのファイルを検索
        for (auto type : {MemoryType::Auto, MemoryType::Manual, MemoryType::API, MemoryType::Cache}) {
            auto file_path = get_memory_file_path(id, type);
            
            if (std::filesystem::exists(file_path)) {
                std::ifstream file(file_path);
                nlohmann::json json_data;
                file >> json_data;
                
                MemoryEntry entry;
                entry.id = json_data.value("id", "");
                entry.type = string_to_memory_type(json_data.value("type", "cache"));
                entry.content = json_data.value("content", nlohmann::json{});
                
                // 時間情報復元（デフォルト値で安全に）
                entry.created_at = std::chrono::system_clock::from_time_t(json_data.value("created_at", 0));
                entry.updated_at = std::chrono::system_clock::from_time_t(json_data.value("updated_at", 0));
                entry.accessed_at = std::chrono::system_clock::now();  // アクセス時刻更新
                
                return entry;
            }
        }
        
        // 見つからない場合は空のエントリ
        throw std::runtime_error("Memory not found: " + id);
    });
}

std::future<bool> FileSystemMemoryTransport::remove(const std::string& id) {
    return std::async(std::launch::async, [this, id]() -> bool {
        bool removed = false;
        
        // 全タイプのファイルを検索して削除
        for (auto type : {MemoryType::Auto, MemoryType::Manual, MemoryType::API, MemoryType::Cache}) {
            auto file_path = get_memory_file_path(id, type);
            
            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);
                removed = true;
            }
        }
        
        return removed;
    });
}

std::future<std::vector<std::string>> FileSystemMemoryTransport::list(MemoryType type) {
    return std::async(std::launch::async, [this, type]() -> std::vector<std::string> {
        std::vector<std::string> result;
        
        if (!std::filesystem::exists(memory_dir_)) {
            return result;
        }
        
        auto files = find_memory_files(type);
        
        for (const auto& file_path : files) {
            try {
                std::ifstream file(file_path);
                nlohmann::json json_data;
                file >> json_data;
                
                result.push_back(json_data.value("id", "unknown"));
            } catch (...) {
                // 破損ファイルは無視
            }
        }
        
        // 作成日時でソート (新しい順)
        std::sort(result.begin(), result.end());
        
        return result;
    });
}

std::future<std::vector<std::string>> FileSystemMemoryTransport::search(const MemoryQuery& query) {
    return std::async(std::launch::async, [this, query]() -> std::vector<std::string> {
        std::vector<std::string> result;
        
        if (!std::filesystem::exists(memory_dir_)) {
            return result;
        }
        
        // 全ファイルをスキャン
        for (const auto& entry : std::filesystem::directory_iterator(memory_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                try {
                    std::ifstream file(entry.path());
                    nlohmann::json json_data;
                    file >> json_data;
                    
                    MemoryEntry memory_entry;
                    memory_entry.id = json_data.value("id", "");
                    memory_entry.type = string_to_memory_type(json_data.value("type", "cache"));
                    memory_entry.content = json_data.value("content", nlohmann::json{});
                    memory_entry.created_at = std::chrono::system_clock::from_time_t(json_data.value("created_at", 0));
                    
                    if (matches_query(memory_entry, query)) {
                        result.push_back(memory_entry.id);
                    }
                    
                } catch (...) {
                    // 破損ファイルは無視
                }
            }
        }
        
        return result;
    });
}

std::future<nlohmann::json> FileSystemMemoryTransport::get_statistics() {
    return std::async(std::launch::async, [this]() -> nlohmann::json {
        nlohmann::json stats = {
            {"transport_type", "FileSystem"},
            {"memory_directory", memory_dir_.string()},
            {"total_memories", 0},
            {"auto_count", 0},
            {"manual_count", 0},
            {"api_count", 0},
            {"cache_count", 0}
        };
        
        if (!std::filesystem::exists(memory_dir_)) {
            return stats;
        }
        
        int total = 0;
        std::map<MemoryType, int> type_counts;
        
        for (const auto& entry : std::filesystem::directory_iterator(memory_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                try {
                    std::ifstream file(entry.path());
                    nlohmann::json json_data;
                    file >> json_data;
                    
                    // Safe JSON access with default value
                    std::string type_str = json_data.value("type", "cache");
                    auto type = string_to_memory_type(type_str);
                    type_counts[type]++;
                    total++;
                    
                } catch (...) {
                    // 破損ファイルは無視
                }
            }
        }
        
        stats["total_memories"] = total;
        stats["auto_count"] = type_counts[MemoryType::Auto];
        stats["manual_count"] = type_counts[MemoryType::Manual];
        stats["api_count"] = type_counts[MemoryType::API];
        stats["cache_count"] = type_counts[MemoryType::Cache];
        
        return stats;
    });
}

bool FileSystemMemoryTransport::is_available() const {
    return true;  // FileSystemは常に利用可能
}

std::future<void> FileSystemMemoryTransport::initialize() {
    return std::async(std::launch::async, [this]() {
        std::filesystem::create_directories(memory_dir_);
    });
}

std::future<void> FileSystemMemoryTransport::cleanup() {
    return std::async(std::launch::async, [this]() {
        // 古いCacheファイルの削除 (30日以上)
        if (!std::filesystem::exists(memory_dir_)) {
            return;
        }
        
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * 30);
        
        for (const auto& entry : std::filesystem::directory_iterator(memory_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                try {
                    std::ifstream file(entry.path());
                    nlohmann::json json_data;
                    file >> json_data;
                    
                    if (json_data.value("type", "") == "cache") {
                        auto created_at = std::chrono::system_clock::from_time_t(json_data.value("created_at", 0));
                        if (created_at < cutoff) {
                            std::filesystem::remove(entry.path());
                        }
                    }
                    
                } catch (...) {
                    // 破損ファイルは削除
                    std::filesystem::remove(entry.path());
                }
            }
        }
    });
}

std::future<bool> FileSystemMemoryTransport::auto_save_analysis(const nlohmann::json& analysis_result,
                                                               const std::string& project_path) {
    return std::async(std::launch::async, [this, analysis_result, project_path]() -> bool {
        try {
            auto memory_name = generate_auto_memory_name(project_path);
            MemoryEntry entry(memory_name, MemoryType::Auto, analysis_result);
            
            auto store_future = store(entry);
            return store_future.get();
            
        } catch (...) {
            return false;
        }
    });
}

//=============================================================================
// Private methods
//=============================================================================

std::filesystem::path FileSystemMemoryTransport::get_memory_file_path(const std::string& id, MemoryType type) const {
    // タイプ別にディレクトリ分け
    auto type_dir = memory_dir_ / memory_type_to_string(type);
    return type_dir / (id + ".json");
}

std::string FileSystemMemoryTransport::generate_auto_memory_name(const std::string& project_path) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time_t);
    
    std::ostringstream oss;
    
    if (!project_path.empty()) {
        // プロジェクトパスから名前を生成
        std::filesystem::path path(project_path);
        oss << path.filename().string() << "_analysis_";
    } else {
        oss << "analysis_";
    }
    
    oss << std::put_time(tm, "%Y_%m_%d_%H_%M");
    
    return oss.str();
}

std::vector<std::filesystem::path> FileSystemMemoryTransport::find_memory_files(MemoryType type) const {
    std::vector<std::filesystem::path> files;
    
    auto type_dir = memory_dir_ / memory_type_to_string(type);
    
    if (std::filesystem::exists(type_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(type_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }
    }
    
    return files;
}

bool FileSystemMemoryTransport::matches_query(const MemoryEntry& entry, const MemoryQuery& query) const {
    // タイプフィルター
    if (!query.types.empty()) {
        bool type_match = false;
        for (auto type : query.types) {
            if (entry.type == type) {
                type_match = true;
                break;
            }
        }
        if (!type_match) return false;
    }
    
    // 時間フィルター
    if (query.after != std::chrono::system_clock::time_point{}) {
        if (entry.created_at < query.after) return false;
    }
    if (query.before != std::chrono::system_clock::time_point{}) {
        if (entry.created_at > query.before) return false;
    }
    
    // テキスト検索
    if (!query.text_search.empty()) {
        std::string content_str = entry.content.dump();
        std::string id_str = entry.id;
        
        std::transform(content_str.begin(), content_str.end(), content_str.begin(), ::tolower);
        std::transform(id_str.begin(), id_str.end(), id_str.begin(), ::tolower);
        
        std::string search_lower = query.text_search;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
        
        if (content_str.find(search_lower) == std::string::npos &&
            id_str.find(search_lower) == std::string::npos) {
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// 🎮 MemoryManager implementation
//=============================================================================

MemoryManager::MemoryManager(std::unique_ptr<MemoryTransport> transport)
    : transport_(std::move(transport)) {
}

std::future<bool> MemoryManager::save(MemoryType type, const std::string& name, const nlohmann::json& content) {
    return std::async(std::launch::async, [this, type, name, content]() -> bool {
        try {
            MemoryEntry entry(name, type, content);
            auto store_future = transport_->store(entry);
            return store_future.get();
        } catch (...) {
            return false;
        }
    });
}

std::future<nlohmann::json> MemoryManager::load(MemoryType type, const std::string& name) {
    return std::async(std::launch::async, [this, type, name]() -> nlohmann::json {
        try {
            auto load_future = transport_->load(name);
            auto entry = load_future.get();
            return entry.content;
        } catch (...) {
            return nlohmann::json{};
        }
    });
}

std::future<std::vector<std::string>> MemoryManager::list(MemoryType type) {
    return transport_->list(type);
}

std::future<std::vector<std::string>> MemoryManager::search(const std::string& text) {
    auto query = MemoryQuery::search_text(text);
    return transport_->search(query);
}

std::future<bool> MemoryManager::remove(MemoryType type, const std::string& name) {
    return transport_->remove(name);
}

std::future<std::vector<std::string>> MemoryManager::timeline(MemoryType type, int days) {
    return std::async(std::launch::async, [this, type, days]() -> std::vector<std::string> {
        auto query = MemoryQuery::for_type(type);
        query.after = std::chrono::system_clock::now() - std::chrono::hours(24 * days);
        
        auto search_future = transport_->search(query);
        return search_future.get();
    });
}

std::future<bool> MemoryManager::cleanup_old(MemoryType type, int days) {
    return std::async(std::launch::async, [this, type, days]() -> bool {
        try {
            auto cleanup_future = transport_->cleanup();
            cleanup_future.get();
            return true;
        } catch (...) {
            return false;
        }
    });
}

std::future<nlohmann::json> MemoryManager::get_stats() {
    return transport_->get_statistics();
}

std::future<nlohmann::json> MemoryManager::complexity_timeline(int days) {
    return std::async(std::launch::async, [this, days]() -> nlohmann::json {
        // TODO: 複雑度の時系列分析実装
        return nlohmann::json{{"timeline", "complexity"}, {"days", days}};
    });
}

std::future<nlohmann::json> MemoryManager::performance_history(int days) {
    return std::async(std::launch::async, [this, days]() -> nlohmann::json {
        // TODO: パフォーマンス履歴分析実装
        return nlohmann::json{{"timeline", "performance"}, {"days", days}};
    });
}

std::future<bool> MemoryManager::auto_save_current_analysis() {
    return std::async(std::launch::async, [this]() -> bool {
        // TODO: 現在の解析結果を自動取得して保存
        nlohmann::json dummy_analysis = {{"auto_saved", true}};
        auto save_future = transport_->auto_save_analysis(dummy_analysis, "");
        return save_future.get();
    });
}

//=============================================================================
// 🏭 MemorySystem Factory
//=============================================================================

std::unique_ptr<MemoryManager> MemorySystem::create(TransportType type, const nlohmann::json& config) {
    switch (type) {
        case TransportType::FileSystem:
        default: {
            // Safe handling of potentially null config
            std::string memory_dir = ".nekocode_memories";  // Default value
            if (!config.is_null() && config.contains("memory_dir")) {
                memory_dir = config["memory_dir"].get<std::string>();
            }
            
            auto transport = std::make_unique<FileSystemMemoryTransport>(memory_dir);
            return std::make_unique<MemoryManager>(std::move(transport));
        }
    }
}

std::unique_ptr<MemoryManager> MemorySystem::create_default() {
    return create(TransportType::FileSystem, nlohmann::json::object());
}

} // namespace memory
} // namespace nekocode