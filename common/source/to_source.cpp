#include <stdio.h>
#include <stdarg.h>

#include "language.h"
#include "to_source.h"
#include "nodes_dsl.h"
#include "colors.h"

static bool is_bigger_priority(language_node_t *first, language_node_t *second);
bool is_leaf(language_node_t *node);
language_error_t to_source_ident(language_t *ctx, language_node_t *node);
language_error_t write_source(language_t *ctx, const char *format, ...);

language_error_t to_source_subtree(language_t *ctx, language_node_t *node) {
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(to_source_ident(ctx, node));
            break;
        }
        case NODE_TYPE_NUMBER: {
            _RETURN_IF_ERROR(write_source(ctx, "%lg", node->value.number));
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(KeyWords[node->value.opcode].to_source(ctx, node));
            break;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_ident(language_t *ctx, language_node_t *node) {
    identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
    switch(ident->type) {
        case IDENTIFIER_FUNCTION: {
            _RETURN_IF_ERROR(write_source(ctx, "%.*s(", (int)ident->length, ident->name));
            if(node->right != NULL) {
                _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
            }
            _RETURN_IF_ERROR(write_source(ctx, ")"));
            if(node->left != NULL) {
                _RETURN_IF_ERROR(write_source(ctx, " {\r\n"));
                _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
                _RETURN_IF_ERROR(write_source(ctx, "}"));
            }
            break;
        }
        case IDENTIFIER_GLOBAL_VAR:
        case IDENTIFIER_LOCAL_VAR: {
            _RETURN_IF_ERROR(write_source(ctx, "%.*s", (int)ident->length, ident->name));
            break;
        }
        default: {
            print_error("Unknown identifier type.\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_math_op(language_t *ctx, language_node_t *node) {
    if(is_node_oper_eq(node, OPERATION_ASSIGNMENT)) {
        _RETURN_IF_ERROR(write_source(ctx, "%*s", ctx->frontstart_info.depth * 8, ""));
    }

    bool branches_left = is_bigger_priority(node, node->left);
    if(branches_left) {
        _RETURN_IF_ERROR(write_source(ctx, "("));
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    if(branches_left) {
        _RETURN_IF_ERROR(write_source(ctx, ")"));
    }

    _RETURN_IF_ERROR(write_source(ctx, " %s ", KeyWords[node->value.opcode].name));

    bool branches_right = is_bigger_priority(node, node->right);
    if(branches_right) {
        _RETURN_IF_ERROR(write_source(ctx, "("));
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    if(branches_right) {
        _RETURN_IF_ERROR(write_source(ctx, ")"));
    }

    if(is_node_oper_eq(node, OPERATION_ASSIGNMENT)) {
        _RETURN_IF_ERROR(write_source(ctx, ";"));
    }
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_math_func(language_t *ctx, language_node_t *node) {
    fprintf(ctx->frontstart_info.output, "%s", KeyWords[node->value.opcode].name);
    bool branches = true;
    if(is_leaf(node->right)) {
        branches = false;
    }

    if(branches) {
        _RETURN_IF_ERROR(write_source(ctx, "("));
    }
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    if(branches) {
        _RETURN_IF_ERROR(write_source(ctx, ")"));
    }
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_statements_line(language_t *ctx, language_node_t *node) {
    ctx->frontstart_info.depth++;
    while(node != NULL) {
        _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
        _RETURN_IF_ERROR(write_source(ctx, "\r\n"));
        node = node->right;
    }
    ctx->frontstart_info.depth--;
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_if(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s(", (int)ctx->frontstart_info.depth * 8, "", KeyWords[OPERATION_IF].name));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _RETURN_IF_ERROR(write_source(ctx, ") {\r\n"));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    _RETURN_IF_ERROR(write_source(ctx, "%*s}", (int)ctx->frontstart_info.depth * 8, ""));
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_while(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s(", (int)ctx->frontstart_info.depth * 8, "", KeyWords[OPERATION_WHILE].name));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
    _RETURN_IF_ERROR(write_source(ctx, ") {\r\n"));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    _RETURN_IF_ERROR(write_source(ctx, "%*s}", (int)ctx->frontstart_info.depth * 8, ""));
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_return(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s ", (int)ctx->frontstart_info.depth * 8, "", KeyWords[OPERATION_RETURN].name));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    _RETURN_IF_ERROR(write_source(ctx, ";"));
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_params_line(language_t *ctx, language_node_t *node) {
    while(node != NULL) {
        if(node->left != NULL) {
            _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
        }
        if(node->right != NULL) {
            _RETURN_IF_ERROR(write_source(ctx, ", "));
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_new_var(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s ", (int)ctx->frontstart_info.depth * 8, "", KeyWords[OPERATION_NEW_VAR].name));
    size_t old_depth = ctx->frontstart_info.depth;
    ctx->frontstart_info.depth = 0;
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    ctx->frontstart_info.depth = old_depth;
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_new_func(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%s ", KeyWords[OPERATION_NEW_FUNC].name));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_in(language_t *ctx, language_node_t */*node*/) {
    _RETURN_IF_ERROR(write_source(ctx, "%s", KeyWords[OPERATION_IN].name))
    return LANGUAGE_SUCCESS;
}

language_error_t to_source_out(language_t *ctx, language_node_t *node) {
    _RETURN_IF_ERROR(write_source(ctx, "%*s%s(", (int)ctx->frontstart_info.depth * 8, "", KeyWords[OPERATION_OUT].name));
    _RETURN_IF_ERROR(to_source_subtree(ctx, node->right));
    _RETURN_IF_ERROR(write_source(ctx, ");"));
    return LANGUAGE_SUCCESS;
}

bool is_bigger_priority(language_node_t *node, language_node_t *child) {
    if(child->type == NODE_TYPE_IDENTIFIER || child->type == NODE_TYPE_NUMBER) {
        return false;
    }

    if(node->type == NODE_TYPE_OPERATION && child->type == NODE_TYPE_OPERATION) {
        if(KeyWords[node->value.opcode].priority >= KeyWords[child->value.opcode].priority) {
            return false;
        }
    }
    return true;
}

bool is_leaf(language_node_t *node) {
    if(node->left == NULL && node->right == NULL) {
        return true;
    }
    return false;
}

language_error_t write_source(language_t *ctx, const char *format, ...) {
    va_list args;
    va_start(args, format);
    if(vfprintf(ctx->frontstart_info.output, format, args) < 0) {
        return LANGUAGE_SOURCE_WRITING_ERROR;
    }
    fflush(ctx->frontstart_info.output);
    va_end(args);
    return LANGUAGE_SUCCESS;
}
