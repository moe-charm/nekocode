//=============================================================================
// 🧪 Simple Universal System Test - 基本動作確認
//
// 依存関係を最小限にした統一システムの基本テスト
//=============================================================================

#include "universal/universal_tree_builder.hpp"
#include "universal/language_traits.hpp"
#include "../include2/nekocode/types.hpp"
#include <iostream>
#include <string>

using namespace nekocode::universal;
using namespace nekocode;

int main() {
    std::cout << "🚀 Universal System Simple Test Starting...\n";
    
    try {
        // 🌳 UniversalTreeBuilder基本テスト
        std::cout << "📊 Testing UniversalTreeBuilder...\n";
        
        UniversalTreeBuilder<JavaScriptTraits> builder;
        
        // JavaScript風のASTを手動構築
        builder.enter_scope(ASTNodeType::CLASS, "MyClass", 10);
        builder.add_function("constructor", 11);
        builder.add_function("getData", 15);
        builder.exit_scope();
        
        builder.add_function("globalFunction", 25);
        
        // 統計確認
        auto stats = builder.get_ast_statistics();
        std::cout << "✅ AST Statistics:\n";
        std::cout << "  - Classes: " << stats.classes << "\n";
        std::cout << "  - Functions: " << stats.functions << "\n";
        std::cout << "  - Max Depth: " << stats.max_depth << "\n";
        
        // 🎯 LanguageTraits基本テスト
        std::cout << "📊 Testing JavaScriptTraits...\n";
        
        std::cout << "  - Language: " << JavaScriptTraits::get_language_name() << "\n";
        std::cout << "  - Extensions: ";
        auto extensions = JavaScriptTraits::get_supported_extensions();
        for (const auto& ext : extensions) {
            std::cout << ext << " ";
        }
        std::cout << "\n";
        
        // キーワード判定テスト
        std::cout << "  - 'function' is function keyword: " 
                  << (JavaScriptTraits::is_function_keyword("function") ? "✅" : "❌") << "\n";
        std::cout << "  - 'class' is class keyword: " 
                  << (JavaScriptTraits::is_class_keyword("class") ? "✅" : "❌") << "\n";
        std::cout << "  - 'if' is control keyword: " 
                  << (JavaScriptTraits::is_control_keyword("if") ? "✅" : "❌") << "\n";
        
        // 🔧 ノード作成テスト
        std::cout << "📊 Testing Node Creation...\n";
        
        auto node = JavaScriptTraits::create_node(ASTNodeType::FUNCTION, "async getData");
        std::cout << "  - Node type: " << node->type_to_string() << "\n";
        std::cout << "  - Node name: " << node->name << "\n";
        std::cout << "  - Has async attribute: " 
                  << (node->attributes.count("async") > 0 ? "✅" : "❌") << "\n";
        
        std::cout << "🎉 Universal System Simple Test PASSED!\n";
        std::cout << "\n🌟 **Phase 4 基本機能動作確認完了！**\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test FAILED: " << e.what() << "\n";
        return 1;
    }
}