#ifndef CORE_H
#define CORE_H

#include <limits.h>

#define EXT_MAX 255
#define TITLE_MAX 255
#define TAG_MAX 255

// See database.c:db_setup_queries
#define EXT_MAX_STR "255"
#define TITLE_MAX_STR "255"
#define TAG_MAX_STR "255"

typedef int timestamp; // UNIX epoch

typedef struct Image {
    int id;
    unsigned char title[TITLE_MAX + 1];
    unsigned char ext[EXT_MAX + 1];
} Image;

#endif /* CORE_H */
