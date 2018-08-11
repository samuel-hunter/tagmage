#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "util.h"


void die(int status, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(status);
}


void *ecalloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL)
        die(1, "Fatal error: no memory available.");
    return ptr;
}

void *emalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
        die(1, "Fatal error: no memory available.");
    return ptr;
}

void *erealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL)
        die(1, "Fatal error: no memory available.");
    return ptr;
}

void newstr(char **dest, const char *src)
{
    if (src != NULL) {
        *dest = emalloc(strlen(src) + 1);
        strcpy(*dest, src);
    } else {
        *dest = emalloc(1);
        *dest[0] = '\0';
    }
}
