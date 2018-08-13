#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


// Filesystem methods

// Create a directories and all its parents
int mkpath(const char *path, mode_t mode);

// Copy a file from one location to the other
int cp(const char *dst, const char *src);

#endif /* UTIL_H */
