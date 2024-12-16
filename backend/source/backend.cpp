#include <stdlib.h>
#include <stdio.h>

#include "language.h"
#include "backend.h"
#include "colors.h"
#include "lang_dump.h"
#include "utils.h"
#include "name_table.h"

static language_error_t compile_globals_vars(language_t *language);
language_error_t compile_functions(language_t *language);
bool is_operation(language_node_t *node, operation_t opcode);

language_error_t backend_ctor(language_t *language, int argc, const char *argv[]) {
    _RETURN_IF_ERROR(parse_flags(language, argc, argv));
    _RETURN_IF_ERROR(read_tree(language));
    _RETURN_IF_ERROR(dump_ctor(language, "backend"));

    return LANGUAGE_SUCCESS;
}

language_error_t compile_code(language_t *language) {
    _RETURN_IF_ERROR(variables_stack_ctor(language, language->nodes.capacity));
    language->backend_info.output = fopen(language->output_file, "wb");
    if(language->backend_info.output == NULL) {
        print_error("Error while opening output file.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }

    _RETURN_IF_ERROR(compile_globals_vars(language));

    fprintf(language->backend_info.output,
            ";setting bx value to global variables number\r\n"
            "\tpush " SZ_SP "\r\n"
            "\tpop bx\r\n\r\n"
            ";calling main\r\n"
            "\tcall main:\r\n"
            "\tpush ax\r\n"
            "\tout\r\n"
            "\thlt\r\n\r\n", language->backend_info.used_globals);

    _RETURN_IF_ERROR(compile_functions(language));

    return LANGUAGE_SUCCESS;
}

language_error_t compile_globals_vars(language_t *language) {
    fprintf(language->backend_info.output,
            ";global variables setting\r\n");
    language_node_t *node = language->root;
    language->backend_info.used_locals = 0;
    while(node != NULL) {
        if(is_operation(node->left, OPERATION_NEW_VAR)) {
            _RETURN_IF_ERROR(compile_subtree(language, node->left));
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t compile_functions(language_t *language) {
    fprintf(language->backend_info.output,
            ";function compilation\r\n");
    language_node_t *node = language->root;
    while(node != NULL) {
        if(is_operation(node->left, OPERATION_NEW_FUNC)) {
            _RETURN_IF_ERROR(compile_subtree(language, node->left));
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t backend_dtor(language_t *language) {
    free(language->input);
    fclose(language->backend_info.output);
    language->backend_info.output = NULL;
    language->input = NULL;
    _RETURN_IF_ERROR(name_table_dtor(language));
    _RETURN_IF_ERROR(nodes_storage_dtor(language));
    _RETURN_IF_ERROR(dump_dtor(language));
    return LANGUAGE_SUCCESS;
}

bool is_operation(language_node_t *node, operation_t opcode) {
    if(node->type == NODE_TYPE_OPERATION && node->value.opcode == opcode) {
        return true;
    }
    return false;
}
