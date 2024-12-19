#ifndef FRONTSTART_H
#define FRONTSTART_H

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t frontstart_ctor    (language_t *ctx,
                                     int         argc,
                                     const char *argv[]);

language_error_t frontstart_dtor    (language_t *ctx);

language_error_t frontstart_write   (language_t *ctx);

//===========================================================================//

#endif
