#include <stdlib.h>
#include <stdio.h>

//===========================================================================//

#include "language.h"
#include "backend.h"
#include "colors.h"
#include "lang_dump.h"
#include "utils.h"
#include "name_table.h"
#include "nodes_dsl.h"
#include "custom_assert.h"

//===========================================================================//

static language_error_t compile_only    (language_t    *ctx,
                                         operation_t    opcode);

//===========================================================================//

language_error_t backend_ctor(language_t *ctx, int argc, const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    _RETURN_IF_ERROR(read_tree(ctx));
    _RETURN_IF_ERROR(dump_ctor(ctx, "backend"));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_code(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_ctor(ctx, ctx->nodes.capacity));
    ctx->backend_info.output = fopen(ctx->output_file, "wb");
    if(ctx->backend_info.output == NULL) {
        print_error("Error while opening output file.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_VAR));
    //-----------------------------------------------------------------------//
    fprintf(ctx->backend_info.output,
            ";setting bx value to global variables number\r\n"
            "\tpush " SZ_SP "\r\n"
            "\tpop bx\r\n\r\n"
            ";calling main\r\n"
            "\tcall main:\r\n"
            "\tpush ax\r\n"
            "\tout\r\n"
            "\thlt\r\n\r\n",
            ctx->backend_info.used_globals);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_FUNC));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_only(language_t *ctx, operation_t opcode) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    language_node_t *node = ctx->root;
    ctx->backend_info.used_locals = 0;
    while(node != NULL) {
        if(is_node_oper_eq(node->left, opcode)) {
            _RETURN_IF_ERROR(compile_subtree(ctx, node->left));
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t backend_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->input);
    fclose(ctx->backend_info.output);
    ctx->backend_info.output = NULL;
    ctx->input = NULL;
    _RETURN_IF_ERROR(name_table_dtor(ctx));
    _RETURN_IF_ERROR(nodes_storage_dtor(ctx));
    _RETURN_IF_ERROR(dump_dtor(ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
