#ifndef LIBTAGMAGE_H
#define LIBTAGMAGE_H

#include "core.h"
#include <unistd.h> // size_t

int tm_init(const char *path);
const char *tm_get_error();

const char *tm_path();
size_t tm_file_path(const TMFile *file, char *dst, size_t n);

int tm_add_file(const char *path, TMFile *file);
int tm_rm_file(const TMFile *file);


#endif // LIBTAGMAGE_H
