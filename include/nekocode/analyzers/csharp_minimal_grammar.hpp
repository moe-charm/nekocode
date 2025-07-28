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
// 🎯 C#キーワード・修飾子（現実対応完全版）
//=============================================================================

// アクセス修飾子
struct public_keyword : TAO_PEGTL_STRING("public") {};
struct protected_keyword : TAO_PEGTL_STRING("protected") {};
struct private_keyword : TAO_PEGTL_STRING("private") {};
struct internal_keyword : TAO_PEGTL_STRING("internal") {};

struct access_modifier : sor<public_keyword, protected_keyword, private_keyword, internal_keyword> {};

// その他修飾子
struct static_keyword : TAO_PEGTL_STRING("static") {};
struct virtual_keyword : TAO_PEGTL_STRING("virtual") {};
struct override_keyword : TAO_PEGTL_STRING("override") {};
struct async_keyword : TAO_PEGTL_STRING("async") {};
struct sealed_keyword : TAO_PEGTL_STRING("sealed") {};
struct abstract_keyword : TAO_PEGTL_STRING("abstract") {};

struct other_modifier : sor<static_keyword, virtual_keyword, override_keyword, async_keyword, sealed_keyword, abstract_keyword> {};

// 修飾子組み合わせ（順序柔軟）
struct modifiers : star<seq<sor<access_modifier, other_modifier>, plus<space>>> {};

//=============================================================================
// 🎯 C#型システム（ジェネリクス対応）
//=============================================================================

// 基本型
struct void_keyword : TAO_PEGTL_STRING("void") {};
struct string_keyword : TAO_PEGTL_STRING("string") {};
struct int_keyword : TAO_PEGTL_STRING("int") {};
struct bool_keyword : TAO_PEGTL_STRING("bool") {};

// ジェネリクス <T> 記法
struct generic_params : seq<one<'<'>, until<one<'>'>>> {};

// 型名（ジェネリクス対応）
struct type_name : seq<
    sor<void_keyword, string_keyword, int_keyword, bool_keyword, identifier>,
    opt<generic_params>
> {};

//=============================================================================
// 🎯 メソッド宣言（現実的C#対応）
//=============================================================================

struct method_params : seq<one<'('>, until<one<')'>>> {};

// 通常メソッド: [修飾子] 戻り値型 メソッド名(引数) {
struct normal_method : seq<
    modifiers,
    type_name,
    plus<space>,
    identifier,
    ignore,
    method_params,
    ignore,
    sor<one<'{'>, one<';'>>
> {};

// コンストラクタ: [修飾子] クラス名(引数) {
struct constructor : seq<
    modifiers,
    identifier,  // コンストラクタ名 = クラス名
    ignore,
    method_params,
    ignore,
    sor<one<'{'>, one<';'>>
> {};

// プロパティ: [修飾子] 型 プロパティ名 => 式;  
struct property_arrow : seq<
    modifiers,
    type_name,
    plus<space>,
    identifier,
    ignore,
    TAO_PEGTL_STRING("=>"),
    until<one<';'>>
> {};

// プロパティ: [修飾子] 型 プロパティ名 { get; set; }
struct property_getset : seq<
    modifiers,
    type_name,
    plus<space>,
    identifier,
    ignore,
    one<'{'>,
    until<one<'}'>>
> {};

// メソッド総合
struct method_decl : sor<normal_method, constructor, property_arrow, property_getset> {};

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