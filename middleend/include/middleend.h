#ifndef MIDDLEEND_H
#define MIDDLEEND_H

#include "language.h"

language_error_t middleend_ctor(language_t *ctx, int argc, const char *argv[]);
language_error_t optimize_tree(language_t *ctx);
language_error_t middleend_dtor(language_t *ctx);

#endif
