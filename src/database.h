#ifndef BACKEND_H
#define BACKEND_H

#include "core.h"

// Return any non-zero value to exit the callback loop.
typedef int (*file_callback)(const TMFile*, void*);
typedef int (*tag_callback)(const char*);

void tagmage_warn();

int tagmage_setup(const char *db_path);
int tagmage_cleanup();

int tagmage_new_file(const char *title);

int tagmage_edit_title(int file_id, const char *title);
int tagmage_add_tag(int file_id, const char *tag_name);
int tagmage_remove_tag(int file_id, const char *tag_name);
int tagmage_delete_file(int file_id);

int tagmage_get_file(int file_id, TMFile *file);
int tagmage_get_files(file_callback callback, void *arg);
int tagmage_get_files_by_tag(const char *tag, file_callback callback, void *arg);

int tagmage_has_tag(int file_id, const char *tag_name);
int tagmage_get_tags(tag_callback callback);
int tagmage_get_tags_by_file(int file_id, tag_callback callback);

int tagmage_has_tags(int file_id);

#endif /* BACKEND_H */
