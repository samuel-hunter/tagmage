#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "core.h"
#include "database.h"
#include "util.h"

// More semantic equality checker.
#define STREQ(S1, S2) (!strcmp((S1),(S2)))

#define APP_NAME "tagmage"
#define TAG_MAX 1024


static char db_path[PATH_MAX+1] = {0};


static void print_usage(int status)
{
    // Print to stderr on error; stdout otherwise
    FILE *f = status ? stderr : stdout;

    fprintf(f,
            "Usage: tagmage [ -f PATH ] COMMAND [ ... ]\n"
            "\n"
            "  -f PATH  - Use the database at PATH over the default\n"
            "\n"
            " Commands:\n"
            "  list   - List all images in id:ext:title format\n"
            "  path [ IMAGE ... ] - List the path of the database.\n"
            "                 If IMAGE is provided, list that path.\n"
            "  add -t [ TAG1,TAG2,... ] IMAGE - Add an image to the\n"
            "                 database\n\n");
    exit(status);
}

static int print_image(const Image *image)
{
    printf("%i %s %s\n", image->id, image->ext, image->title);
    return 0;
}

static void list_images(int argc, char **argv)
{
    if (argc == 1) {
        if (tagmage_get_images(print_image) < 0)
            tagmage_err(1);
        else
            return;
    }

    for (int i = 1; i < argc; i++) {
        if (tagmage_get_images_by_tag(argv[i], print_image) < 0)
            tagmage_err(1);

        printf("\n");
    }
}

static void print_path(int argc, char **argv)
{
    Image img;
    int item_id;

    if (argc == 1) {
        // print Database path if no image id provided
        printf("%s\n", db_path);
        return;
    }

    // Each subsequent argument is an image id
    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &item_id) < 0)
            errx(1, "Invalid number '%s'", argv[i]);

        if (tagmage_get_image(item_id, &img) < 0)
            tagmage_err(1);

        printf("%s/%i.%s\n", db_path, img.id, img.ext);
    }
}

static void add_image(int argc, char **argv)
{
    char *path = NULL, *basename = NULL, *ext = NULL;
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
            print_usage(1);
            break;
        }
    }

    // Images expected here
    if (optind >= argc) {
        fprintf(stderr, "Missing file operand.");
        print_usage(1);
    }

    // Iterate through each image
    for (int i = optind; i < argc; i++) {
        ext = basename = path = argv[i];

        // Search for the basename.
        for (size_t i = 0; i < strlen(path); i++)
            if (path[i] == '/') ext = basename = path + i + 1;

        // Search for the file's extension.
        for (size_t i = 0; i < strlen(basename); i++)
            if (basename[i] == '.') ext = basename + i + 1;

        if (ext == basename) {
            // If the extension still points to the basename, then
            // there is no extension.
            ext = NULL;
        } else {
            // Split the basename from the extension.
            ext[-1] = 0;
        }

        image_id = tagmage_new_image(basename, ext);
        if (image_id < 0)
            tagmage_err(1);

        // Rejoin the basename to the extension
        if (ext) ext[-1] = '.';

        snprintf(image_dest, NAME_MAX, "%s/%i.%s",
                 db_path, image_id, ext);
        // Copy file and handle file errors.
        switch (cp(image_dest, path)) {
        case -1:
            if (tagmage_delete_image(image_id) < 0)
                tagmage_err(1);
            err(1, "%s", image_dest);
        case -2:
            if (tagmage_delete_image(image_id) < 0)
                tagmage_err(1);
            err(1, "%s", path);
        }

        printf("%i\n", image_id);

        // Add each tag to the image
        for(size_t ti = 0; ti < num_tags; ti++) {
            if (tagmage_add_tag(image_id, tags[ti]) < 0)
                tagmage_err(1);
        }
    }
}

static void rm_image(int argc, char **argv)
{
    int id = 0;
    Image img;
    char path[PATH_MAX + 1];

    if (argc == 1) {
        fprintf(stderr, "Missing file operand.\n");
        print_usage(1);
    }

    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &id) != 1)
            errx(1, "Unknown number '%s'.", argv[i]);

        if (tagmage_get_image(id, &img) < 0)
            tagmage_err(1);

        snprintf(path, PATH_MAX, "%s/%i.%s", db_path, id, img.ext);
        int status = remove(path);
        if (status != 0) {
            // I/O Error
            warn("%s", path);
            if (status != ENOENT)
                exit(1);

            // File doesn't exist; we can recover
            fprintf(stderr, "Removing reference anyway...\n");
        }

        if (tagmage_delete_image(id) < 0) {
            tagmage_err(1);
        }
    }
}

static void edit_image(int argc, char **argv)
{
    int id = 0;
    char *newtitle = NULL;

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        print_usage(1);
    }

    if (sscanf(argv[1], "%i", &id) != 1)
        errx(1, "'%s' is not a valid number.", argv[1]);

    newtitle = argv[2];

    if (tagmage_edit_title(id, newtitle) < 0)
        tagmage_err(1);
}

int main(int argc, char **argv)
{
    int opt;

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
            print_usage(1);
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

    if (tagmage_setup(db_file) < 0)
        tagmage_err(1);

    // Shift argc, argv to subcommands
    argc -= optind;
    argv += optind;
    optind = 1; // Reset getopt


    // Sub-Commands
    if (argc == 0 || STREQ(argv[0], "help")) {
        // Default command; also runs if no other arguments exist
        print_usage(0);
    } else if (STREQ(argv[0], "list")) {
        list_images(argc, argv);
    } else if (STREQ(argv[0], "path")) {
        // Give arguments to print_path for everythign past "path"
        print_path(argc, argv);
    } else if (STREQ(argv[0], "add")) {
        // Give arguments to add_image for everything past "add".
        add_image(argc, argv);
    } else if (STREQ(argv[0], "rm")) {
        rm_image(argc, argv);
    } else if (STREQ(argv[0], "edit")) {
        edit_image(argc, argv);
    } else {
        // Unknown command
        fprintf(stderr, "Unknown command '%s'\n", argv[0]);
        print_usage(1);
    }

    if (tagmage_cleanup() < 0)
        tagmage_err(1);
    return 0;
}
