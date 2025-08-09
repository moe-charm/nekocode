#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "nekocode/types.hpp"
#include "nekocode/session_manager.hpp"
#include "nekocode/moveclass.hpp"
#include "nekocode/dependency_graph.hpp"

namespace nekocode {

//=============================================================================
// 📦 MoveClassHandler - クラス移動コマンドハンドラー
//=============================================================================

class MoveClassHandler {
private:
    std::shared_ptr<SessionManager> session_manager_;
    std::string memory_dir_;
    
public:
    MoveClassHandler();
    ~MoveClassHandler() = default;
    
    /// 直接実行（即座にクラスを移動）
    nlohmann::json execute(const std::string& session_id,
                           const std::string& symbol_id,
                           const std::string& target_file);
    
    /// プレビュー生成
    nlohmann::json preview(const std::string& session_id,
                           const std::string& symbol_id,
                           const std::string& target_file);
    
    /// プレビュー確認実行
    nlohmann::json confirm(const std::string& preview_id);
    
private:
    /// セッションからシンボル情報取得
    std::optional<UniversalSymbolInfo> get_symbol_from_session(
        const std::string& session_id,
        const std::string& symbol_id);
    
    /// ファイルからクラス定義を抽出
    std::string extract_class_definition(const std::string& file_path,
                                        const UniversalSymbolInfo& symbol);
    
    /// import/include文の更新
    std::string update_imports(const std::string& content,
                              const std::string& old_file,
                              const std::string& new_file,
                              Language language);
    
    /// プレビューID生成
    std::string generate_preview_id();
    
    /// プレビューデータ保存
    void save_preview_data(const std::string& preview_id,
                          const nlohmann::json& data);
    
    /// プレビューデータ読み込み
    std::optional<nlohmann::json> load_preview_data(const std::string& preview_id);
    
    /// 編集履歴保存
    void save_edit_history(const std::string& edit_id,
                          const nlohmann::json& data);
    
    /// タイムスタンプ生成
    std::string generate_timestamp();
};

} // namespace nekocode