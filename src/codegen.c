#include "codegen.h"

static void emit_escaped_html_text(StringBuilder *sb, const char *str) {
    if (!str) return;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&': sb_append(sb, "&amp;"); break;
            case '<': sb_append(sb, "&lt;"); break;
            case '>': sb_append(sb, "&gt;"); break;
            default: sb_append_char(sb, *p); break;
        }
    }
}

static void emit_escaped_attr_val(StringBuilder *sb, const char *str) {
    if (!str) return;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&': sb_append(sb, "&amp;"); break;
            case '"': sb_append(sb, "&quot;"); break;
            case '<': sb_append(sb, "&lt;"); break;
            case '>': sb_append(sb, "&gt;"); break;
            default: sb_append_char(sb, *p); break;
        }
    }
}

static void emit_style_rules(StringBuilder *sb, const char *selector, const StyleModifiers *style) {
    sb_append(sb, selector);
    sb_append(sb, " {\n");

    if (style->alignment) {
        if (strcmp(style->alignment, "center") == 0) {
            sb_append(sb, "    display: block;\n");
            sb_append(sb, "    margin-left: auto;\n");
            sb_append(sb, "    margin-right: auto;\n");
            sb_append(sb, "    text-align: center;\n");
        } else if (strcmp(style->alignment, "left") == 0) {
            sb_append(sb, "    text-align: left;\n");
        } else if (strcmp(style->alignment, "right") == 0) {
            sb_append(sb, "    text-align: right;\n");
        }
    }

    if (style->text_color) {
        sb_append(sb, "    color: ");
        sb_append(sb, style->text_color);
        sb_append(sb, ";\n");
    }

    if (style->bg_color) {
        sb_append(sb, "    background-color: ");
        sb_append(sb, style->bg_color);
        sb_append(sb, ";\n");
    }

    if (style->font_family) {
        sb_append(sb, "    font-family: '");
        sb_append(sb, style->font_family);
        sb_append(sb, "', sans-serif;\n");
    }

    if (style->font_size_rem > 0.0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2frem", style->font_size_rem);
        sb_append(sb, "    font-size: ");
        sb_append(sb, buf);
        sb_append(sb, ";\n");
    }

    if (style->is_bold) {
        sb_append(sb, "    font-weight: bold;\n");
    }

    if (style->is_italic) {
        sb_append(sb, "    font-style: italic;\n");
    }

    if (style->is_hidden) {
        sb_append(sb, "    display: none;\n");
    }

    sb_append(sb, "}\n");
}

/* Recursive CSS generator for elements */
static void generate_element_css(StringBuilder *sb, BobElement *elem) {
    const char *id = elem->custom_id ? elem->custom_id : elem->auto_id;
    if (id) {
        char selector[128];
        snprintf(selector, sizeof(selector), "#%s", id);
        emit_style_rules(sb, selector, &elem->style);

        /* Pre-wrap multiline text */
        if (elem->content && elem->content->is_multiline) {
            sb_append(sb, selector);
            sb_append(sb, " {\n    white-space: pre-wrap;\n}\n");
        }
    }
    for (size_t i = 0; i < elem->child_count; i++) {
        generate_element_css(sb, elem->children[i]);
    }
}

/* Recursive HTML generator for elements */
static void generate_element_html(StringBuilder *sb, BobElement *elem, int depth) {
    for (int i = 0; i < depth; i++) sb_append(sb, "  ");

    const char *id = elem->custom_id ? elem->custom_id : elem->auto_id;
    sb_append_char(sb, '<');
    sb_append(sb, elem->tag);

    /* SVG namespace on root svg */
    if (strcmp(elem->tag, "svg") == 0) {
        bool has_xmlns = false;
        for (size_t a = 0; a < elem->attr_count; a++) {
            if (strcmp(elem->attributes[a].name, "xmlns") == 0) {
                has_xmlns = true;
                break;
            }
        }
        if (!has_xmlns) {
            sb_append(sb, " xmlns=\"http://www.w3.org/2000/svg\"");
        }
    }

    if (id) {
        sb_append(sb, " id=\"");
        sb_append(sb, id);
        sb_append(sb, "\"");
    }

    /* Attributes e.g. [href="...", target="_blank", viewBox="...", required] */
    for (size_t a = 0; a < elem->attr_count; a++) {
        Attribute *attr = &elem->attributes[a];
        sb_append_char(sb, ' ');
        sb_append(sb, attr->name);
        if (!attr->is_boolean && attr->value) {
            sb_append(sb, "=\"");
            if (attr->value->type == EXPR_STRING) {
                emit_escaped_attr_val(sb, attr->value->as.str_value);
            } else if (attr->value->type == EXPR_NUMBER) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%g", attr->value->as.num_value);
                sb_append(sb, buf);
            } else if (attr->value->type == EXPR_VAR) {
                sb_append(sb, attr->value->as.var_name);
            }
            sb_append(sb, "\"");
        }
    }

    /* Input placeholder rule */
    if (strcmp(elem->tag, "input") == 0) {
        if (elem->content && elem->content->type == EXPR_STRING) {
            sb_append(sb, " placeholder=\"");
            emit_escaped_attr_val(sb, elem->content->as.str_value);
            sb_append(sb, "\"");
        }
        sb_append(sb, " />\n");
        return;
    }

    /* Self-closing SVG or void elements when no children and no content */
    bool can_self_close = (elem->child_count == 0 && elem->content == NULL) &&
                          (elem->is_svg || strcmp(elem->tag, "col") == 0 || strcmp(elem->tag, "hr") == 0 || strcmp(elem->tag, "br") == 0);

    if (can_self_close && strcmp(elem->tag, "svg") != 0 && strcmp(elem->tag, "div") != 0) {
        sb_append(sb, " />\n");
        return;
    }

    sb_append_char(sb, '>');

    /* Content */
    if (elem->content && elem->content->type == EXPR_STRING) {
        emit_escaped_html_text(sb, elem->content->as.str_value);
    }

    if (elem->child_count > 0) {
        sb_append_char(sb, '\n');
        for (size_t i = 0; i < elem->child_count; i++) {
            generate_element_html(sb, elem->children[i], depth + 1);
        }
        for (int i = 0; i < depth; i++) sb_append(sb, "  ");
    }

    sb_append(sb, "</");
    sb_append(sb, elem->tag);
    sb_append(sb, ">\n");
}

/* JS Expression emission */
static void emit_js_expr(StringBuilder *sb, Expr *expr, bool in_condition) {
    if (!expr) {
        sb_append(sb, "undefined");
        return;
    }

    switch (expr->type) {
        case EXPR_UNINIT_ZERO:
            sb_append(sb, "undefined");
            break;
        case EXPR_NUMBER:
            if (expr->is_zero_zero) {
                sb_append(sb, "0");
            } else {
                char buf[32];
                if (expr->as.num_value == (long)expr->as.num_value) {
                    snprintf(buf, sizeof(buf), "%ld", (long)expr->as.num_value);
                } else {
                    snprintf(buf, sizeof(buf), "%g", expr->as.num_value);
                }
                sb_append(sb, buf);
            }
            break;
        case EXPR_STRING:
            sb_append_char(sb, '"');
            for (const char *p = expr->as.str_value; *p; p++) {
                if (*p == '"') sb_append(sb, "\\\"");
                else if (*p == '\\') sb_append(sb, "\\\\");
                else if (*p == '\n') sb_append(sb, "\\n");
                else if (*p == '\r') sb_append(sb, "\\r");
                else sb_append_char(sb, *p);
            }
            sb_append_char(sb, '"');
            break;
        case EXPR_VAR:
            sb_append(sb, expr->as.var_name);
            break;
        case EXPR_READ:
            sb_append(sb, "__bob_read(\"");
            sb_append(sb, expr->as.read.target_name);
            sb_append(sb, "\", _event)");
            break;
        case EXPR_JS:
            sb_append(sb, "(");
            sb_append(sb, expr->as.js_expr.code);
            sb_append(sb, ")");
            break;
        case EXPR_BINARY:
        case EXPR_ASSIGN:
            sb_append(sb, "(");
            emit_js_expr(sb, expr->as.binary.left, in_condition);
            if (expr->as.binary.op == '=') {
                if (in_condition) {
                    sb_append(sb, " === ");
                } else {
                    sb_append(sb, " = ");
                }
            } else {
                char op_str[4] = {' ', (char)expr->as.binary.op, ' ', '\0'};
                sb_append(sb, op_str);
            }
            emit_js_expr(sb, expr->as.binary.right, in_condition);
            sb_append(sb, ")");
            break;
    }
}

/* Recursive JS statement emission */
static void emit_js_stmt(StringBuilder *sb, Stmt *stmt, int indent) {
    if (!stmt) return;

    for (int i = 0; i < indent; i++) sb_append(sb, "    ");

    switch (stmt->type) {
        case STMT_VAR_ASSIGN:
            sb_append(sb, stmt->as.var_assign.name);
            sb_append(sb, " = ");
            emit_js_expr(sb, stmt->as.var_assign.value, false);
            sb_append(sb, ";\n");
            break;

        case STMT_ELEMENT_UPDATE:
            sb_append(sb, "__bob_update(\"");
            sb_append(sb, stmt->as.elem_update.target);
            sb_append(sb, "\", ");
            emit_js_expr(sb, stmt->as.elem_update.new_content, false);
            sb_append(sb, ", _event);\n");
            break;

        case STMT_OUTPUT_WHISPER:
            sb_append(sb, "console.log(");
            emit_js_expr(sb, stmt->as.output.expr, false);
            sb_append(sb, ");\n");
            break;

        case STMT_OUTPUT_YELL:
            sb_append(sb, "alert(");
            emit_js_expr(sb, stmt->as.output.expr, false);
            sb_append(sb, ");\n");
            break;

        case STMT_SHOW:
            sb_append(sb, "__bob_show(\"");
            sb_append(sb, stmt->as.visibility.target);
            sb_append(sb, "\", _event);\n");
            break;

        case STMT_HIDE:
            sb_append(sb, "__bob_hide(\"");
            sb_append(sb, stmt->as.visibility.target);
            sb_append(sb, "\", _event);\n");
            break;

        case STMT_CLASS_MUTATE:
            if (stmt->as.class_mutate.action == 0) {
                sb_append(sb, "__bob_add_class(\"");
            } else if (stmt->as.class_mutate.action == 1) {
                sb_append(sb, "__bob_remove_class(\"");
            } else {
                sb_append(sb, "__bob_toggle_class(\"");
            }
            sb_append(sb, stmt->as.class_mutate.target);
            sb_append(sb, "\", ");
            emit_js_expr(sb, stmt->as.class_mutate.class_expr, false);
            sb_append(sb, ", _event);\n");
            break;

        case STMT_JS_STMT:
            sb_append(sb, stmt->as.js_stmt.code);
            sb_append(sb, "\n");
            break;

        case STMT_IF:
            sb_append(sb, "if (");
            emit_js_expr(sb, stmt->as.if_stmt.condition, true);
            sb_append(sb, ") {\n");
            for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                emit_js_stmt(sb, stmt->as.if_stmt.then_body[i], indent + 1);
            }
            for (size_t i = 0; i < stmt->as.if_stmt.elif_count; i++) {
                for (int j = 0; j < indent; j++) sb_append(sb, "    ");
                sb_append(sb, "} else if (");
                emit_js_expr(sb, stmt->as.if_stmt.elifs[i].condition, true);
                sb_append(sb, ") {\n");
                for (size_t k = 0; k < stmt->as.if_stmt.elifs[i].body_count; k++) {
                    emit_js_stmt(sb, stmt->as.if_stmt.elifs[i].body[k], indent + 1);
                }
            }
            if (stmt->as.if_stmt.else_count > 0) {
                for (int j = 0; j < indent; j++) sb_append(sb, "    ");
                sb_append(sb, "} else {\n");
                for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                    emit_js_stmt(sb, stmt->as.if_stmt.else_body[i], indent + 1);
                }
            }
            for (int j = 0; j < indent; j++) sb_append(sb, "    ");
            sb_append(sb, "}\n");
            break;

        case STMT_WHEN_CLICK:
            sb_append(sb, "__bob_bind_click(\"");
            sb_append(sb, stmt->as.when_click.target);
            sb_append(sb, "\", function(_event) {\n");
            for (size_t i = 0; i < stmt->as.when_click.body_count; i++) {
                emit_js_stmt(sb, stmt->as.when_click.body[i], indent + 1);
            }
            for (int j = 0; j < indent; j++) sb_append(sb, "    ");
            sb_append(sb, "});\n");
            break;

        default:
            break;
    }
}

bool codegen_generate(Program *prog, const CodegenOptions *options) {
    StringBuilder *css = sb_create();
    StringBuilder *html = sb_create();
    StringBuilder *js = sb_create();

    /* Base CSS styles */
    sb_append(css, "/* Bob Generated CSS */\n");
    sb_append(css, "* { box-sizing: border-box; }\n");
    sb_append(css, "body {\n    margin: 0;\n    padding: 2rem;\n    font-family: sans-serif;\n}\n");
    sb_append(css, "button, input { padding: 0.5rem 1rem; margin: 0.5rem; border-radius: 4px; border: 1px solid #ccc; font-size: 1rem; cursor: pointer; }\n");
    sb_append(css, "input { cursor: text; }\n");
    sb_append(css, "table { border-collapse: collapse; margin: 1rem 0; width: 100%; }\n");
    sb_append(css, "th, td { border: 1px solid #ccc; padding: 0.5rem 0.75rem; text-align: left; }\n");

    /* JS Runtime */
    sb_append(js, "/* Bob Generated JavaScript Runtime */\n");
    sb_append(js, "\"use strict\";\n\n");
    sb_append(js, "function __bob_get(selector, evt) {\n");
    sb_append(js, "    if (evt && evt.currentTarget) {\n");
    sb_append(js, "        if (evt.currentTarget.id === selector || evt.currentTarget.tagName.toLowerCase() === selector.toLowerCase()) {\n");
    sb_append(js, "            return [evt.currentTarget];\n");
    sb_append(js, "        }\n");
    sb_append(js, "    }\n");
    sb_append(js, "    if (selector.includes('!')) {\n");
    sb_append(js, "        let [tag, excl] = selector.split('!');\n");
    sb_append(js, "        return Array.from(document.querySelectorAll(tag || '*')).filter(el => !el.classList.contains(excl) && el.tagName.toLowerCase() !== excl.toLowerCase() && el.id !== excl);\n");
    sb_append(js, "    }\n");
    sb_append(js, "    let el = document.getElementById(selector);\n");
    sb_append(js, "    if (el) return [el];\n");
    sb_append(js, "    return Array.from(document.querySelectorAll(selector));\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_resolve(selector, evt) {\n");
    sb_append(js, "    if (selector === \"root\" || selector === \"document\" || selector === \"html\") {\n");
    sb_append(js, "        return [document.documentElement];\n");
    sb_append(js, "    }\n");
    sb_append(js, "    return __bob_get(selector, evt);\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_read(selector, evt) {\n");
    sb_append(js, "    let list = __bob_get(selector, evt);\n");
    sb_append(js, "    if (list.length > 0) return list[0].value !== undefined ? list[0].value : (list[0].textContent || '');\n");
    sb_append(js, "    return '';\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_update(selector, val, evt) {\n");
    sb_append(js, "    let list = __bob_get(selector, evt);\n");
    sb_append(js, "    list.forEach(el => {\n");
    sb_append(js, "        if (el.tagName.toLowerCase() === 'input') {\n");
    sb_append(js, "            el.value = (val === undefined) ? '' : val;\n");
    sb_append(js, "        } else {\n");
    sb_append(js, "            el.textContent = (val === undefined) ? '' : val;\n");
    sb_append(js, "        }\n");
    sb_append(js, "    });\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_show(selector, evt) {\n");
    sb_append(js, "    let list = __bob_get(selector, evt);\n");
    sb_append(js, "    list.forEach(el => { el.style.display = ''; });\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_hide(selector, evt) {\n");
    sb_append(js, "    let list = __bob_get(selector, evt);\n");
    sb_append(js, "    list.forEach(el => { el.style.display = 'none'; });\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_add_class(selector, cls, evt) {\n");
    sb_append(js, "    let list = __bob_resolve(selector, evt);\n");
    sb_append(js, "    list.forEach(el => el.classList.add(cls));\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_remove_class(selector, cls, evt) {\n");
    sb_append(js, "    let list = __bob_resolve(selector, evt);\n");
    sb_append(js, "    list.forEach(el => el.classList.remove(cls));\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_toggle_class(selector, cls, evt) {\n");
    sb_append(js, "    let list = __bob_resolve(selector, evt);\n");
    sb_append(js, "    list.forEach(el => el.classList.toggle(cls));\n");
    sb_append(js, "}\n\n");

    sb_append(js, "function __bob_bind_click(target, handler) {\n");
    sb_append(js, "    let targets = __bob_get(target, null);\n");
    sb_append(js, "    targets.forEach(el => {\n");
    sb_append(js, "        el.addEventListener('click', handler);\n");
    sb_append(js, "    });\n");
    sb_append(js, "}\n\n");

    /* Collect declared state variables */
    sb_append(js, "// State Variables\n");
    for (size_t i = 0; i < prog->count; i++) {
        Stmt *s = prog->statements[i];
        if (s->type == STMT_VAR_ASSIGN) {
            sb_append(js, "let ");
            sb_append(js, s->as.var_assign.name);
            sb_append(js, " = ");
            emit_js_expr(js, s->as.var_assign.value, false);
            sb_append(js, ";\n");
        }
    }
    sb_append(js, "\nwindow.addEventListener('DOMContentLoaded', function() {\n");
    sb_append(js, "    const _event = null;\n");

    /* Pass through statements */
    for (size_t i = 0; i < prog->count; i++) {
        Stmt *s = prog->statements[i];
        switch (s->type) {
            case STMT_PAGE_STYLE:
                emit_style_rules(css, "body", &s->as.global_style.style);
                break;
            case STMT_ALL_STYLE:
                emit_style_rules(css, "*", &s->as.global_style.style);
                break;
            case STMT_CLASS_STYLE: {
                char sel[128];
                snprintf(sel, sizeof(sel), ".%s", s->as.class_style.class_name);
                emit_style_rules(css, sel, &s->as.class_style.style);
                break;
            }
            case STMT_ELEMENT_CREATE:
                generate_element_html(html, s->as.elem_create.element, 2);
                generate_element_css(css, s->as.elem_create.element);
                break;
            case STMT_SMOOTH: {
                char sel[128];
                snprintf(sel, sizeof(sel), "#%s, %s", s->as.smooth.target, s->as.smooth.target);
                sb_append(css, sel);
                sb_append(css, " {\n    transition: all ");
                char dur_buf[32];
                snprintf(dur_buf, sizeof(dur_buf), "%.2fs ease-in-out;\n}\n", s->as.smooth.duration);
                sb_append(css, dur_buf);
                break;
            }
            case STMT_WHEN_CLICK:
            case STMT_IF:
            case STMT_OUTPUT_WHISPER:
            case STMT_OUTPUT_YELL:
            case STMT_SHOW:
            case STMT_HIDE:
            case STMT_CLASS_MUTATE:
            case STMT_JS_STMT:
                emit_js_stmt(js, s, 1);
                break;
            default:
                break;
        }
    }

    sb_append(js, "});\n");

    /* Write Single HTML file */
    if (options->single_file_mode) {
        FILE *f = fopen(options->output_html_file, "w");
        if (!f) {
            fprintf(stderr, "Failed to open output file: %s\n", options->output_html_file);
            sb_free(css);
            sb_free(html);
            sb_free(js);
            return false;
        }

        fprintf(f, "<!DOCTYPE html>\n");
        fprintf(f, "<html lang=\"en\">\n<head>\n");
        fprintf(f, "  <meta charset=\"UTF-8\">\n");
        fprintf(f, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
        fprintf(f, "  <title>Bob Application</title>\n");
        fprintf(f, "  <style>\n%s\n  </style>\n", css->data);
        fprintf(f, "</head>\n<body>\n");
        fprintf(f, "%s\n", html->data);
        fprintf(f, "  <script>\n%s\n  </script>\n", js->data);
        fprintf(f, "</body>\n</html>\n");
        fclose(f);
    } else {
        /* Split files mode */
        char path[512];
        snprintf(path, sizeof(path), "%s/style.css", options->output_dir);
        FILE *fcss = fopen(path, "w");
        if (fcss) { fputs(css->data, fcss); fclose(fcss); }

        snprintf(path, sizeof(path), "%s/script.js", options->output_dir);
        FILE *fjs = fopen(path, "w");
        if (fjs) { fputs(js->data, fjs); fclose(fjs); }

        snprintf(path, sizeof(path), "%s/index.html", options->output_dir);
        FILE *fhtml = fopen(path, "w");
        if (fhtml) {
            fprintf(fhtml, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n");
            fprintf(fhtml, "  <meta charset=\"UTF-8\">\n");
            fprintf(fhtml, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
            fprintf(fhtml, "  <title>Bob Application</title>\n");
            fprintf(fhtml, "  <link rel=\"stylesheet\" href=\"style.css\">\n");
            fprintf(fhtml, "</head>\n<body>\n%s\n", html->data);
            fprintf(fhtml, "  <script src=\"script.js\"></script>\n");
            fprintf(fhtml, "</body>\n</html>\n");
            fclose(fhtml);
        }
    }

    sb_free(css);
    sb_free(html);
    sb_free(js);
    return true;
}
