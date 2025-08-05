//=============================================================================
// 🧠 NekoCode Memory Command - シンプル & 人間フレンドリーCLI
//=============================================================================

#include "nekocode/memory_command.hpp"
#include "../memory/memory_system.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace nekocode {

//=============================================================================
// 🎮 MemoryCommand Implementation
//=============================================================================

MemoryCommand::MemoryCommand() {
    try {
        memory_manager_ = memory::MemorySystem::create_default();
        
        // 初期化
        auto init_future = memory_manager_->get_transport()->initialize();
        init_future.wait();
    } catch (const std::exception& e) {
        throw;
    }
}

MemoryCommand::~MemoryCommand() = default;

bool MemoryCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_usage();
        return false;
    }
    
    const std::string& command = args[0];
    
    try {
        if (command == "save") {
            return handle_save(args);
        } else if (command == "load") {
            return handle_load(args);
        } else if (command == "list") {
            return handle_list(args);
        } else if (command == "search") {
            return handle_search(args);
        } else if (command == "remove" || command == "rm") {
            return handle_remove(args);
        } else if (command == "timeline") {
            return handle_timeline(args);
        } else if (command == "cleanup") {
            return handle_cleanup(args);
        } else if (command == "stats") {
            return handle_stats(args);
        } else if (command == "help") {
            print_usage();
            return true;
        } else {
            std::cout << "🚨 Unknown command: " << command << std::endl;
            print_usage();
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        return false;
    }
}

//=============================================================================
// 💾 Command Handlers
//=============================================================================

bool MemoryCommand::handle_save(const std::vector<std::string>& args) {
    // nekocode save {type} {name} [content]
    if (args.size() < 3) {
        std::cout << "🚨 Usage: nekocode save {auto|memo|api|cache} {name} [content]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  nekocode save auto project_analysis_jan15" << std::endl;
        std::cout << "  nekocode save memo refactor_plan" << std::endl;
        return false;
    }
    
    std::string type_str = args[1];
    std::string name = args[2];
    
    // Type validation
    auto type = memory::string_to_memory_type(type_str);
    if (memory::memory_type_to_string(type) != type_str) {
        std::cout << "🚨 Invalid type: " << type_str << std::endl;
        std::cout << "Valid types: auto, memo, api, cache" << std::endl;
        return false;
    }
    
    nlohmann::json content;
    
    if (type == memory::MemoryType::Auto) {
        // 自動: 現在の解析結果を取得
        content = get_current_analysis_result();
        std::cout << "🔄 Auto-collecting current analysis result..." << std::endl;
    } else if (args.size() > 3) {
        // 手動コンテンツ指定
        std::string content_str = args[3];
        content = nlohmann::json{{"content", content_str}, {"manual_entry", true}};
    } else {
        // 空コンテンツ（後で編集用）
        content = nlohmann::json{{"content", ""}, {"created_empty", true}};
    }
    
    // 保存実行
    auto save_future = memory_manager_->save(type, name, content);
    bool success = save_future.get();
    
    if (success) {
        std::cout << "✅ Memory saved: " << memory::memory_type_to_string(type) 
                  << "/" << name << std::endl;
        std::cout << "📁 Location: .nekocode_memories/" << memory::memory_type_to_string(type) 
                  << "/" << name << ".json" << std::endl;
        
        // 内容プレビュー
        if (content.contains("functions_found")) {
            std::cout << "📊 Analysis: " << content["functions_found"] << " functions, "
                      << content.value("complexity_score", 0) << " complexity" << std::endl;
        }
    } else {
        std::cout << "❌ Failed to save memory: " << name << std::endl;
    }
    
    return success;
}

bool MemoryCommand::handle_load(const std::vector<std::string>& args) {
    // nekocode load {type} {name}
    if (args.size() < 3) {
        std::cout << "🚨 Usage: nekocode load {auto|memo|api|cache} {name}" << std::endl;
        return false;
    }
    
    std::string type_str = args[1];
    std::string name = args[2];
    
    auto type = memory::string_to_memory_type(type_str);
    
    auto load_future = memory_manager_->load(type, name);
    auto content = load_future.get();
    
    if (content.empty()) {
        std::cout << "❌ Memory not found: " << memory::memory_type_to_string(type) 
                  << "/" << name << std::endl;
        return false;
    }
    
    std::cout << "📖 Memory: " << memory::memory_type_to_string(type) 
              << "/" << name << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    
    // Pretty print based on content type
    if (content.contains("functions_found")) {
        // 解析結果の場合
        print_analysis_summary(content);
    } else if (content.contains("content")) {
        // テキストコンテンツの場合
        std::cout << content["content"].get<std::string>() << std::endl;
    } else {
        // JSONそのまま表示
        std::cout << content.dump(2) << std::endl;
    }
    
    return true;
}

bool MemoryCommand::handle_list(const std::vector<std::string>& args) {
    // nekocode list [type]
    memory::MemoryType type = memory::MemoryType::Auto;  // デフォルト
    
    if (args.size() > 1) {
        type = memory::string_to_memory_type(args[1]);
    }
    
    auto list_future = memory_manager_->list(type);
    auto memories = list_future.get();
    
    if (memories.empty()) {
        std::cout << "📭 No memories found for type: " << memory::memory_type_to_string(type) << std::endl;
        return true;
    }
    
    std::cout << "📋 " << memory::memory_type_to_string(type) << " memories (" 
              << memories.size() << " items):" << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    
    for (const auto& memory_name : memories) {
        std::cout << "📄 " << memory_name << std::endl;
    }
    
    return true;
}

bool MemoryCommand::handle_search(const std::vector<std::string>& args) {
    // nekocode search {text}
    if (args.size() < 2) {
        std::cout << "🚨 Usage: nekocode search {text}" << std::endl;
        return false;
    }
    
    std::string search_text = args[1];
    
    auto search_future = memory_manager_->search(search_text);
    auto results = search_future.get();
    
    if (results.empty()) {
        std::cout << "🔍 No results found for: " << search_text << std::endl;
        return true;
    }
    
    std::cout << "🔍 Search results for \"" << search_text << "\" (" 
              << results.size() << " matches):" << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    
    for (const auto& result : results) {
        std::cout << "📄 " << result << std::endl;
    }
    
    return true;
}

bool MemoryCommand::handle_remove(const std::vector<std::string>& args) {
    // nekocode remove {type} {name}
    if (args.size() < 3) {
        std::cout << "🚨 Usage: nekocode remove {auto|memo|api|cache} {name}" << std::endl;
        return false;
    }
    
    std::string type_str = args[1];
    std::string name = args[2];
    
    auto type = memory::string_to_memory_type(type_str);
    
    // 確認プロンプト
    std::cout << "❓ Remove memory: " << memory::memory_type_to_string(type) 
              << "/" << name << "? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);
    
    if (confirm != "y" && confirm != "Y") {
        std::cout << "🚫 Cancelled" << std::endl;
        return true;
    }
    
    auto remove_future = memory_manager_->remove(type, name);
    bool success = remove_future.get();
    
    if (success) {
        std::cout << "🗑️ Memory removed: " << memory::memory_type_to_string(type) 
                  << "/" << name << std::endl;
    } else {
        std::cout << "❌ Failed to remove memory (not found?): " << name << std::endl;
    }
    
    return success;
}

bool MemoryCommand::handle_timeline(const std::vector<std::string>& args) {
    // nekocode timeline [type] [days]
    memory::MemoryType type = memory::MemoryType::Auto;
    int days = 7;  // デフォルト7日間
    
    if (args.size() > 1) {
        type = memory::string_to_memory_type(args[1]);
    }
    if (args.size() > 2) {
        days = std::stoi(args[2]);
    }
    
    auto timeline_future = memory_manager_->timeline(type, days);
    auto memories = timeline_future.get();
    
    std::cout << "📅 Timeline: " << memory::memory_type_to_string(type) 
              << " (past " << days << " days, " << memories.size() << " items)" << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    
    if (memories.empty()) {
        std::cout << "📭 No memories in this timeframe" << std::endl;
        return true;
    }
    
    for (const auto& memory_name : memories) {
        std::cout << "📄 " << memory_name << std::endl;
    }
    
    return true;
}

bool MemoryCommand::handle_cleanup(const std::vector<std::string>& args) {
    // nekocode cleanup [type] [days]
    memory::MemoryType type = memory::MemoryType::Cache;  // デフォルトはCache
    int days = 30;  // デフォルト30日
    
    if (args.size() > 1) {
        type = memory::string_to_memory_type(args[1]);
    }
    if (args.size() > 2) {
        days = std::stoi(args[2]);
    }
    
    std::cout << "🧹 Cleaning up " << memory::memory_type_to_string(type) 
              << " older than " << days << " days..." << std::endl;
    
    auto cleanup_future = memory_manager_->cleanup_old(type, days);
    bool success = cleanup_future.get();
    
    if (success) {
        std::cout << "✅ Cleanup completed" << std::endl;
    } else {
        std::cout << "❌ Cleanup failed" << std::endl;
    }
    
    return success;
}

bool MemoryCommand::handle_stats(const std::vector<std::string>& args) {
    auto stats_future = memory_manager_->get_stats();
    auto stats = stats_future.get();
    
    std::cout << "📊 Memory System Statistics" << std::endl;
    std::cout << "────────────────────────────────────────" << std::endl;
    std::cout << "📁 Directory: " << stats.value("memory_directory", ".nekocode_memories") << std::endl;
    std::cout << "📄 Total memories: " << stats.value("total_memories", 0) << std::endl;
    std::cout << "🤖 Auto (analysis): " << stats.value("auto_count", 0) << std::endl;
    std::cout << "📝 Memo (manual): " << stats.value("manual_count", 0) << std::endl;
    std::cout << "🌐 API (external): " << stats.value("api_count", 0) << std::endl;
    std::cout << "💾 Cache (temp): " << stats.value("cache_count", 0) << std::endl;
    
    return true;
}

//=============================================================================
// 🎨 Utility Methods
//=============================================================================

void MemoryCommand::print_usage() const {
    std::cout << "🧠 NekoCode Memory System - 時間軸Memory革命" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "📋 Commands:" << std::endl;
    std::cout << "  save {type} {name} [content]  - Save memory" << std::endl;
    std::cout << "  load {type} {name}            - Load memory" << std::endl;
    std::cout << "  list [type]                   - List memories" << std::endl;
    std::cout << "  search {text}                 - Search memories" << std::endl;
    std::cout << "  remove {type} {name}          - Remove memory" << std::endl;
    std::cout << "  timeline [type] [days]        - Show timeline" << std::endl;
    std::cout << "  cleanup [type] [days]         - Cleanup old memories" << std::endl;
    std::cout << "  stats                         - Show statistics" << std::endl;
    std::cout << std::endl;
    std::cout << "🎯 Types: auto, memo, api, cache" << std::endl;
    std::cout << "  auto  - 🤖 Analysis results (auto-generated)" << std::endl;
    std::cout << "  memo  - 📝 Manual notes & plans" << std::endl;
    std::cout << "  api   - 🌐 External system data" << std::endl;
    std::cout << "  cache - 💾 Temporary data (わからないやつもここ)" << std::endl;
    std::cout << std::endl;
    std::cout << "💡 Examples:" << std::endl;
    std::cout << "  nekocode save auto project_analysis_jan15" << std::endl;
    std::cout << "  nekocode save memo refactor_plan_phase2" << std::endl;
    std::cout << "  nekocode list auto" << std::endl;
    std::cout << "  nekocode search complexity" << std::endl;
    std::cout << "  nekocode timeline auto 7" << std::endl;
    std::cout << "  nekocode cleanup cache 30" << std::endl;
}

nlohmann::json MemoryCommand::get_current_analysis_result() const {
    // TODO: 実際の解析結果取得システムと連携
    // 現在は仮のデータを返す
    return nlohmann::json{
        {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
        {"functions_found", 127},
        {"classes_found", 15},
        {"complexity_score", 435},
        {"files_analyzed", 47},
        {"analysis_type", "Universal AST Revolution"},
        {"performance_ms", 194},
        {"generated_by", "NekoCode Memory System"},
        {"project_path", std::filesystem::current_path().string()}
    };
}

void MemoryCommand::print_analysis_summary(const nlohmann::json& content) const {
    std::cout << "🎯 Analysis Summary:" << std::endl;
    if (content.contains("functions_found")) {
        std::cout << "  📊 Functions: " << content["functions_found"] << std::endl;
    }
    if (content.contains("classes_found")) {
        std::cout << "  🏗️ Classes: " << content["classes_found"] << std::endl;
    }
    if (content.contains("complexity_score")) {
        std::cout << "  🧮 Complexity: " << content["complexity_score"] << std::endl;
    }
    if (content.contains("files_analyzed")) {
        std::cout << "  📁 Files: " << content["files_analyzed"] << std::endl;
    }
    if (content.contains("performance_ms")) {
        std::cout << "  ⚡ Performance: " << content["performance_ms"] << "ms" << std::endl;
    }
    if (content.contains("project_path")) {
        std::cout << "  📍 Project: " << content["project_path"].get<std::string>() << std::endl;
    }
}

} // namespace nekocode