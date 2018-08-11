#include <stdio.h>
#include "core.h"
#include "database.h"

int print_image(const Image *img)
{
    printf("%4i  %10s %5s\n", img->id, img->title, img->ext);
    return 0;
}

int print_tag(const Tag *tag)
{
    printf("%4i  %10s %i\n", tag->id, tag->name, tag->category);
    return 0;
}

int main (void)
{
    int tags[100] = {0};
    int images[100] = {0};
    char name[10] = {0};

    tagmage_setup(NULL);

    for (int ti = 0; ti < 100; ti++) {
        sprintf(name, "tag-%03i", ti);
        tags[ti] = tagmage_new_tag(name, CAT_NORMAL);
    }

    for (int ii = 0; ii < 100; ii++) {
        sprintf(name, "image-%03i", ii);
        images[ii] = tagmage_new_image(name, ".png");

        for (int ti = 0; ti < 100; ti++) {
            if (ii % (ti + 1) == 0)
                tagmage_tag_image(images[ii], tags[ti]);
        }
    }

    tagmage_get_images(&print_image);
    printf("\n");

    tagmage_get_tags_by_image(images[60], print_tag);
    printf("\n");

    return 0;
}
