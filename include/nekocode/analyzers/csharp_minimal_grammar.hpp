#pragma once

//=============================================================================
// 🎯 C# Minimal Grammar - 極限まで簡単なPEGTL文法
//
// 無限ループを避けるため、最小限の要素のみ
//=============================================================================

#include <tao/pegtl.hpp>

namespace nekocode {
namespace csharp {
namespace minimal_grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素（拡張版）
//=============================================================================

// 識別子（英数字+アンダースコア）
struct identifier : seq<alpha, star<sor<alnum, one<'_'>>>> {};

// 空白とコメント
struct ws : star<space> {};
struct newline : sor<one<'\n'>, one<'\r'>> {};
struct comment : seq<TAO_PEGTL_STRING("//"), star<not_one<'\n', '\r'>>> {};
struct ignore : star<sor<space, comment, newline>> {};

//=============================================================================
// 🎯 メソッド（基本）
//=============================================================================

struct void_keyword : TAO_PEGTL_STRING("void") {};
struct method_params : seq<one<'('>, until<one<')'>>> {};
struct public_keyword : TAO_PEGTL_STRING("public") {};

struct method_decl : seq<
    opt<seq<public_keyword, plus<space>>>,
    opt<seq<void_keyword, plus<space>>>,
    identifier,
    ignore,
    method_params,
    ignore,
    one<'{'>
> {};

//=============================================================================
// 🏛️ クラスブロック（{}対応）
//=============================================================================

struct class_keyword : TAO_PEGTL_STRING("class") {};

// クラス宣言ヘッダー
struct class_header : seq<
    opt<seq<public_keyword, plus<space>>>,
    class_keyword,
    plus<space>,
    identifier
> {};

// ブロック内容（メソッドまたは任意の文字）
struct block_content : star<sor<method_decl, not_one<'}'>>> {};

// クラスブロック
struct class_block : seq<
    class_header,
    ignore,
    one<'{'>,
    block_content,
    one<'}'>
> {};

//=============================================================================
// 🔍 メインルール（拡張版）
//=============================================================================

struct csharp_minimal : seq<
    ignore,
    sor<class_block, class_header>,
    ignore,
    eof
> {};

} // namespace minimal_grammar
} // namespace csharp
} // namespace nekocode