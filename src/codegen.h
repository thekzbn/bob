#ifndef BOB_CODEGEN_H
#define BOB_CODEGEN_H

#include "ast.h"
#include "meta.h"

typedef struct {
    bool single_file_mode;
    const char *output_dir;
    const char *output_html_file;
} CodegenOptions;

bool codegen_generate(Program *prog, const CodegenOptions *options);

#endif /* BOB_CODEGEN_H */
