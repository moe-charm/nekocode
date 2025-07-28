#pragma once

//=============================================================================
// 🎯 C# PEGTL Grammar - PEG文法によるC#構文定義
//
// std::regexの限界を超えた正確な構文解析
// PEGTL (Parsing Expression Grammar Template Library) による革新的実装
//=============================================================================

#include <tao/pegtl.hpp>
#include <string>
#include <vector>

namespace nekocode {
namespace csharp {
namespace grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素
//=============================================================================

// 空白文字
struct ws : star<space> {};
struct ws1 : plus<space> {};

// 改行
struct newline : sor<string<'\r', '\n'>, one<'\n'>, one<'\r'>> {};

// 識別子（C#のルールに従う）
struct identifier_start : sor<alpha, one<'_'>, one<'@'>> {};
struct identifier_cont : sor<alnum, one<'_'>> {};
struct identifier : seq<identifier_start, star<identifier_cont>> {};

// 修飾子付き識別子（namespace.class.method形式）
struct qualified_identifier : seq<identifier, star<seq<one<'.'>, identifier>>> {};

// 文字列リテラル（簡易版）
struct string_literal : seq<one<'"'>, until<one<'"'>>> {};
struct verbatim_string : seq<string<'@', '"'>, until<one<'"'>>> {};

// コメント
struct line_comment : seq<string<'/', '/'>, until<newline>> {};
struct block_comment : seq<string<'/', '*'>, until<string<'*', '/'>>> {};
struct comment : sor<line_comment, block_comment> {};

// 無視する要素
struct ignored : sor<ws, comment> {};

//=============================================================================
// 🏷️ 属性（Attributes）
//=============================================================================

struct attribute_target : seq<identifier, one<':'>> {};
struct attribute_argument : until<sor<one<','>, one<')'>>> {};
struct attribute_arguments : seq<one<'('>, list<attribute_argument, one<','>>, one<')'>> {};
struct attribute : seq<opt<attribute_target>, qualified_identifier, opt<attribute_arguments>> {};
struct attribute_section : seq<one<'['>, list<attribute, one<','>>, one<']'>> {};
struct attributes : star<seq<attribute_section, star<ignored>>> {};

//=============================================================================
// 🔑 アクセス修飾子・修飾子
//=============================================================================

struct access_modifier : sor<
    keyword<'p','u','b','l','i','c'>,
    keyword<'p','r','i','v','a','t','e'>,
    keyword<'p','r','o','t','e','c','t','e','d'>,
    keyword<'i','n','t','e','r','n','a','l'>
> {};

struct class_modifier : sor<
    keyword<'a','b','s','t','r','a','c','t'>,
    keyword<'s','e','a','l','e','d'>,
    keyword<'s','t','a','t','i','c'>,
    keyword<'p','a','r','t','i','a','l'>
> {};

struct method_modifier : sor<
    keyword<'s','t','a','t','i','c'>,
    keyword<'v','i','r','t','u','a','l'>,
    keyword<'o','v','e','r','r','i','d','e'>,
    keyword<'a','b','s','t','r','a','c','t'>,
    keyword<'a','s','y','n','c'>,
    keyword<'n','e','w'>
> {};

//=============================================================================
// 🎨 型（Types）
//=============================================================================

// ジェネリック型引数
struct type_parameter : identifier {};
struct type_parameters : seq<one<'<'>, list<type_parameter, one<','>>, one<'>'>> {};

// 型名（ジェネリック対応）
struct type_name : seq<qualified_identifier, opt<type_parameters>> {};

// Nullable型
struct nullable_type : seq<type_name, one<'?'>> {};

// 配列型
struct array_type : seq<type_name, plus<seq<one<'['>, star<one<','>>, one<']'>>>> {};

// 完全な型
struct type : sor<array_type, nullable_type, type_name> {};

//=============================================================================
// 📝 using文
//=============================================================================

struct using_alias : seq<keyword<'u','s','i','n','g'>, ws1, identifier, ws, one<'='>, ws, type> {};
struct using_static : seq<keyword<'u','s','i','n','g'>, ws1, keyword<'s','t','a','t','i','c'>, ws1, type> {};
struct using_namespace : seq<keyword<'u','s','i','n','g'>, ws1, qualified_identifier> {};
struct using_directive : seq<sor<using_alias, using_static, using_namespace>, ws, one<';'>> {};

//=============================================================================
// 🏛️ namespace
//=============================================================================

struct namespace_name : qualified_identifier {};
struct namespace_declaration : seq<
    keyword<'n','a','m','e','s','p','a','c','e'>, ws1, namespace_name
> {};

//=============================================================================
// 🎯 メソッド
//=============================================================================

// パラメータ
struct parameter_modifier : sor<
    keyword<'r','e','f'>,
    keyword<'o','u','t'>,
    keyword<'i','n'>,
    keyword<'p','a','r','a','m','s'>
> {};

struct parameter : seq<
    opt<seq<attributes, ws>>,
    opt<seq<parameter_modifier, ws1>>,
    type, ws1, identifier,
    opt<seq<ws, one<'='>, ws, until<sor<one<','>, one<')'>>>>> // デフォルト値
> {};

struct parameter_list : seq<one<'('>, opt<list<parameter, seq<one<','>, ws>>>, one<')'>> {};

// メソッド宣言
struct method_declaration : seq<
    opt<seq<attributes, ws>>,
    star<seq<sor<access_modifier, method_modifier>, ws1>>,
    type, ws1, identifier, ws,
    parameter_list
> {};

//=============================================================================
// 🏗️ クラス・構造体・インターフェース
//=============================================================================

// 継承
struct base_list : seq<one<':'>, ws, list<type, seq<one<','>, ws>>> {};

// クラス宣言
struct class_declaration : seq<
    opt<seq<attributes, ws>>,
    star<seq<sor<access_modifier, class_modifier>, ws1>>,
    keyword<'c','l','a','s','s'>, ws1, identifier,
    opt<type_parameters>,
    opt<seq<ws, base_list>>
> {};

// インターフェース宣言
struct interface_declaration : seq<
    opt<seq<attributes, ws>>,
    star<seq<access_modifier, ws1>>,
    keyword<'i','n','t','e','r','f','a','c','e'>, ws1, identifier,
    opt<type_parameters>,
    opt<seq<ws, base_list>>
> {};

// 構造体宣言
struct struct_declaration : seq<
    opt<seq<attributes, ws>>,
    star<seq<sor<access_modifier, keyword<'r','e','a','d','o','n','l','y'>>, ws1>>,
    keyword<'s','t','r','u','c','t'>, ws1, identifier,
    opt<type_parameters>,
    opt<seq<ws, base_list>>
> {};

//=============================================================================
// 🔍 パーサールール（エントリーポイント）
//=============================================================================

struct csharp_file : seq<
    star<ignored>,
    star<seq<using_directive, star<ignored>>>,
    opt<seq<namespace_declaration, star<ignored>>>,
    star<seq<
        sor<class_declaration, interface_declaration, struct_declaration>,
        star<ignored>
    >>,
    eof
> {};

} // namespace grammar
} // namespace csharp
} // namespace nekocode