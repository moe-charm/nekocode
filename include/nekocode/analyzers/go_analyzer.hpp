#pragma once

//=============================================================================
// 🐹 Go Language Analyzer - Goroutine & Channel Detection
//
// Go言語特化解析エンジン:
// - Goroutine並行処理解析
// - Channel通信パターン検出  
// - Select文・Defer文解析
// - 複数戻り値関数検出
// - メソッドレシーバー解析
//=============================================================================

#include "base_analyzer.hpp"
// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"
#include <tao/pegtl.hpp>
#include <unordered_set>

namespace nekocode {

//=============================================================================
// 🐹 Go-Specific Data Structures
//=============================================================================

/// Goroutine情報
struct GoroutineInfo {
    std::string function_name;
    LineNumber line_number = 0;
    bool is_anonymous = false;  // 無名関数か
    
    nlohmann::json to_json() const;
};

/// Channel情報
struct ChannelInfo {
    std::string variable_name;
    std::string type;          // int, string等
    LineNumber declaration_line = 0;
    bool is_buffered = false;  // バッファありか
    int buffer_size = 0;
    
    nlohmann::json to_json() const;
};

/// Go関数情報（複数戻り値対応）
struct GoFunctionInfo {
    std::string name;
    LineNumber line_number = 0;
    std::vector<std::string> parameters;
    std::vector<std::string> return_types;  // Go特有：複数戻り値
    bool has_receiver = false;              // メソッドレシーバーあり
    std::string receiver_type;              // レシーバーの型
    
    nlohmann::json to_json() const;
};

//=============================================================================
// 🐹 Go PEGTL Grammar Rules
//=============================================================================

namespace go_pegtl {
    namespace pegtl = tao::pegtl;
    
    // Go keywords
    struct package_kw : pegtl::string<'p','a','c','k','a','g','e'> {};
    struct import_kw : pegtl::string<'i','m','p','o','r','t'> {};
    struct func_kw : pegtl::string<'f','u','n','c'> {};
    struct go_kw : pegtl::string<'g','o'> {};
    struct chan_kw : pegtl::string<'c','h','a','n'> {};
    struct select_kw : pegtl::string<'s','e','l','e','c','t'> {};
    struct defer_kw : pegtl::string<'d','e','f','e','r'> {};
    struct make_kw : pegtl::string<'m','a','k','e'> {};
    struct type_kw : pegtl::string<'t','y','p','e'> {};
    struct struct_kw : pegtl::string<'s','t','r','u','c','t'> {};
    struct interface_kw : pegtl::string<'i','n','t','e','r','f','a','c','e'> {};
    
    // Identifiers and basic types
    struct identifier : pegtl::seq<
        pegtl::ranges<'a','z','A','Z','_'>,
        pegtl::star<pegtl::ranges<'a','z','A','Z','0','9','_'>>
    > {};
    
    struct ws : pegtl::star<pegtl::space> {};
    
    // Package declaration
    struct package_decl : pegtl::seq<
        package_kw, pegtl::plus<pegtl::space>, identifier
    > {};
    
    // Import statements
    struct import_path : pegtl::seq<
        pegtl::one<'"'>,
        pegtl::star<pegtl::not_one<'"'>>,
        pegtl::one<'"'>
    > {};
    
    struct import_stmt : pegtl::seq<
        import_kw, pegtl::plus<pegtl::space>,
        pegtl::sor<
            import_path,  // single import
            pegtl::seq<   // multiple imports
                pegtl::one<'('>, ws,
                pegtl::star<pegtl::seq<import_path, ws>>,
                pegtl::one<')'>
            >
        >
    > {};
    
    // Function declaration
    struct parameter_list : pegtl::seq<
        pegtl::one<'('>,
        pegtl::star<pegtl::not_one<')'>>,
        pegtl::one<')'>
    > {};
    
    struct return_types : pegtl::seq<
        pegtl::sor<
            pegtl::seq<  // single return
                identifier
            >,
            pegtl::seq<  // multiple returns
                pegtl::one<'('>,
                pegtl::star<pegtl::not_one<')'>>,
                pegtl::one<')'>
            >
        >
    > {};
    
    // Method receiver (Go特有)
    struct receiver : pegtl::seq<
        pegtl::one<'('>,
        pegtl::star<pegtl::not_one<')'>>,
        pegtl::one<')'>
    > {};
    
    struct func_decl : pegtl::seq<
        func_kw, pegtl::plus<pegtl::space>,
        pegtl::opt<receiver>, ws,  // optional receiver
        identifier, ws,
        parameter_list, ws,
        pegtl::opt<return_types>
    > {};
    
    // Goroutine detection
    struct goroutine : pegtl::seq<
        go_kw, pegtl::plus<pegtl::space>,
        pegtl::sor<
            pegtl::seq<identifier, parameter_list>,  // go function()
            pegtl::seq<func_kw, parameter_list>       // go func() {}
        >
    > {};
    
    // Channel operations
    struct make_chan : pegtl::seq<
        make_kw, pegtl::one<'('>,
        ws, chan_kw, ws,
        identifier,  // type
        pegtl::opt<pegtl::seq<pegtl::one<','>, ws, pegtl::plus<pegtl::digit>>>, // buffer size
        ws, pegtl::one<')'>
    > {};
    
    struct channel_send : pegtl::seq<
        identifier, ws,
        pegtl::string<'<','-'>, ws
    > {};
    
    struct channel_receive : pegtl::seq<
        pegtl::opt<pegtl::seq<identifier, ws, pegtl::string<':','='>, ws>>,
        pegtl::string<'<','-'>, ws, identifier
    > {};
    
    // Select statement
    struct select_stmt : pegtl::seq<
        select_kw, ws, pegtl::one<'{'>
    > {};
    
    // Defer statement
    struct defer_stmt : pegtl::seq<
        defer_kw, pegtl::plus<pegtl::space>
    > {};
    
    // Struct definition
    struct struct_decl : pegtl::seq<
        type_kw, pegtl::plus<pegtl::space>,
        identifier, ws,
        struct_kw
    > {};
    
    // Interface definition
    struct interface_decl : pegtl::seq<
        type_kw, pegtl::plus<pegtl::space>,
        identifier, ws,
        interface_kw
    > {};
}

//=============================================================================
// 🐹 Go Language Analyzer Class
//=============================================================================

class GoAnalyzer : public BaseAnalyzer {
public:
    GoAnalyzer() = default;
    ~GoAnalyzer() override = default;
    
    // BaseAnalyzer interface
    Language get_language() const override;
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    std::string get_language_name() const override;
    std::vector<std::string> get_supported_extensions() const override;
    
    // Go-specific analysis
    std::vector<GoroutineInfo> analyze_goroutines(const std::string& content);
    std::vector<ChannelInfo> analyze_channels(const std::string& content);
    std::vector<GoFunctionInfo> analyze_go_functions(const std::string& content);
    
private:
    // PEGTL action handlers
    template<typename Rule> struct go_action {};
    
    // Internal state
    std::vector<GoroutineInfo> goroutines_;
    std::vector<ChannelInfo> channels_;
    std::vector<GoFunctionInfo> go_functions_;
    std::unordered_set<std::string> imports_;
    std::string package_name_;
    
    // Helper methods
    void reset_state();
    int calculate_go_complexity(const std::string& content);
    std::vector<std::string> extract_imports(const std::string& content);
    std::string extract_package_name(const std::string& content);
};

//=============================================================================
// 🐹 PEGTL Action Specializations
//=============================================================================

template<> struct GoAnalyzer::go_action<go_pegtl::package_decl> {
    template<typename Input>
    static void apply(const Input& in, GoAnalyzer* analyzer) {
        // Package name extraction logic
    }
};

template<> struct GoAnalyzer::go_action<go_pegtl::func_decl> {
    template<typename Input>
    static void apply(const Input& in, GoAnalyzer* analyzer) {
        // Function declaration extraction logic
    }
};

template<> struct GoAnalyzer::go_action<go_pegtl::goroutine> {
    template<typename Input>
    static void apply(const Input& in, GoAnalyzer* analyzer) {
        // Goroutine detection logic
    }
};

template<> struct GoAnalyzer::go_action<go_pegtl::make_chan> {
    template<typename Input>
    static void apply(const Input& in, GoAnalyzer* analyzer) {
        // Channel creation detection logic
    }
};

} // namespace nekocode