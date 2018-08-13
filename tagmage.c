#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "database.h"
#include "util.h"

// Increment the variable and assert that it's a valid argument index.
#define INCARG(I) if (++I >= argc) \
        die(1, "Expected value after '%s'\n", argv[I-1])

#define ARGEQ(I, STR) !strcmp(argv[I], STR)

#define ERR_UNKNOWN_FLAG "Unknown flag '%s'."
#define ERR_UNKNOWN_SUBCMD "Unknown command '%s'."
#define TAG_MAX 1024


static char db_path[PATH_MAX+1] = {0};
static char prog_name[NAME_MAX+1] = {0};


// Recursively create a directory path


static void print_usage(int status)
{
    // Print to stderr on error; stdout otherwise
    FILE *f = status ? stderr : stdout;

    fprintf(f,
            "Usage: %s [ -f PATH ] COMMAND [ ... ]\n"
            "\n"
            "  -f PATH  - Use the database at PATH over the default\n"
            "\n"
            " Commands:\n"
            "  list   - List all images in id:ext:title format\n"
            "  path [ IMAGE ... ] - List the path of the database.\n"
            "                 If IMAGE is provided, list that path.\n"
            "  add -t [ TAG1,TAG2,... ] IMAGE - Add an image to the\n"
            "                 database\n\n",
            prog_name);
    exit(status);
}

static int print_image(const Image *image)
{
    printf("%i %s %s\n", image->id, image->ext, image->title);
    return 0;
}

static void list_images(int argc, char **argv)
{
    if (argc == 0) {
        tagmage_get_images(print_image);
        return;
    }

    for (int i = 0; i < argc; i++) {
        tagmage_get_images_by_tag(argv[i], print_image);
        printf("\n");
    }
}

static void print_path(int argc, char **argv)
{
    Image img;
    int item_id;

    if (argc == 0) {
        // print Database path if no image id provided
        printf("%s\n", db_path);
        return;
    }

    // Each subsequent argument is an image id
    for (int i = 0; i < argc; i++) {
        if (sscanf(argv[i], "%i", &item_id) < 0)
            die(1, "Invalid number '%s'\n", argv[i]);

        if (tagmage_get_image(item_id, &img) != 0)
            die(1, "Image %i doesn't exist.\n", item_id);

        printf("%s/%i.%s\n", db_path, img.id, img.ext);
    }

    return;
}

static void add_image(int argc, char **argv)
{
    char *path = NULL, *basename = NULL, *ext = NULL;
    char image_dest[NAME_MAX + 1] = {'\0'};
    int image_id = 0, i = 0;
    char *tags[TAG_MAX] = {NULL};
    size_t num_tags = 0;

    for (i = 0; i < argc; i++) {
        // [ -t TAG1,TAG2,.. ] - optional list of comma separated tags
        if (ARGEQ(i, "-t")) {
            char *seltok;

            // Get the comma-separated tag list and start tokenizing.
            INCARG(i);
            seltok = strtok(argv[i], ",");

            // Go through the comma-separated taglist and add tags to
            // the taglist for later.
            while (seltok != NULL) {
                tags[num_tags] = seltok;
                if (++num_tags == TAG_MAX)
                    die(1, "Too many tags!");
                seltok = strtok(NULL, ",");
            }
        } else if (ARGEQ(i, "--")) {
            // Everything after this is the images
            i++;
            break;
        } else if (argv[i][0] == '-') {
            // Unknown flag
            fprintf(stderr, ERR_UNKNOWN_FLAG, argv[i]);
            print_usage(1);
        } else {
            // Images start at this point in the argument list.
            break;
        }
    }

    // Images expected here
    if (i >= argc)
        die(1, "Missing file operand after '%s'.\n", argv[i-1]);

    // Iterate through each image
    for (; i < argc; i++) {
        ext = basename = path = argv[i];

        // Search for the basename.
        for (size_t i = 0; i < strlen(path); i++)
            if (path[i] == '/') ext = basename = path + i + 1;

        // Search for the file's extension.
        for (size_t i = 0; i < strlen(basename); i++)
            if (basename[i] == '.') ext = basename + i + 1;

        if (ext == basename) {
            // If the extension still points to the basename, then there
            // is no extension.
            ext = NULL;
        } else {
            // Split the basename from the extension.
            ext[-1] = 0;
        }

        image_id = tagmage_new_image(basename, ext);
        if (image_id < 0)
            die(1, "Unknown image id %i\n", image_id);

        // Rejoin the basename to the extension
        if (ext) ext[-1] = '.';

        snprintf(image_dest, NAME_MAX, "%s/%i.%s", db_path, image_id, ext);
        if (cp(image_dest, path) != 0) {
            // Operation failed; remove image data from database.
            tagmage_delete_image(image_id);
            die(1, "Unexpected I/O error\n");
        }

        printf("%i\n", image_id);

        // Add each tag to the image
        for(size_t ti = 0; ti < num_tags; ti++) {
            tagmage_add_tag(image_id, tags[ti]);
        }
    }
}

int main(int argc, char **argv)
{
    strncpy(prog_name, argv[0], NAME_MAX);

    int i;
    for (i = 1; i < argc; i++) {
        // Flags and Options
        if (ARGEQ(i, "-f")) {
            INCARG(i);
            strncpy(db_path, argv[i], PATH_MAX);
        } else if (ARGEQ(i, "-h")) {
            print_usage(0);
            // exits
        } else if (argv[i][0] == '-') {
            // Unknown flag
            fprintf(stderr, "Unknown flag '%s'\n", argv[i]);
            print_usage(1);
        } else {
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
    mkpath(db_path, 0700);

    // Set up backend database
    char db_file[PATH_MAX + 1];
    strncpy(db_file, db_path, PATH_MAX);
    strncat(db_file, "/db.sqlite", PATH_MAX);
    tagmage_setup(db_file);

    // Sub-Commands
    if (i == argc || ARGEQ(i, "help")) {
        // Default command; also runs if no other arguments exist
        print_usage(1);
    } else if (ARGEQ(i, "list")) {
        list_images(argc - i - 1, argv + i + 1);
    } else if (ARGEQ(i, "path")) {
        // Give arguments to print_path for everythign past "path"
        print_path(argc - i - 1, argv + i + 1);
        return 0;
    } else if (ARGEQ(i, "add")) {
        // Give arguments to add_image for everything past "add".
        add_image(argc - i - 1, argv + i + 1);
        return 0;
    } else {
        // Unknown command
        fprintf(stderr, "Unknown command '%s'\n", argv[i]);
        print_usage(1);
    }
}
