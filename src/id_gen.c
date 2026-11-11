#include "id_gen.h"

IdGenerator *id_gen_create(void) {
    IdGenerator *gen = (IdGenerator *)bob_calloc(1, sizeof(IdGenerator));
    gen->prefix = bob_strdup("bob");
    gen->suffix = bob_strdup(".com");
    gen->capacity = 16;
    gen->generated_ids = (char **)bob_malloc(sizeof(char *) * gen->capacity);
    gen->count = 0;
    srand((unsigned int)time(NULL));
    return gen;
}

void id_gen_set_config(IdGenerator *gen, const char *prefix, const char *suffix) {
    if (gen->prefix) free(gen->prefix);
    if (gen->suffix) free(gen->suffix);
    gen->prefix = prefix ? bob_strdup(prefix) : bob_strdup("");
    gen->suffix = suffix ? bob_strdup(suffix) : bob_strdup("");
}

/* Scramble hex string by permutation (preserving character counts) */
static void scramble_hex(char *hex_str, size_t len) {
    for (size_t i = len - 1; i > 0; i--) {
        size_t j = (size_t)(rand() % (i + 1));
        char temp = hex_str[i];
        hex_str[i] = hex_str[j];
        hex_str[j] = temp;
    }
}

static uint32_t generate_scrambled_octal_block(uint64_t seed) {
    uint32_t val = (uint32_t)(seed ^ (rand() * 1103515245 + 12345));
    char hex[16];
    snprintf(hex, sizeof(hex), "%06x", val & 0xFFFFFF);

    /* Permutation */
    scramble_hex(hex, 6);

    /* Convert back from hex and produce 6 octal digits */
    uint32_t permuted_val = (uint32_t)strtoul(hex, NULL, 16);
    uint32_t octal_block = (permuted_val % 700000) + 100000;
    return octal_block;
}

static bool id_exists(IdGenerator *gen, const char *id) {
    for (size_t i = 0; i < gen->count; i++) {
        if (strcmp(gen->generated_ids[i], id) == 0) return true;
    }
    return false;
}

static void register_id(IdGenerator *gen, const char *id) {
    if (gen->count >= gen->capacity) {
        gen->capacity *= 2;
        gen->generated_ids = (char **)bob_realloc(gen->generated_ids, sizeof(char *) * gen->capacity);
    }
    gen->generated_ids[gen->count++] = bob_strdup(id);
}

char *id_gen_generate(IdGenerator *gen, MetadataRegistry *meta, ElementMeta *elem_meta) {
    char buffer[256];
    uint32_t file_oct = meta ? meta->file_stable_octal : 123;
    uint32_t elem_oct = elem_meta ? elem_meta->stable_octal : 456;

    /* Stable 6-digit chunk = file 3 digits + elem 3 digits */
    char stable_chunk[16];
    snprintf(stable_chunk, sizeof(stable_chunk), "%03o%03o", file_oct, elem_oct);

    int attempts = 0;
    while (attempts < 100) {
        attempts++;
        uint32_t r1 = generate_scrambled_octal_block((uint64_t)time(NULL) + attempts);
        uint32_t r2 = generate_scrambled_octal_block((uint64_t)rand() + attempts * 31);

        if (strlen(gen->prefix) > 0 && strlen(gen->suffix) > 0) {
            snprintf(buffer, sizeof(buffer), "%s_%s_%06o_%06o%s",
                     gen->prefix, stable_chunk, r1, r2, gen->suffix);
        } else if (strlen(gen->prefix) > 0) {
            snprintf(buffer, sizeof(buffer), "%s_%s_%06o_%06o",
                     gen->prefix, stable_chunk, r1, r2);
        } else if (strlen(gen->suffix) > 0) {
            snprintf(buffer, sizeof(buffer), "%s_%06o_%06o%s",
                     stable_chunk, r1, r2, gen->suffix);
        } else {
            /* e.g. id / / */
            snprintf(buffer, sizeof(buffer), "%s_%06o_%06o",
                     stable_chunk, r1, r2);
        }

        if (!id_exists(gen, buffer)) {
            register_id(gen, buffer);
            return bob_strdup(buffer);
        }
    }

    /* Fallback unique ID */
    snprintf(buffer, sizeof(buffer), "%s_%s_%06o_%06o%s",
             gen->prefix, stable_chunk, (unsigned int)rand() % 1000000, (unsigned int)rand() % 1000000, gen->suffix);
    register_id(gen, buffer);
    return bob_strdup(buffer);
}

void id_gen_free(IdGenerator *gen) {
    if (!gen) return;
    if (gen->prefix) free(gen->prefix);
    if (gen->suffix) free(gen->suffix);
    for (size_t i = 0; i < gen->count; i++) {
        free(gen->generated_ids[i]);
    }
    free(gen->generated_ids);
    free(gen);
}
