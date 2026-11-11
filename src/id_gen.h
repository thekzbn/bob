#ifndef BOB_ID_GEN_H
#define BOB_ID_GEN_H

#include "common.h"
#include "meta.h"

typedef struct {
    char *prefix;
    char *suffix;
    char **generated_ids;
    size_t count;
    size_t capacity;
} IdGenerator;

IdGenerator *id_gen_create(void);
void id_gen_set_config(IdGenerator *gen, const char *prefix, const char *suffix);
char *id_gen_generate(IdGenerator *gen, MetadataRegistry *meta, ElementMeta *elem_meta);
void id_gen_free(IdGenerator *gen);

#endif /* BOB_ID_GEN_H */
