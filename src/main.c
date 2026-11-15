#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "meta.h"
#include "id_gen.h"

#ifdef _WIN32
#include <direct.h>
#define bob_mkdir(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#define bob_mkdir(dir) mkdir(dir, 0755)
#endif

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
    printf("Usage:\n");
    printf("  %s <source.bob> [options]    Compile a Bob application\n", prog_name);
    printf("  %s init [directory]          Initialize a new Bob project with starter files\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <file.html>     Specify output single-file HTML path (default: dist/index.html)\n");
    printf("  --split <dir>      Emit split files (index.html, style.css, script.js)\n");
    printf("  -h, --help         Display this help message\n");
    printf("  -v, --version      Display compiler version\n");
}

static int handle_init_command(int argc, char **argv) {
    const char *target_dir = (argc > 2) ? argv[2] : ".";
    if (strcmp(target_dir, ".") != 0) {
        bob_mkdir(target_dir);
    }

    char app_bob_path[512];
    char style_css_path[512];
    char dist_dir_path[512];

    if (strcmp(target_dir, ".") == 0) {
        snprintf(app_bob_path, sizeof(app_bob_path), "app.bob");
        snprintf(style_css_path, sizeof(style_css_path), "style.css");
        snprintf(dist_dir_path, sizeof(dist_dir_path), "dist");
    } else {
        snprintf(app_bob_path, sizeof(app_bob_path), "%s/app.bob", target_dir);
        snprintf(style_css_path, sizeof(style_css_path), "%s/style.css", target_dir);
        snprintf(dist_dir_path, sizeof(dist_dir_path), "%s/dist", target_dir);
    }

    bob_mkdir(dist_dir_path);

    /* Create starter app.bob */
    FILE *fbob = fopen(app_bob_path, "r");
    if (fbob) {
        fclose(fbob);
        printf("Note: '%s' already exists, leaving it unchanged.\n", app_bob_path);
    } else {
        fbob = fopen(app_bob_path, "w");
        if (fbob) {
            fprintf(fbob, "name: \"My Bob Project\"\n");
            fprintf(fbob, "css+: style.css\n");
            fprintf(fbob, "favicon+: favicon.ico\n\n");
            fprintf(fbob, "page: bg #121212 #f5f5f5 'Inter'\n\n");
            fprintf(fbob, "main make-->\n");
            fprintf(fbob, "    section make-->\n");
            fprintf(fbob, "        h1: \"Welcome to Bob\" bold #1e90ff 2.5\n");
            fprintf(fbob, "        p: \"Built with Bob - the esoteric web language.\"\n");
            fprintf(fbob, "        button(counter_btn): \"Click me (0)\" #ffffff bg #1e90ff\n");
            fprintf(fbob, "    <--\n");
            fprintf(fbob, "<--\n\n");
            fprintf(fbob, "clicks = 0\n");
            fprintf(fbob, "when click(counter_btn) make-->\n");
            fprintf(fbob, "    clicks = clicks + 1\n");
            fprintf(fbob, "    counter_btn: \"Clicks: \" + clicks\n");
            fprintf(fbob, "<--\n");
            fclose(fbob);
            printf("Created %s\n", app_bob_path);
        }
    }

    /* Create starter style.css */
    FILE *fcss = fopen(style_css_path, "r");
    if (fcss) {
        fclose(fcss);
    } else {
        fcss = fopen(style_css_path, "w");
        if (fcss) {
            fprintf(fcss, "/* Custom CSS styles */\n");
            fprintf(fcss, "body {\n    margin: 0;\n    padding: 2rem;\n    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n}\n");
            fprintf(fcss, "button {\n    cursor: pointer;\n    border: 1px solid #1e90ff;\n    padding: 0.75rem 1.5rem;\n    border-radius: 4px;\n    font-size: 1rem;\n    margin-top: 1rem;\n}\n");
            fclose(fcss);
            printf("Created %s\n", style_css_path);
        }
    }

    printf("\n========================================\n");
    printf(" Bob Project Initialized!\n");
    printf(" Location: %s\n", target_dir);
    printf(" To compile:\n");
    if (strcmp(target_dir, ".") != 0) {
        printf("   cd %s\n", target_dir);
    }
    printf("   bobc app.bob\n");
    printf(" Or with split files:\n");
    printf("   bobc app.bob --split dist\n");
    printf("========================================\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        return handle_init_command(argc, argv);
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
