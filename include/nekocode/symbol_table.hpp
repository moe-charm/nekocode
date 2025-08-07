//=============================================================================
// 🗂️ NekoCode Symbol Table Management
//
// Phase 3: シンボルテーブル管理システム
// UniversalSymbolInfoを効率的に管理・検索
//=============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include "nekocode/universal_symbol.hpp"

namespace nekocode {

//=============================================================================
// 📊 Symbol Table Class
//=============================================================================

class SymbolTable {
private:
    // IDベースのシンボル管理
    std::unordered_map<std::string, UniversalSymbolInfo> symbols_;
    
    // ルートシンボル（親を持たないトップレベルシンボル）
    std::vector<std::string> root_symbols_;
    
    // 検索用インデックス
    std::unordered_map<std::string, std::vector<std::string>> name_index_;  // 名前→ID群
    std::unordered_map<SymbolType, std::vector<std::string>> type_index_;   // 型→ID群
    
    // ID生成用のカウンタ（重複回避）
    std::unordered_map<std::string, int> id_counters_;
    
public:
    // ========== 基本操作 ==========
    
    // シンボルの追加（IDが自動生成される場合）
    std::string add_symbol(UniversalSymbolInfo symbol);
    
    // シンボルの取得（変更可能）
    UniversalSymbolInfo* get_symbol(const std::string& id);
    
    // シンボルの取得（読み取り専用）
    const UniversalSymbolInfo* get_symbol(const std::string& id) const;
    
    // シンボルの存在チェック
    bool has_symbol(const std::string& id) const;
    
    // シンボルの削除（子要素も再帰的に削除）
    bool remove_symbol(const std::string& id);
    
    // ========== 階層操作 ==========
    
    // 子シンボルの取得
    std::vector<UniversalSymbolInfo> get_children(const std::string& parent_id) const;
    
    // ルートシンボルの取得
    std::vector<UniversalSymbolInfo> get_roots() const;
    
    // 親シンボルの取得
    UniversalSymbolInfo* get_parent(const std::string& child_id);
    const UniversalSymbolInfo* get_parent(const std::string& child_id) const;
    
    // 階層の深さを取得
    size_t get_depth(const std::string& id) const;
    
    // ========== 検索機能 ==========
    
    // 名前で検索（部分一致）
    std::vector<UniversalSymbolInfo> find_by_name(const std::string& name, bool exact_match = false) const;
    
    // 型で検索
    std::vector<UniversalSymbolInfo> find_by_type(SymbolType type) const;
    
    // メタデータで検索
    std::vector<UniversalSymbolInfo> find_by_metadata(
        const std::string& key, 
        const std::string& value) const;
    
    // 完全修飾名で検索
    UniversalSymbolInfo* find_by_qualified_name(const std::string& qualified_name);
    
    // ========== 統計情報 ==========
    
    // 全シンボル数
    size_t size() const { return symbols_.size(); }
    
    // 型別のシンボル数
    std::unordered_map<SymbolType, size_t> get_type_statistics() const;
    
    // 全シンボルの取得
    std::vector<UniversalSymbolInfo> get_all_symbols() const;
    
    // ========== JSON入出力 ==========
    
    // JSON形式に変換（階層構造を保持）
    nlohmann::json to_json() const;
    
    // JSONから読み込み
    static SymbolTable from_json(const nlohmann::json& j);
    
    // フラットなJSON形式（デバッグ用）
    nlohmann::json to_flat_json() const;
    
    // ========== ユーティリティ ==========
    
    // テーブルのクリア
    void clear();
    
    // 整合性チェック（親子関係の検証）
    bool validate() const;
    
    // デバッグ出力
    std::string dump_tree(const std::string& indent = "") const;
    
private:
    // ========== 内部ヘルパー関数 ==========
    
    // ユニークなIDを生成
    std::string generate_unique_id(const UniversalSymbolInfo& symbol);
    
    // インデックスの更新
    void update_indices(const std::string& id, const UniversalSymbolInfo& symbol);
    
    // インデックスからの削除
    void remove_from_indices(const std::string& id, const UniversalSymbolInfo& symbol);
    
    // 子要素を再帰的に収集
    void collect_children_recursive(const std::string& parent_id, 
                                   std::vector<UniversalSymbolInfo>& result) const;
    
    // 階層構造をJSONに変換（再帰的）
    nlohmann::json symbol_to_json_tree(const UniversalSymbolInfo& symbol) const;
};

//=============================================================================
// 実装
//=============================================================================

inline std::string SymbolTable::add_symbol(UniversalSymbolInfo symbol) {
    // IDが空の場合は自動生成
    if (symbol.symbol_id.empty()) {
        symbol.symbol_id = generate_unique_id(symbol);
    }
    
    // 既存のIDと重複していないかチェック
    while (symbols_.find(symbol.symbol_id) != symbols_.end()) {
        // 重複している場合は連番を増やして再生成
        int& counter = id_counters_[symbol.name];
        counter++;
        symbol.symbol_id = UniversalSymbolInfo::generate_id(
            symbol.symbol_type, symbol.name, counter);
    }
    
    // 親がいない場合はルートシンボルとして登録
    if (symbol.parent_id.empty()) {
        root_symbols_.push_back(symbol.symbol_id);
    } else {
        // 親シンボルの子リストに追加
        auto* parent = get_symbol(symbol.parent_id);
        if (parent) {
            parent->child_ids.push_back(symbol.symbol_id);
        }
    }
    
    // インデックスを更新
    update_indices(symbol.symbol_id, symbol);
    
    // シンボルを追加
    symbols_[symbol.symbol_id] = std::move(symbol);
    
    return symbols_[symbol.symbol_id].symbol_id;
}

inline UniversalSymbolInfo* SymbolTable::get_symbol(const std::string& id) {
    auto it = symbols_.find(id);
    return (it != symbols_.end()) ? &it->second : nullptr;
}

inline const UniversalSymbolInfo* SymbolTable::get_symbol(const std::string& id) const {
    auto it = symbols_.find(id);
    return (it != symbols_.end()) ? &it->second : nullptr;
}

inline bool SymbolTable::has_symbol(const std::string& id) const {
    return symbols_.find(id) != symbols_.end();
}

inline std::vector<UniversalSymbolInfo> SymbolTable::get_children(const std::string& parent_id) const {
    std::vector<UniversalSymbolInfo> result;
    
    auto parent = get_symbol(parent_id);
    if (parent) {
        for (const auto& child_id : parent->child_ids) {
            auto child = get_symbol(child_id);
            if (child) {
                result.push_back(*child);
            }
        }
    }
    
    return result;
}

inline std::vector<UniversalSymbolInfo> SymbolTable::get_roots() const {
    std::vector<UniversalSymbolInfo> result;
    
    for (const auto& root_id : root_symbols_) {
        auto symbol = get_symbol(root_id);
        if (symbol) {
            result.push_back(*symbol);
        }
    }
    
    return result;
}

inline std::vector<UniversalSymbolInfo> SymbolTable::find_by_name(
    const std::string& name, bool exact_match) const {
    
    std::vector<UniversalSymbolInfo> result;
    
    if (exact_match) {
        // 完全一致検索（インデックス使用）
        auto it = name_index_.find(name);
        if (it != name_index_.end()) {
            for (const auto& id : it->second) {
                auto symbol = get_symbol(id);
                if (symbol) {
                    result.push_back(*symbol);
                }
            }
        }
    } else {
        // 部分一致検索（全検索）
        for (const auto& [id, symbol] : symbols_) {
            if (symbol.name.find(name) != std::string::npos) {
                result.push_back(symbol);
            }
        }
    }
    
    return result;
}

inline std::vector<UniversalSymbolInfo> SymbolTable::find_by_type(SymbolType type) const {
    std::vector<UniversalSymbolInfo> result;
    
    auto it = type_index_.find(type);
    if (it != type_index_.end()) {
        for (const auto& id : it->second) {
            auto symbol = get_symbol(id);
            if (symbol) {
                result.push_back(*symbol);
            }
        }
    }
    
    return result;
}

inline std::vector<UniversalSymbolInfo> SymbolTable::get_all_symbols() const {
    std::vector<UniversalSymbolInfo> result;
    result.reserve(symbols_.size());
    
    for (const auto& [id, symbol] : symbols_) {
        result.push_back(symbol);
    }
    
    return result;
}

inline void SymbolTable::clear() {
    symbols_.clear();
    root_symbols_.clear();
    name_index_.clear();
    type_index_.clear();
    id_counters_.clear();
}

inline std::string SymbolTable::generate_unique_id(const UniversalSymbolInfo& symbol) {
    int& counter = id_counters_[symbol.name];
    std::string id = UniversalSymbolInfo::generate_id(
        symbol.symbol_type, symbol.name, counter);
    
    // 重複チェック
    while (symbols_.find(id) != symbols_.end()) {
        counter++;
        id = UniversalSymbolInfo::generate_id(
            symbol.symbol_type, symbol.name, counter);
    }
    
    return id;
}

inline void SymbolTable::update_indices(const std::string& id, const UniversalSymbolInfo& symbol) {
    // 名前インデックス
    name_index_[symbol.name].push_back(id);
    
    // 型インデックス
    type_index_[symbol.symbol_type].push_back(id);
}

inline nlohmann::json SymbolTable::to_json() const {
    nlohmann::json j = nlohmann::json::array();
    
    // ルートシンボルから階層的に出力
    for (const auto& root : get_roots()) {
        j.push_back(symbol_to_json_tree(root));
    }
    
    return j;
}

inline nlohmann::json SymbolTable::symbol_to_json_tree(const UniversalSymbolInfo& symbol) const {
    nlohmann::json j = symbol.to_json();
    
    // 子要素を再帰的に追加
    if (!symbol.child_ids.empty()) {
        nlohmann::json children = nlohmann::json::array();
        for (const auto& child_id : symbol.child_ids) {
            auto child = get_symbol(child_id);
            if (child) {
                children.push_back(symbol_to_json_tree(*child));
            }
        }
        j["children"] = children;
    }
    
    return j;
}

inline nlohmann::json SymbolTable::to_flat_json() const {
    nlohmann::json j = nlohmann::json::array();
    
    for (const auto& [id, symbol] : symbols_) {
        j.push_back(symbol.to_json());
    }
    
    return j;
}

} // namespace nekocode