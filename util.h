#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

void die(int status, const char *format, ...);


// Allocation methods

void *ecalloc(size_t nmemb, size_t size);
void *emalloc(size_t size);
void *erealloc(void *ptr, size_t size);


// Filesystem methods

// Create a directories and all its parents
void mkpath(const char *path, mode_t mode);

// Copy a file from one location to the other
int cp(const char *dst, const char *src);

#endif /* UTIL_H */
