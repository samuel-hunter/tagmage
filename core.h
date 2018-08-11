#ifndef CORE_H
#define CORE_H

#include <limits.h>

#define UTF8_MAX NAME_MAX

typedef int timestamp; // UNIX epoch
typedef unsigned char utf8; // UTF8-encoded byte

typedef enum category {CAT_NORMAL, CAT_META} category;

typedef struct Image {
    int id;
    utf8 title[UTF8_MAX + 1];
    utf8 ext[UTF8_MAX + 1];
} Image;

typedef struct Tag {
    int id;
    utf8 name[UTF8_MAX + 1];
    category category;
} Tag;

#endif /* CORE_H */
