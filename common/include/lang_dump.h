#ifndef LANG_DUMP_H
#define LANG_DUMP_H

//===========================================================================//

#include <stdarg.h>

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t dump_ctor  (language_t    *ctx,
                             const char    *filename);

language_error_t dump_tree  (language_t    *ctx,
                             const char    *format, ...);

language_error_t dump_dtor  (language_t    *ctx);

//===========================================================================//

#endif
