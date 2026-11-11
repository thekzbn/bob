#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "meta.h"
#include "id_gen.h"

static char *read_entire_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)bob_malloc(size + 1);
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

static void print_usage(const char *prog_name) {
    printf("Bob Compiler (bobc) v0.1.0\n");
    printf("Usage: %s <source.bob> [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <file.html>     Specify output single-file HTML path (default: dist/index.html)\n");
    printf("  --split <dir>      Emit split files (index.html, style.css, script.js)\n");
    printf("  -h, --help         Display this help message\n");
    printf("  -v, --version      Display compiler version\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *source_path = NULL;
    const char *output_html = "dist/index.html";
    const char *output_dir = "dist";
    bool single_file = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("bobc version 0.1.0 (esoteric web compiler)\n");
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_html = argv[++i];
            single_file = true;
        } else if (strcmp(argv[i], "--split") == 0) {
            single_file = false;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                output_dir = argv[++i];
            }
        } else if (argv[i][0] != '-') {
            source_path = argv[i];
        }
    }

    if (!source_path) {
        fprintf(stderr, "Error: No source file provided.\n");
        return 1;
    }

    char *source_text = read_entire_file(source_path);
    if (!source_text) return 1;

    MetadataRegistry *meta = meta_load_or_create(source_path);
    IdGenerator *id_gen = id_gen_create();

    Lexer *lexer = lexer_create(source_text);
    Parser *parser = parser_create(lexer, meta, id_gen);

    Program *prog = parser_parse_program(parser);

    if (parser->has_error) {
        fprintf(stderr, "Compilation aborted due to syntax errors.\n");
        parser_free(parser);
        lexer_free(lexer);
        meta_free(meta);
        id_gen_free(id_gen);
        free(source_text);
        return 1;
    }

    CodegenOptions opts;
    opts.single_file_mode = single_file;
    opts.output_html_file = output_html;
    opts.output_dir = output_dir;

    if (!codegen_generate(prog, &opts)) {
        fprintf(stderr, "Code generation failed.\n");
        program_free(prog);
        parser_free(parser);
        lexer_free(lexer);
        meta_free(meta);
        id_gen_free(id_gen);
        free(source_text);
        return 1;
    }

    meta_save(meta);

    printf("========================================\n");
    printf(" Bob Compilation Successful!\n");
    printf(" Source : %s\n", source_path);
    if (single_file) {
        printf(" Output : %s (standalone HTML bundle)\n", output_html);
    } else {
        printf(" Output : %s (index.html, style.css, script.js)\n", output_dir);
    }
    printf(" Metadata: %s.meta\n", source_path);
    printf("========================================\n");

    program_free(prog);
    parser_free(parser);
    lexer_free(lexer);
    meta_free(meta);
    id_gen_free(id_gen);
    free(source_text);
    return 0;
}
