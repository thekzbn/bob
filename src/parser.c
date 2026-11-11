#include "parser.h"

static void advance(Parser *p) {
    token_free(&p->current);
    p->current = p->peek;
    p->peek = lexer_next_token(p->lexer);
}

static bool check(Parser *p, TokenType type) {
    return p->current.type == type;
}

static bool match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return true;
    }
    return false;
}

static void skip_newlines(Parser *p) {
    while (check(p, TOKEN_NEWLINE)) {
        advance(p);
    }
}

static void parse_error(Parser *p, const char *msg) {
    p->has_error = true;
    snprintf(p->error_msg, sizeof(p->error_msg),
             "Error at line %d, col %d (token '%s'): %s",
             p->current.line, p->current.col,
             p->current.text ? p->current.text : token_type_name(p->current.type),
             msg);
    fprintf(stderr, "%s\n", p->error_msg);
}

static void push_block(Parser *p, BlockType type, BobElement *elem) {
    if (p->block_depth < sizeof(p->block_stack) / sizeof(p->block_stack[0])) {
        p->block_stack[p->block_depth].type = type;
        p->block_stack[p->block_depth].element = elem;
        p->block_depth++;
    }
}

static void pop_block(Parser *p) {
    if (p->block_depth > 0) {
        p->block_depth--;
    }
}

static BobElement *current_parent_element(Parser *p) {
    for (int i = (int)p->block_depth - 1; i >= 0; i--) {
        if (p->block_stack[i].type == BLOCK_ELEMENT) {
            return p->block_stack[i].element;
        }
    }
    return NULL;
}

Parser *parser_create(Lexer *lexer, MetadataRegistry *meta, IdGenerator *id_gen) {
    Parser *p = (Parser *)bob_calloc(1, sizeof(Parser));
    p->lexer = lexer;
    p->meta = meta;
    p->id_gen = id_gen;
    p->in_block = false;
    p->current_event_target = NULL;
    p->element_counter = 0;
    p->has_error = false;
    p->block_depth = 0;

    /* Prime current and peek */
    p->current = lexer_next_token(lexer);
    p->peek = lexer_next_token(lexer);
    return p;
}

void parser_free(Parser *parser) {
    if (parser) {
        token_free(&parser->current);
        token_free(&parser->peek);
        if (parser->current_event_target) free(parser->current_event_target);
        free(parser);
    }
}

/* Forward declarations */
static Expr *parse_expression(Parser *p);
static Stmt *parse_statement(Parser *p);

static bool is_svg_tag(const char *tag) {
    if (!tag) return false;
    const char *svg_tags[] = {
        "svg", "g", "path", "rect", "circle", "ellipse", "line",
        "polygon", "polyline", "text", "defs", "marker", "mask",
        "clipPath", "foreignObject", "use", "linearGradient",
        "radialGradient", "stop", "pattern", "symbol", "filter",
        "feDropShadow", "feGaussianBlur", "feOffset", NULL
    };
    for (int i = 0; svg_tags[i]; i++) {
        if (strcmp(tag, svg_tags[i]) == 0) return true;
    }
    return false;
}

/* Parse primary expressions including JS expressions & multiline strings */
static Expr *parse_primary(Parser *p) {
    if (check(p, TOKEN_KW_READ)) {
        int line = p->current.line;
        advance(p);
        if (!check(p, TOKEN_IDENT)) {
            parse_error(p, "Expected identifier after 'read'");
            return NULL;
        }
        char *target = p->current.text;
        Expr *e = expr_create_read(target, line);
        advance(p);
        return e;
    }

    /* JS expression: theme = js localStorage.getItem("theme") */
    if (check(p, TOKEN_KW_JS)) {
        int line = p->current.line;
        advance(p); /* consume 'js' */

        if (check(p, TOKEN_STRING) || check(p, TOKEN_TRIPLE_STRING)) {
            Expr *e = expr_create_js(p->current.text, line);
            advance(p);
            return e;
        }

        /* Read raw tokens up to newline, comma, or delimiter */
        StringBuilder *sb = sb_create();
        while (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_EOF) && !check(p, TOKEN_COMMA) && !check(p, TOKEN_RBRACKET)) {
            if (p->current.type == TOKEN_STRING) {
                sb_append_char(sb, '"');
                sb_append(sb, p->current.text);
                sb_append_char(sb, '"');
            } else {
                sb_append(sb, p->current.text ? p->current.text : token_type_name(p->current.type));
            }
            advance(p);
        }
        Expr *e = expr_create_js(sb->data, line);
        sb_free(sb);
        return e;
    }

    if (check(p, TOKEN_NUMBER)) {
        int line = p->current.line;
        bool is_uninit_zero = (strcmp(p->current.text, "0") == 0);
        bool is_zero_zero = (strcmp(p->current.text, "00") == 0);
        double val = atof(p->current.text);
        advance(p);
        if (is_uninit_zero) {
            return expr_create_uninit_zero(line);
        }
        return expr_create_number(val, is_zero_zero, line);
    }

    /* Single-line String */
    if (check(p, TOKEN_STRING)) {
        int line = p->current.line;
        Expr *e = expr_create_string(p->current.text, false, line);
        advance(p);
        return e;
    }

    /* Multi-line Triple String """ ... """ */
    if (check(p, TOKEN_TRIPLE_STRING)) {
        int line = p->current.line;
        Expr *e = expr_create_string(p->current.text, true, line);
        advance(p);
        return e;
    }

    if (check(p, TOKEN_IDENT)) {
        int line = p->current.line;
        Expr *e = expr_create_var(p->current.text, line);
        advance(p);
        return e;
    }

    if (match(p, TOKEN_LPAREN)) {
        Expr *e = parse_expression(p);
        if (!match(p, TOKEN_RPAREN)) {
            parse_error(p, "Expected ')' after parenthesized expression");
        }
        return e;
    }

    parse_error(p, "Unexpected token in expression");
    return NULL;
}

/* Parse multiplicative: *, / */
static Expr *parse_multiplicative(Parser *p) {
    Expr *left = parse_primary(p);
    if (!left) return NULL;

    while (check(p, TOKEN_STAR) || check(p, TOKEN_SLASH)) {
        int op = p->current.type == TOKEN_STAR ? '*' : '/';
        int line = p->current.line;
        advance(p);
        Expr *right = parse_primary(p);
        if (!right) return NULL;
        left = expr_create_binary(op, left, right, line);
    }
    return left;
}

/* Parse additive: +, - */
static Expr *parse_additive(Parser *p) {
    Expr *left = parse_multiplicative(p);
    if (!left) return NULL;

    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        int op = p->current.type == TOKEN_PLUS ? '+' : '-';
        int line = p->current.line;
        advance(p);
        Expr *right = parse_multiplicative(p);
        if (!right) return NULL;
        left = expr_create_binary(op, left, right, line);
    }
    return left;
}

/* Parse comparison / assignment: = */
static Expr *parse_expression(Parser *p) {
    Expr *left = parse_additive(p);
    if (!left) return NULL;

    if (match(p, TOKEN_EQUALS)) {
        int line = p->current.line;
        Expr *right = parse_expression(p);
        if (!right) return NULL;
        left = expr_create_binary('=', left, right, line);
    }
    return left;
}

/* Parse attribute block: [href="...", target="_blank", required, viewBox="0 0 100 100"] */
static void parse_attributes(Parser *p, BobElement *elem) {
    if (!match(p, TOKEN_LBRACKET)) return;

    while (!check(p, TOKEN_RBRACKET) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_IDENT) || check(p, TOKEN_STRING)) {
            char *attr_name = bob_strdup(p->current.text);
            advance(p);

            if (match(p, TOKEN_EQUALS)) {
                Expr *val = parse_expression(p);
                element_add_attribute(elem, attr_name, val, false);
            } else {
                /* Boolean attribute */
                element_add_attribute(elem, attr_name, NULL, true);
            }
            free(attr_name);
        } else {
            parse_error(p, "Expected attribute name in attribute list");
            break;
        }

        if (!match(p, TOKEN_COMMA)) {
            break;
        }
    }

    if (!match(p, TOKEN_RBRACKET)) {
        parse_error(p, "Expected ']' at end of attribute block");
    }
}

/* Helper to parse style modifiers on content side */
static void parse_content_modifiers(Parser *p, StyleModifiers *style) {
    while (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_COMMA) && !check(p, TOKEN_EOF) && !check(p, TOKEN_MAKE_ARROW)) {
        if (check(p, TOKEN_HEX_COLOR)) {
            if (style->text_color) free(style->text_color);
            style->text_color = bob_strdup(p->current.text);
            advance(p);
        } else if (check(p, TOKEN_FONT_LITERAL)) {
            if (style->font_family) free(style->font_family);
            style->font_family = bob_strdup(p->current.text);
            advance(p);
        } else if (check(p, TOKEN_NUMBER)) {
            style->font_size_rem = atof(p->current.text);
            advance(p);
        } else if (check(p, TOKEN_KW_BG)) {
            advance(p);
            if (check(p, TOKEN_HEX_COLOR) || check(p, TOKEN_IDENT)) {
                if (style->bg_color) free(style->bg_color);
                style->bg_color = bob_strdup(p->current.text);
                advance(p);
            }
        } else if (check(p, TOKEN_KW_BOLD)) {
            style->is_bold = true;
            advance(p);
        } else if (check(p, TOKEN_KW_ITALIC)) {
            style->is_italic = true;
            advance(p);
        } else if (check(p, TOKEN_KW_HIDE)) {
            style->is_hidden = true;
            advance(p);
        } else if (check(p, TOKEN_IDENT)) {
            if (style->text_color) free(style->text_color);
            style->text_color = bob_strdup(p->current.text);
            advance(p);
        } else {
            break;
        }
    }
}

/* Check if the upcoming tokens look like a child element */
static bool looks_like_element_start(Parser *p) {
    TokenType cur = p->current.type;
    TokenType nxt = p->peek.type;

    if (cur == TOKEN_IDENT || cur == TOKEN_KW_TELL) {
        if (nxt == TOKEN_COLON || nxt == TOKEN_LPAREN || nxt == TOKEN_LBRACKET || nxt == TOKEN_MAKE_ARROW) return true;
        if (strcmp(p->current.text, "center") == 0 ||
            strcmp(p->current.text, "left") == 0 ||
            strcmp(p->current.text, "right") == 0) {
            return true;
        }
    }
    return false;
}

/* Parse a single element (tag, custom id, attributes, content, modifiers) */
static BobElement *parse_single_element(Parser *p) {
    int line = p->current.line;
    char *alignment = NULL;

    /* Check element modifier before tag */
    if (check(p, TOKEN_IDENT) &&
        (strcmp(p->current.text, "center") == 0 ||
         strcmp(p->current.text, "left") == 0 ||
         strcmp(p->current.text, "right") == 0)) {
        alignment = bob_strdup(p->current.text);
        advance(p);
    }

    if (!check(p, TOKEN_IDENT) && !check(p, TOKEN_KW_TELL)) {
        parse_error(p, "Expected HTML/SVG tag name");
        if (alignment) free(alignment);
        return NULL;
    }

    char *tag = bob_strdup(p->current.type == TOKEN_KW_TELL ? "p" : p->current.text);
    advance(p);

    char *custom_id = NULL;
    if (match(p, TOKEN_LPAREN)) {
        if (!check(p, TOKEN_IDENT)) {
            parse_error(p, "Expected custom ID inside parentheses");
            free(tag);
            if (alignment) free(alignment);
            return NULL;
        }
        custom_id = bob_strdup(p->current.text);
        advance(p);
        if (!match(p, TOKEN_RPAREN)) {
            parse_error(p, "Expected ')' after custom ID");
        }
    }

    BobElement *elem = element_create(tag, custom_id, line);
    elem->style.alignment = alignment;

    /* Namespace detection */
    BobElement *parent = current_parent_element(p);
    if (is_svg_tag(tag) || (parent && parent->is_svg && strcmp(parent->tag, "foreignObject") != 0)) {
        elem->is_svg = true;
    }

    /* Attributes block [attr="val", ...] */
    if (check(p, TOKEN_LBRACKET)) {
        parse_attributes(p, elem);
    }

    free(tag);
    if (custom_id) free(custom_id);

    /* Fingerprint and generate automatic ID if no custom ID */
    if (!elem->custom_id) {
        char fp[256];
        snprintf(fp, sizeof(fp), "%s::elem_%d", elem->tag, p->element_counter++);
        ElementMeta *em = meta_find_or_register_element(p->meta, fp);
        elem->auto_id = id_gen_generate(p->id_gen, p->meta, em);
    }

    if (match(p, TOKEN_COLON)) {
        if (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_COMMA) && !check(p, TOKEN_EOF)) {
            elem->content = parse_expression(p);
        }
        parse_content_modifiers(p, &elem->style);
    }

    return elem;
}

/* Parse element statement with Rule C inline nesting OR make--> explicit blocks */
static Stmt *parse_element_statement(Parser *p) {
    int line = p->current.line;
    char *alignment = NULL;

    if (check(p, TOKEN_IDENT) &&
        (strcmp(p->current.text, "center") == 0 ||
         strcmp(p->current.text, "left") == 0 ||
         strcmp(p->current.text, "right") == 0)) {
        alignment = bob_strdup(p->current.text);
        advance(p);
    }

    if (!check(p, TOKEN_IDENT) && !check(p, TOKEN_KW_TELL)) {
        parse_error(p, "Expected element declaration");
        if (alignment) free(alignment);
        return NULL;
    }

    char *tag = bob_strdup(p->current.type == TOKEN_KW_TELL ? "p" : p->current.text);
    advance(p);

    char *custom_id = NULL;
    if (match(p, TOKEN_LPAREN)) {
        if (!check(p, TOKEN_IDENT)) {
            parse_error(p, "Expected custom ID inside parentheses");
            free(tag);
            if (alignment) free(alignment);
            return NULL;
        }
        custom_id = bob_strdup(p->current.text);
        advance(p);
        if (!match(p, TOKEN_RPAREN)) {
            parse_error(p, "Expected ')' after custom ID");
        }
    }

    BobElement *elem = element_create(tag, custom_id, line);
    elem->style.alignment = alignment;

    BobElement *parent = current_parent_element(p);
    if (is_svg_tag(tag) || (parent && parent->is_svg && strcmp(parent->tag, "foreignObject") != 0)) {
        elem->is_svg = true;
    }

    /* Attributes block */
    if (check(p, TOKEN_LBRACKET)) {
        parse_attributes(p, elem);
    }

    free(tag);
    if (custom_id) free(custom_id);

    if (!elem->custom_id) {
        char fp[256];
        snprintf(fp, sizeof(fp), "%s::elem_%d", elem->tag, p->element_counter++);
        ElementMeta *em = meta_find_or_register_element(p->meta, fp);
        elem->auto_id = id_gen_generate(p->id_gen, p->meta, em);
    }

    /* Check if opening an explicit element block: tag make--> */
    if (match(p, TOKEN_MAKE_ARROW)) {
        push_block(p, BLOCK_ELEMENT, elem);

        if (parent) {
            element_add_child(parent, elem);
        }

        skip_newlines(p);
        while (!check(p, TOKEN_CLOSE_ARROW) && !check(p, TOKEN_EOF)) {
            Stmt *child_stmt = parse_statement(p);
            (void)child_stmt;
            skip_newlines(p);
        }

        if (match(p, TOKEN_CLOSE_ARROW)) {
            pop_block(p);
        } else {
            parse_error(p, "Expected '<--' to close element block");
        }

        if (parent) {
            return NULL;
        }

        Stmt *stmt = stmt_create(STMT_ELEMENT_CREATE, line);
        stmt->as.elem_create.element = elem;
        return stmt;
    }

    /* If colonized: direct content or Rule C inline comma nesting */
    if (match(p, TOKEN_COLON)) {
        if (looks_like_element_start(p)) {
            while (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_EOF)) {
                BobElement *child = parse_single_element(p);
                if (child) {
                    element_add_child(elem, child);
                }
                if (!match(p, TOKEN_COMMA)) {
                    break;
                }
            }
        } else {
            if (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_EOF)) {
                elem->content = parse_expression(p);
            }
            parse_content_modifiers(p, &elem->style);
        }
    }

    if (parent) {
        element_add_child(parent, elem);
        return NULL;
    }

    Stmt *stmt = stmt_create(STMT_ELEMENT_CREATE, line);
    stmt->as.elem_create.element = elem;
    return stmt;
}

/* Parse class styling: .card: bg #ffffff */
static Stmt *parse_class_style(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume '.' */

    if (!check(p, TOKEN_IDENT)) {
        parse_error(p, "Expected class name after '.' in class selector");
        return NULL;
    }

    char *class_name = bob_strdup(p->current.text);
    advance(p);
    match(p, TOKEN_COLON);

    Stmt *stmt = stmt_create(STMT_CLASS_STYLE, line);
    stmt->as.class_style.class_name = class_name;
    parse_content_modifiers(p, &stmt->as.class_style.style);
    return stmt;
}

/* Parse class mutations: add_class | remove_class | toggle_class */
static Stmt *parse_class_mutate(Parser *p, int action) {
    int line = p->current.line;
    advance(p); /* consume keyword */

    if (!check(p, TOKEN_IDENT)) {
        parse_error(p, "Expected target identifier after class mutation command");
        return NULL;
    }
    char *target = bob_strdup(p->current.text);
    advance(p);

    Expr *cls_expr = parse_expression(p);

    Stmt *stmt = stmt_create(STMT_CLASS_MUTATE, line);
    stmt->as.class_mutate.action = action;
    stmt->as.class_mutate.target = target;
    stmt->as.class_mutate.class_expr = cls_expr;
    return stmt;
}

/* Parse JS statement: js: ... or js: """ ... """ */
static Stmt *parse_js_statement(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'js' */
    match(p, TOKEN_COLON);

    char *code = NULL;
    if (check(p, TOKEN_TRIPLE_STRING) || check(p, TOKEN_STRING)) {
        code = bob_strdup(p->current.text);
        advance(p);
    } else {
        StringBuilder *sb = sb_create();
        while (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_EOF)) {
            if (p->current.type == TOKEN_STRING) {
                sb_append_char(sb, '"');
                sb_append(sb, p->current.text);
                sb_append_char(sb, '"');
            } else {
                sb_append(sb, p->current.text ? p->current.text : token_type_name(p->current.type));
            }
            advance(p);
        }
        code = bob_strdup(sb->data);
        sb_free(sb);
    }

    Stmt *stmt = stmt_create(STMT_JS_STMT, line);
    stmt->as.js_stmt.code = code;
    return stmt;
}

/* Parse ID configuration: id .org | id dhar .ng | id / / */
static Stmt *parse_id_config(Parser *p) {
    int line = p->current.line;
    advance(p);

    char *prefix = NULL;
    char *suffix = NULL;

    if (match(p, TOKEN_SLASH)) {
        if (match(p, TOKEN_SLASH)) {
            prefix = bob_strdup("");
            suffix = bob_strdup("");
        }
    } else if (check(p, TOKEN_IDENT)) {
        prefix = bob_strdup(p->current.text);
        advance(p);
        if (check(p, TOKEN_IDENT)) {
            suffix = bob_strdup(p->current.text);
            advance(p);
        } else {
            suffix = prefix;
            prefix = bob_strdup("bob");
        }
    }

    id_gen_set_config(p->id_gen, prefix, suffix);

    Stmt *stmt = stmt_create(STMT_ID_CONFIG, line);
    stmt->as.id_config.prefix = prefix;
    stmt->as.id_config.suffix = suffix;
    return stmt;
}

/* Parse global styles: page: ... or all: ... */
static Stmt *parse_global_style(Parser *p, StmtType type) {
    int line = p->current.line;
    advance(p);
    match(p, TOKEN_COLON);

    Stmt *stmt = stmt_create(type, line);
    parse_content_modifiers(p, &stmt->as.global_style.style);
    return stmt;
}

/* Parse smooth transition */
static Stmt *parse_smooth(Parser *p) {
    int line = p->current.line;
    advance(p);

    char *prop = NULL;
    if (check(p, TOKEN_KW_TEXT) || check(p, TOKEN_IDENT)) {
        prop = bob_strdup(p->current.text);
        advance(p);
    }

    if (!match(p, TOKEN_SMOOTH_ARROW)) {
        parse_error(p, "Expected '->' in smooth transition declaration");
        if (prop) free(prop);
        return NULL;
    }

    if (!check(p, TOKEN_IDENT)) {
        parse_error(p, "Expected target identifier after '->'");
        if (prop) free(prop);
        return NULL;
    }
    char *target = bob_strdup(p->current.text);
    advance(p);

    double duration = 0.3;
    if (check(p, TOKEN_NUMBER)) {
        duration = atof(p->current.text);
        advance(p);
    }

    Stmt *stmt = stmt_create(STMT_SMOOTH, line);
    stmt->as.smooth.property = prop;
    stmt->as.smooth.target = target;
    stmt->as.smooth.duration = duration;
    return stmt;
}

/* Parse block body until one of the closing markers */
static void parse_block_body(Parser *p, Stmt ***out_body, size_t *out_count) {
    size_t capacity = 8;
    Stmt **body = (Stmt **)bob_malloc(sizeof(Stmt *) * capacity);
    size_t count = 0;

    skip_newlines(p);
    while (!check(p, TOKEN_KW_OR_ELSE) &&
           !check(p, TOKEN_KW_OR_ELSE_IF) &&
           !check(p, TOKEN_CLOSE_ARROW) &&
           !check(p, TOKEN_EOF)) {
        Stmt *s = parse_statement(p);
        if (s) {
            if (count >= capacity) {
                capacity *= 2;
                body = (Stmt **)bob_realloc(body, sizeof(Stmt *) * capacity);
            }
            body[count++] = s;
        }
        skip_newlines(p);
    }

    *out_body = body;
    *out_count = count;
}

/* Parse conditional block */
static Stmt *parse_if_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'if' */

    Expr *condition = parse_expression(p);
    if (!match(p, TOKEN_MAKE_ARROW)) {
        parse_error(p, "Expected 'make-->' after if condition");
        expr_free(condition);
        return NULL;
    }

    Stmt *stmt = stmt_create(STMT_IF, line);
    stmt->as.if_stmt.condition = condition;

    bool prev_in_block = p->in_block;
    p->in_block = true;
    push_block(p, BLOCK_IF, NULL);

    parse_block_body(p, &stmt->as.if_stmt.then_body, &stmt->as.if_stmt.then_count);

    size_t elif_cap = 4;
    stmt->as.if_stmt.elifs = (ElseIfBranch *)bob_malloc(sizeof(ElseIfBranch) * elif_cap);
    stmt->as.if_stmt.elif_count = 0;

    while (match(p, TOKEN_KW_OR_ELSE_IF)) {
        Expr *elif_cond = parse_expression(p);
        if (!match(p, TOKEN_MAKE_ARROW)) {
            parse_error(p, "Expected 'make-->' after 'or else if' condition");
        }
        if (stmt->as.if_stmt.elif_count >= elif_cap) {
            elif_cap *= 2;
            stmt->as.if_stmt.elifs = (ElseIfBranch *)bob_realloc(stmt->as.if_stmt.elifs, sizeof(ElseIfBranch) * elif_cap);
        }
        ElseIfBranch *b = &stmt->as.if_stmt.elifs[stmt->as.if_stmt.elif_count++];
        b->condition = elif_cond;
        parse_block_body(p, &b->body, &b->body_count);
    }

    if (match(p, TOKEN_KW_OR_ELSE)) {
        parse_block_body(p, &stmt->as.if_stmt.else_body, &stmt->as.if_stmt.else_count);
    }

    if (!match(p, TOKEN_CLOSE_ARROW)) {
        parse_error(p, "Expected '<--' to close if statement");
    }

    pop_block(p);
    p->in_block = prev_in_block;
    return stmt;
}

/* Parse event listener */
static Stmt *parse_when_click(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'when' */

    if (!match(p, TOKEN_KW_CLICK)) {
        parse_error(p, "Expected 'click' after 'when'");
        return NULL;
    }

    if (!match(p, TOKEN_LPAREN)) {
        parse_error(p, "Expected '(' after 'click'");
        return NULL;
    }

    if (!check(p, TOKEN_IDENT)) {
        parse_error(p, "Expected target identifier inside click(...)");
        return NULL;
    }

    char *target = bob_strdup(p->current.text);
    advance(p);

    if (!match(p, TOKEN_RPAREN)) {
        parse_error(p, "Expected ')' after target in click(...)");
    }

    if (!match(p, TOKEN_MAKE_ARROW)) {
        parse_error(p, "Expected 'make-->' after when click(...)");
    }

    Stmt *stmt = stmt_create(STMT_WHEN_CLICK, line);
    stmt->as.when_click.target = target;

    bool prev_in_block = p->in_block;
    char *prev_event_target = p->current_event_target;
    p->in_block = true;
    p->current_event_target = bob_strdup(target);
    push_block(p, BLOCK_WHEN, NULL);

    parse_block_body(p, &stmt->as.when_click.body, &stmt->as.when_click.body_count);

    if (!match(p, TOKEN_CLOSE_ARROW)) {
        parse_error(p, "Expected '<--' to close when block");
    }

    pop_block(p);
    p->in_block = prev_in_block;
    if (p->current_event_target) free(p->current_event_target);
    p->current_event_target = prev_event_target;

    return stmt;
}

/* Parse a single statement */
static Stmt *parse_statement(Parser *p) {
    skip_newlines(p);
    if (check(p, TOKEN_EOF)) return NULL;

    int line = p->current.line;

    /* Class styling: .card: bg #ffffff */
    if (check(p, TOKEN_DOT)) {
        return parse_class_style(p);
    }

    /* Class mutations: add_class, remove_class, toggle_class */
    if (check(p, TOKEN_KW_ADD_CLASS)) {
        return parse_class_mutate(p, 0);
    }
    if (check(p, TOKEN_KW_REMOVE_CLASS)) {
        return parse_class_mutate(p, 1);
    }
    if (check(p, TOKEN_KW_TOGGLE_CLASS)) {
        return parse_class_mutate(p, 2);
    }

    /* JS statement */
    if (check(p, TOKEN_KW_JS)) {
        return parse_js_statement(p);
    }

    /* ID config */
    if (check(p, TOKEN_KW_ID)) {
        return parse_id_config(p);
    }

    /* Page styling */
    if (check(p, TOKEN_KW_PAGE)) {
        return parse_global_style(p, STMT_PAGE_STYLE);
    }

    /* All styling */
    if (check(p, TOKEN_KW_ALL)) {
        return parse_global_style(p, STMT_ALL_STYLE);
    }

    /* Output commands */
    if (check(p, TOKEN_KW_WHISPER) || check(p, TOKEN_KW_YELL)) {
        StmtType type = check(p, TOKEN_KW_WHISPER) ? STMT_OUTPUT_WHISPER : STMT_OUTPUT_YELL;
        advance(p);
        match(p, TOKEN_COLON);
        Expr *e = parse_expression(p);
        Stmt *s = stmt_create(type, line);
        s->as.output.expr = e;
        return s;
    }

    /* Visibility: show, hide */
    if (check(p, TOKEN_KW_SHOW) || check(p, TOKEN_KW_HIDE)) {
        StmtType type = check(p, TOKEN_KW_SHOW) ? STMT_SHOW : STMT_HIDE;
        advance(p);
        if (!check(p, TOKEN_IDENT)) {
            parse_error(p, "Expected target identifier after show/hide");
            return NULL;
        }
        char *target = bob_strdup(p->current.text);
        advance(p);
        Stmt *s = stmt_create(type, line);
        s->as.visibility.target = target;
        return s;
    }

    /* Smooth transition */
    if (check(p, TOKEN_KW_SMOOTH)) {
        return parse_smooth(p);
    }

    /* Conditionals */
    if (check(p, TOKEN_KW_IF)) {
        return parse_if_stmt(p);
    }

    /* Event listeners */
    if (check(p, TOKEN_KW_WHEN)) {
        return parse_when_click(p);
    }

    /* Variable assignment: var = expr */
    if (check(p, TOKEN_IDENT) && p->peek.type == TOKEN_EQUALS) {
        char *var_name = bob_strdup(p->current.text);
        advance(p); /* ident */
        advance(p); /* '=' */
        Expr *val = parse_expression(p);
        Stmt *s = stmt_create(STMT_VAR_ASSIGN, line);
        s->as.var_assign.name = var_name;
        s->as.var_assign.value = val;
        return s;
    }

    /* Inside block: element update */
    if (p->in_block && (check(p, TOKEN_IDENT) || check(p, TOKEN_KW_TELL))) {
        if (p->peek.type == TOKEN_COLON || p->peek.type == TOKEN_EXCLAMATION) {
            char *target = bob_strdup(p->current.type == TOKEN_KW_TELL ? "p" : p->current.text);
            advance(p);
            if (match(p, TOKEN_EXCLAMATION)) {
                if (check(p, TOKEN_IDENT)) {
                    size_t new_len = strlen(target) + strlen(p->current.text) + 2;
                    char *new_target = (char *)bob_malloc(new_len);
                    snprintf(new_target, new_len, "%s!%s", target, p->current.text);
                    free(target);
                    target = new_target;
                    advance(p);
                }
            }
            match(p, TOKEN_COLON);
            Expr *content = parse_expression(p);
            Stmt *s = stmt_create(STMT_ELEMENT_UPDATE, line);
            s->as.elem_update.target = target;
            s->as.elem_update.new_content = content;
            parse_content_modifiers(p, &s->as.elem_update.style);
            return s;
        }
    }

    /* Element creation / declaration */
    return parse_element_statement(p);
}

Program *parser_parse_program(Parser *p) {
    Program *prog = program_create();
    skip_newlines(p);

    while (!check(p, TOKEN_EOF)) {
        Stmt *s = parse_statement(p);
        if (s) {
            program_add_stmt(prog, s);
        } else if (p->has_error) {
            break;
        }
        skip_newlines(p);
    }

    return prog;
}
