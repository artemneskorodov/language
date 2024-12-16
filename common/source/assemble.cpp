#include <stdio.h>
#include <stdarg.h>

#include "language.h"
#include "assemble.h"
#include "utils.h"
#include "colors.h"

language_error_t compile_function_call(language_t *language, language_node_t *root);
language_error_t compile_identifier(language_t *language, language_node_t *root);
language_error_t write_command(language_t *language, const char *format, ...);

language_error_t compile_subtree(language_t *language, language_node_t *root) {
    if(root == NULL) {
        return LANGUAGE_SUCCESS;
    }
    switch(root->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(compile_identifier(language, root));
            break;
        }
        case NODE_TYPE_NUMBER: {
            _RETURN_IF_ERROR(write_command(language, "push %lg", root->value.number));
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(KeyWords[root->value.opcode].assemble(language, root));
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t compile_identifier(language_t *language, language_node_t *root) {
    identifier_t *identifier = language->name_table.identifiers + root->value.identifier;
    switch(identifier->type) {
        case IDENTIFIER_FUNCTION: {
            _RETURN_IF_ERROR(compile_function_call(language, root));
            break;
        }
        case IDENTIFIER_GLOBAL_VAR: {
            _RETURN_IF_ERROR(write_command(language, "push [" SZ_SP "]", identifier->memory_addr));
            break;
        }
        case IDENTIFIER_LOCAL_VAR: {
            _RETURN_IF_ERROR(write_command(language, "push [bx + " SZ_SP "]", identifier->memory_addr));
            break;
        }
        default: {
            print_error("Unknown identifier type.\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t compile_function_call(language_t *language, language_node_t *root) {
    identifier_t *identifier = language->name_table.identifiers + root->value.identifier;
    language_node_t *param = root->right;
    _RETURN_IF_ERROR(write_command(language, ";saving BX"));
    _RETURN_IF_ERROR(write_command(language, "push bx\r\n"));

    _RETURN_IF_ERROR(write_command(language, ";function parameters"));

    language->backend_info.scope++;
    while(param != NULL) {
        _RETURN_IF_ERROR(compile_subtree(language, param->left));
        param = param->right;
    }
    language->backend_info.scope--;

    _RETURN_IF_ERROR(write_command(language, "\r\n"));
    _RETURN_IF_ERROR(write_command(language, ";incrementing bx"));
    _RETURN_IF_ERROR(write_command(language, "push bx"));
    _RETURN_IF_ERROR(write_command(language, "push " SZ_SP,language->backend_info.used_globals + language->backend_info.used_locals));
    _RETURN_IF_ERROR(write_command(language, "add"));
    _RETURN_IF_ERROR(write_command(language, "pop bx\r\n"))
    _RETURN_IF_ERROR(write_command(language, ";pushing arguments to function memory"));

    for(size_t i = 0; i < identifier->parameters_number; i++) {
        _RETURN_IF_ERROR(write_command(language, "pop [bx + " SZ_SP "]", i));
    }
    _RETURN_IF_ERROR(write_command(language, "call %.*s:", (int)identifier->length, identifier->name));
    _RETURN_IF_ERROR(write_command(language, "pop bx"));
    _RETURN_IF_ERROR(write_command(language, "push ax\r\n"));
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_two_args(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left));
    _RETURN_IF_ERROR(compile_subtree(language, node->right));

    _RETURN_IF_ERROR(write_command(language, "%s", KeyWords[node->value.opcode].assembler_command));

    return LANGUAGE_SUCCESS;
}

language_error_t assemble_one_arg(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right));

    _RETURN_IF_ERROR(write_command(language, "%s", KeyWords[node->value.opcode].assembler_command));

    return LANGUAGE_SUCCESS;
}


/*
push LEFT
push RIGHT

jb comp_false_NUM:
push 1
jmp comp_fale_end_NUM:
comp_false_NUM:
    push 0
comp_false_end_NUM:
*/
language_error_t assemble_comparison(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left));
    _RETURN_IF_ERROR(compile_subtree(language, node->right));
    size_t num = language->backend_info.used_labels++;

    _RETURN_IF_ERROR(write_command(language, "%s comp_false_" SZ_SP ":", KeyWords[node->value.opcode].assembler_command, num));
    _RETURN_IF_ERROR(write_command(language, "push 1"));
    _RETURN_IF_ERROR(write_command(language, "jmp comp_false_end_" SZ_SP ":\r\n", num));
    _RETURN_IF_ERROR(write_command(language, "comp_false_" SZ_SP ":", num));
    _RETURN_IF_ERROR(write_command(language, "push 0"));
    _RETURN_IF_ERROR(write_command(language, "comp_false_end_" SZ_SP ":\r\n", num));

    return LANGUAGE_SUCCESS;
}

/*
push RIGHT
pop [addr]
*/
language_error_t assemble_assignment(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right));
    identifier_t *identifier = language->name_table.identifiers + node->left->value.identifier;
    switch(identifier->type) {
        case IDENTIFIER_FUNCTION: {
            print_error("Unexpected to see a function in assignment.\n");
            return LANGUAGE_TREE_ERROR;
        }
        case IDENTIFIER_GLOBAL_VAR: {
            _RETURN_IF_ERROR(write_command(language, "pop [" SZ_SP "]    ;'%.*s'", identifier->memory_addr, (int)identifier->length, identifier->name));
            break;
        }
        case IDENTIFIER_LOCAL_VAR: {
            _RETURN_IF_ERROR(write_command(language, "pop [bx + " SZ_SP "]    ;'%.*s'", identifier->memory_addr, (int)identifier->length, identifier->name));
        }
        default: {
            print_error("Expected to see identifier in lvalue of assignment.\n");
            return LANGUAGE_TREE_ERROR;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_statements_line(language_t *language, language_node_t *node) {
    size_t old_used_locals = language->backend_info.used_locals;
    while(node != NULL) {
        _RETURN_IF_ERROR(compile_subtree(language, node->left));
        node = node->right;
    }
    language->backend_info.used_locals = old_used_locals;
    return LANGUAGE_SUCCESS;
}

/*
push LEFT
push 0
je skip_left_NUM:
{body}
skip_left_NUM:
*/
language_error_t assemble_if(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left));
    size_t num = language->backend_info.used_labels++;
    _RETURN_IF_ERROR(write_command(language, "push 0"));
    _RETURN_IF_ERROR(write_command(language, "je skip_if_" SZ_SP ":", num));

    language->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(language, node->right));
    language->backend_info.scope--;

    _RETURN_IF_ERROR(write_command(language, "skip_if_" SZ_SP ":", num));

    return LANGUAGE_SUCCESS;
}

language_error_t assemble_while(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left));
    size_t num = language->backend_info.used_labels++;
    _RETURN_IF_ERROR(write_command(language, "while_start_" SZ_SP ":", num));
    _RETURN_IF_ERROR(write_command(language, "push 0"));
    _RETURN_IF_ERROR(write_command(language, "je skip_while_" SZ_SP ":", num));

    language->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(language, node->right));
    language->backend_info.scope--;

    _RETURN_IF_ERROR(write_command(language, "jmp while_start_" SZ_SP ":", num));
    _RETURN_IF_ERROR(write_command(language, "skip_if_" SZ_SP ":", num));

    return LANGUAGE_SUCCESS;
}

language_error_t assemble_return(language_t *language, language_node_t *node) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right));

    _RETURN_IF_ERROR(write_command(language, "pop ax"));
    _RETURN_IF_ERROR(write_command(language, "ret"));

    return LANGUAGE_SUCCESS;
}

language_error_t assemble_params_line(language_t *language, language_node_t *node) {
    while(node != NULL) {
        _RETURN_IF_ERROR(compile_subtree(language, node->left));
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_new_var(language_t *language, language_node_t *node) {
    language_node_t *identifier = NULL;
    //TODO check that identifiers
    if(node->right->type == NODE_TYPE_OPERATION) {
        identifier = node->right->left;
    }
    else {
        identifier = node->right;
    }

    language->name_table.identifiers[identifier->value.identifier].memory_addr = language->backend_info.used_locals;
    if(language->name_table.identifiers[identifier->value.identifier].type == IDENTIFIER_LOCAL_VAR) {
        language->backend_info.used_locals++;
    }
    if(node->right->type == NODE_TYPE_OPERATION) {
        //TODO check that =
        _RETURN_IF_ERROR(assemble_assignment(language, node->right));
    }
    if(language->name_table.identifiers[identifier->value.identifier].type == IDENTIFIER_GLOBAL_VAR) {
        language->backend_info.used_globals++;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_new_func(language_t *language, language_node_t *node) {
    identifier_t *identifier = language->name_table.identifiers + node->right->value.identifier;
    _RETURN_IF_ERROR(write_command(language, ";compiling %.*s", (int)identifier->length, identifier->name));
    _RETURN_IF_ERROR(write_command(language, "jmp skip_%.*s:", (int)identifier->length, identifier->name));
    _RETURN_IF_ERROR(write_command(language, "%.*s:", (int)identifier->length, identifier->name));

    language->backend_info.used_locals = 0;
    language->backend_info.scope++;
    _RETURN_IF_ERROR(compile_subtree(language, node->right->left));
    _RETURN_IF_ERROR(compile_subtree(language, node->right->right));
    language->backend_info.scope--;

    _RETURN_IF_ERROR(write_command(language, "skip_%.*s:\r\n", (int)identifier->length, identifier->name));
    return LANGUAGE_SUCCESS;
}

language_error_t write_command(language_t *language, const char *format, ...) {
    fprintf(language->backend_info.output, "%*s", 8 * language->backend_info.scope, "");
    va_list args;
    va_start(args, format);
    vfprintf(language->backend_info.output, format, args);
    va_end(args);
    fprintf(language->backend_info.output, "\r\n");
    return LANGUAGE_SUCCESS;
}
