#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

#define MEMERROR "Memory allocation error.\n"


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
        die(1, MEMERROR);
    return ptr;
}

void *emalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
        die(1, MEMERROR);
    return ptr;
}

void *erealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL)
        die(1, MEMERROR);
    return ptr;
}

void mkpath(const char *path, mode_t mode)
{
    char curpath[PATH_MAX+1];
    int len = strlen(path);

    strncpy(curpath, path, PATH_MAX);

    // Go through each pathectory in the path and create them from
    // beginning to end.
    for (int i = 1; i < len; i++) {
        if (curpath[i] == '/') {
            curpath[i] = '\0';
            mkdir(curpath, mode);
            curpath[i] = '/';
        }
    }

    mkdir(curpath, mode);
}

int cp(const char *dst, const char *src)
{
    char buf[4096];
    FILE *fd_dst, *fd_src;
    ssize_t nread;
    int errcode = 0;

    fd_dst = fopen(dst, "w");
    if (fd_dst == NULL)
        return -1;

    fd_src = fopen(src, "r");
    if (fd_src == NULL) {
        fclose(fd_dst);
        return -1;
    }

    while (nread = fread(buf, 1, sizeof buf, fd_src), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten ;

        while (nread > 0) {
            nwritten = fwrite(buf, 1, sizeof buf, fd_dst);
            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (ferror(fd_dst)) {
                errcode = ferror(fd_dst);
                goto cleanup;
            }
        }
    }

 cleanup:
    fclose(fd_dst);
    fclose(fd_src);
    return errcode;
}
