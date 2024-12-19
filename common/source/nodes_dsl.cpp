#include <math.h>

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

language_error_t set_val(language_node_t   *node,
                         node_type_t        type,
                         value_t            value,
                         language_node_t   *left,
                         language_node_t   *right) {
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    node->type = type;
    node->value = value;
    node->left = left;
    node->right = right;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

bool is_number_eq(language_node_t *node, double value) {
    _C_ASSERT(node != NULL, return false);
    //-----------------------------------------------------------------------//
    double epsilon = 10e-6;
    if(node->type != NODE_TYPE_NUMBER) {
        return false;
    }
    if(fabs(node->value.number - value) < epsilon) {
        return true;
    }
    return false;
}

//===========================================================================//

