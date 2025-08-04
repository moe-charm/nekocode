#pragma once

//=============================================================================
// 🔥 C++ Minimal Grammar - 最終ボス戦用極小PEGTL文法
//
// Claude Code支援作戦：段階的PEGTL移行
// JavaScript成功パターン適用：極小 → シンプル → 完全
//=============================================================================

#include <tao/pegtl.hpp>

namespace nekocode {
namespace cpp {
namespace minimal_grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素（JavaScript成功パターン適用）
//=============================================================================

// 識別子（C++仕様: 英数字+アンダースコア、数字開始不可）
struct identifier : seq<sor<alpha, one<'_'>>, star<sor<alnum, one<'_'>>>> {};

// 空白とコメント（改善版）
struct ws : star<space> {};
struct newline : sor<one<'\n'>, one<'\r'>> {};
struct line_comment : seq<TAO_PEGTL_STRING("//"), star<not_one<'\n', '\r'>>> {};
struct block_comment : seq<TAO_PEGTL_STRING("/*"), until<TAO_PEGTL_STRING("*/")>> {};
struct comment : sor<line_comment, block_comment> {};
struct ignore : star<sor<space, comment, newline>> {};

// より柔軟な空白処理
struct optional_ws : star<space> {};
struct required_ws : plus<space> {};

// ブロックをスキップする汎用ルール
struct block;
struct block : seq<one<'{'>, star<sor<block, not_one<'}'>>>, one<'}'>> {};

//=============================================================================
// 🎯 キーワード定義
//=============================================================================

struct namespace_keyword : TAO_PEGTL_STRING("namespace") {};
struct class_keyword : TAO_PEGTL_STRING("class") {};
struct struct_keyword : TAO_PEGTL_STRING("struct") {};
struct public_keyword : TAO_PEGTL_STRING("public") {};
struct private_keyword : TAO_PEGTL_STRING("private") {};
struct protected_keyword : TAO_PEGTL_STRING("protected") {};

//=============================================================================
// 🏛️ 名前空間（C++の特徴）
//=============================================================================

// namespace name { ... } （柔軟バージョン）
struct simple_namespace : seq<
    star<space>,  // インデントを許可
    namespace_keyword,
    required_ws,  // 必須空白
    identifier,
    optional_ws,  // オプション空白
    block  // ブロック全体を読み飛ばす
> {};

//=============================================================================
// 🏛️ クラス定義（C++の核心）
//=============================================================================

// class Name { ... } （柔軟バージョン）
struct simple_class : seq<
    star<space>,  // インデントを許可
    class_keyword,
    required_ws,
    identifier,
    optional_ws,
    block  // ブロック全体を読み飛ばす
> {};

// struct Name { ... } （柔軟バージョン）
struct simple_struct : seq<
    star<space>,  // インデントを許可
    struct_keyword,
    required_ws,
    identifier,
    optional_ws,
    block  // ブロック全体を読み飛ばす
> {};

//=============================================================================
// 🎯 関数定義（C++基本）
//=============================================================================

// パラメータリスト（簡易版）
struct function_params : seq<one<'('>, until<one<')'>>> {};

// 戻り値型（簡易版：単一識別子のみ）
struct return_type : identifier {};

// 関数定義: type name() { ... } （柔軟バージョン）
struct simple_function : seq<
    star<space>,  // インデントを許可
    return_type,
    required_ws,
    identifier,
    function_params,
    optional_ws,
    block  // ブロック全体を読み飛ばす
> {};

//=============================================================================
// 🔍 メインルール（JavaScript成功パターン適用）
//=============================================================================

// C++要素（namespace, class, struct, function）
struct cpp_element : sor<
    simple_namespace,
    simple_class,
    simple_struct,
    simple_function
> {};

// 複数要素対応版: 全C++要素を検出
struct cpp_minimal : seq<
    ignore,
    star<seq<cpp_element, ignore>>,  // 複数要素対応
    star<any>  // 残りの部分は何でもOK
> {};

// デバッグ用: 各要素を個別検出
struct cpp_grammar_debug : sor<
    simple_namespace,
    simple_class,
    simple_struct,
    simple_function
> {};

} // namespace minimal_grammar
} // namespace cpp
} // namespace nekocode