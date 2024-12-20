#include <stdio.h>
#include <stdarg.h>

//===========================================================================//

#include "language.h"
#include "to_source.h"
#include "nodes_dsl.h"
#include "colors.h"
#include "custom_assert.h"

//===========================================================================//

static bool             is_bigger_priority  (language_node_t   *node,
                                             language_node_t   *child);

static bool             is_leaf             (language_node_t   *node);

static language_error_t to_source_ident     (language_t        *ctx,
                                             language_node_t   *node);

static language_error_t write_source        (language_t        *ctx,
                                             const char        *format, ...);

//===========================================================================//

#define _WRITE_SRC(_format, ...) _RETURN_IF_ERROR(write_source(ctx, (_format) ,##__VA_ARGS__));

//===========================================================================//

language_error_t to_source_subtree(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(to_source_ident(ctx, node));
            break;
        }
        case NODE_TYPE_NUMBER: {
            _WRITE_SRC("%lg", node->value.number);
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(KeyWords[node->value.opcode].to_source(ctx, node));
            break;
        }
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_ident(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
    switch(ident->type) {
        case IDENTIFIER_FUNCTION: {
            _WRITE_SRC("%.*s(", (int)ident->length, ident->name);
            if(node->left != NULL) {
                _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
            }
            _WRITE_SRC(")");
            if(node->right != NULL) {
                _WRITE_SRC(" {\r\n");
                _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
                _WRITE_SRC("}");
            }
            break;
        }
        case IDENTIFIER_VARIABLE: {
            _WRITE_SRC("%.*s", (int)ident->length, ident->name);
            break;
        }
        default: {
            print_error("Unknown identifier type.\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_math_op(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_node_oper_eq(node, OPERATION_ASSIGNMENT)) {
        _RETURN_IF_ERROR(write_source(ctx, "%*s", ctx->frontstart_info.depth * 8, ""));
    }
    //-----------------------------------------------------------------------//
    bool branches_left = is_bigger_priority(node, node->left);
    if(branches_left) {
        _WRITE_SRC("(");
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    if(branches_left) {
        _WRITE_SRC(")");
    }
    //-----------------------------------------------------------------------//
    _WRITE_SRC(" %s ", KeyWords[node->value.opcode].name);
    //-----------------------------------------------------------------------//
    bool branches_right = is_bigger_priority(node, node->right);
    if(branches_right) {
        _WRITE_SRC("(");
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    if(branches_right) {
        _WRITE_SRC(")");
    }
    //-----------------------------------------------------------------------//
    if(is_node_oper_eq(node, OPERATION_ASSIGNMENT)) {
        _WRITE_SRC(";");
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_math_func(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%s", KeyWords[node->value.opcode].name);
    bool branches = true;
    if(is_leaf(node->left)) {
        branches = false;
    }

    if(branches) {
        _WRITE_SRC("(");
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    if(branches) {
        _WRITE_SRC(")");
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_statements_line(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    ctx->frontstart_info.depth++;
    while(node != NULL) {
        _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
        _WRITE_SRC("\r\n");
        node = node->right;
    }
    ctx->frontstart_info.depth--;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_if(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%*s%s(",
               ctx->frontstart_info.depth * 8, "",
               KeyWords[OPERATION_IF].name);

    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _WRITE_SRC(") {\r\n");
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));

    _WRITE_SRC("%*s}",
               ctx->frontstart_info.depth * 8, "");
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_while(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%*s%s(",
               ctx->frontstart_info.depth * 8, "",
               KeyWords[OPERATION_WHILE].name);

    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _WRITE_SRC(") {\r\n");
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));

    _WRITE_SRC("%*s}",
               ctx->frontstart_info.depth * 8, "");
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_return(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%*s%s ",
               ctx->frontstart_info.depth * 8, "",
               KeyWords[OPERATION_RETURN].name);

    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _WRITE_SRC(";");
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_params_line(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    while(node != NULL) {
        if(node->left != NULL) {
            _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
        }
        if(node->right != NULL) {
            _WRITE_SRC(", ");
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_new_var(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s ",
                                  ctx->frontstart_info.depth * 8, "",
                                  KeyWords[OPERATION_NEW_VAR].name));
    int old_depth = ctx->frontstart_info.depth;
    ctx->frontstart_info.depth = 0;
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    ctx->frontstart_info.depth = old_depth;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_new_func(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%s ", KeyWords[OPERATION_NEW_FUNC].name);
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_in(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%*s%s(",
               ctx->frontstart_info.depth * 8, "",
               KeyWords[OPERATION_IN].name);
    if(node->left == NULL || node->left->left == NULL ||
       !is_ident_type(ctx, node->left->left, IDENTIFIER_VARIABLE)) {
        print_error("Old input format.\n");
        return LANGUAGE_TREE_ERROR;
    }
    identifier_t *ident = ctx->name_table.identifiers +
                          node->left->left->value.identifier;
    _WRITE_SRC("%*s", ident->length, ident->name);
    _WRITE_SRC(")");

    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_out(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _WRITE_SRC("%*s%s(",
               ctx->frontstart_info.depth * 8, "",
               KeyWords[OPERATION_OUT].name);
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _WRITE_SRC(");");
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t to_source_call(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

bool is_bigger_priority(language_node_t *node, language_node_t *child) {
    _C_ASSERT(node  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(child != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(child->type == NODE_TYPE_IDENTIFIER || child->type == NODE_TYPE_NUMBER) {
        return false;
    }
    //-----------------------------------------------------------------------//
    if(node->type == NODE_TYPE_OPERATION && child->type == NODE_TYPE_OPERATION) {
        size_t priority_node  = KeyWords[node->value.opcode].priority;
        size_t priority_child = KeyWords[node->value.opcode].priority;

        if(priority_node >= priority_child) {
            return false;
        }
    }
    //-----------------------------------------------------------------------//
    return true;
}

//===========================================================================//

bool is_leaf(language_node_t *node) {
    if(node->left == NULL && node->right == NULL) {
        return true;
    }
    return false;
}

//===========================================================================//

language_error_t write_source(language_t *ctx, const char *format, ...) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(format != NULL, return LANGUAGE_STRING_FORMAT_NULL);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    va_list args;
    va_start(args, format);
    if(vfprintf(ctx->frontstart_info.output, format, args) < 0) {
        return LANGUAGE_SOURCE_WRITING_ERROR;
    }
    fflush(ctx->frontstart_info.output);
    va_end(args);
#pragma clang diagnostic pop
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
