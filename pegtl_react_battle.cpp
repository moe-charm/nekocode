#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <chrono>

// 🔥 React.lazy パターンとの究極対決！
// 問題: export const Throw = React.lazy(() => { ... }); でパーサーが壊れる

namespace battle {
using namespace tao::pegtl;

// =============================================================================
// 🎯 基本要素定義
// =============================================================================

// 識別子（英数字+アンダースコア+$）
struct simple_identifier : seq<sor<alpha, one<'_', '$'>>, star<sor<alnum, one<'_', '$'>>>> {};

// Property Access対応識別子 (obj.prop.method 形式)
struct dotted_identifier : seq<simple_identifier, star<seq<one<'.'>, simple_identifier>>> {};

// 空白
struct ws : star<space> {};
struct newline : sor<one<'\n'>, one<'\r'>> {};

// キーワード
struct export_keyword : seq<TAO_PEGTL_STRING("export"), not_at<sor<alnum, one<'_', '$'>>>> {};
struct const_keyword : seq<TAO_PEGTL_STRING("const"), not_at<sor<alnum, one<'_', '$'>>>> {};
struct class_keyword : seq<TAO_PEGTL_STRING("class"), not_at<sor<alnum, one<'_', '$'>>>> {};
struct extends_keyword : seq<TAO_PEGTL_STRING("extends"), not_at<sor<alnum, one<'_', '$'>>>> {};

// =============================================================================
// 🔥 問題のパターン: Arrow Function
// =============================================================================

// ブロック
struct block;
struct block_content : star<sor<block, not_one<'}'>>> {};
struct block : seq<one<'{'>, block_content, one<'}'>> {};

// パラメータリスト
struct param_list : seq<one<'('>, star<not_one<')'>>, one<')'>> {};

// アロー関数: () => { ... }
struct arrow_function : seq<
    param_list,
    ws,
    TAO_PEGTL_STRING("=>"),
    ws,
    block
> {};

// =============================================================================
// 🎯 React.lazy パターン専用ルール
// =============================================================================

// メソッド呼び出し: React.lazy(...)
struct method_call : seq<
    dotted_identifier,    // React.lazy
    ws,
    one<'('>,
    ws,
    arrow_function,       // () => { ... }
    ws,
    one<')'>
> {};

// export const文: export const Name = ...
struct export_const : seq<
    export_keyword,
    plus<space>,
    const_keyword,
    plus<space>,
    simple_identifier,    // Name
    ws,
    one<'='>,
    ws,
    method_call,          // React.lazy(() => { ... })
    opt<one<';'>>
> {};

// =============================================================================
// 🏛️ クラス定義（検出したい部分）
// =============================================================================

struct class_definition : seq<
    opt<export_keyword>,
    opt<plus<space>>,
    class_keyword,
    plus<space>,
    simple_identifier,    // クラス名
    ws,
    opt<seq<
        extends_keyword,
        plus<space>,
        dotted_identifier // React.Component
    >>,
    ws,
    block
> {};

// =============================================================================
// 🔥 メインパーサー（問題の核心）
// =============================================================================

// 各要素をスキップまたは検出
struct javascript_line : sor<
    export_const,         // React.lazyパターン（スキップ）
    class_definition,     // クラス定義（検出）
    seq<star<not_one<'\n'>>, newline>  // その他の行（スキップ）
> {};

// ファイル全体
struct javascript_file : seq<
    star<javascript_line>,
    eof
> {};

// =============================================================================
// 🎯 アクション（何が検出されたか記録）
// =============================================================================

template<typename Rule>
struct action : nothing<Rule> {};

template<>
struct action<class_definition> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, std::vector<std::string>& classes) {
        std::cout << "✅ CLASS DETECTED at line " << in.position().line << ": " 
                  << in.string().substr(0, 50) << "...\n";
        classes.push_back(in.string());
    }
};

template<>
struct action<export_const> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, std::vector<std::string>& classes) {
        std::cout << "⚠️  React.lazy pattern skipped at line " << in.position().line << "\n";
    }
};

} // namespace battle

// =============================================================================
// 🔥 テスト実行
// =============================================================================

void test_pattern(const std::string& name, const std::string& code) {
    std::cout << "\n========================================\n";
    std::cout << "🎯 Testing: " << name << "\n";
    std::cout << "========================================\n";
    std::cout << "Code:\n" << code << "\n";
    std::cout << "----------------------------------------\n";
    
    std::vector<std::string> classes;
    
    try {
        auto start = std::chrono::high_resolution_clock::now();
        
        tao::pegtl::memory_input input(code, "test");
        bool success = tao::pegtl::parse<battle::javascript_file, battle::action>(input, classes);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (success) {
            std::cout << "✅ Parse SUCCESS in " << duration.count() << "ms\n";
            std::cout << "📊 Classes found: " << classes.size() << "\n";
        } else {
            std::cout << "❌ Parse FAILED\n";
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Exception: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "🔥🔥🔥 PEGTL vs React.lazy Ultimate Battle! 🔥🔥🔥\n";
    
    // Test 1: Simple class (should work)
    test_pattern("Simple Class", R"(
class SimpleClass {
    constructor() {
        this.value = 42;
    }
}
)");
    
    // Test 2: Export class with extends (should work)
    test_pattern("Export Class with Extends", R"(
export class NativeClass extends React.Component {
    render() {
        return this.props.children;
    }
}
)");
    
    // Test 3: React.lazy BEFORE class (THE PROBLEM!)
    test_pattern("React.lazy + Class", R"(
export const Throw = React.lazy(() => {
    throw new Error('Example');
});

export class NativeClass extends React.Component {
    render() {
        return this.props.children;
    }
}
)");
    
    // Test 4: Full Components.js content
    test_pattern("Full Components.js", R"(// Example

export const Throw = React.lazy(() => {
  throw new Error('Example');
});

export const Component = React.memo(function Component({children}) {
  return children;
});

export function DisplayName({children}) {
  return children;
}
DisplayName.displayName = 'Custom Name';

export class NativeClass extends React.Component {
  render() {
    return this.props.children;
  }
}

export class FrozenClass extends React.Component {
  constructor() {
    super();
  }
  render() {
    return this.props.children;
  }
}
Object.freeze(FrozenClass.prototype);
)");
    
    std::cout << "\n🏁 Battle Complete!\n";
    
    return 0;
}