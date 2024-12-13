#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

size_t file_size        (FILE   *file);

bool   is_equal         (double  first,
                         double  second);

size_t get_random_index (size_t  size);

#endif

