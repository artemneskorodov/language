#ifndef NODES_DSL_H
#define NODES_DSL_H

//===========================================================================//

#include "language.h"

//===========================================================================//

bool is_node_type_eq    (language_node_t   *node,
                         node_type_t        type);

bool is_node_oper_eq    (language_node_t   *node,
                         operation_t        opcode);

bool is_ident_type      (language_t        *language,
                         language_node_t   *node,
                         identifier_type_t  type);

//===========================================================================//

#endif
