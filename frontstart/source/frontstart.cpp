#include <stdio.h>
#include <stdlib.h>

//===========================================================================//

#include "language.h"
#include "frontstart.h"
#include "lang_dump.h"
#include "colors.h"
#include "name_table.h"
#include "custom_assert.h"

//===========================================================================//

language_error_t frontstart_ctor(language_t *ctx, int argc, const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    _RETURN_IF_ERROR(read_tree(ctx));
    _RETURN_IF_ERROR(dump_ctor(ctx, "frontstart"));

    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t frontstart_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->input);
    ctx->input = NULL;
    _RETURN_IF_ERROR(nodes_storage_dtor(ctx));
    _RETURN_IF_ERROR(name_table_dtor(ctx));
    _RETURN_IF_ERROR(dump_dtor(ctx));

    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t frontstart_write(language_t *ctx) {
    _C_ASSERT(ctx       != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(ctx->root != NULL, return LANGUAGE_ROOT_NULL);
    //-----------------------------------------------------------------------//
    ctx->frontstart_info.output = fopen(ctx->output_file, "wb");
    if(ctx->frontstart_info.output == NULL) {
        print_error("Error while opening output file.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    language_node_t *node = ctx->root;
    while(node != NULL) {
        _RETURN_IF_ERROR(to_source_subtree(ctx, node->left));
        fprintf(ctx->frontstart_info.output, "\r\n\r\n");
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
