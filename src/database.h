#ifndef BACKEND_H
#define BACKEND_H

#include "core.h"

// Return any non-zero value to exit the callback loop.
typedef int (*file_callback)(const TMFile*, void*);
typedef int (*tag_callback)(const char*);

void tmdb_warn();

int tmdb_setup(const char *db_path);
int tmdb_cleanup();

int tmdb_new_file(const char *title);

int tmdb_edit_title(int file_id, const char *title);
int tmdb_add_tag(int file_id, const char *tag_name);
int tmdb_remove_tag(int file_id, const char *tag_name);
int tmdb_delete_file(int file_id);

int tmdb_get_file(int file_id, TMFile *file);
int tmdb_get_files(file_callback callback, void *arg);
int tmdb_get_files_by_tag(const char *tag, file_callback callback, void *arg);

int tmdb_has_tag(int file_id, const char *tag_name);
int tmdb_get_tags(tag_callback callback);
int tmdb_get_tags_by_file(int file_id, tag_callback callback);

int tmdb_has_tags(int file_id);

#endif /* BACKEND_H */
