#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

void die(int status, const char *format, ...);

void *ecalloc(size_t nmemb, size_t size);
void *emalloc(size_t size);
void *erealloc(void *ptr, size_t size);

void newstr(char **dest, const char *src);

#endif /* UTIL_H */
