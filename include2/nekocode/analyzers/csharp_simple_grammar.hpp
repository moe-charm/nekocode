#pragma once

//=============================================================================
// 🎯 C# Simple Grammar - 最小限PEGTLで動作確認
//
// 複雑な文法を避けて、まず基本的な要素のみ解析
//=============================================================================

#include <tao/pegtl.hpp>

namespace nekocode {
namespace csharp {
namespace simple_grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素（最小限）
//=============================================================================

// 空白文字
struct ws : star<space> {};

// 識別子（簡単版）
struct identifier : seq<alpha, star<alnum>> {};

// 文字列リテラル（簡単版）
struct string_literal : seq<one<'"'>, until<one<'"'>>> {};

//=============================================================================
// 🏷️ using文（シンプル版）
//=============================================================================

struct using_stmt : seq<
    TAO_PEGTL_STRING("using"), plus<space>,
    identifier,
    star<seq<one<'.'>, identifier>>,
    opt<ws>, one<';'>
> {};

//=============================================================================
// 🏛️ クラス宣言（シンプル版）
//=============================================================================

struct class_decl : seq<
    opt<seq<TAO_PEGTL_STRING("public"), plus<space>>>,
    TAO_PEGTL_STRING("class"), plus<space>,
    identifier,
    opt<ws>
> {};

//=============================================================================
// 🎯 メソッド宣言（シンプル版）
//=============================================================================

struct method_decl : seq<
    opt<seq<TAO_PEGTL_STRING("public"), plus<space>>>,
    opt<seq<TAO_PEGTL_STRING("private"), plus<space>>>,
    opt<seq<TAO_PEGTL_STRING("void"), plus<space>>>,
    opt<seq<identifier, plus<space>>>,  // 戻り値型
    identifier,                         // メソッド名
    opt<ws>, one<'('>, until<one<')'>>  // パラメータ
> {};

//=============================================================================
// 🔍 メインルール（シンプル版）
//=============================================================================

struct csharp_simple : seq<
    star<ws>,
    star<seq<
        sor<using_stmt, class_decl, method_decl>,
        star<ws>
    >>,
    eof
> {};

} // namespace simple_grammar
} // namespace csharp
} // namespace nekocode