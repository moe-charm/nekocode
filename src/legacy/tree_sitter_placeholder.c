//=============================================================================
// 🌳 Tree-sitter プレースホルダー実装
// 
// 本格Tree-sitter統合までの暫定実装
// 基本API構造のみ提供
//=============================================================================

#include <tree-sitter/api.h>
#include <stdlib.h>
#include <string.h>

//=============================================================================
// プレースホルダー構造体定義
//=============================================================================

// TSNodeはapi.hで定義されているので、内部実装用の構造体を別途定義
struct TSNodeImpl {
    int type;
    int start_byte;
    int end_byte;
    int start_row;
    int start_column;
    int end_row;
    int end_column;
    int child_count;
    struct TSNodeImpl* children;
};

struct TSLanguage {
    const char* name;
    int version;
};

struct TSParser {
    const struct TSLanguage* language;
    int state;
};

struct TSTree {
    struct TSNodeImpl* root_impl;
    int valid;
};

struct TSTreeCursor {
    struct TSNodeImpl* current_node_impl;
    int position;
};

//=============================================================================
// ヘルパー関数
//=============================================================================

// TSNodeImplからTSNodeへの変換
static TSNode node_impl_to_node(const struct TSNodeImpl* impl) {
    TSNode node = {0};
    if (impl) {
        // プレースホルダーなので簡易変換
        node.context[0] = impl->type;
        node.context[1] = impl->start_byte;
        node.context[2] = impl->end_byte;
        node.context[3] = impl->child_count;
        node.id = impl;
        node.tree = NULL;
    }
    return node;
}

// TSNodeからTSNodeImplへの変換
static struct TSNodeImpl* node_to_impl(TSNode node) {
    return (struct TSNodeImpl*)node.id;
}

//=============================================================================
// 言語定義（プレースホルダー）
//=============================================================================

static struct TSLanguage javascript_lang = {"javascript", 1};
static struct TSLanguage typescript_lang = {"typescript", 1};
static struct TSLanguage cpp_lang = {"cpp", 1};
static struct TSLanguage c_lang = {"c", 1};

//=============================================================================
// 言語取得関数
//=============================================================================

const TSLanguage *tree_sitter_javascript(void) {
    return &javascript_lang;
}

const TSLanguage *tree_sitter_typescript(void) {
    return &typescript_lang;
}

const TSLanguage *tree_sitter_cpp(void) {
    return &cpp_lang;
}

const TSLanguage *tree_sitter_c(void) {
    return &c_lang;
}

//=============================================================================
// Parser API
//=============================================================================

TSParser *ts_parser_new(void) {
    TSParser* parser = malloc(sizeof(TSParser));
    if (parser) {
        parser->language = NULL;
        parser->state = 0;
    }
    return parser;
}

void ts_parser_delete(TSParser *parser) {
    if (parser) {
        free(parser);
    }
}

bool ts_parser_set_language(TSParser *parser, const TSLanguage *language) {
    if (parser && language) {
        parser->language = language;
        return true;
    }
    return false;
}

TSTree *ts_parser_parse_string(TSParser *parser, const TSTree *old_tree, const char *string, uint32_t length) {
    (void)old_tree; // 未使用パラメータ
    
    if (!parser || !string) {
        return NULL;
    }
    
    TSTree* tree = malloc(sizeof(TSTree));
    if (tree) {
        tree->valid = 1;
        
        // ダミーのルートノード作成
        struct TSNodeImpl* root_impl = malloc(sizeof(struct TSNodeImpl));
        if (root_impl) {
            root_impl->type = 1; // program
            root_impl->start_byte = 0;
            root_impl->end_byte = length;
            root_impl->start_row = 0;
            root_impl->start_column = 0;
            root_impl->end_row = 0;
            root_impl->end_column = length;
            root_impl->child_count = 0;
            root_impl->children = NULL;
            tree->root_impl = root_impl;
        }
    }
    
    return tree;
}

//=============================================================================
// Tree API
//=============================================================================

void ts_tree_delete(TSTree *tree) {
    if (tree) {
        if (tree->root_impl) {
            if (tree->root_impl->children) {
                free(tree->root_impl->children);
            }
            free(tree->root_impl);
        }
        free(tree);
    }
}

TSNode ts_tree_root_node(const TSTree *tree) {
    if (tree && tree->root_impl) {
        return node_impl_to_node(tree->root_impl);
    }
    
    // 無効なノードを返す
    TSNode invalid_node = {0};
    return invalid_node;
}

//=============================================================================
// Node API
//=============================================================================

uint32_t ts_node_start_byte(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    return impl ? impl->start_byte : 0;
}

uint32_t ts_node_end_byte(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    return impl ? impl->end_byte : 0;
}

TSPoint ts_node_start_point(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    TSPoint point = {0, 0};
    if (impl) {
        point.row = impl->start_row;
        point.column = impl->start_column;
    }
    return point;
}

TSPoint ts_node_end_point(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    TSPoint point = {0, 0};
    if (impl) {
        point.row = impl->end_row;
        point.column = impl->end_column;
    }
    return point;
}

const char *ts_node_type(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    if (!impl) return "null";
    
    switch (impl->type) {
        case 1: return "program";
        case 2: return "function_declaration";
        case 3: return "class_declaration";
        case 4: return "identifier";
        default: return "unknown";
    }
}

uint32_t ts_node_child_count(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    return impl ? impl->child_count : 0;
}

TSNode ts_node_child(TSNode node, uint32_t index) {
    struct TSNodeImpl* impl = node_to_impl(node);
    if (impl && index < (uint32_t)impl->child_count && impl->children) {
        return node_impl_to_node(&impl->children[index]);
    }
    
    // 無効なノードを返す
    TSNode invalid_node = {0};
    return invalid_node;
}

bool ts_node_is_null(TSNode node) {
    return node_to_impl(node) == NULL;
}

bool ts_node_is_named(TSNode node) {
    struct TSNodeImpl* impl = node_to_impl(node);
    return impl && impl->type > 0;
}

bool ts_node_has_error(TSNode node) {
    (void)node; // 未使用パラメータ
    return false; // プレースホルダーではエラーなし
}

//=============================================================================
// Tree cursor API
//=============================================================================

TSTreeCursor ts_tree_cursor_new(TSNode node) {
    TSTreeCursor cursor = {0};
    cursor.current_node_impl = node_to_impl(node);
    cursor.position = 0;
    return cursor;
}

void ts_tree_cursor_delete(TSTreeCursor *cursor) {
    (void)cursor; // プレースホルダーでは特に処理なし
}

bool ts_tree_cursor_goto_first_child(TSTreeCursor *cursor) {
    if (cursor && cursor->current_node_impl && cursor->current_node_impl->child_count > 0) {
        cursor->position = 0;
        return true;
    }
    return false;
}

bool ts_tree_cursor_goto_next_sibling(TSTreeCursor *cursor) {
    if (cursor && cursor->current_node_impl && 
        cursor->position + 1 < cursor->current_node_impl->child_count) {
        cursor->position++;
        return true;
    }
    return false;
}

TSNode ts_tree_cursor_current_node(const TSTreeCursor *cursor) {
    if (cursor && cursor->current_node_impl) {
        return node_impl_to_node(cursor->current_node_impl);
    }
    
    TSNode invalid_node = {0};
    return invalid_node;
}