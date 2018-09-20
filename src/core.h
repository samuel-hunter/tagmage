#ifndef CORE_H
#define CORE_H

#define TITLE_MAX 4096
#define TAG_MAX 4096

// See database.c:db_setup_queries
#define TITLE_MAX_STR "4096"
#define TAG_MAX_STR "4096"

typedef int timestamp; // UNIX epoch

typedef struct Image {
    int id;
    unsigned char title[TITLE_MAX + 1];
} Image;

#endif /* CORE_H */
