#ifndef BOB_LEXER_H
#define BOB_LEXER_H

#include "common.h"

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_NEWLINE,
    
    /* Literals */
    TOKEN_IDENT,
    TOKEN_STRING,        /* "double-quoted" */
    TOKEN_TRIPLE_STRING, /* """multi-line literal""" */
    TOKEN_FONT_LITERAL,  /* 'single-quoted' */
    TOKEN_NUMBER,        /* 123, 1.25, 0, 00 */
    TOKEN_HEX_COLOR,     /* #0b2e1e */
    
    /* Punctuation & Operators */
    TOKEN_COLON,        /* : */
    TOKEN_COMMA,        /* , */
    TOKEN_DOT,          /* . */
    TOKEN_EQUALS,       /* = */
    TOKEN_LPAREN,       /* ( */
    TOKEN_RPAREN,       /* ) */
    TOKEN_LBRACKET,     /* [ */
    TOKEN_RBRACKET,     /* ] */
    TOKEN_EXCLAMATION,  /* ! */
    TOKEN_PLUS,         /* + */
    TOKEN_MINUS,        /* - */
    TOKEN_STAR,         /* * */
    TOKEN_SLASH,        /* / */
    
    /* Arrows and Block Markers */
    TOKEN_MAKE_ARROW,   /* make--> */
    TOKEN_CLOSE_ARROW,  /* <-- */
    TOKEN_SMOOTH_ARROW, /* -> */
    
    /* Keywords */
    TOKEN_KW_IF,
    TOKEN_KW_OR_ELSE_IF, /* or else if */
    TOKEN_KW_OR_ELSE,    /* or else--> */
    TOKEN_KW_WHEN,
    TOKEN_KW_CLICK,
    TOKEN_KW_WHISPER,
    TOKEN_KW_YELL,
    TOKEN_KW_TELL,
    TOKEN_KW_SHOW,
    TOKEN_KW_HIDE,
    TOKEN_KW_READ,
    TOKEN_KW_SMOOTH,
    TOKEN_KW_TEXT,
    TOKEN_KW_PAGE,
    TOKEN_KW_ALL,
    TOKEN_KW_ID,
    TOKEN_KW_BG,
    TOKEN_KW_BOLD,
    TOKEN_KW_ITALIC,
    
    /* Class and Interop Keywords */
    TOKEN_KW_ADD_CLASS,    /* add_class */
    TOKEN_KW_REMOVE_CLASS, /* remove_class */
    TOKEN_KW_TOGGLE_CLASS, /* toggle_class */
    TOKEN_KW_JS,           /* js */
    
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int line;
    int col;
} Token;

typedef struct {
    const char *source;
    size_t length;
    size_t pos;
    int line;
    int col;
} Lexer;

Lexer *lexer_create(const char *source);
void lexer_free(Lexer *lexer);
Token lexer_next_token(Lexer *lexer);
void token_free(Token *token);
const char *token_type_name(TokenType type);

#endif /* BOB_LEXER_H */
