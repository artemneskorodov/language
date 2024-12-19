#ifndef FRONTEND_UTILS_H
#define FRONTEND_UTILS_H

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t move_next_symbol   (language_t        *ctx);

char             current_symbol     (language_t        *ctx);

const char      *input_position     (language_t        *ctx);

language_error_t move_next_token    (language_t        *ctx);

language_node_t *token_position     (language_t        *ctx);

bool             is_on_operation    (language_t        *ctx,
                                     operation_t        opcode);

language_error_t syntax_error       (language_t        *ctx,
                                     const char        *format, ...);

bool             is_on_ident_type   (language_t        *ctx,
                                     identifier_type_t  type);

bool             is_on_type         (language_t        *ctx,
                                     node_type_t        type);

//===========================================================================//

#endif
