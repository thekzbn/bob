#ifndef BOB_META_H
#define BOB_META_H

#include "common.h"

typedef struct {
    char *fingerprint;
    uint64_t birth_timestamp;
    uint32_t stable_octal; /* 3 octal digits: 000 - 777 */
} ElementMeta;

typedef struct {
    char *filepath;
    uint64_t file_timestamp;
    uint32_t file_stable_octal; /* 3 octal digits: 000 - 777 */
    ElementMeta **elements;
    size_t element_count;
    size_t element_capacity;
} MetadataRegistry;

MetadataRegistry *meta_load_or_create(const char *source_path);
void meta_save(MetadataRegistry *meta);
void meta_free(MetadataRegistry *meta);

ElementMeta *meta_find_or_register_element(MetadataRegistry *meta, const char *fingerprint);

#endif /* BOB_META_H */
