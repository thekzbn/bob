#include "meta.h"

static char *get_meta_path(const char *source_path) {
    size_t len = strlen(source_path) + 16;
    char *meta_path = (char *)bob_malloc(len);
    snprintf(meta_path, len, "%s.meta", source_path);
    return meta_path;
}

static uint32_t derive_3digit_octal(uint64_t val) {
    /* Map any value to a 3-digit octal number (100 - 777 in octal = 64 to 511 in dec) */
    uint32_t oct = (uint32_t)(val % 448) + 64; /* 64..511 */
    return oct;
}

MetadataRegistry *meta_load_or_create(const char *source_path) {
    MetadataRegistry *meta = (MetadataRegistry *)bob_calloc(1, sizeof(MetadataRegistry));
    meta->filepath = get_meta_path(source_path);
    meta->element_capacity = 16;
    meta->elements = (ElementMeta **)bob_malloc(sizeof(ElementMeta *) * meta->element_capacity);
    meta->element_count = 0;

    FILE *f = fopen(meta->filepath, "r");
    if (!f) {
        /* Create fresh metadata */
        meta->file_timestamp = (uint64_t)time(NULL);
        meta->file_stable_octal = derive_3digit_octal(meta->file_timestamp ^ 0xA5A5);
        return meta;
    }

    char line[512];
    ElementMeta *current_elem = NULL;

    while (fgets(line, sizeof(line), f)) {
        /* Strip trailing newline/carriage return */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0 || line[0] == '#') continue;

        if (strncmp(line, "file_timestamp:", 15) == 0) {
            meta->file_timestamp = (uint64_t)strtoull(line + 15, NULL, 10);
        } else if (strncmp(line, "file_octal:", 11) == 0) {
            meta->file_stable_octal = (uint32_t)strtoul(line + 11, NULL, 8);
        } else if (strcmp(line, "[element]") == 0) {
            current_elem = (ElementMeta *)bob_calloc(1, sizeof(ElementMeta));
            if (meta->element_count >= meta->element_capacity) {
                meta->element_capacity *= 2;
                meta->elements = (ElementMeta **)bob_realloc(meta->elements, sizeof(ElementMeta *) * meta->element_capacity);
            }
            meta->elements[meta->element_count++] = current_elem;
        } else if (current_elem && strncmp(line, "fingerprint:", 12) == 0) {
            current_elem->fingerprint = bob_strdup(line + 12);
        } else if (current_elem && strncmp(line, "birth_timestamp:", 16) == 0) {
            current_elem->birth_timestamp = (uint64_t)strtoull(line + 16, NULL, 10);
        } else if (current_elem && strncmp(line, "stable_octal:", 13) == 0) {
            current_elem->stable_octal = (uint32_t)strtoul(line + 13, NULL, 8);
        }
    }

    fclose(f);

    if (meta->file_stable_octal == 0) {
        if (meta->file_timestamp == 0) meta->file_timestamp = (uint64_t)time(NULL);
        meta->file_stable_octal = derive_3digit_octal(meta->file_timestamp ^ 0xA5A5);
    }

    return meta;
}

ElementMeta *meta_find_or_register_element(MetadataRegistry *meta, const char *fingerprint) {
    for (size_t i = 0; i < meta->element_count; i++) {
        if (strcmp(meta->elements[i]->fingerprint, fingerprint) == 0) {
            return meta->elements[i];
        }
    }

    /* Register new element */
    ElementMeta *new_elem = (ElementMeta *)bob_calloc(1, sizeof(ElementMeta));
    new_elem->fingerprint = bob_strdup(fingerprint);
    new_elem->birth_timestamp = (uint64_t)time(NULL) + (meta->element_count * 7);
    
    /* Mix with fingerprint hash */
    uint32_t h = 5381;
    for (const char *p = fingerprint; *p; p++) {
        h = ((h << 5) + h) + (unsigned char)*p;
    }
    new_elem->stable_octal = derive_3digit_octal(new_elem->birth_timestamp ^ h);

    if (meta->element_count >= meta->element_capacity) {
        meta->element_capacity *= 2;
        meta->elements = (ElementMeta **)bob_realloc(meta->elements, sizeof(ElementMeta *) * meta->element_capacity);
    }
    meta->elements[meta->element_count++] = new_elem;
    return new_elem;
}

void meta_save(MetadataRegistry *meta) {
    if (!meta || !meta->filepath) return;
    FILE *f = fopen(meta->filepath, "w");
    if (!f) return;

    fprintf(f, "# Bob Persistent Element Identity Metadata\n");
    fprintf(f, "file_timestamp:%llu\n", (unsigned long long)meta->file_timestamp);
    fprintf(f, "file_octal:%03o\n", meta->file_stable_octal);
    fprintf(f, "elements_count:%zu\n", meta->element_count);

    for (size_t i = 0; i < meta->element_count; i++) {
        ElementMeta *e = meta->elements[i];
        fprintf(f, "[element]\n");
        fprintf(f, "fingerprint:%s\n", e->fingerprint ? e->fingerprint : "");
        fprintf(f, "birth_timestamp:%llu\n", (unsigned long long)e->birth_timestamp);
        fprintf(f, "stable_octal:%03o\n", e->stable_octal);
    }

    fclose(f);
}

void meta_free(MetadataRegistry *meta) {
    if (!meta) return;
    if (meta->filepath) free(meta->filepath);
    for (size_t i = 0; i < meta->element_count; i++) {
        if (meta->elements[i]->fingerprint) free(meta->elements[i]->fingerprint);
        free(meta->elements[i]);
    }
    free(meta->elements);
    free(meta);
}
