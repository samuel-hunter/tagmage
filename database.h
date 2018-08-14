#ifndef BACKEND_H
#define BACKEND_H

#include "core.h"

// Return any non-zero value to exit the callback loop.
typedef int (*image_callback)(const Image*);
typedef int (*tag_callback)(const Tag*);

void tagmage_warn();

int tagmage_setup(const char *db_path);
int tagmage_cleanup();

int tagmage_new_image(const char *title, const char *ext);

int tagmage_edit_title(int image_id, char *title);
int tagmage_add_tag(int image_id, char *tag_name);
int tagmage_remove_tag(int image_id, char *tag_name);
int tagmage_delete_image(int image_id);

int tagmage_get_image(int image_id, Image *image);
int tagmage_get_images(image_callback callback);
int tagmage_get_untagged_images(image_callback callback);
int tagmage_get_images_by_tag(char *tag, image_callback callback);
int tagmage_search_images(int *tag_ids, image_callback callback);

int tagmage_get_tags(tag_callback callback);
int tagmage_get_tags_by_image(int image_id, tag_callback callback);

#endif /* BACKEND_H */
