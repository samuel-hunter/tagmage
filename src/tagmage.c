#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <bsd/string.h>

#include "core.h"
#include "database.h"
#include "util.h"

#define APP_NAME "tagmage"
#define BUFF_MAX 4096


#define TAGMAGE_ASSERT(EXPR) do {               \
        if ((EXPR) < 0) {                       \
            tagmage_warn();                     \
            return -1;                          \
        }} while (0)

#define INCOPT() do {                               \
        if (++optind >= argc) {                     \
            warnx("Missing operand after '%s'.",    \
                  argv[optind-1]);                  \
            return -1;                              \
        }} while (0)

typedef struct TagVector {
    int size;
    char **tags;
} TagVector;


static char db_path[PATH_MAX+1] = {0};


static void estrlcpy(char *dst, const char *src, size_t size)
{
    if (strlcpy(dst, src, size) > size)
        errx(1, "strlcpy: string truncated: %s", src);
}

static void estrlcat(char *dst, const char *src, size_t size)
{
    if (strlcat(dst, src, size) > size)
        errx(1, "strlcat: string truncated: %s", src);
}

static void print_usage(FILE *f)
{
    fprintf(f,
            "Usage: tagmage [ -f PATH ] COMMAND [ ... ]\n"
            "\n"
            "  -f SAVE  - Set custom save directory.\n"
            "\n"
            "  add [-t TAG1 TAG2 ... +] IMAGES..\n"
            "  edit IMAGE TITLE\n"
            "  list [TAGS..]\n"
            "  untagged\n"
            "  tag IMAGE [TAGS..]\n"
            "  untag IMAGE [TAGS..]\n"
            "  tags IMAGE\n"
            "  path [IMAGES..]\n"
            "  rm IMAGES..\n"
            "\n"
            "Visit `man 1 tagmage` for more details.\n");
}

static int print_image(const Image *image, void *arg)
{
    UNUSED(arg);
    printf("%i %s\n", image->id, image->title);
    return 0;
}

static int print_tag(const char *tag)
{
    printf("%s\n", tag);
    return 0;
}

static int print_image_with_tags(const Image *image, void *vecptr)
{
    TagVector *tagvec = vecptr;
    int status;

    // Go through each tag and return early if the image doesn't have
    // all tags.
    for (int i = 0; i < tagvec->size; i++) {
        // Check if image has tag.
        status = tagmage_has_tag(image->id, tagvec->tags[i]);
        switch (status) {
        case -1:
            // Error; report it and exit early.
            tagmage_warn();
            return -1;
        case 0:
            // Tag doesn't exist; break out.
            return 0;
        }
    }

    printf("%i %s\n", image->id, image->title);

    return 0;
}

static int list_images(int argc, char **argv)
{
    // All remaining arguments should be tags.
    TagVector args = {.size = argc - 1, .tags = argv + 1};
    TAGMAGE_ASSERT(tagmage_get_images(&print_image_with_tags, &args));
    return 0;
}

static int list_untagged()
{
    TAGMAGE_ASSERT(tagmage_get_untagged_images(print_image, NULL));
    return 0;
}

static int print_path(int argc, char **argv)
{
    Image img;
    int item_id = 0;

    if (argc == 1) {
        // print Database path if no image id provided
        printf("%s\n", db_path);
        return 0;
    }

    // Each subsequent argument is an image id
    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &item_id) != 1)
            errx(1, "Invalid number '%s'", argv[i]);

        TAGMAGE_ASSERT(tagmage_get_image(item_id, &img));

        printf("%s/%i\n", db_path, img.id);
    }

    return 0;
}

static int add_image(int argc, char **argv)
{
    char *path = NULL, *basename = NULL;
    char image_dest[NAME_MAX + 1] = {'\0'};
    int image_id = 0;
    char *tags[BUFF_MAX] = {NULL};
    size_t num_tags = 0;
    int optind;

    for (optind = 1; optind < argc; optind++) {
        // Non-option reached
        if (argv[optind][0] != '-')
            goto optbreak;

        if (argv[optind][0] == '\0')
            errx(1, "Unexpected empty argument after '%s'.",
                 argv[optind-1]);

        switch (argv[optind][1]) {
        case '-':
            // --  option breaker
            optind++;
            goto optbreak;
        case 't':
            // -t [tag1] [tag2] ... +   supplementary tags
            INCOPT();
            while (!STREQ(argv[optind], "+")) {
                tags[num_tags] = argv[optind];
                if (++num_tags == BUFF_MAX) {
                    warnx("Too many tags after '%s'.",
                          argv[optind-1]);
                    return -1;
                }
                INCOPT();
            }
            break;
        default:
            warnx("Unexpected argument '%s'.",
                 argv[optind]);
            return -1;
        }
    }
 optbreak:

    // Expect at least one file.
    if (optind == argc) {
        warnx("Missing file operand.");
        return -1;
    }


    for (int i = optind; i < argc; i++) {
        // Add image
        path = argv[i];

        // Search for the basename.
        basename = strrchr(path, '/');
        if (basename == NULL)
            basename = path;
        else
            basename++;

        // Create an image in the database early to grab the ID.
        TAGMAGE_ASSERT(image_id =
                       tagmage_new_image(basename));

        snprintf(image_dest, NAME_MAX, "%s/%i",
                 db_path, image_id);

        // Copy file and handle file errors.
        switch (cp(image_dest, path)) {
        case -1: // Couldn't open the destination file.

            // Attempt to remove image from the database; don't
            // error-check, since we're already failing.
            tagmage_delete_image(image_id);
            warn("%s", image_dest);
            return -1;
        case -2: // Couldn't open the source file.

            // Same as above.
            tagmage_delete_image(image_id);
            warn("%s", path);
            return 01;
        }

        // Print ID of new image.
        printf("%i\n", image_id);

        // Add each tag to the new image.
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
        warnx("Missing file operand.\n");
        print_usage(stderr);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (sscanf(argv[i], "%i", &id) != 1)
            errx(1, "Unknown number '%s'.", argv[i]);

        TAGMAGE_ASSERT(tagmage_get_image(id, &img));

        snprintf(path, PATH_MAX, "%s/%i", db_path, id);
        int status = remove(path);
        if (status != 0) {
            // I/O Error
            warn("%s", path);

            if (status != ENOENT)
                return -1;

            // File doesn't exist; we can recover
            fprintf(stderr, "Removing reference from database anyway...\n");
        }

        TAGMAGE_ASSERT(tagmage_delete_image(id));
    }

    return 0;
}

static int edit_image(int argc, char **argv)
{
    int id = 0;

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        print_usage(stderr);
        return -1;
    }

    if (sscanf(argv[1], "%i", &id) != 1)
        errx(1, "'%s' is not a valid number.", argv[1]);

    TAGMAGE_ASSERT(tagmage_edit_title(id, argv[2]));

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
        TAGMAGE_ASSERT(tagmage_get_tags(&print_tag));
        return 0;
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
    int optind = 0, status = 0;
    char *env = NULL;
    char db_file[PATH_MAX + 1] = {0};

    for (optind = 1; optind < argc; optind++) {
        // Non-option reached
        if (argv[optind][0] != '-')
            // Non-option reached; exit loop.
            goto optbreak;

        if (argv[optind][0] == '\0')
            errx(1, "Unexpected empty argument after '%s'.",
                 argv[optind-1]);

        switch (argv[optind][1]) {
        case '-':
            // --  option breaker
            optind++;
            goto optbreak;
        case 'h':
            // -h  help
            print_usage(stdout);
            return 0;
        case 'f':
            // Set custom database directory
            INCOPT(); // increase optind
            estrlcpy(db_path, argv[optind], sizeof(db_path));
            break;
        default:
            errx(1, "Unexpected argument '%s'.",
                 argv[optind]);
            break;
        }

    }
 optbreak:


    // Set-up db_path if it's not yet initialized
    if (db_path[0] == '\0') {
        if (env = getenv("TAGMAGE_HOME"), env) {
            // Use $TAGMAGE_SAVE as first default
            estrlcpy(db_path, env, sizeof(db_path));
        } else if (env = getenv("XDG_DATA_HOME"), env) {
            // Use $XDG_DATA_HOME/tagmage as default
            snprintf(db_path, PATH_MAX,
                     "%s/tagmage", env);
        } else {
            // Use $HOME/.local/share/tagmage as backup default
            snprintf(db_path, PATH_MAX,
                     "%s/.local/share/tagmage", getenv("HOME"));
        }
    }

    // Create path to database if it's not set up already
    if (mkpath(db_path, 0700) < 0)
        err(1, "db_path");

    // Set up backend database
    estrlcpy(db_file, db_path, sizeof(db_file));
    estrlcat(db_file, "/db.sqlite", sizeof(db_file));

    TAGMAGE_ASSERT(tagmage_setup(db_file));

    // Shift argc, argv to subcommands
    argc -= optind;
    argv += optind;

    // Sub-Commands
    if (argc == 0 || STREQ(argv[0], "help")) {
        print_usage(stdout);
    } else if (STREQ(argv[0], "list")) {
        status = list_images(argc, argv);
    } else if (STREQ(argv[0], "untagged")) {
        status = list_untagged();

    } else if (STREQ(argv[0], "path")) {
        status = print_path(argc, argv);

    } else if (STREQ(argv[0], "add")) {
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
        warnx("Unknown command '%s'\n", argv[0]);
        print_usage(stderr);
        status = -1;
    }

    // Close and clean up database before exiting.
    TAGMAGE_ASSERT(tagmage_cleanup());

    return status;
}
