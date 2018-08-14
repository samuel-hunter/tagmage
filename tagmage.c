#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "core.h"
#include "database.h"
#include "util.h"

// More semantic equality checker.
#define STREQ(S1, S2) (!strcmp((S1),(S2)))

#define APP_NAME "tagmage"
#define TAG_MAX 1024

#define TAGMAGE_ASSERT(EXPR) if ((EXPR) < 0) { \
    tagmage_warn(); \
    return 1; \
    } do {} while (0)


static char db_path[PATH_MAX+1] = {0};


static void print_usage(FILE *f)
{

    fprintf(f,
            "Usage: tagmage [ -f PATH ] COMMAND [ ... ]\n"
            "\n"
            "  -f SAVE  - Set custom save directory.\n"
            "\n"
            "  add [-f TAG1,TAG2,...] IMAGES..\n"
            "  edit IMAGE TITLE\n"
            "  list [TAGS..]\n"
            "  tag IMAGE [TAGS..]\n"
            "  untag IMAGE [TAGS..]\n"
            "  tags IMAGE\n"
            "  path [IMAGES..]\n"
            "  rm IMAGES..\n"
            "\n"
            "Visit `man 1 tagmage` for more details.\n");
}

static int print_image(const Image *image)
{
    printf("%i %s %s\n", image->id, image->ext, image->title);
    return 0;
}

static int print_tag(const Tag *tag)
{
    printf("%s\n", tag->name);
    return 0;
}

static int list_images(int argc, char **argv)
{
    if (argc == 1) {
        TAGMAGE_ASSERT(tagmage_get_images(print_image));
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        TAGMAGE_ASSERT(tagmage_get_images_by_tag(argv[i],
                                                 print_image));
        printf("\n");
    }

    return 0;
}

static int print_path(int argc, char **argv)
{
    Image img;
    int item_id;

    if (argc == 1) {
        // print Database path if no image id provided
        printf("%s\n", db_path);
        return 0;
    }

    // Each subsequent argument is an image id
    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &item_id) < 0)
            errx(1, "Invalid number '%s'", argv[i]);

        TAGMAGE_ASSERT(tagmage_get_image(item_id, &img));

        printf("%s/%i%s\n", db_path, img.id, img.ext);
    }

    return 0;
}

static int add_image(int argc, char **argv)
{
    char *path = NULL, *basename = NULL, *ext = NULL;
    char ext_buf[NAME_MAX + 1] = {'\0'};
    char *seltok;
    char image_dest[NAME_MAX + 1] = {'\0'};
    int image_id = 0, opt = 0;
    char *tags[TAG_MAX] = {NULL};
    size_t num_tags = 0;

    while (opt = getopt(argc, argv, "t:"), opt != -1) {
        switch (opt) {
        case 't':
            seltok = strtok(optarg, ",");

            // Go through the comma-separated taglist and add tags to
            // the taglist for later.
            while (seltok != NULL) {
                tags[num_tags] = seltok;
                if (++num_tags == TAG_MAX)
                    errx(1, "Too many tags.");
                seltok = strtok(NULL, ",");
            }
            break;
        case '?':
            print_usage(stderr);
            return 1;
        }
    }

    // Images expected here
    if (optind >= argc) {
        warnx("Missing file operand.");
        print_usage(stderr);
        return 1;
    }

    // Iterate through each image
    for (int i = optind; i < argc; i++) {
        ext = basename = path = argv[i];

        // Search for the basename.
        for (size_t i = 0; i < strlen(path); i++)
            if (path[i] == '/') ext = basename = path + i + 1;

        // Search for the file's extension.
        for (size_t i = 0; i < strlen(basename); i++)
            if (basename[i] == '.') ext = basename + i;

        if (ext != basename) {
            strcpy(ext_buf, ext);
            ext[0] = '\0';
        }

        TAGMAGE_ASSERT(image_id = tagmage_new_image(basename,
                                                    ext_buf));

        snprintf(image_dest, NAME_MAX, "%s/%i%s",
                 db_path, image_id, ext_buf);
        ext[0] = '.';
        // Copy file and handle file errors.
        switch (cp(image_dest, path)) {
        case -1:
            // Attempt to delete image; don't error-check, since we're
            // already failing.
            tagmage_delete_image(image_id);
            err(1, "%s", image_dest);
        case -2:
            // Don't error-check, since we're already failing.
            tagmage_delete_image(image_id);
            err(1, "%s", path);
        }

        printf("%i\n", image_id);

        // Add each tag to the image
        for(size_t ti = 0; ti < num_tags; ti++) {
            TAGMAGE_ASSERT(tagmage_add_tag(image_id, tags[ti]));
        }
    }

    return 0;
}

static int rm_image(int argc, char **argv)
{
    int id = 0;
    Image img;
    char path[PATH_MAX + 1];

    if (argc == 1) {
        fprintf(stderr, "Missing file operand.\n");
        print_usage(stderr);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &id) != 1)
            errx(1, "Unknown number '%s'.", argv[i]);

        TAGMAGE_ASSERT(tagmage_get_image(id, &img));

        snprintf(path, PATH_MAX, "%s/%i%s", db_path, id, img.ext);
        int status = remove(path);
        if (status != 0) {
            // I/O Error
            warn("%s", path);
            if (status != ENOENT)
                exit(1);

            // File doesn't exist; we can recover
            fprintf(stderr, "Removing reference anyway...\n");
        }

        TAGMAGE_ASSERT(tagmage_delete_image(id));
    }

    return 0;
}

static int edit_image(int argc, char **argv)
{
    int id = 0;
    char *newtitle = NULL;

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        print_usage(stderr);
        return 1;
    }

    if (sscanf(argv[1], "%i", &id) != 1)
        errx(1, "'%s' is not a valid number.", argv[1]);

    newtitle = argv[2];

    TAGMAGE_ASSERT(tagmage_edit_title(id, newtitle));

    return 0;
}

int tag_image(int argc, char **argv)
{
    int image_id = 0;

    if (argc == 1) {
        warnx("Missing image after '%s'.", argv[0]);
        return -1;
    }

    if (sscanf(argv[1], "%i", &image_id) != 1) {
        warnx("'%s' is not a valid number.", argv[1]);
        return -1;
    }

    for (int i = 2; i < argc; i++) {
        TAGMAGE_ASSERT(tagmage_add_tag(image_id, argv[i]));
    }

    return 0;
}

int untag_image(int argc, char **argv)
{
    int image_id = 0;

    if (argc == 1) {
        warnx("Missing image after '%s'.", argv[0]);
        return -1;
    }

    if (sscanf(argv[1], "%i", &image_id) != 1) {
        warnx("'%s' is not a valid number.", argv[1]);
        return -1;
    }

    for (int i = 2; i < argc; i++) {
        TAGMAGE_ASSERT(tagmage_remove_tag(image_id, argv[i]));
    }

    return 0;
}

int list_tags(int argc, char **argv)
{
    int image_id = 0;

    if (argc == 1) {
        warnx("Missing image after '%s'.", argv[0]);
        return -1;
    }

    if (sscanf(argv[1], "%i", &image_id) != 1) {
        warnx("'%s' is not a valid number.", argv[1]);
        return -1;
    }

    TAGMAGE_ASSERT(tagmage_get_tags_by_image(image_id, &print_tag));
    return 0;
}

int main(int argc, char **argv)
{
    int opt, status = 0;

    while (opt = getopt(argc, argv, "hf:"), opt != -1) {
        switch (opt) {
        case 'h':
            // alias for 'help' subcommand
            print_usage(0);
            break;
        case 'f':
            // Set custom database folder
            strncpy(db_path, optarg, PATH_MAX);
            break;
        case '?':
            // Error
            print_usage(stderr);
            return 1;
            break;
        }
    }

    // Set-up db_path if it's not yet initialized
    if (db_path[0] == '\0') {
        char *xdg = getenv("XDG_DATA_HOME");
        if (xdg) {
            // Use $XDG_DATA_HOME/tagmage as default
            strncat(db_path, xdg, PATH_MAX);
        } else {
            // Use $HOME/.local/share/tagmage as backup default
            strncpy(db_path, getenv("HOME"), PATH_MAX);
            strncat(db_path, "/.local/share/tagmage", PATH_MAX);
        }
    }

    // Create path to database if it's not set up already
    if (mkpath(db_path, 0700) < 0)
        err(1, "db_path");

    // Set up backend database
    char db_file[PATH_MAX + 1];
    strncpy(db_file, db_path, PATH_MAX);
    strncat(db_file, "/db.sqlite", PATH_MAX);

    TAGMAGE_ASSERT(tagmage_setup(db_file));

    // Shift argc, argv to subcommands
    argc -= optind;
    argv += optind;
    optind = 1; // Reset getopt


    // Sub-Commands
    if (argc == 0 || STREQ(argv[0], "help")) {
        // Default command; also runs if no other arguments exist
        print_usage(stdout);
    } else if (STREQ(argv[0], "list")) {
        status = list_images(argc, argv);
    } else if (STREQ(argv[0], "path")) {
        // Give arguments to print_path for everythign past "path"
        status = print_path(argc, argv);
    } else if (STREQ(argv[0], "add")) {
        // Give arguments to add_image for everything past "add".
        status = add_image(argc, argv);
    } else if (STREQ(argv[0], "rm")) {
        status = rm_image(argc, argv);
    } else if (STREQ(argv[0], "tag")) {
        status = tag_image(argc, argv);
    } else if (STREQ(argv[0], "untag")) {
        status = untag_image(argc, argv);
    } else if (STREQ(argv[0], "tags")) {
        status = list_tags(argc, argv);
    } else if (STREQ(argv[0], "edit")) {
        status = edit_image(argc, argv);
    } else {
        // Unknown command
        fprintf(stderr, "Unknown command '%s'\n", argv[0]);
        print_usage(stderr);
        status = 1;
    }

    TAGMAGE_ASSERT(tagmage_cleanup());
    return status;
}
