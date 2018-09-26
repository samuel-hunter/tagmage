#include <bsd/string.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "tags.h"
#include "util.h"

#define ERRCHECK(EXPR) ((EXPR) > 0 ? 1 : 0)

typedef int (*file_filter)(const TMFile *, const char *);

typedef struct PseudoTag {
    char prefix;
    file_filter filter;
} PseudoTag;

static int bool_flag(const TMFile *, const char *);
static int invert_tag(const TMFile *, const char *);

static const PseudoTag pseudotags[] = {
    {':', &bool_flag},
    {'!', &invert_tag}
};

static char err_buf[BUFF_MAX] = {0};

static int check_tmdb(int status)
{
    if (status < 0)
        strlcpy(err_buf, tmdb_get_error(), sizeof(err_buf));
    return status;
}

// Generic flag with no other arguments.
static int bool_flag(const TMFile *file, const char *flag)
{
    if (STREQ(flag, "tagged")) {
        return check_tmdb(tmdb_has_tags(file->id));
    } else if (STREQ(flag, "untagged")) {
        int status = check_tmdb(tmdb_has_tags(file->id));
        if (status < 0)
            return status;
        else
            return !status;
    } else {
        snprintf(err_buf, sizeof(err_buf), "'%s' isn't a valid flag!", flag);
        return -1;
    }

    return 0;
}

// Return true if the file does *not* have the tag.
static int invert_tag(const TMFile *file, const char *flag)
{
    int status = check_tmdb(tmdb_has_tag(file->id, flag));
    if (status < 0)
        return status;
    else
        return !status;
}

// Looks up the prefix and returns a file filter if found. Otherwise,
// returns NULL.
static file_filter find_filter(char c)
{
    for (size_t i = 0; i < sizeof(pseudotags); i++) {
        if (pseudotags[i].prefix == c)
            return pseudotags[i].filter;
    }

    return NULL;
}

const char *tmtag_get_err()
{
    return err_buf;
}

int tmtag_is_valid(const char *tag, int must_be_real)
{
    // Empty tags are invalid.
    if (strlen(tag) == 0)
        return 0;

    if (!isalnum(tag[0])) {
        // The starting character must be alphanumeric.
        if (must_be_real)
            return 0;

        // A pseudotag that does not start with an alphanumeric
        // character must start with a pseudotag prefix character
        // instead.
        if (!find_filter(tag[0]))
            return 0;

        // A pseudotag must also be at least 2 characters, i.e. a
        // pseudotag can't only be the prefix character.
        if (strlen(tag) == 1)
            return 0;
    }

    // Any remaining characters must *not* be whitespace.
    for (size_t i = 1; tag[i] != '\0'; i++) {
        if (isspace(tag[i]))
            return 0;
    }

    // The tag passed through all the checks; it's a valid tag.
    return 1;
}

int tmtag_file_has_tags(const TMFile *file, const TagVector *tags)
{
    file_filter filter = NULL;
    int passes_filter = 0;

    for (int i = 0; i < tags->size; i++) {
        // Check if the tag is a special filter.
        filter = find_filter(tags->tags[i][0]);


        if (filter)
            // Go through the special filter to check if the image.
            // Add +1 to the tag name so that it doesn't catch the
            // prefix.
            passes_filter = filter(file, tags->tags[i]+1);
        else
            // It's a real tag; check if the image has it in the
            // database.
            passes_filter = check_tmdb(tmdb_has_tag(file->id, tags->tags[i]));

        // Exit early if the current filter doesnt pass or errs.
        if (passes_filter <= 0)
            return passes_filter;
    }

    // File passed through every filter.
    return 1;
}
