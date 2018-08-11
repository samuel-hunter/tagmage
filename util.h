#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

void die(int status, const char *format, ...);

void *ecalloc(size_t nmemb, size_t size);
void *emalloc(size_t size);
void *erealloc(void *ptr, size_t size);

// FS functions
void mkpath(const char *path, mode_t mode);

#endif /* UTIL_H */
