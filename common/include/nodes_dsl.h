#ifndef NODES_DSL_H
#define NODES_DSL_H

//===========================================================================//

#include "language.h"

//===========================================================================//

bool             is_node_type_eq    (language_node_t   *node,
                                     node_type_t        type);

bool             is_node_oper_eq    (language_node_t   *node,
                                     operation_t        opcode);

bool             is_ident_type      (language_t        *ctx,
                                     language_node_t   *node,
                                     identifier_type_t  type);

bool             is_number_eq       (language_node_t   *node,
                                     double             value);

language_error_t set_val            (language_node_t   *node,
                                     node_type_t        type,
                                     value_t            value,
                                     language_node_t   *left,
                                     language_node_t   *right);


//===========================================================================//

#endif
