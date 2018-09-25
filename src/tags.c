#include <ctype.h>
#include <string.h>
#include "database.h"
#include "tags.h"
#include "util.h"

typedef int (*file_filter)(const TMFile*, const char*);

typedef struct PseudoTag {
    char prefix;
    file_filter filter;
} PseudoTag;

static const PseudoTag pseudotags[] = {
    // Lovely thing abotu `find_filter` is that you can put in a NULL
    // function pointer here and it still works as intended.
    {':', NULL}
};

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

// Returns 1 if the tag is valid, or 0 otherwise. If must_be_real is
// truthy, the tag can *not* be a pseudotag.
int tagmage_is_valid_tag(const char *tag, int must_be_real)
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

// Returns true if the file contains every tag and pseudotag. Contains
// undefined behavior if not all tags are valid.
int tagmage_file_has_tags(const TMFile *file, const TagVector *tags)
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
            passes_filter = tagmage_has_tag(file->id, tags->tags[i]);

        // Exit early if the current filter doesnt pass.
        if (!passes_filter)
            return 0;
    }

    // File passed through every filter.
    return 1;
}
