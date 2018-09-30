#ifndef TAGS_H
#define TAGS_H

#include "core.h"

typedef struct TagVector {
    int size;
    char **tags;
} TagVector;

/**
 * tmtag_get_err() - Return a cstring containing the latest error.
 */
const char *tmtag_get_err();

/**
 * tmtag_is_valid_tag() - Returns 1 if the tag is valid, 0 otherwise.
 *
 * must_be_real - truthy if the tag must be valid, or falsy if it can also
 * be a pseudotag.
 */
int tmtag_is_valid(const char *tag, int must_be_real);

/**
 * tmtag_file_has_tags() - Returns 1 if the specified file has every
 * tag in the TagVector; returns 0 otherwise.
 */
int tmtag_file_has_tags(const TMFile *file, const TagVector *filters);

#endif // TAGS_H
