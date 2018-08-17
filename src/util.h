#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h> // mode_t


// More understandable than !strcmp.
#define STREQ(S1, S2) (!strcmp((S1),(S2)))

// Prevents "unused parameter" compiler warning.
#define UNUSED(X) ((void)(X))


// Create a directories and all its parents
int mkpath(const char *path, mode_t mode);

// Copy a file from one location to the other
int cp(const char *dst, const char *src);

#endif /* UTIL_H */
