#include <stdio.h>
#include <string.h>
#include "core.h"
#include "database.h"

int print_image(const Image *img)
{
    printf("%4i  %10s %5s\n", img->id, img->title, img->ext);
    return 0;
}

int print_tag(const Tag *tag)
{
    printf("%4i  %10s\n", tag->id, tag->name);
    return 0;
}

/* int main (void) */
/* { */
/*     char tags[100][100]; */
/*     int images[100] = {0}; */
/*     char name[10] = {0}; */

/*     if (tagmage_setup(NULL) < 0) */
/*         tagmage_err(1); */

/*     for (int ti = 0; ti < 100; ti++) { */
/*         sprintf(tags[ti], "tag-%03i", ti); */
/*     } */

/*     for (int ii = 0; ii < 100; ii++) { */
/*         sprintf(name, "image-%03i", ii); */
/*         images[ii] = tagmage_new_image(name, ".png"); */
/*         if (images[ii] < 0) */
/*             tagmage_err(1); */

/*         for (int ti = 0; ti < 100; ti++) { */
/*             if (ii % (ti + 1) == 0) { */
/*                 if (tagmage_add_tag(images[ii], tags[ti]) < 0) */
/*                     tagmage_err(1); */
/*             } */
/*         } */
/*     } */

/*     if (tagmage_get_images(&print_image) < 0) */
/*         tagmage_err(1); */
/*     printf("\n"); */

/*     if (tagmage_get_tags_by_image(images[60], print_tag) < 0) */
/*         tagmage_err(1); */
/*     printf("\n"); */

/*     if (tagmage_cleanup() < 0) */
/*         tagmage_err(1); */
/*     return 0; */
/* } */

int main(void)
{
    if (tagmage_setup(NULL) < 0)
        tagmage_err(1);

    if (tagmage_cleanup() < 0)
        tagmage_err(1);
}
