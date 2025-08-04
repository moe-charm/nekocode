#ifndef TREE_SITTER_API_H_
#define TREE_SITTER_API_H_

// Tree-sitter本格統合用ヘッダー
// プレースホルダーからの移行

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Point構造体
typedef struct {
    uint32_t row;
    uint32_t column;
} TSPoint;

// Node構造体の完全定義
typedef struct TSNode {
    uint32_t context[4];
    const void* id;
    const void* tree;
} TSNode;

// 前方宣言
typedef struct TSLanguage TSLanguage;
typedef struct TSParser TSParser;
typedef struct TSTree TSTree;
typedef struct TSTreeCursor TSTreeCursor;

// Tree-sitter言語関数の宣言
// これらの関数は各言語パーサーライブラリで実装される
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_c(void);

// Parser API
TSParser *ts_parser_new(void);
void ts_parser_delete(TSParser *parser);
bool ts_parser_set_language(TSParser *parser, const TSLanguage *language);
TSTree *ts_parser_parse_string(TSParser *parser, const TSTree *old_tree, const char *string, uint32_t length);

// Tree API
void ts_tree_delete(TSTree *tree);
TSNode ts_tree_root_node(const TSTree *tree);

// Node API
uint32_t ts_node_start_byte(TSNode node);
uint32_t ts_node_end_byte(TSNode node);
TSPoint ts_node_start_point(TSNode node);
TSPoint ts_node_end_point(TSNode node);
const char *ts_node_type(TSNode node);
uint32_t ts_node_child_count(TSNode node);
TSNode ts_node_child(TSNode node, uint32_t index);
bool ts_node_is_null(TSNode node);
bool ts_node_is_named(TSNode node);
bool ts_node_has_error(TSNode node);

// Tree cursor API
TSTreeCursor ts_tree_cursor_new(TSNode node);
void ts_tree_cursor_delete(TSTreeCursor *cursor);
bool ts_tree_cursor_goto_first_child(TSTreeCursor *cursor);
bool ts_tree_cursor_goto_next_sibling(TSTreeCursor *cursor);
TSNode ts_tree_cursor_current_node(const TSTreeCursor *cursor);

#ifdef __cplusplus
}
#endif

#endif // TREE_SITTER_API_H_