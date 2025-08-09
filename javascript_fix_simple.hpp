#pragma once

// Simple fix for React.lazy pattern
// Add this pattern to javascript_minimal_grammar.hpp

namespace nekocode {
namespace javascript {
namespace minimal_grammar {

using namespace tao::pegtl;

// ============================================================================
// ADD THESE RULES to javascript_minimal_grammar.hpp
// ============================================================================

// React.lazy pattern: export const Name = Object.method(() => { ... });
struct react_pattern : seq<
    star<space>,
    opt<export_keyword>,
    opt<plus<space>>,
    const_keyword,
    plus<space>,
    identifier,
    star<space>,
    one<'='>,
    star<space>,
    identifier,  // Object.method (like React.lazy)
    star<space>,
    one<'('>,
    star<not_one<')'>>,  // Skip arrow function content
    one<')'>,
    star<space>,
    opt<one<';'>>
> {};

// Then modify javascript_element to include react_pattern:
// struct javascript_element : sor<
//     react_pattern,    // ADD THIS LINE FIRST
//     export_class,
//     export_function,
//     ...
// > {};

} // namespace minimal_grammar
} // namespace javascript
} // namespace nekocode