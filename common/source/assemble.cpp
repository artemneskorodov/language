#include <stdio.h>
#include <stdarg.h>

//===========================================================================//

#include "language.h"
#include "assemble.h"
#include "utils.h"
#include "colors.h"
#include "nodes_dsl.h"
#include "name_table.h"
#include "custom_assert.h"

//===========================================================================//

static language_error_t compile_function_call (language_t      *ctx,
                                               language_node_t *root);

static language_error_t compile_identifier    (language_t      *ctx,
                                               language_node_t *root,
                                               const char      *command);

static language_error_t write_command         (language_t      *ctx,
                                               const char      *format, ...);

//===========================================================================//

#define _CMD_WRITE(_format, ...) \
    _RETURN_IF_ERROR(write_command((ctx), (_format) ,##__VA_ARGS__))

//===========================================================================//

language_error_t compile_subtree(language_t      *ctx,
                                 language_node_t *node) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(node == NULL) {
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(compile_identifier(ctx, node, "push"));
            break;
        }
        case NODE_TYPE_NUMBER: {
            _CMD_WRITE("push %lg", node->value.number   );
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(KeyWords[node->value.opcode].assemble(ctx, node));
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

language_error_t compile_identifier(language_t      *ctx,
                                    language_node_t *node,
                                    const char      *command) {
    _C_ASSERT(ctx                         != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node                        != NULL, return LANGUAGE_NODE_NULL  );
    _C_ASSERT(ctx->name_table.identifiers != NULL, return LANGUAGE_NT_NOT_INIT);
    //-----------------------------------------------------------------------//
    identifier_t *ident = ctx->name_table.identifiers +
                          node->value.identifier;
    switch(ident->type) {
        case IDENTIFIER_FUNCTION: {
            _RETURN_IF_ERROR(compile_function_call(ctx, node));
            break;
        }
        case IDENTIFIER_VARIABLE: {
            _CMD_WRITE("%s [%s" SZ_SP "] ;%.*s",
                       command,
                       !ident->is_global ? "bx + " : "",
                       ident->memory_addr,
                       (int)ident->length,
                       ident->name);
            break;
        }
        default: {
            print_error("Unknown identifier type.\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_function_call(language_t      *ctx,
                                       language_node_t *node) {
    _C_ASSERT(ctx                         != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node                        != NULL, return LANGUAGE_NODE_NULL  );
    _C_ASSERT(ctx->name_table.identifiers != NULL, return LANGUAGE_NT_NOT_INIT);
    //-----------------------------------------------------------------------//
    if(!is_ident_type(ctx, node, IDENTIFIER_FUNCTION)) {
        print_error("Expected to call compile_function_call() "
                    "only for function ids");
        return LANGUAGE_UNEXPECTED_ID_TYPE;
    }
    identifier_t    *ident = ctx->name_table.identifiers +
                             node->value.identifier;
    language_node_t *param = node->left;
    //-----------------------------------------------------------------------//
    _CMD_WRITE(";saving BX"                    );
    _CMD_WRITE("push bx\r\n"                   );
    //-----------------------------------------------------------------------//
    _CMD_WRITE(";function parameters"          );
    ctx->backend_info.scope++;
    while(param != NULL) {
        _RETURN_IF_ERROR(compile_subtree(ctx, param->left));
        param = param->right;
    }
    ctx->backend_info.scope--;
    _CMD_WRITE("\r\n"                          );
    //-----------------------------------------------------------------------//
    size_t used_locals = ctx->backend_info.used_locals;
    _CMD_WRITE(";incrementing bx"              );
    _CMD_WRITE("push bx"                       );
    _CMD_WRITE("push " SZ_SP, used_locals      );
    _CMD_WRITE("add"                           );
    _CMD_WRITE("pop bx\r\n"                    );
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(write_command(ctx, ";pushing arguments to function"));
    for(size_t i = ident->parameters_number; i > 0; i--) {
        _RETURN_IF_ERROR(write_command(ctx, "pop [bx + " SZ_SP "]", i - 1));
    }
    //-----------------------------------------------------------------------//
    _CMD_WRITE("call %.*s:",
               (int)ident->length,
               ident->name                     );
    //-----------------------------------------------------------------------//
    _CMD_WRITE(";resetting bx"                 );
    _CMD_WRITE("pop bx"                        );
    //-----------------------------------------------------------------------//
    _CMD_WRITE(";pushing return value to stack");
    _CMD_WRITE("push ax\r\n"                   );
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_two_args(language_t      *ctx,
                                   language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_type_eq(node, NODE_TYPE_OPERATION)) {
        return LANGUAGE_UNEXPECTED_NODE_TYPE;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left ));
    _RETURN_IF_ERROR(compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    const char *asm_cmd = KeyWords[node->value.opcode].assembler_command;
    _RETURN_IF_ERROR(write_command(ctx, "%s", asm_cmd));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_one_arg(language_t      *ctx,
                                  language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_type_eq(node, NODE_TYPE_OPERATION)) {
        return LANGUAGE_UNEXPECTED_NODE_TYPE;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    const char *asm_cmd = KeyWords[node->value.opcode].assembler_command;
    _CMD_WRITE("%s", asm_cmd                       );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_comparison(language_t      *ctx,
                                     language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_BIGGER ) &&
       !is_node_oper_eq(node, OPERATION_SMALLER)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left ));
    _RETURN_IF_ERROR(compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    size_t num = ctx->backend_info.used_labels++;
    const char *asm_cmd = KeyWords[node->value.opcode].assembler_command;
    _CMD_WRITE("%s _cmp_t_" SZ_SP ":", asm_cmd, num);
    _CMD_WRITE("push 0"                            );
    _CMD_WRITE("jmp _cmp_t_end_" SZ_SP ":\r\n", num);
    _CMD_WRITE("_cmp_t_" SZ_SP ":", num            );
    _CMD_WRITE("push 1"                            );
    _CMD_WRITE("_cmp_t_end_" SZ_SP ":\r\n", num    );
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_assignment(language_t      *ctx,
                                     language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_ASSIGNMENT)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_identifier(ctx, node->left, "pop"));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_statements_line(language_t      *ctx,
                                          language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    size_t old_used_locals = ctx->backend_info.used_locals;
    while(node != NULL) {
        if(!is_node_oper_eq(node, OPERATION_STATEMENT)) {
            return LANGUAGE_UNEXPECTED_OPER;
        }
        _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
        node = node->right;
    }
    ctx->backend_info.used_locals = old_used_locals;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_if(language_t      *ctx,
                             language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_IF)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    size_t num = ctx->backend_info.used_labels++;
    _CMD_WRITE("push 0"                    );
    _CMD_WRITE("je skip_if_" SZ_SP ":", num);
    //-----------------------------------------------------------------------//
    ctx->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(ctx, node->right));
    ctx->backend_info.scope--;
    //-----------------------------------------------------------------------//
    _CMD_WRITE("skip_if_" SZ_SP ":", num   );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_while(language_t      *ctx,
                                language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_WHILE)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    //-----------------------------------------------------------------------//
    size_t num = ctx->backend_info.used_labels++;
    _CMD_WRITE("_while_start_" SZ_SP ":", num    );
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    _CMD_WRITE("push 0"                          );
    _CMD_WRITE("je _skip_while_" SZ_SP ":", num  );
    //-----------------------------------------------------------------------//
    ctx->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(ctx, node->right));
    ctx->backend_info.scope--;
    //-----------------------------------------------------------------------//
    _CMD_WRITE("jmp _while_start_" SZ_SP ":", num);
    _CMD_WRITE("_skip_while_" SZ_SP ":", num     );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_return(language_t      *ctx,
                                 language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_RETURN)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    _CMD_WRITE("pop ax"                         );
    _CMD_WRITE("ret"                            );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_params_line(language_t      *ctx,
                                      language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    while(node != NULL) {
        if(!is_node_oper_eq(node, OPERATION_PARAM_LINKER)) {
            return LANGUAGE_UNEXPECTED_OPER;
        }
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
        node = node->right;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_new_var(language_t      *ctx,
                                  language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_NEW_VAR)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }

    language_node_t *ident = NULL;
    if      (is_node_oper_eq(node->left, OPERATION_ASSIGNMENT)) {
        ident = node->left->left;
    }
    else if (is_node_type_eq(node->left, NODE_TYPE_IDENTIFIER)) {
        ident = node->left;
    }
    else    /*Unknown_new_var_subtree_structure_______________*/ {
        print_error("Unknown new variable subtree structure.\n");
        return LANGUAGE_UNSUPPORTED_TREE;
    }
    //-----------------------------------------------------------------------//
    if(!is_node_type_eq(ident, NODE_TYPE_IDENTIFIER)) {
        print_error("Unknown new variable subtree structure.\n");
        return LANGUAGE_UNSUPPORTED_TREE;
    }
    //-----------------------------------------------------------------------//
    size_t addr = 0;
    identifier_t *nt_info = ctx->name_table.identifiers +
                            ident->value.identifier;
    if     (!nt_info->is_global) {
        addr = ctx->backend_info.used_locals;
    }
    else if(nt_info->is_global ) {
        addr = ctx->backend_info.used_globals;
    }
    else   /*Function_or_unset_id__________________________*/ {
        print_error("Unexpected id type.\n");
        return LANGUAGE_UNEXPECTED_ID_TYPE;
    }

    _RETURN_IF_ERROR(set_memory_addr(ctx, ident, addr));
    //-----------------------------------------------------------------------//
    if(is_node_oper_eq(node->left, OPERATION_ASSIGNMENT)) {
        _RETURN_IF_ERROR(assemble_assignment(ctx, node->left));
    }
    //-----------------------------------------------------------------------//
    if     (!nt_info->is_global) {
        ctx->backend_info.used_locals++;
    }
    else if(nt_info->is_global ) {
        ctx->backend_info.used_globals++;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_new_func(language_t      *ctx,
                                   language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_NEW_FUNC)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    identifier_t *ident = ctx->name_table.identifiers +
                          node->left->value.identifier;
    _CMD_WRITE(";compiling %.*s",
               (int)ident->length,
               ident->name                 );
    _CMD_WRITE("jmp skip_%.*s:",
               (int)ident->length,
               ident->name                 );
    _CMD_WRITE("%.*s:",
               (int)ident->length,
               ident->name                 );
    //-----------------------------------------------------------------------//
    ctx->backend_info.used_locals = 0;

    ctx->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left->left));
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left->right ));
    ctx->backend_info.scope--;

    //-----------------------------------------------------------------------//
    _CMD_WRITE("skip_%.*s:\r\n",
               (int)ident->length,
               ident->name                      );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_in(language_t      *ctx,
                             language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_IN)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _CMD_WRITE("in"                             );
    _RETURN_IF_ERROR(compile_identifier(ctx, node->left->left, "pop"));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_out(language_t      *ctx,
                              language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(!is_node_oper_eq(node, OPERATION_OUT)) {
        return LANGUAGE_UNEXPECTED_OPER;
    }
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    _CMD_WRITE("out"                            );
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t assemble_call(language_t      *ctx,
                               language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t write_command(language_t *ctx,
                               const char *format, ...) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(format != NULL, return LANGUAGE_STRING_FORMAT_NULL);
    //-----------------------------------------------------------------------//
    if(fprintf(ctx->backend_info.output, "%*s",
               8 * ctx->backend_info.scope, "") < 0) {
        print_error("Error while writing asm code to file.\n");
        return LANGUAGE_WRITING_ASM_ERROR;
    }
    //-----------------------------------------------------------------------//
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    va_list args;
    va_start(args, format);
    if(vfprintf(ctx->backend_info.output, format, args) < 0) {
        print_error("Error while writing asm code to file.\n");
        return LANGUAGE_WRITING_ASM_ERROR;
    }
    va_end(args);
#pragma clang diagnostic pop
    //-----------------------------------------------------------------------//
    if(fprintf(ctx->backend_info.output, "\r\n") < 0) {
        print_error("Error while writing asm code to file.\n");
        return LANGUAGE_WRITING_ASM_ERROR;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

