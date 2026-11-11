#ifndef BOB_AST_H
#define BOB_AST_H

#include "common.h"

typedef enum {
    EXPR_UNINIT_ZERO,  /* 0 */
    EXPR_NUMBER,       /* 00, 5, 1.25 */
    EXPR_STRING,       /* "hello", """multiline""" */
    EXPR_VAR,          /* dog */
    EXPR_BINARY,       /* expr + expr */
    EXPR_ASSIGN,       /* var = expr */
    EXPR_READ,         /* read target */
    EXPR_JS            /* js expression */
} ExprType;

typedef struct Expr {
    ExprType type;
    int line;
    union {
        double num_value;
        char *str_value;
        char *var_name;
        struct {
            int op; /* '+', '-', '*', '/', '=' */
            struct Expr *left;
            struct Expr *right;
        } binary;
        struct {
            char *target_name;
        } read;
        struct {
            char *code;
        } js_expr;
    } as;
    bool is_zero_zero; /* true for '00' */
    bool is_multiline; /* true for """ */
} Expr;

/* Modifiers for elements and content */
typedef struct {
    char *alignment;      /* "center", "left", "right" */
    char *text_color;     /* "red", "#0b2e1e", "#1e90ff" */
    char *bg_color;       /* "#d4af37" */
    char *font_family;    /* "Inter" */
    double font_size_rem; /* 1.25 (0 means not specified) */
    bool is_bold;
    bool is_italic;
    bool is_hidden;
} StyleModifiers;

/* Attribute for elements: e.g. [href="...", target="_blank", required, viewBox="0 0 100 100"] */
typedef struct {
    char *name;
    Expr *value; /* NULL if boolean attribute */
    bool is_boolean;
} Attribute;

/* DOM Element Declaration (HTML or SVG) */
typedef struct BobElement {
    char *tag;
    char *custom_id;
    char *auto_id;
    Expr *content;
    StyleModifiers style;
    int line;
    bool is_svg;

    /* Attributes e.g. [href="...", aria-label="..."] */
    Attribute *attributes;
    size_t attr_count;
    size_t attr_capacity;

    /* Children for nesting rule: div: h1: "...", a: "..." or explicit blocks */
    struct BobElement **children;
    size_t child_count;
    size_t child_capacity;
} BobElement;

typedef enum {
    STMT_VAR_ASSIGN,
    STMT_ID_CONFIG,
    STMT_PAGE_STYLE,
    STMT_ALL_STYLE,
    STMT_CLASS_STYLE,
    STMT_ELEMENT_CREATE,
    STMT_ELEMENT_UPDATE,
    STMT_OUTPUT_WHISPER,
    STMT_OUTPUT_YELL,
    STMT_SHOW,
    STMT_HIDE,
    STMT_SMOOTH,
    STMT_IF,
    STMT_WHEN_CLICK,
    STMT_CLASS_MUTATE,
    STMT_JS_STMT
} StmtType;

struct Stmt;

typedef struct {
    Expr *condition;
    struct Stmt **body;
    size_t body_count;
} ElseIfBranch;

typedef struct Stmt {
    StmtType type;
    int line;
    union {
        struct {
            char *name;
            Expr *value;
        } var_assign;

        struct {
            char *prefix;
            char *suffix;
        } id_config;

        struct {
            StyleModifiers style;
        } global_style;

        struct {
            char *class_name;
            StyleModifiers style;
        } class_style;

        struct {
            BobElement *element;
        } elem_create;

        struct {
            char *target;
            Expr *new_content;
            StyleModifiers style;
        } elem_update;

        struct {
            Expr *expr;
        } output;

        struct {
            char *target;
        } visibility;

        struct {
            char *property; /* "text" */
            char *target;
            double duration;
        } smooth;

        struct {
            Expr *condition;
            struct Stmt **then_body;
            size_t then_count;
            ElseIfBranch *elifs;
            size_t elif_count;
            struct Stmt **else_body;
            size_t else_count;
        } if_stmt;

        struct {
            char *target; /* e.g. "button", "hello_btn" */
            struct Stmt **body;
            size_t body_count;
        } when_click;

        struct {
            int action; /* 0: add, 1: remove, 2: toggle */
            char *target;
            Expr *class_expr;
        } class_mutate;

        struct {
            char *code;
        } js_stmt;
    } as;
} Stmt;

typedef struct {
    Stmt **statements;
    size_t count;
    size_t capacity;
} Program;

Expr *expr_create_uninit_zero(int line);
Expr *expr_create_number(double val, bool is_zero_zero, int line);
Expr *expr_create_string(const char *str, bool is_multiline, int line);
Expr *expr_create_var(const char *name, int line);
Expr *expr_create_binary(int op, Expr *left, Expr *right, int line);
Expr *expr_create_read(const char *target, int line);
Expr *expr_create_js(const char *code, int line);
void expr_free(Expr *expr);

BobElement *element_create(const char *tag, const char *custom_id, int line);
void element_add_child(BobElement *parent, BobElement *child);
void element_add_attribute(BobElement *elem, const char *name, Expr *value, bool is_boolean);
void element_free(BobElement *elem);

Stmt *stmt_create(StmtType type, int line);
void stmt_free(Stmt *stmt);

Program *program_create(void);
void program_add_stmt(Program *prog, Stmt *stmt);
void program_free(Program *prog);

#endif /* BOB_AST_H */
