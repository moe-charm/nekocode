#pragma once

//=============================================================================
// 🐍 Python Minimal Grammar - インデント地獄攻略PEGTL文法
//
// Python特殊事情:
// - インデント依存構文（{}なし）
// - def/class + : + インデントブロック
// - import文の多様性
//=============================================================================

#include <tao/pegtl.hpp>

namespace nekocode {
namespace python {
namespace minimal_grammar {

using namespace tao::pegtl;

//=============================================================================
// 🔤 基本要素（Python特化）
//=============================================================================

// 識別子（Python仕様: 英数字+アンダースコア、Unicode対応）
struct identifier : seq<sor<alpha, one<'_'>>, star<sor<alnum, one<'_'>>>> {};

// 空白とコメント（Pythonスタイル）
struct python_comment : seq<one<'#'>, star<not_one<'\n', '\r'>>> {};
struct newline : sor<one<'\n'>, seq<one<'\r'>, opt<one<'\n'>>>> {};
struct space_no_newline : sor<one<' '>, one<'\t'>> {};
struct ignore : star<sor<space_no_newline, python_comment>> {};

// インデント検出（Python核心）
struct indent : plus<sor<one<' '>, one<'\t'>>> {};
struct line_start : seq<star<space_no_newline>> {};

//=============================================================================
// 🎯 Pythonキーワード定義
//=============================================================================

struct def_keyword : TAO_PEGTL_STRING("def") {};
struct class_keyword : TAO_PEGTL_STRING("class") {};
struct import_keyword : TAO_PEGTL_STRING("import") {};
struct from_keyword : TAO_PEGTL_STRING("from") {};
struct as_keyword : TAO_PEGTL_STRING("as") {};

//=============================================================================
// 🐍 Pythonクラス定義（インデント依存）
//=============================================================================

// パラメータリスト（Python版）
struct python_params : seq<one<'('>, until<one<')'>>> {};

// class Name: または class Name(Base):
struct python_class : seq<
    class_keyword,
    plus<space_no_newline>,
    identifier,
    opt<python_params>,  // 継承パラメータ（オプション）
    ignore,
    one<':'>
> {};

//=============================================================================
// 🎯 Python関数定義
//=============================================================================

// def name(): または def name(params):
struct python_function : seq<
    def_keyword,
    plus<space_no_newline>,
    identifier,
    python_params,
    ignore,
    one<':'>
> {};

// メソッド（クラス内関数）も同じパターン
struct python_method : python_function {};

//=============================================================================
// 📦 Pythonインポート文
//=============================================================================

// import module
struct simple_import : seq<
    import_keyword,
    plus<space_no_newline>,
    identifier,
    opt<seq<star<space_no_newline>, as_keyword, plus<space_no_newline>, identifier>>
> {};

// from module import name
struct from_import : seq<
    from_keyword,
    plus<space_no_newline>,
    identifier,
    plus<space_no_newline>,
    import_keyword,
    plus<space_no_newline>,
    identifier
> {};

// 全import文
struct python_import : sor<from_import, simple_import> {};

//=============================================================================
// 🔍 メインルール（JavaScript成功パターン適用）
//=============================================================================

// Python要素（class, def, import）
struct python_element : sor<
    python_class,
    python_function,
    python_import
> {};

// 極シンプル版: 無限ループ回避
struct python_minimal : seq<
    ignore,
    opt<python_element>,
    star<any>  // 残りは何でもOK
> {};

// デバッグ用: 行ベース解析
struct python_line : seq<
    line_start,
    opt<python_element>,
    until<newline>
> {};

} // namespace minimal_grammar
} // namespace python
} // namespace nekocode