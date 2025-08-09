#pragma once

// 🔧 JavaScript Grammar Fix for React.lazy pattern
// Problem: React.lazy(() => {}) breaks the parser
// Solution: Add better expression skipping and error recovery

#include <tao/pegtl.hpp>

namespace nekocode {
namespace javascript {
namespace minimal_grammar {

using namespace tao::pegtl;

// ============================================================================
// 🆕 NEW: Better expression handling for React patterns
// ============================================================================

// Skip parenthesized expressions (including arrow functions as arguments)
struct paren_expr;
struct paren_content : star<sor<paren_expr, not_one<')'>>> {};
struct paren_expr : seq<one<'('>, paren_content, one<')'>> {};

// Skip arrow function in any context (not just const assignments)
struct arrow_expr : seq<
    opt<function_params>,  // Optional params (could be single param without parens)
    star<space>,
    TAO_PEGTL_STRING("=>"),
    star<space>,
    sor<block, not_one<';', '\n'>>  // Block or expression
> {};

// Method call pattern: object.method(args)
struct method_call : seq<
    identifier,
    one<'('>,
    star<sor<
        arrow_expr,      // Arrow function as argument
        paren_expr,      // Nested parentheses
        not_one<')'>     // Any other content
    >>,
    one<')'>
> {};

// Export const with any expression (including React.lazy)
struct export_const_expr : seq<
    star<space>,
    export_keyword,
    plus<space>,
    const_keyword,
    plus<space>,
    identifier,
    star<space>,
    one<'='>,
    star<space>,
    sor<
        method_call,     // React.lazy(...) pattern
        arrow_expr,      // Direct arrow function
        until<one<';'>>  // Any other expression until semicolon
    >,
    opt<one<';'>>
> {};

// ============================================================================
// 🔧 FIXED: Better error recovery
// ============================================================================

// Skip any statement that we don't understand
struct skip_statement : seq<
    star<not_one<';', '\n', '{', '}'>>,  // Skip until statement end
    sor<one<';'>, newline, at<one<'{'>>>  // Stop at semicolon, newline, or block start
> {};

// Improved javascript_element with better recovery
struct javascript_element_fixed : sor<
    export_class,
    export_function,
    export_const_expr,   // 🆕 Handle React.lazy pattern
    simple_class,
    class_method,
    async_function,
    async_arrow,
    simple_function,
    simple_arrow,
    simple_import,
    skip_statement       // 🆕 Skip unrecognized statements gracefully
> {};

// Main grammar with better error recovery
struct javascript_minimal_fixed : seq<
    ignore,
    star<seq<
        sor<
            javascript_element_fixed,
            skip_statement  // Skip any unrecognized content
        >,
        ignore
    >>,
    eof  // Ensure we process the entire file
> {};

} // namespace minimal_grammar
} // namespace javascript
} // namespace nekocode

// ============================================================================
// Usage in JavaScriptPEGTLAnalyzer:
// ============================================================================
// Replace:
//   tao::pegtl::parse<minimal_grammar::javascript_minimal>(...);
// With:
//   tao::pegtl::parse<minimal_grammar::javascript_minimal_fixed>(...);
//
// This should allow the parser to:
// 1. Skip React.lazy(() => {}) patterns without breaking
// 2. Continue parsing classes after complex expressions
// 3. Recover from unrecognized patterns gracefully
// ============================================================================