#include <math.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "middleend.h"
#include "name_table.h"
#include "lang_dump.h"
#include "colors.h"
#include "nodes_dsl.h"
#include "custom_assert.h"

//===========================================================================//

static language_error_t constant_folding    (language_t        *ctx,
                                             language_node_t   *node,
                                             double            *result);

static language_error_t simplify_neutrals   (language_t        *ctx,
                                             language_node_t  **node);

static language_error_t constant_folding_op (language_t        *ctx,
                                             language_node_t   *node);

static double           run_operation       (operation_t        opcode,
                                             double             left,
                                             double             right);

//===========================================================================//

language_error_t middleend_ctor(language_t *ctx,
                                int         argc,
                                const char *argv[]) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL  );
    _C_ASSERT(argv != NULL, return LANGUAGE_INPUT_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    _RETURN_IF_ERROR(read_tree(ctx));
    _RETURN_IF_ERROR(dump_ctor(ctx, "middleend"));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t optimize_tree(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    while(true) {
        _RETURN_IF_ERROR(constant_folding(ctx, ctx->root, NULL));
        _RETURN_IF_ERROR(simplify_neutrals(ctx, &ctx->root));

        if(ctx->middleend_info.changes_counter == 0) {
            break;
        }
        ctx->middleend_info.changes_counter = 0;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t middleend_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(nodes_storage_dtor(ctx));
    _RETURN_IF_ERROR(name_table_dtor(ctx));
    _RETURN_IF_ERROR(dump_dtor(ctx));
    free(ctx->input);
    memset(ctx, 0, sizeof(*ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t constant_folding(language_t      *ctx,
                                  language_node_t *node,
                                  double          *result) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(node == NULL) {
        if(result != NULL) {
            *result = 0;
        }
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(constant_folding(ctx, node->left, NULL));
            _RETURN_IF_ERROR(constant_folding(ctx, node->right, NULL))
            break;
        }
        case NODE_TYPE_NUMBER: {
            *result = node->value.number;
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(constant_folding_op(ctx, node));
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t constant_folding_op(language_t      *ctx,
                                     language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    double val_left  = NAN;
    double val_right = NAN;
    _RETURN_IF_ERROR(constant_folding(ctx, node->left, &val_left));
    _RETURN_IF_ERROR(constant_folding(ctx, node->right, &val_right));
    //-----------------------------------------------------------------------//
    if(isnan(val_left) || isnan(val_right)) {
        return LANGUAGE_SUCCESS;
    }
    double value = run_operation(node->value.opcode, val_left, val_right);
    if(isinf(value)) {
        return LANGUAGE_SUCCESS;
    }
    _RETURN_IF_ERROR(set_val(node,
                             NODE_TYPE_NUMBER,
                             NUMBER(value),
                             NULL, NULL));
    ctx->middleend_info.changes_counter++;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t simplify_neutrals(language_t       *ctx,
                                   language_node_t **node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(*node == NULL) {
        return LANGUAGE_SUCCESS;
    }
    if((*node)->type == NODE_TYPE_OPERATION) {
        operation_t opcode = (*node)->value.opcode;
        if(KeyWords[opcode].simplifier != NULL) {
            _RETURN_IF_ERROR(KeyWords[opcode].simplifier(ctx, node));
        }
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(simplify_neutrals(ctx, &(*node)->left));
    _RETURN_IF_ERROR(simplify_neutrals(ctx, &(*node)->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

double run_operation(operation_t opcode, double left, double right) {
    if(opcode == OPERATION_ADD) {
        return left + right;
    }
    if(opcode == OPERATION_SUB) {
        return left - right;
    }
    if(opcode == OPERATION_MUL) {
        return left * right;
    }
    if(opcode == OPERATION_DIV) {
        return left / right;
    }
    if(opcode == OPERATION_POW) {
        return pow(left, right);
    }
    if(opcode == OPERATION_COS) {
        return cos(right);
    }
    if(opcode == OPERATION_SIN) {
        return sin(right);
    }
    if(opcode == OPERATION_BIGGER) {
        if(left > right) {
            return 1;
        }
        return 0;
    }
    if(opcode == OPERATION_SMALLER) {
        if(left < right) {
            return 0;
        }
        return 1;
    }

    return INFINITY;
}

//===========================================================================//
