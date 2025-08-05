//=============================================================================
// 🧠 NekoCode Memory Command Header
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace nekocode {

// Forward declaration
namespace memory {
    class MemoryManager;
}

//=============================================================================
// 🎮 MemoryCommand - シンプル & 人間フレンドリーCLI
//=============================================================================

class MemoryCommand {
private:
    std::unique_ptr<memory::MemoryManager> memory_manager_;
    
public:
    MemoryCommand();
    ~MemoryCommand();
    
    // メインエントリーポイント
    bool execute(const std::vector<std::string>& args);
    
private:
    // コマンドハンドラー
    bool handle_save(const std::vector<std::string>& args);      // save {type} {name}
    bool handle_load(const std::vector<std::string>& args);      // load {type} {name}
    bool handle_list(const std::vector<std::string>& args);      // list [type]
    bool handle_search(const std::vector<std::string>& args);    // search {text}
    bool handle_remove(const std::vector<std::string>& args);    // remove {type} {name}
    bool handle_timeline(const std::vector<std::string>& args);  // timeline [type] [days]
    bool handle_cleanup(const std::vector<std::string>& args);   // cleanup [type] [days]
    bool handle_stats(const std::vector<std::string>& args);     // stats
    
    // ユーティリティ
    void print_usage() const;
    nlohmann::json get_current_analysis_result() const;
    void print_analysis_summary(const nlohmann::json& content) const;
};

} // namespace nekocode