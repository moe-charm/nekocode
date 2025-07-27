#pragma once

//=============================================================================
// 🌳 Tree-sitter API プレースホルダー - Phase 1実装
// 
// 段階的Tree-sitter統合のためのプレースホルダー実装
// 最終的に本物のTree-sitter APIに置き換える予定
//=============================================================================

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Basic Types
//=============================================================================

typedef struct TSLanguage TSLanguage;
typedef struct TSParser TSParser; 
typedef struct TSTree TSTree;

typedef struct {
    uint32_t row;
    uint32_t column;
} TSPoint;

typedef struct {
    void *id;
    const TSLanguage *language;
} TSNode;

typedef struct {
    const char *(*read)(void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read);
    void *payload;
    const char *encoding;
} TSInput;

//=============================================================================
// Parser Functions (Placeholder)
//=============================================================================

TSParser *ts_parser_new(void);
void ts_parser_delete(TSParser *parser);
bool ts_parser_set_language(TSParser *parser, const TSLanguage *language);
TSTree *ts_parser_parse_string(TSParser *parser, const TSTree *old_tree, const char *string, uint32_t length);

//=============================================================================
// Tree Functions (Placeholder)
//=============================================================================

TSNode ts_tree_root_node(const TSTree *tree);
void ts_tree_delete(TSTree *tree);

//=============================================================================
// Node Functions (Placeholder)
//=============================================================================

uint32_t ts_node_start_byte(TSNode node);
uint32_t ts_node_end_byte(TSNode node);
TSPoint ts_node_start_point(TSNode node);
TSPoint ts_node_end_point(TSNode node);
uint32_t ts_node_child_count(TSNode node);
TSNode ts_node_child(TSNode node, uint32_t index);
TSNode ts_node_child_by_field_name(TSNode node, const char *field_name, uint32_t field_name_length);
const char *ts_node_type(TSNode node);
bool ts_node_is_null(TSNode node);
bool ts_node_has_error(TSNode node);

//=============================================================================
// Language Functions (Placeholder)
//=============================================================================

// プレースホルダー言語パーサー関数
const TSLanguage *tree_sitter_javascript(void);
const TSLanguage *tree_sitter_typescript(void);
const TSLanguage *tree_sitter_cpp(void);

#ifdef __cplusplus
}
#endif