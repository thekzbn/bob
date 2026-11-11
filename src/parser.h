#ifndef BOB_PARSER_H
#define BOB_PARSER_H

#include "lexer.h"
#include "ast.h"
#include "meta.h"
#include "id_gen.h"

typedef enum {
    BLOCK_IF,
    BLOCK_WHEN,
    BLOCK_ELEMENT
} BlockType;

typedef struct {
    BlockType type;
    BobElement *element; /* if BLOCK_ELEMENT */
} BlockFrame;

typedef struct {
    Lexer *lexer;
    Token current;
    Token peek;
    MetadataRegistry *meta;
    IdGenerator *id_gen;
    bool in_block;              /* true inside if or when blocks */
    char *current_event_target; /* for implicit this targeting */
    int element_counter;        /* to aid fingerprinting sibling order */
    bool has_error;
    char error_msg[256];

    /* Block frame stack for arbitrary-depth nesting and control flow */
    BlockFrame block_stack[128];
    size_t block_depth;
} Parser;

Parser *parser_create(Lexer *lexer, MetadataRegistry *meta, IdGenerator *id_gen);
void parser_free(Parser *parser);
Program *parser_parse_program(Parser *parser);

#endif /* BOB_PARSER_H */
