#include "ast.h"

Expr *expr_create_uninit_zero(int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_UNINIT_ZERO;
    e->line = line;
    e->is_zero_zero = false;
    e->is_multiline = false;
    return e;
}

Expr *expr_create_number(double val, bool is_zero_zero, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_NUMBER;
    e->line = line;
    e->as.num_value = val;
    e->is_zero_zero = is_zero_zero;
    e->is_multiline = false;
    return e;
}

Expr *expr_create_string(const char *str, bool is_multiline, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_STRING;
    e->line = line;
    e->as.str_value = bob_strdup(str);
    e->is_zero_zero = false;
    e->is_multiline = is_multiline;
    return e;
}

Expr *expr_create_var(const char *name, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_VAR;
    e->line = line;
    e->as.var_name = bob_strdup(name);
    e->is_zero_zero = false;
    e->is_multiline = false;
    return e;
}

Expr *expr_create_binary(int op, Expr *left, Expr *right, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = (op == '=') ? EXPR_ASSIGN : EXPR_BINARY;
    e->line = line;
    e->as.binary.op = op;
    e->as.binary.left = left;
    e->as.binary.right = right;
    e->is_zero_zero = false;
    e->is_multiline = false;
    return e;
}

Expr *expr_create_read(const char *target, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_READ;
    e->line = line;
    e->as.read.target_name = bob_strdup(target);
    e->is_zero_zero = false;
    e->is_multiline = false;
    return e;
}

Expr *expr_create_js(const char *code, int line) {
    Expr *e = (Expr *)bob_malloc(sizeof(Expr));
    e->type = EXPR_JS;
    e->line = line;
    e->as.js_expr.code = bob_strdup(code);
    e->is_zero_zero = false;
    e->is_multiline = false;
    return e;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    switch (expr->type) {
        case EXPR_STRING:
            free(expr->as.str_value);
            break;
        case EXPR_VAR:
            free(expr->as.var_name);
            break;
        case EXPR_BINARY:
        case EXPR_ASSIGN:
            expr_free(expr->as.binary.left);
            expr_free(expr->as.binary.right);
            break;
        case EXPR_READ:
            free(expr->as.read.target_name);
            break;
        case EXPR_JS:
            free(expr->as.js_expr.code);
            break;
        default:
            break;
    }
    free(expr);
}

BobElement *element_create(const char *tag, const char *custom_id, int line) {
    BobElement *elem = (BobElement *)bob_calloc(1, sizeof(BobElement));
    elem->tag = bob_strdup(tag);
    if (custom_id) elem->custom_id = bob_strdup(custom_id);
    elem->line = line;
    elem->is_svg = false;

    elem->attr_capacity = 4;
    elem->attributes = (Attribute *)bob_malloc(sizeof(Attribute) * elem->attr_capacity);
    elem->attr_count = 0;

    elem->child_capacity = 4;
    elem->children = (BobElement **)bob_malloc(sizeof(BobElement *) * elem->child_capacity);
    elem->child_count = 0;
    return elem;
}

void element_add_child(BobElement *parent, BobElement *child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = (BobElement **)bob_realloc(parent->children, sizeof(BobElement *) * parent->child_capacity);
    }
    parent->children[parent->child_count++] = child;
}

void element_add_attribute(BobElement *elem, const char *name, Expr *value, bool is_boolean) {
    if (elem->attr_count >= elem->attr_capacity) {
        elem->attr_capacity *= 2;
        elem->attributes = (Attribute *)bob_realloc(elem->attributes, sizeof(Attribute) * elem->attr_capacity);
    }
    Attribute *attr = &elem->attributes[elem->attr_count++];
    attr->name = bob_strdup(name);
    attr->value = value;
    attr->is_boolean = is_boolean;
}

static void free_style_modifiers(StyleModifiers *style) {
    if (style->alignment) free(style->alignment);
    if (style->text_color) free(style->text_color);
    if (style->bg_color) free(style->bg_color);
    if (style->font_family) free(style->font_family);
}

void element_free(BobElement *elem) {
    if (!elem) return;
    if (elem->tag) free(elem->tag);
    if (elem->custom_id) free(elem->custom_id);
    if (elem->auto_id) free(elem->auto_id);
    expr_free(elem->content);
    free_style_modifiers(&elem->style);

    for (size_t i = 0; i < elem->attr_count; i++) {
        free(elem->attributes[i].name);
        if (elem->attributes[i].value) {
            expr_free(elem->attributes[i].value);
        }
    }
    free(elem->attributes);

    for (size_t i = 0; i < elem->child_count; i++) {
        element_free(elem->children[i]);
    }
    free(elem->children);
    free(elem);
}

Stmt *stmt_create(StmtType type, int line) {
    Stmt *s = (Stmt *)bob_calloc(1, sizeof(Stmt));
    s->type = type;
    s->line = line;
    return s;
}

void stmt_free(Stmt *stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case STMT_VAR_ASSIGN:
            free(stmt->as.var_assign.name);
            expr_free(stmt->as.var_assign.value);
            break;
        case STMT_ID_CONFIG:
            if (stmt->as.id_config.prefix) free(stmt->as.id_config.prefix);
            if (stmt->as.id_config.suffix) free(stmt->as.id_config.suffix);
            break;
        case STMT_PAGE_STYLE:
        case STMT_ALL_STYLE:
            free_style_modifiers(&stmt->as.global_style.style);
            break;
        case STMT_CLASS_STYLE:
            free(stmt->as.class_style.class_name);
            free_style_modifiers(&stmt->as.class_style.style);
            break;
        case STMT_ELEMENT_CREATE:
            element_free(stmt->as.elem_create.element);
            break;
        case STMT_ELEMENT_UPDATE:
            free(stmt->as.elem_update.target);
            expr_free(stmt->as.elem_update.new_content);
            free_style_modifiers(&stmt->as.elem_update.style);
            break;
        case STMT_OUTPUT_WHISPER:
        case STMT_OUTPUT_YELL:
            expr_free(stmt->as.output.expr);
            break;
        case STMT_SHOW:
        case STMT_HIDE:
            free(stmt->as.visibility.target);
            break;
        case STMT_SMOOTH:
            free(stmt->as.smooth.property);
            free(stmt->as.smooth.target);
            break;
        case STMT_IF:
            expr_free(stmt->as.if_stmt.condition);
            for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                stmt_free(stmt->as.if_stmt.then_body[i]);
            }
            free(stmt->as.if_stmt.then_body);
            for (size_t i = 0; i < stmt->as.if_stmt.elif_count; i++) {
                expr_free(stmt->as.if_stmt.elifs[i].condition);
                for (size_t j = 0; j < stmt->as.if_stmt.elifs[i].body_count; j++) {
                    stmt_free(stmt->as.if_stmt.elifs[i].body[j]);
                }
                free(stmt->as.if_stmt.elifs[i].body);
            }
            free(stmt->as.if_stmt.elifs);
            for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                stmt_free(stmt->as.if_stmt.else_body[i]);
            }
            free(stmt->as.if_stmt.else_body);
            break;
        case STMT_WHEN_CLICK:
            free(stmt->as.when_click.target);
            for (size_t i = 0; i < stmt->as.when_click.body_count; i++) {
                stmt_free(stmt->as.when_click.body[i]);
            }
            free(stmt->as.when_click.body);
            break;
        case STMT_CLASS_MUTATE:
            free(stmt->as.class_mutate.target);
            expr_free(stmt->as.class_mutate.class_expr);
            break;
        case STMT_JS_STMT:
            free(stmt->as.js_stmt.code);
            break;
        default:
            break;
    }
    free(stmt);
}

Program *program_create(void) {
    Program *p = (Program *)bob_calloc(1, sizeof(Program));
    p->capacity = 16;
    p->statements = (Stmt **)bob_malloc(sizeof(Stmt *) * p->capacity);
    p->count = 0;
    return p;
}

void program_add_stmt(Program *prog, Stmt *stmt) {
    if (prog->count >= prog->capacity) {
        prog->capacity *= 2;
        prog->statements = (Stmt **)bob_realloc(prog->statements, sizeof(Stmt *) * prog->capacity);
    }
    prog->statements[prog->count++] = stmt;
}

void program_free(Program *prog) {
    if (!prog) return;
    for (size_t i = 0; i < prog->count; i++) {
        stmt_free(prog->statements[i]);
    }
    free(prog->statements);
    free(prog);
}
