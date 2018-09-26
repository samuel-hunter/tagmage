#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <bsd/string.h>
#include <bsd/inttypes.h>

#include "core.h"
#include "database.h"
#include "util.h"
#include "tags.h"

#define APP_NAME "tagmage"

#define TAGMAGE_ASSERT(EXPR) \
    if ((EXPR) < 0)          \
        tmdb_error();

#define INCOPT()                               \
    if (++optind >= argc)                      \
        errx(1, "Missing operand after '%s'.", \
             argv[optind-1])


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

static void esnprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    if ((size_t)vsnprintf(str, size, format, ap) >= size)
        errx(1, "snprintf: string truncated: %s", str);
}

static int estrtoid(const char *str)
{
    int rstatus = 0, id = 0;

    id = (int) strtoi(str, NULL, 0, 1, INT_MAX, &rstatus);
    if (rstatus)
        errx(1, "Failed to convert to id: '%s'.", str);

    return id;
}

static void print_usage(int status)
{
    fprintf(status ? stderr : stdout,
            "Usage: tagmage [ -f PATH ] COMMAND [ ... ]\n"
            "\n"
            "  -f SAVE  - Set custom save directory.\n"
            "\n"
            "  add [-t TAG1 TAG2 ... +] FILES..\n"
            "  edit FILE TITLE\n"
            "  list [TAGS..]\n"
            "  tag FILE [TAGS..]\n"
            "  untag FILE [TAGS..]\n"
            "  tags FILE\n"
            "  path [FILES..]\n"
            "  rm FILES..\n"
            "\n"
            "Visit `man 1 tagmage` for more details.\n");

    exit(status);
}

static int print_tag(const char *tag)
{
    printf("%s\n", tag);
    return 0;
}

static int print_file_filtered(const TMFile *file, void *vecptr)
{
    TagVector *tagvec = vecptr;
    int has_tags = tmtag_file_has_tags(file, tagvec);

    if (has_tags < 0)
        errx(1, "%s", tmtag_get_err());
    else if (has_tags)
        printf("%i %s\n", file->id, file->title);

    return 0;
}

static void list_files(int argc, char **argv)
{
    // All remaining arguments should be tags.
    TagVector args = {.size = argc - 1, .tags = argv + 1};

    // Sanity check on all the tags before starting.
    for (int i = 1; i < argc; i++) {
        if (!tmtag_is_valid(argv[i], 0))
            errx(1, "Invalid tag '%s'.", argv[i]);
    }

    TAGMAGE_ASSERT(tmdb_get_files(&print_file_filtered, &args));
}

static void print_path(int argc, char **argv)
{
    TMFile img;
    int item_id = 0;

    if (argc == 1) {
        // print Database path if no file id provided
        printf("%s\n", db_path);
        return;
    }

    // Each subsequent argument is an file id
    for (int i = 1; i < argc; i++) {
        item_id = estrtoid(argv[i]);

        TAGMAGE_ASSERT(tmdb_get_file(item_id, &img));

        printf("%s/%i\n", db_path, img.id);
    }
}

static void add_file(int argc, char **argv)
{
    char *path = NULL, *basename = NULL;
    char file_dest[NAME_MAX + 1] = {'\0'};
    int file_id = 0;
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
                // Double-check the tag is valid.
                if (!tmtag_is_valid(argv[optind], 1)) {
                    errx(1, "Invalid tag '%s'.",
                         argv[optind]);
                }

                tags[num_tags] = argv[optind];

                // Double-check there isn't too many tags.
                if (++num_tags == BUFF_MAX) {
                    errx(1, "Too many tags after '%s'.",
                         argv[optind-1]);
                }
                INCOPT();
            }
            break;
        default:
            errx(1, "Unexpected argument '%s'.",
                 argv[optind]);
        }
    }
 optbreak:

    // Expect at least one file.
    if (optind == argc)
        errx(1, "Missing file operand.");


    for (int i = optind; i < argc; i++) {
        // Add file
        path = argv[i];

        // Search for the basename.
        basename = strrchr(path, '/');
        if (basename == NULL)
            basename = path;
        else
            basename++;

        // Create an file in the database early to grab the ID.
        TAGMAGE_ASSERT(file_id =
                       tmdb_new_file(basename));

        esnprintf(file_dest, NAME_MAX, "%s/%i",
                  db_path, file_id);

        // Copy file and handle file errors.
        switch (cp(file_dest, path)) {
        case -1: // Couldn't open the destination file.

            // Attempt to remove file from the database; don't
            // error-check, since we're already failing.
            tmdb_delete_file(file_id);
            err(1, "%s", file_dest);
        case -2: // Couldn't open the source file.

            // Same as above.
            tmdb_delete_file(file_id);
            err(1, "%s", path);
        }

        // Print ID of new file.
        printf("%i\n", file_id);

        // Add each tag to the new file.
        for(size_t ti = 0; ti < num_tags; ti++) {
            TAGMAGE_ASSERT(tmdb_add_tag(file_id, tags[ti]));
        }
    }
}

static void rm_file(int argc, char **argv)
{
    int id = 0;
    TMFile img;
    char path[PATH_MAX + 1];

    if (argc == 1) {
        warnx("Missing file operand.\n");
        print_usage(1);
    }

    for (int i = 1; i < argc; i++) {
        id = estrtoid(argv[i]);
        TAGMAGE_ASSERT(tmdb_get_file(id, &img));

        esnprintf(path, PATH_MAX, "%s/%i", db_path, id);
        int status = remove(path);
        if (status != 0) {
            // I/O Error
            warn("%s", path);

            if (status != ENOENT)
                return;

            // File doesn't exist; we can recover
            fprintf(stderr, "Removing reference from database anyway...\n");
        }

        TAGMAGE_ASSERT(tmdb_delete_file(id));
    }
}

static void edit_file(int argc, char **argv)
{
    int id = 0;

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        print_usage(1);
    }

    id = estrtoid(argv[1]);

    TAGMAGE_ASSERT(tmdb_edit_title(id, argv[2]));
}

static void tag_file(int argc, char **argv)
{
    int file_id = 0;

    if (argc == 1)
        errx(1, "Missing file after '%s'.", argv[0]);

    file_id = estrtoid(argv[1]);

    for (int i = 2; i < argc; i++) {
        if (tmtag_is_valid(argv[i], 1)) {
            TAGMAGE_ASSERT(tmdb_add_tag(file_id, argv[i]));
        } else {
            errx(1, "Invalid tag '%s'.", argv[i]);
        }
    }
}

static void untag_file(int argc, char **argv)
{
    int file_id = 0;

    if (argc == 1)
        errx(1, "Missing file after '%s'.", argv[0]);

    file_id = estrtoid(argv[1]);

    for (int i = 2; i < argc; i++) {
        TAGMAGE_ASSERT(tmdb_remove_tag(file_id, argv[i]));
    }
}

static void list_tags(int argc, char **argv)
{
    int file_id = 0;

    if (argc == 1) {
        TAGMAGE_ASSERT(tmdb_get_tags(&print_tag));
        return;
    }

    file_id = estrtoid(argv[1]);

    TAGMAGE_ASSERT(tmdb_get_tags_by_file(file_id, &print_tag));
}

int main(int argc, char **argv)
{
    int optind = 0;
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
            print_usage(0);
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
            esnprintf(db_path, PATH_MAX,
                     "%s/tagmage", env);
        } else {
            // Use $HOME/.local/share/tagmage as backup default
            esnprintf(db_path, PATH_MAX,
                     "%s/.local/share/tagmage", getenv("HOME"));
        }
    }

    // Create path to database if it's not set up already
    if (mkpath(db_path, 0700) < 0)
        err(1, "db_path");

    // Set up backend database
    estrlcpy(db_file, db_path, sizeof(db_file));
    estrlcat(db_file, "/db.sqlite", sizeof(db_file));

    TAGMAGE_ASSERT(tmdb_setup(db_file));

    // Shift argc, argv to subcommands
    argc -= optind;
    argv += optind;

    // Sub-Commands
    if (argc == 0 || STREQ(argv[0], "help")) {
        print_usage(0);
    } else if (STREQ(argv[0], "list")) {
        list_files(argc, argv);

    } else if (STREQ(argv[0], "path")) {
        print_path(argc, argv);

    } else if (STREQ(argv[0], "add")) {
        add_file(argc, argv);

    } else if (STREQ(argv[0], "rm")) {
        rm_file(argc, argv);

    } else if (STREQ(argv[0], "tag")) {
        tag_file(argc, argv);

    } else if (STREQ(argv[0], "untag")) {
        untag_file(argc, argv);

    } else if (STREQ(argv[0], "tags")) {
        list_tags(argc, argv);

    } else if (STREQ(argv[0], "edit")) {
        edit_file(argc, argv);

    } else {
        // Unknown command
        warnx("Unknown command '%s'\n", argv[0]);
        print_usage(1);
    }

    // Close and clean up database before exiting.
    TAGMAGE_ASSERT(tmdb_cleanup());

    return 0;
}
