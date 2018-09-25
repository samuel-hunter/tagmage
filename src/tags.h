#ifndef TAGS_H
#define TAGS_H

#include "core.h"

typedef struct TagVector {
    int size;
    char **tags;
} TagVector;

int tagmage_is_valid_tag(const char *tag, int must_be_real);
int tagmage_file_has_tags(const TMFile *file, const TagVector *filters);

#endif /* TAGS_H */
