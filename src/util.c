#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include "limits.h"
#include "util.h"


int mkpath(const char *path, mode_t mode)
{
    char curpath[PATH_MAX+1];
    int len = strlen(path);

    strncpy(curpath, path, PATH_MAX);

    // Go through each pathectory in the path and create them from
    // beginning to end.
    for (int i = 1; i < len; i++) {
        if (curpath[i] == '/') {
            curpath[i] = '\0';
            if (mkdir(curpath, mode) != 0 && errno != EEXIST)
                return -1;
            curpath[i] = '/';
        }
    }

    if (mkdir(curpath, mode) != 0 && errno != EEXIST)
        return -1;

    return 0;
}

int cp(const char *dst, const char *src)
{
    char buf[4096];
    FILE *fd_dst = NULL, *fd_src = NULL;
    size_t nread = 0;
    int errcode = 0;

    fd_dst = fopen(dst, "w");
    if (fd_dst == NULL)
        return -1;

    fd_src = fopen(src, "r");
    if (fd_src == NULL) {
        fclose(fd_dst);
        return -2;
    }

    while (nread = fread(buf, 1, sizeof buf, fd_src), nread > 0) {
        char *out_ptr = buf;
        size_t nwritten;

        while (nread > 0) {
            nwritten = fwrite(buf, 1, MIN(sizeof buf, nread), fd_dst);
            if (ferror(fd_dst)) {
                errcode = ferror(fd_dst);
                goto cleanup;
            }

            nread -= nwritten;
            out_ptr += nwritten;
        }
    }

 cleanup:
    fclose(fd_dst);
    fclose(fd_src);
    return errcode;
}
