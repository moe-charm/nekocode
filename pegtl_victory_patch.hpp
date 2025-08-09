#pragma once

// 🏆 PEGTL Victory Patch for JavaScript Grammar
// React.lazy pattern fix based on successful battle test

namespace nekocode {
namespace javascript {
namespace minimal_grammar {

using namespace tao::pegtl;

// =============================================================================
// 🔥 Victory Pattern: Properly handle React.lazy and similar patterns
// =============================================================================

// Arrow function in method call: () => { ... }
struct arrow_in_call : seq<
    one<'('>,
    star<space>,
    one<')'>,
    star<space>,
    TAO_PEGTL_STRING("=>"),
    star<space>,
    block  // Use existing block definition
> {};

// Method call with arrow: React.lazy(() => { ... })
struct method_with_arrow : seq<
    identifier,           // React.lazy (using existing identifier that supports dots)
    star<space>,
    one<'('>,
    star<space>,
    arrow_in_call,        // () => { ... }
    star<space>,
    one<')'>
> {};

// Export const with method call: export const Name = React.lazy(() => { ... });
struct export_const_method : seq<
    star<space>,
    export_keyword,
    plus<space>,
    const_keyword,
    plus<space>,
    simple_identifier,    // Variable name
    star<space>,
    one<'='>,
    star<space>,
    method_with_arrow,    // React.lazy(() => { ... })
    star<space>,
    opt<one<';'>>
> {};

// =============================================================================
// 🏆 Updated javascript_element with victory pattern FIRST
// =============================================================================

// This should replace the existing javascript_element definition:
/*
struct javascript_element : sor<
    export_const_method,  // 🔥 Handle React.lazy FIRST (victory pattern)
    export_class,
    export_function,
    simple_class,
    class_method,
    async_function,
    async_arrow,
    simple_function,
    simple_arrow,
    simple_import
> {};
*/

} // namespace minimal_grammar
} // namespace javascript
} // namespace nekocode