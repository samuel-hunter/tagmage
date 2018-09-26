#ifndef TAGS_H
#define TAGS_H

#include "core.h"

typedef struct TagVector {
    int size;
    char **tags;
} TagVector;

/**
 * tagmage_is_valid_tag() - Returns 1 if the tag is valid, 0 otherwise.
 *
 * must_be_real - truthy if the tag must be valid, or falsy if it can also
 * be a pseudotag.
 */
int tagmage_is_valid_tag(const char *tag, int must_be_real);

/**
 * tagmage_file_has_tags() - Returns 1 if the specified file has every
 * tag in the TagVector; returns 0 otherwise.
 */
int tagmage_file_has_tags(const TMFile *file, const TagVector *filters);

#endif /* TAGS_H */
