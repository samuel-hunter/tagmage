#ifndef CORE_H
#define CORE_H

typedef int timestamp; // UNIX epoch
typedef unsigned char utf8; // UTF8-encoded byte

typedef enum category {CAT_NORMAL, CAT_META} category;

typedef struct Image {
    int id;
    utf8 *title;
    utf8 *ext;
} Image;

typedef struct Tag {
    int id;
    utf8 *name;
    category category;
} Tag;

#endif /* CORE_H */
