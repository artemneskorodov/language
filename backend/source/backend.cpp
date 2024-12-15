#include <stdlib.h>
#include <stdio.h>

#include "language.h"
#include "backend.h"
#include "colors.h"
#include "lang_dump.h"

language_error_t backend_ctor(language_t *language, int argc, const char *argv[]) {
    _RETURN_IF_ERROR(parse_flags(language, argc, argv));
    _RETURN_IF_ERROR(read_tree(language));
    _RETURN_IF_ERROR(dump_ctor(language, "backend"));

    return LANGUAGE_SUCCESS;
}

language_error_t compile_code(language_t *language) {
    FILE *output = fopen(language->output_file, "wb");
    if(output == NULL) {
        print_error("Error while opening output file.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }

    fprintf(output, "call main:\r\nhlt\r\n\r\n");

    language_node_t *node = language->root;

    while(node != NULL) {
        language->backend_info.used_locals = 0;
        language_error_t error_code = compile_subtree(language, node->left, output);
        if(error_code != LANGUAGE_SUCCESS) {
            fclose(output);
            return error_code;
        }
        node = node->right;
    }
    fclose(output);
    return LANGUAGE_SUCCESS;
}

language_error_t backend_dtor(language_t *language) {
    free(language->input);
    _RETURN_IF_ERROR(name_table_dtor(language));
    _RETURN_IF_ERROR(nodes_storage_dtor(language));
    _RETURN_IF_ERROR(dump_dtor(language));
    return LANGUAGE_SUCCESS;
}
