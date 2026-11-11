#include "lexer.h"

Lexer *lexer_create(const char *source) {
    Lexer *lexer = (Lexer *)bob_malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->col = 1;
    return lexer;
}

void lexer_free(Lexer *lexer) {
    if (lexer) free(lexer);
}

void token_free(Token *token) {
    if (token && token->text) {
        free(token->text);
        token->text = NULL;
    }
}

static char peek(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->source[lexer->pos];
}

static char peek_next(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->pos + 1];
}

static char peek_at(Lexer *lexer, size_t offset) {
    if (lexer->pos + offset >= lexer->length) return '\0';
    return lexer->source[lexer->pos + offset];
}

static char advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

static bool match_str(Lexer *lexer, const char *pattern) {
    size_t len = strlen(pattern);
    if (lexer->pos + len > lexer->length) return false;
    if (strncmp(lexer->source + lexer->pos, pattern, len) == 0) {
        for (size_t i = 0; i < len; i++) {
            advance(lexer);
        }
        return true;
    }
    return false;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            /* Comment until newline */
            while (peek(lexer) != '\0' && peek(lexer) != '\n') {
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static Token make_token(TokenType type, const char *text, int line, int col) {
    Token token;
    token.type = type;
    token.text = text ? bob_strdup(text) : NULL;
    token.line = line;
    token.col = col;
    return token;
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int start_line = lexer->line;
    int start_col = lexer->col;

    if (lexer->pos >= lexer->length) {
        return make_token(TOKEN_EOF, "", start_line, start_col);
    }

    char c = peek(lexer);

    /* Newline handling */
    if (c == '\n') {
        advance(lexer);
        /* Collapse consecutive newlines and spaces */
        while (1) {
            skip_whitespace_and_comments(lexer);
            if (peek(lexer) == '\n') {
                advance(lexer);
            } else {
                break;
            }
        }
        return make_token(TOKEN_NEWLINE, "\\n", start_line, start_col);
    }

    /* Arrow check: <-- */
    if (c == '<' && peek_next(lexer) == '-') {
        if (match_str(lexer, "<--")) {
            return make_token(TOKEN_CLOSE_ARROW, "<--", start_line, start_col);
        }
    }

    /* Arrow check: -> */
    if (c == '-' && peek_next(lexer) == '>') {
        advance(lexer);
        advance(lexer);
        return make_token(TOKEN_SMOOTH_ARROW, "->", start_line, start_col);
    }

    /* Multi-line Triple String: """ ... """ */
    if (c == '"' && peek_next(lexer) == '"' && peek_at(lexer, 2) == '"') {
        advance(lexer); /* " */
        advance(lexer); /* " */
        advance(lexer); /* " */
        StringBuilder *sb = sb_create();
        while (lexer->pos < lexer->length) {
            if (peek(lexer) == '"' && peek_next(lexer) == '"' && peek_at(lexer, 2) == '"') {
                advance(lexer);
                advance(lexer);
                advance(lexer);
                break;
            }
            sb_append_char(sb, advance(lexer));
        }
        Token t = make_token(TOKEN_TRIPLE_STRING, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* Single-line String: "..." */
    if (c == '"') {
        advance(lexer); /* consume opening quote */
        StringBuilder *sb = sb_create();
        while (peek(lexer) != '\0' && peek(lexer) != '"') {
            if (peek(lexer) == '\\') {
                advance(lexer);
                char esc = advance(lexer);
                if (esc == 'n') sb_append_char(sb, '\n');
                else if (esc == 't') sb_append_char(sb, '\t');
                else if (esc == '"') sb_append_char(sb, '"');
                else if (esc == '\\') sb_append_char(sb, '\\');
                else sb_append_char(sb, esc);
            } else {
                sb_append_char(sb, advance(lexer));
            }
        }
        if (peek(lexer) == '"') {
            advance(lexer); /* consume closing quote */
        }
        Token t = make_token(TOKEN_STRING, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* Font Literals: '...' */
    if (c == '\'') {
        advance(lexer); /* consume opening quote */
        StringBuilder *sb = sb_create();
        while (peek(lexer) != '\0' && peek(lexer) != '\'') {
            sb_append_char(sb, advance(lexer));
        }
        if (peek(lexer) == '\'') {
            advance(lexer); /* consume closing quote */
        }
        Token t = make_token(TOKEN_FONT_LITERAL, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* Hex Color: #0b2e1e */
    if (c == '#') {
        StringBuilder *sb = sb_create();
        sb_append_char(sb, advance(lexer)); /* consume # */
        while (isxdigit((unsigned char)peek(lexer))) {
            sb_append_char(sb, advance(lexer));
        }
        Token t = make_token(TOKEN_HEX_COLOR, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* Numbers: 0, 00, 123, 1.25 */
    if (isdigit((unsigned char)c)) {
        StringBuilder *sb = sb_create();
        while (isdigit((unsigned char)peek(lexer))) {
            sb_append_char(sb, advance(lexer));
        }
        if (peek(lexer) == '.' && isdigit((unsigned char)peek_next(lexer))) {
            sb_append_char(sb, advance(lexer)); /* . */
            while (isdigit((unsigned char)peek(lexer))) {
                sb_append_char(sb, advance(lexer));
            }
        }
        Token t = make_token(TOKEN_NUMBER, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* Single character tokens */
    if (c == ':') { advance(lexer); return make_token(TOKEN_COLON, ":", start_line, start_col); }
    if (c == ',') { advance(lexer); return make_token(TOKEN_COMMA, ",", start_line, start_col); }
    if (c == '.') { advance(lexer); return make_token(TOKEN_DOT, ".", start_line, start_col); }
    if (c == '=') { advance(lexer); return make_token(TOKEN_EQUALS, "=", start_line, start_col); }
    if (c == '(') { advance(lexer); return make_token(TOKEN_LPAREN, "(", start_line, start_col); }
    if (c == ')') { advance(lexer); return make_token(TOKEN_RPAREN, ")", start_line, start_col); }
    if (c == '[') { advance(lexer); return make_token(TOKEN_LBRACKET, "[", start_line, start_col); }
    if (c == ']') { advance(lexer); return make_token(TOKEN_RBRACKET, "]", start_line, start_col); }
    if (c == '!') { advance(lexer); return make_token(TOKEN_EXCLAMATION, "!", start_line, start_col); }
    if (c == '+') { advance(lexer); return make_token(TOKEN_PLUS, "+", start_line, start_col); }
    if (c == '-') { advance(lexer); return make_token(TOKEN_MINUS, "-", start_line, start_col); }
    if (c == '*') { advance(lexer); return make_token(TOKEN_STAR, "*", start_line, start_col); }
    if (c == '/') { advance(lexer); return make_token(TOKEN_SLASH, "/", start_line, start_col); }

    /* Special compounds starting with keywords */
    /* Check "make-->" */
    if (strncmp(lexer->source + lexer->pos, "make-->", 7) == 0) {
        for (int i = 0; i < 7; i++) advance(lexer);
        return make_token(TOKEN_MAKE_ARROW, "make-->", start_line, start_col);
    }

    /* Check "or else if" */
    if (strncmp(lexer->source + lexer->pos, "or else if", 10) == 0 &&
        !isalnum((unsigned char)lexer->source[lexer->pos + 10])) {
        for (int i = 0; i < 10; i++) advance(lexer);
        return make_token(TOKEN_KW_OR_ELSE_IF, "or else if", start_line, start_col);
    }

    /* Check "or else-->" */
    if (strncmp(lexer->source + lexer->pos, "or else-->", 10) == 0) {
        for (int i = 0; i < 10; i++) advance(lexer);
        return make_token(TOKEN_KW_OR_ELSE, "or else-->", start_line, start_col);
    }

    /* Identifiers / Keywords / Dashed attributes (e.g. aria-label, data-state, stroke-width) */
    if (isalpha((unsigned char)c) || c == '_') {
        StringBuilder *sb = sb_create();
        while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_' || 
               (peek(lexer) == '-' && (isalnum((unsigned char)peek_next(lexer)) || peek_next(lexer) == '_'))) {
            sb_append_char(sb, advance(lexer));
        }

        TokenType type = TOKEN_IDENT;
        if (strcmp(sb->data, "if") == 0) type = TOKEN_KW_IF;
        else if (strcmp(sb->data, "when") == 0) type = TOKEN_KW_WHEN;
        else if (strcmp(sb->data, "click") == 0) type = TOKEN_KW_CLICK;
        else if (strcmp(sb->data, "whisper") == 0) type = TOKEN_KW_WHISPER;
        else if (strcmp(sb->data, "yell") == 0) type = TOKEN_KW_YELL;
        else if (strcmp(sb->data, "tell") == 0) type = TOKEN_KW_TELL;
        else if (strcmp(sb->data, "show") == 0) type = TOKEN_KW_SHOW;
        else if (strcmp(sb->data, "hide") == 0) type = TOKEN_KW_HIDE;
        else if (strcmp(sb->data, "read") == 0) type = TOKEN_KW_READ;
        else if (strcmp(sb->data, "smooth") == 0) type = TOKEN_KW_SMOOTH;
        else if (strcmp(sb->data, "text") == 0) type = TOKEN_KW_TEXT;
        else if (strcmp(sb->data, "page") == 0) type = TOKEN_KW_PAGE;
        else if (strcmp(sb->data, "all") == 0) type = TOKEN_KW_ALL;
        else if (strcmp(sb->data, "id") == 0) type = TOKEN_KW_ID;
        else if (strcmp(sb->data, "bg") == 0) type = TOKEN_KW_BG;
        else if (strcmp(sb->data, "bold") == 0) type = TOKEN_KW_BOLD;
        else if (strcmp(sb->data, "italic") == 0) type = TOKEN_KW_ITALIC;
        else if (strcmp(sb->data, "add_class") == 0) type = TOKEN_KW_ADD_CLASS;
        else if (strcmp(sb->data, "remove_class") == 0) type = TOKEN_KW_REMOVE_CLASS;
        else if (strcmp(sb->data, "toggle_class") == 0) type = TOKEN_KW_TOGGLE_CLASS;
        else if (strcmp(sb->data, "js") == 0) type = TOKEN_KW_JS;

        Token t = make_token(type, sb->data, start_line, start_col);
        sb_free(sb);
        return t;
    }

    /* If we get here, unknown char */
    char err_buf[2] = {advance(lexer), '\0'};
    return make_token(TOKEN_ERROR, err_buf, start_line, start_col);
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_IDENT: return "IDENT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_TRIPLE_STRING: return "TRIPLE_STRING";
        case TOKEN_FONT_LITERAL: return "FONT_LITERAL";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_HEX_COLOR: return "HEX_COLOR";
        case TOKEN_COLON: return ":";
        case TOKEN_COMMA: return ",";
        case TOKEN_DOT: return ".";
        case TOKEN_EQUALS: return "=";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACKET: return "[";
        case TOKEN_RBRACKET: return "]";
        case TOKEN_EXCLAMATION: return "!";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_MAKE_ARROW: return "make-->";
        case TOKEN_CLOSE_ARROW: return "<--";
        case TOKEN_SMOOTH_ARROW: return "->";
        case TOKEN_KW_IF: return "if";
        case TOKEN_KW_OR_ELSE_IF: return "or else if";
        case TOKEN_KW_OR_ELSE: return "or else-->";
        case TOKEN_KW_WHEN: return "when";
        case TOKEN_KW_CLICK: return "click";
        case TOKEN_KW_WHISPER: return "whisper";
        case TOKEN_KW_YELL: return "yell";
        case TOKEN_KW_TELL: return "tell";
        case TOKEN_KW_SHOW: return "show";
        case TOKEN_KW_HIDE: return "hide";
        case TOKEN_KW_READ: return "read";
        case TOKEN_KW_SMOOTH: return "smooth";
        case TOKEN_KW_TEXT: return "text";
        case TOKEN_KW_PAGE: return "page";
        case TOKEN_KW_ALL: return "all";
        case TOKEN_KW_ID: return "id";
        case TOKEN_KW_BG: return "bg";
        case TOKEN_KW_BOLD: return "bold";
        case TOKEN_KW_ITALIC: return "italic";
        case TOKEN_KW_ADD_CLASS: return "add_class";
        case TOKEN_KW_REMOVE_CLASS: return "remove_class";
        case TOKEN_KW_TOGGLE_CLASS: return "toggle_class";
        case TOKEN_KW_JS: return "js";
        default: return "UNKNOWN";
    }
}
