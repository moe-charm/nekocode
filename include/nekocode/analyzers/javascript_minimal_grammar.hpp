#pragma once

//=============================================================================
// 🌟 JavaScript Minimal Grammar - C#成功パターン適用
//
// 段階的PEGTL移行：極小 → シンプル → 完全
//=============================================================================

#include <tao/pegtl.hpp>

namespace nekocode {
namespace javascript {
namespace minimal_grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素（C#パターン流用）
//=============================================================================

// 識別子（英数字+アンダースコア）
struct identifier : seq<sor<alpha, one<'_', '$'>>, star<sor<alnum, one<'_', '$'>>>> {};

// 空白とコメント
struct ws : star<space> {};
struct newline : sor<one<'\n'>, one<'\r'>> {};
struct single_comment : seq<TAO_PEGTL_STRING("//"), star<not_one<'\n', '\r'>>> {};
struct multi_comment : seq<TAO_PEGTL_STRING("/*"), until<TAO_PEGTL_STRING("*/")>> {};
struct comment : sor<single_comment, multi_comment> {};
struct ignore : star<sor<space, comment, newline>> {};

//=============================================================================
// 🎯 関数（基本）
//=============================================================================

struct function_keyword : TAO_PEGTL_STRING("function") {};
struct async_keyword : TAO_PEGTL_STRING("async") {};
struct const_keyword : TAO_PEGTL_STRING("const") {};
struct let_keyword : TAO_PEGTL_STRING("let") {};
struct var_keyword : TAO_PEGTL_STRING("var") {};
struct class_keyword : TAO_PEGTL_STRING("class") {};
struct export_keyword : TAO_PEGTL_STRING("export") {};

// ブロックをスキップする汎用ルール
struct block;
struct block : seq<one<'{'>, star<sor<block, not_one<'}'>>>, one<'}'>> {};

// パラメータリスト
struct function_params : seq<one<'('>, until<one<')'>>> {};

// TypeScript型注釈 (: type をスキップ)
struct type_annotation : seq<one<':'>, star<not_one<'{', ';'>>> {};

// TypeScriptジェネリクス (<T> をスキップ)
struct generics : seq<one<'<'>, until<one<'>'>>> {};

// 極限シンプル関数: function name() {
struct function_decl : seq<
    function_keyword,
    plus<space>,
    identifier,
    one<'('>,
    one<')'>,
    plus<space>,
    one<'{'>
> {};

// アロー関数: const name = () => {}
struct arrow_function : seq<
    sor<const_keyword, let_keyword, var_keyword>,
    plus<space>,
    identifier,
    ignore,
    one<'='>,
    ignore,
    function_params,
    ignore,
    TAO_PEGTL_STRING("=>"),
    ignore,
    one<'{'>
> {};

//=============================================================================
// 🏛️ クラス（ES6）
//=============================================================================

struct extends_keyword : TAO_PEGTL_STRING("extends") {};

// クラス宣言ヘッダー
struct class_header : seq<
    class_keyword,
    plus<space>,
    identifier,
    opt<seq<
        plus<space>,
        extends_keyword,
        plus<space>,
        identifier
    >>
> {};

// クラスブロック（メソッド含む）
struct class_method : seq<
    identifier,
    ignore,
    function_params,
    ignore,
    one<'{'>
> {};

// クラス内容（とりあえず}まで読み飛ばし）
struct class_content : star<sor<class_method, not_one<'}'>>> {};

// クラスブロック
struct class_block : seq<
    class_header,
    ignore,
    one<'{'>,
    class_content,
    one<'}'>
> {};

//=============================================================================
// 📦 import（基本）
//=============================================================================

struct import_keyword : TAO_PEGTL_STRING("import") {};
struct from_keyword : TAO_PEGTL_STRING("from") {};

// 文字列リテラル（シンプル版）
struct string_literal : sor<
    seq<one<'"'>, until<one<'"'>>>,
    seq<one<'\''>, until<one<'\''>>>
> {};

// import文: import name from 'module'
struct import_stmt : seq<
    import_keyword,
    plus<space>,
    identifier,
    plus<space>,
    from_keyword,
    plus<space>,
    string_literal
> {};

//=============================================================================
// 🔍 メインルール（極限極小版 - C#成功パターン適用）
//=============================================================================

// 通常関数: function name(params) { ... }
struct simple_function : seq<
    star<space>,  // インデントを許可
    function_keyword,
    plus<space>,
    identifier,
    star<space>,
    opt<generics>,  // TypeScriptジェネリクス
    star<space>,
    function_params,
    star<space>,
    opt<type_annotation>,  // TypeScript型注釈
    star<space>,
    block  // 関数本体全体を読み飛ばす
> {};

// async関数: async function name(params) { ... }
struct async_function : seq<
    star<space>,  // インデントを許可
    async_keyword,
    plus<space>,
    function_keyword,
    plus<space>,
    identifier,
    star<space>,
    function_params,
    star<space>,
    block  // 関数本体全体を読み飛ばす
> {};

// export関数: export function name(params) { ... }
struct export_function : seq<
    star<space>,  // インデントを許可
    export_keyword,
    plus<space>,
    function_keyword,
    plus<space>,
    identifier,
    star<space>,
    opt<generics>,  // TypeScriptジェネリクス
    star<space>,
    function_params,
    star<space>,
    opt<type_annotation>,  // TypeScript型注釈
    star<space>,
    block  // 関数本体全体を読み飛ばす
> {};

// arrow function 基本版: const name = () => { ... }
struct simple_arrow : seq<
    star<space>,  // インデントを許可
    const_keyword,
    plus<space>,
    identifier,
    star<space>,
    one<'='>,
    star<space>,
    function_params,
    star<space>,
    TAO_PEGTL_STRING("=>"),
    star<space>,
    block  // 関数本体全体を読み飛ばす
> {};

// import文 基本版: import { name } from 'module' 
struct simple_import : seq<
    import_keyword,
    plus<space>,
    one<'{'>,
    star<not_one<'}'>>,
    one<'}'>,
    star<space>,
    from_keyword,
    star<space>,
    one<'\''>,
    star<not_one<'\''>>,
    one<'\''>
> {};

// class定義: class Name { ... }
struct simple_class : seq<
    star<space>,  // インデントを許可
    class_keyword,
    plus<space>,
    identifier,
    star<space>,
    block  // クラス本体全体を読み飛ばす
> {};

// export class: export class Name { ... }
struct export_class : seq<
    star<space>,  // インデントを許可
    export_keyword,
    plus<space>,
    class_keyword,
    plus<space>,
    identifier,
    star<space>,
    block  // クラス本体全体を読み飛ばす
> {};

// 拡張版: function, async, arrow, import, class を検出
struct javascript_element : sor<
    export_class,
    export_function,  // TypeScript対応
    simple_class,
    async_function,
    simple_function,
    simple_arrow,
    simple_import
> {};

struct javascript_minimal : seq<
    ignore,
    star<seq<javascript_element, ignore>>,
    star<any>  // 残りの部分は何でもOK
> {};

} // namespace minimal_grammar
} // namespace javascript
} // namespace nekocode