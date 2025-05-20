//===========================================================================//
#ifndef BACKEND_H
#define BACKEND_H
//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t backend_ctor   (language_t    *language,
                                 int            argc,
                                 const char    *argv[]);

language_error_t compile_elf    (language_t    *language);

language_error_t compile_spu    (language_t    *language);

language_error_t compile_nasm   (language_t    *language);

language_error_t backend_dtor   (language_t    *language);

language_error_t add_stdlib_id  (language_t    *language);

long             current_rip    (language_t    *ctx);

//===========================================================================//
#endif
//===========================================================================//
