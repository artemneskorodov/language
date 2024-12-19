#include <math.h>

//===========================================================================//

#include "language.h"
#include "simplify_rules.h"
#include "nodes_dsl.h"
#include "custom_assert.h"

//===========================================================================//

language_error_t simplify_add(language_t *ctx, language_node_t **node) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(*node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 0)) {
        *node = (*node)->right;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 0)) {
        *node = (*node)->left;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t simplify_sub(language_t *ctx, language_node_t **node) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(*node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->right, 0)) {
        *node = (*node)->left;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t simplify_mul(language_t *ctx, language_node_t **node) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(*node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 0) || is_number_eq((*node)->right, 0)) {
        _RETURN_IF_ERROR(set_val(*node, NODE_TYPE_NUMBER, NUMBER(0), NULL, NULL));
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 1)) {
        *node = (*node)->right;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->right, 1)) {
        *node = (*node)->left;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t simplify_div(language_t *ctx, language_node_t **node) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(*node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 0)) {
        _RETURN_IF_ERROR(set_val(*node, NODE_TYPE_NUMBER, NUMBER(0), NULL, NULL));
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->right, 1)) {
        *node = (*node)->left;
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t simplify_pow(language_t *ctx, language_node_t **node) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(*node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 0) && !is_number_eq((*node)->right, 0)) {
        _RETURN_IF_ERROR(set_val(*node, NODE_TYPE_NUMBER, NUMBER(0), NULL, NULL));
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->right, 0)) {
        _RETURN_IF_ERROR(set_val(*node, NODE_TYPE_NUMBER, NUMBER(1), NULL, NULL));
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_number_eq((*node)->left, 1)) {
        _RETURN_IF_ERROR(set_val(*node, NODE_TYPE_NUMBER, NUMBER(1), NULL, NULL));
        ctx->middleend_info.changes_counter++;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
