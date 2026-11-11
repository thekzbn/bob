#include "../src/common.h"
#include "../src/lexer.h"
#include "../src/ast.h"
#include "../src/meta.h"
#include "../src/id_gen.h"
#include "../src/parser.h"
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

TEST(test_lexer_tokens) {
    const char *src = "car = 1 * 1\nwhisper car\ndog = 00\ncat = 0\n";
    Lexer *lex = lexer_create(src);

    Token t = lexer_next_token(lex);
    assert(t.type == TOKEN_IDENT && strcmp(t.text, "car") == 0);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_EQUALS);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_NUMBER && strcmp(t.text, "1") == 0);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_STAR);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_NUMBER && strcmp(t.text, "1") == 0);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_NEWLINE);
    token_free(&t);

    t = lexer_next_token(lex);
    assert(t.type == TOKEN_KW_WHISPER);
    token_free(&t);

    lexer_free(lex);
}

TEST(test_zero_vs_zero_zero) {
    const char *src = "uninit = 0\nzero = 00\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_zero.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 2);

    /* uninit = 0 */
    assert(prog->statements[0]->type == STMT_VAR_ASSIGN);
    assert(prog->statements[0]->as.var_assign.value->type == EXPR_UNINIT_ZERO);

    /* zero = 00 */
    assert(prog->statements[1]->type == STMT_VAR_ASSIGN);
    assert(prog->statements[1]->as.var_assign.value->type == EXPR_NUMBER);
    assert(prog->statements[1]->as.var_assign.value->is_zero_zero == true);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_id_stability_and_invariants) {
    const char *meta_file = "tests/test_id_stability.bob";
    MetadataRegistry *meta = meta_load_or_create(meta_file);
    IdGenerator *gen = id_gen_create();

    ElementMeta *em1 = meta_find_or_register_element(meta, "button::btn1:0");
    ElementMeta *em2 = meta_find_or_register_element(meta, "h1::title:1");

    char *id1_first = id_gen_generate(gen, meta, em1);
    char *id2_first = id_gen_generate(gen, meta, em2);

    char *id1_second = id_gen_generate(gen, meta, em1);
    char *id2_second = id_gen_generate(gen, meta, em2);

    assert(strncmp(id1_first, "bob_", 4) == 0);
    assert(strstr(id1_first, ".com") != NULL);

    char block1_first[7], block1_second[7];
    char block2_first[7], block2_second[7];

    strncpy(block1_first, id1_first + 4, 6); block1_first[6] = '\0';
    strncpy(block1_second, id1_second + 4, 6); block1_second[6] = '\0';

    strncpy(block2_first, id2_first + 4, 6); block2_first[6] = '\0';
    strncpy(block2_second, id2_second + 4, 6); block2_second[6] = '\0';

    assert(strncmp(block1_first, block2_first, 3) == 0);
    assert(strcmp(block1_first, block1_second) == 0);
    assert(strcmp(block2_first, block2_second) == 0);
    assert(strcmp(id1_first, id1_second) != 0);

    free(id1_first);
    free(id1_second);
    free(id2_first);
    free(id2_second);

    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_dom_nesting_rule_c) {
    const char *src = "div: h1: \"Hello\", a: \"Link\", button: \"Click\"\nbutton: \"Outside\"\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_nest.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 2);

    Stmt *s1 = prog->statements[0];
    assert(s1->type == STMT_ELEMENT_CREATE);
    assert(strcmp(s1->as.elem_create.element->tag, "div") == 0);
    assert(s1->as.elem_create.element->child_count == 3);

    Stmt *s2 = prog->statements[1];
    assert(s2->type == STMT_ELEMENT_CREATE);
    assert(strcmp(s2->as.elem_create.element->tag, "button") == 0);
    assert(s2->as.elem_create.element->child_count == 0);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_multiline_strings) {
    const char *src = "tell: \"\"\"\nLine 1\nLine 2 with \"quotes\"\n\"\"\"\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_multiline.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 1);
    Stmt *s = prog->statements[0];
    assert(s->type == STMT_ELEMENT_CREATE);
    assert(s->as.elem_create.element->content != NULL);
    assert(s->as.elem_create.element->content->is_multiline == true);
    assert(strstr(s->as.elem_create.element->content->as.str_value, "Line 2 with \"quotes\"") != NULL);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_attributes_and_classes) {
    const char *src = "a[href=\"https://example.com\", target=\"_blank\", required]: \"Link\"\n.btn: bold\nadd_class root \"dark\"\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_attrs.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 3);

    /* Attributes */
    BobElement *el = prog->statements[0]->as.elem_create.element;
    assert(el->attr_count == 3);
    assert(strcmp(el->attributes[0].name, "href") == 0);
    assert(strcmp(el->attributes[0].value->as.str_value, "https://example.com") == 0);
    assert(strcmp(el->attributes[1].name, "target") == 0);
    assert(strcmp(el->attributes[2].name, "required") == 0);
    assert(el->attributes[2].is_boolean == true);

    /* Class style */
    assert(prog->statements[1]->type == STMT_CLASS_STYLE);
    assert(strcmp(prog->statements[1]->as.class_style.class_name, "btn") == 0);
    assert(prog->statements[1]->as.class_style.style.is_bold == true);

    /* Class mutate */
    assert(prog->statements[2]->type == STMT_CLASS_MUTATE);
    assert(strcmp(prog->statements[2]->as.class_mutate.target, "root") == 0);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_arbitrary_depth_nesting) {
    const char *src =
        "article make-->\n"
        "    section make-->\n"
        "        h1: \"Nested Title\"\n"
        "        div make-->\n"
        "            p: \"Leaf paragraph\"\n"
        "        <--\n"
        "    <--\n"
        "<--\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_deep.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 1);

    BobElement *art = prog->statements[0]->as.elem_create.element;
    assert(strcmp(art->tag, "article") == 0);
    assert(art->child_count == 1);

    BobElement *sec = art->children[0];
    assert(strcmp(sec->tag, "section") == 0);
    assert(sec->child_count == 2);
    assert(strcmp(sec->children[0]->tag, "h1") == 0);

    BobElement *d = sec->children[1];
    assert(strcmp(d->tag, "div") == 0);
    assert(d->child_count == 1);
    assert(strcmp(d->children[0]->tag, "p") == 0);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

TEST(test_js_interop) {
    const char *src =
        "mode = js localStorage.getItem(\"theme\")\n"
        "js: localStorage.setItem(\"active\", \"1\")\n";
    MetadataRegistry *meta = meta_load_or_create("tests/test_js.bob");
    IdGenerator *gen = id_gen_create();
    Lexer *lex = lexer_create(src);
    Parser *p = parser_create(lex, meta, gen);

    Program *prog = parser_parse_program(p);
    assert(prog->count == 2);

    /* Assignment with JS expression */
    assert(prog->statements[0]->type == STMT_VAR_ASSIGN);
    assert(prog->statements[0]->as.var_assign.value->type == EXPR_JS);
    assert(strstr(prog->statements[0]->as.var_assign.value->as.js_expr.code, "localStorage.getItem") != NULL);

    /* JS statement */
    assert(prog->statements[1]->type == STMT_JS_STMT);
    assert(strstr(prog->statements[1]->as.js_stmt.code, "localStorage.setItem") != NULL);

    program_free(prog);
    parser_free(p);
    lexer_free(lex);
    meta_free(meta);
    id_gen_free(gen);
}

int main(void) {
    printf("========================================\n");
    printf(" Running Bob Compiler Unit Test Suite\n");
    printf("========================================\n");

    RUN_TEST(test_lexer_tokens);
    RUN_TEST(test_zero_vs_zero_zero);
    RUN_TEST(test_id_stability_and_invariants);
    RUN_TEST(test_dom_nesting_rule_c);
    RUN_TEST(test_multiline_strings);
    RUN_TEST(test_attributes_and_classes);
    RUN_TEST(test_arbitrary_depth_nesting);
    RUN_TEST(test_js_interop);

    printf("========================================\n");
    printf(" Test Results: %d/%d Passed!\n", tests_passed, tests_run);
    printf("========================================\n");
    return (tests_passed == tests_run) ? 0 : 1;
}
