#include "language.h"
#include "nodes_dsl.h"
#include "custom_assert.h"

//===========================================================================//

bool is_node_type_eq(language_node_t *node, node_type_t type) {
    _C_ASSERT(node != NULL, return false);
    //-----------------------------------------------------------------------//
    if(node->type == type) {
        return true;
    }
    return false;
}

//===========================================================================//

bool is_node_oper_eq(language_node_t *node, operation_t opcode) {
    _C_ASSERT(node != NULL, return false);
    //-----------------------------------------------------------------------//
    if(node->type == NODE_TYPE_OPERATION && node->value.opcode == opcode) {
        return true;
    }
    return false;
}

//===========================================================================//

bool is_ident_type(language_t       *ctx,
                   language_node_t  *node,
                   identifier_type_t type) {
    _C_ASSERT(ctx  != NULL, return false);
    _C_ASSERT(node != NULL, return false);
    //-----------------------------------------------------------------------//
    if(node->type == NODE_TYPE_IDENTIFIER &&
       ctx->name_table.identifiers[node->value.identifier].type == type) {
        return true;
    }
    return false;
}

//===========================================================================//
