#ifndef FRONTEND_H
#define FRONTEND_H

#include "language.h"

language_error_t frontend_ctor(language_t *language, int argc, const char *argv[]);
language_error_t parse_tokens(language_t *language);
language_error_t parse_syntax(language_t *language);
language_error_t frontend_dtor(language_t *language);

#endif
