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
            "  images   - List all images in id:ext:title format\n"
            "  path [ IMAGE ] - List the path of the database. If\n"
            "                   IMAGE is provided, list that path.\n",
            prog_name);
    exit(status);
}

static int print_image(const Image *image)
{
    printf("%i:%s:%s", image->id, image->ext, image->title);
    return 0;
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
    } else if (ARGEQ(i, "images")) {
        tagmage_get_images(&print_image);
    } else if (ARGEQ(i, "path")) {
        if (++i == argc) {
            // print Database path if no image id provided
            printf("%s\n", db_path);
        } else {
            int item_id;
            Image img;
            char path[PATH_MAX + 1];
            char buf[PATH_MAX + 1];
            if (sscanf(argv[i], "%i", &item_id) < 0)
                die(1, "Invalid number '%s'\n", argv[i]);

            if (tagmage_get_image(item_id, &img) != 0) {
                die(1, "Image %i doesn't exist.\n", item_id);
            }
            snprintf(buf, PATH_MAX, "/%i.", img.id);

            strncpy(path, db_path, PATH_MAX);
            strncat(path, buf, PATH_MAX);
            strncat(path, (char*) img.ext, PATH_MAX);
            printf("%s\n", path);
        }
    } else {
        // Unknown command
        fprintf(stderr, "Unknown command '%s'\n", argv[i]);
        print_usage(1);
    }
}
