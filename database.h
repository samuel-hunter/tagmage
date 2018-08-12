#ifndef BACKEND_H
#define BACKEND_H

#include "core.h"

// Return any non-zero value to exit the callback loop.
typedef int (*image_callback)(const Image*);
typedef int (*tag_callback)(const Tag*);

void tagmage_setup(const char *db_path);
void tagmage_cleanup();

int tagmage_new_image(const char *title, const char *ext);

void tagmage_edit_title(int image_id, char *title);
void tagmage_add_tag(int image_id, char *tag_name);
void tagmage_delete_image(int image_id);
void tagmage_delete_tag(int tag_id);

int tagmage_get_image(int image_id, Image *image);
void tagmage_get_images(image_callback callback);
void tagmage_get_untagged_images(image_callback callback);
void tagmage_get_images_by_tag(int tag_id, image_callback callback);
void tagmage_search_images(int *tag_ids, image_callback callback);

int tagmage_get_tag(int tag_id, Tag *tag);
void tagmage_get_tags(tag_callback callback);
void tagmage_get_tags_by_image(int image_id, tag_callback callback);

#endif /* BACKEND_H */
