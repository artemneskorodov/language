#ifndef BACKEND_H
#define BACKEND_H

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t backend_ctor   (language_t    *language,
                                 int            argc,
                                 const char    *argv[]);

language_error_t compile_code   (language_t    *language);

language_error_t backend_dtor   (language_t    *language);

//===========================================================================//

#endif
