#include <bsd/string.h> // strlcpy
#include <errno.h> // errno, ENOBUFS
#include <stdio.h> // snprintf
#include <stdlib.h> // getenv
#include <limits.h> // PATH_MAX

#include "database.h"
#include "util.h" // mkpath
#include "libtagmage.h"

static enum { ERR_OK, ERR_LIBC, ERR_DATABASE } err_status = ERR_OK;

static char tagmage_path[PATH_MAX + 1] = {0};

int tm_init(const char *path)
{
    char *env = NULL;
    size_t len = 0;

    if (path) {
        len = strlcpy(tagmage_path, path, sizeof(tagmage_path));
    } else if (env = getenv("TAGMAGE_HOME"), env) {
        len = strlcpy(tagmage_path, env, sizeof(tagmage_path));
    } else if (env = getenv("XDG_DATA_HOME"), env) {
        len = snprintf(tagmage_path, sizeof(tagmage_path),
                       "%s/tagmage", env);
    } else {
        len = snprintf(tagmage_path, sizeof(tagmage_path),
                       "%s/.local/share/tagmage", getenv("HOME"));
    }

    // Double-check to make sure the buffer size was all right.
    if (len >= sizeof(tagmage_path)) {
        errno = ENOBUFS;
        return -1;
    }

    // Create path to database if it's not set up already
    if (mkpath(tagmage_path, 0700) < 0)
        return -1; // Errno is set correctly.

    // Set up database.
    char db_path[PATH_MAX + 1] = {0};
    len = snprintf(db_path, sizeof(db_path), "%s/db.sqlite", tagmage_path);

    // Double-check to make sure the buffer size was all right.
    if (len >= sizeof(tagmage_path)) {
        errno = ENOBUFS;
        return -1;
    }

    if (tmdb_setup(db_path) < 0)
        return -1;

    return 0;
}

const char *tm_get_error() {
    switch (err_status) {
    case ERR_LIBC:
        return strerror(errno);
    case ERR_DATABASE:
        return tmdb_get_error();
    default:
        return NULL;
    }
}

const char *tm_path()
{
    return tagmage_path;
}

size_t tm_file_path(const TMFile *file, char *dst, size_t n)
{
    return snprintf(dst, n, "%s/%i", tagmage_path, file->id);
}

int tm_add_file(const char *path, TMFile *file)
{
    const char *basename = NULL;
    char path_buf[PATH_MAX + 1] = {0};
    size_t len = 0;

    // Search for the basename.
    basename = strrchr(path, '/');
    if (basename) {
        basename++;
    } else {
        basename = path;
    }

    // Copy the basename to the image struct.
    len = strlcpy((char*) file->title, basename, sizeof(file->title));
    if (len >= sizeof(file->title)) {
        errno = ENOBUFS;
        return -1;
    }

    // Get the file id.
    file->id = tmdb_new_file(basename);
    if (file->id < 0)
        return -1;

    // Prepare the file path
    if (tm_file_path(file, path_buf, sizeof(path_buf)) >= sizeof(path_buf)) {
        errno = ENOBUFS;
        return -1;
    }

    // Copy the file and handle file errors.
    if (cp(path_buf, path) < 0) {
        tmdb_delete_file(file->id);
        return -1;
    }

    // Everything OK!
    return 0;
}

int tm_rm_file(const TMFile *file)
{
    char path_buf[PATH_MAX + 1];
    size_t len;

    // Copy file path to buffer.
    len = tm_file_path(file, path_buf, sizeof(path_buf));
    if (len >= sizeof(path_buf)) {
        errno = ENOBUFS;
        return -1;
    }

    // Remove the file.
    int status = remove(path_buf);
    if (status != 0) {
        // If the file never existed in the first place, we can
        // recover. Otherwise, return an error code.
        if (status != ENOENT)
            return -1;
    }

    // Delete the reference from the data base.
    if (tmdb_delete_file(file->id) < 0)
        return -1;

    return 0;
}
