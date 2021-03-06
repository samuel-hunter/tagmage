#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

#define MIN(X,Y) ((X)<(Y) ? (X) : (Y))
#define LEN(A) (sizeof(A)/sizeof((A)[0]))

// Limits
#define PATH_MAX 4096
#define NAME_MAX 255

// More understandable than !strcmp.
#define STREQ(S1, S2) (!strcmp((S1),(S2)))

// Prevents "unused parameter" compiler warning.
#define UNUSED(X) ((void)(X))


/**
 * mkpath() - Create a directories and all its parents.
 */
int mkpath(const char *path, mode_t mode);

/**
 * cp() - Copy a file from one location to the other
 */
int cp(const char *dst, const char *src);

#endif // UTIL_H
