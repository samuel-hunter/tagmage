#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "database.h"
#include "util.h"
#include "tags.h"
#include "libtagmage.h"

#define TAGMAGE_ASSERT(EXPR) \
    if ((EXPR) < 0)          \
        errx(1, "%s", tmdb_get_error());

#define INCOPT()                               \
    if (++optind >= argc)                      \
        errx(1, "Missing operand after '%s'.", \
             argv[optind-1])

static int estrtoid(const char *str)
{
    long id_l = 0;
    int id = 0;

    id_l = (int) strtol(str, NULL, 0);
    if (errno) {
        err(1, "Failed to convert to id: '%s'", str);
    } else if (id_l < 1 || id_l > INT_MAX) {
        errno = ERANGE;
        err(1, "Failed to convert to id: '%s'", str);
    }

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
    char path_buf[PATH_MAX + 1] = {0};

    if (argc == 1) {
        // print Database path if no file id provided
        printf("%s\n", tm_path());
        return;
    }

    // Each subsequent argument is an file id
    for (int i = 1; i < argc; i++) {
        item_id = estrtoid(argv[i]);
        TAGMAGE_ASSERT(tmdb_get_file(item_id, &img));

        if (tm_file_path(&img, path_buf, sizeof(path_buf)) >= sizeof(path_buf)) {
            errno = ENOBUFS;
            err(1, "tm_file_path: %s", tm_get_error());
        }
        puts(path_buf);
    }
}

static void add_file(int argc, char **argv)
{
    char **tags = NULL;
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

            // The '-t' flag can only be used once:
            if (tags != NULL)
                errx(1, "The -t flag can only be used once.");

            // Point to first tag in list.
            tags = argv + optind;

            // Verify that every tag is valid.
            while (!STREQ(argv[optind], "+")) {
                if (!tmtag_is_valid(argv[optind], 1)) {
                    // The tag is invalid.
                    errx(1, "Invalid tag '%s'.",
                         argv[optind]);
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
        // Add file to database.
        TMFile file;
        if (tm_add_file(argv[i], &file) < 0)
            err(1, "tm_add_file: %s", tm_get_error());

        // Print ID of new file.
        printf("%i\n", file.id);

        // Add each tag to the new file.
        if (tags) {
            for(size_t ti = 0; !STREQ(tags[ti], "+"); ti++) {
                TAGMAGE_ASSERT(tmdb_add_tag(file.id, tags[ti]));
            }
        }
    }
}

static void rm_file(int argc, char **argv)
{
    if (argc == 1) {
        warnx("Missing file operand.\n");
        print_usage(1);
    }

    for (int i = 1; i < argc; i++) {
        TMFile file;
        file.id = estrtoid(argv[i]);

        if (tm_rm_file(&file) < 0)
            err(1, "tm_rm_file: %s", tm_get_error());
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
    char *db_path = NULL;

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
            db_path = argv[optind];
            break;
        default:
            errx(1, "Unexpected argument '%s'.",
                 argv[optind]);
            break;
        }

    }
 optbreak:

    if (tm_init(db_path) < 0)
        err(1, "tm_init: %s", tm_get_error());

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
