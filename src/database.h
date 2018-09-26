#ifndef BACKEND_H
#define BACKEND_H

#include "core.h"

// Return any non-zero value to exit the callback loop.
typedef int (*file_callback)(const TMFile*, void*);
typedef int (*tag_callback)(const char*);

/*
 * database.h -- tagmage database commands. All methods act as the backend for
 * tagmage and interacts with the SQLite database engine. All successful
 * calls return a non-negative number on success.
 *
 * If documentation does not specify, the method returns 0 on success, or -1
 * on error.
 */

/**
 * tmdb_get_error() - Return a cstring containing the latest error.
 */
const char *tmdb_get_error();

/**
 * tmdb_setup() - Set up and load the database.
 *
 * db_path - the path to the tagmage database directory.
 */
int tmdb_setup(const char *db_path);

/**
 * tmdb_cleanup() - Clean up any loose ends in the database and close the
 * connection.
 */
int tmdb_cleanup();

/**
 * tmdb_new_file() - Add a file record to the database.
 */
int tmdb_new_file(const char *title);

/**
 * tmdb_edit_title() - Change the title of a file record.
 */
int tmdb_edit_title(int file_id, const char *title);

/**
 * tmdb_add_tag() - Add a tag to a file record.
 */
int tmdb_add_tag(int file_id, const char *tag_name);

/**
 * tmdb_remove_tag() - Remove tag from the file record.
 */
int tmdb_remove_tag(int file_id, const char *tag_name);

/**
 * tmdb_delete_file() - Remove a file record.
 */
int tmdb_delete_file(int file_id);

/**
 * tmdb_get_file() - Retrieve file data from its id.
 *
 * file - Pointer where the information will be stored.
 */
int tmdb_get_file(int file_id, TMFile *file);

/**
 * tmdb_get_files() - Get every file, and call `callback` for each file.
 *
 * arg - A void pointer that also gets passed to `callback`.
 */
int tmdb_get_files(file_callback callback, void *arg);

/**
 * tmdb_has_tag() - Returns 1 if the specified file has the tag, -1 on error,
 * and 0 otherwise.
 */
int tmdb_has_tag(int file_id, const char *tag_name);

/**
 * tmdb_get_tags() - Calls `callback` for every real tag.
 */
int tmdb_get_tags(tag_callback callback);

/**
 * tmdb_get_tags_by_file() - Calls `callback` for every tag that the specified
 * file has.
 */
int tmdb_get_tags_by_file(int file_id, tag_callback callback);

/**
 * tmdb_has_tags() - Returns 1 if the specified file has any tags, -1 on error,
 * and 0 otherwise.
 */
int tmdb_has_tags(int file_id);

#endif /* BACKEND_H */
