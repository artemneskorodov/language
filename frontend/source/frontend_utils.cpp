#include <stdio.h>
#include <stdarg.h>

//===========================================================================//

#include "language.h"
#include "frontend_utils.h"
#include "colors.h"
#include "custom_assert.h"

//===========================================================================//

language_error_t move_next_token(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    ctx->frontend_info.position++;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_node_t *token_position(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return NULL);
    return ctx->frontend_info.position;
}

//===========================================================================//

language_error_t move_next_symbol(language_t *ctx) {
    _C_ASSERT(ctx                 != NULL, return LANGUAGE_CTX_NULL           );
    _C_ASSERT(ctx->input          != NULL, return LANGUAGE_INPUT_NULL         );
    _C_ASSERT(ctx->input_position != NULL, return LANGUAGE_INPUT_POSITION_NULL);
    //-----------------------------------------------------------------------//
    ctx->input_position++;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

char current_symbol(language_t *ctx) {
    _C_ASSERT(ctx                 != NULL, return EOF);
    _C_ASSERT(ctx->input          != NULL, return EOF);
    _C_ASSERT(ctx->input_position != NULL, return EOF);
    return *ctx->input_position;
}

//===========================================================================//

const char *input_position(language_t *ctx) {
    _C_ASSERT(ctx                 != NULL, return NULL);
    _C_ASSERT(ctx->input          != NULL, return NULL);
    _C_ASSERT(ctx->input_position != NULL, return NULL);
    return ctx->input_position;
}

//===========================================================================//

bool is_on_ident_type(language_t *ctx, identifier_type_t type) {
    _C_ASSERT(ctx != NULL, return false);
    //-----------------------------------------------------------------------//
    language_node_t *node = token_position(ctx);
    if(node->type == NODE_TYPE_IDENTIFIER &&
       ctx->name_table.identifiers[node->value.identifier].type == type) {
        return true;
    }
    //-----------------------------------------------------------------------//
    return false;
}

//===========================================================================//

bool is_on_type(language_t *ctx, node_type_t type) {
    _C_ASSERT(ctx != NULL, return false);
    //-----------------------------------------------------------------------//
    if(token_position(ctx)->type == type) {
        return true;
    }
    //-----------------------------------------------------------------------//
    return false;
}

//===========================================================================//

bool is_on_operation(language_t *ctx, operation_t opcode) {
    _C_ASSERT(ctx != NULL, return false);
    //-----------------------------------------------------------------------//
    if(ctx->frontend_info.position->type == NODE_TYPE_OPERATION &&
       ctx->frontend_info.position->value.opcode == opcode) {
        return true;
    }
    //-----------------------------------------------------------------------//
    return false;
}

//===========================================================================//

language_error_t syntax_error(language_t *ctx, const char *format, ...) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(format != NULL, return LANGUAGE_STRING_FORMAT_NULL);
    //-----------------------------------------------------------------------//
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                  "%s:%llu",
                  ctx->input_file,
                  token_position(ctx)->source_info.line);
    //-----------------------------------------------------------------------//
    color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, " ~ ");
    //-----------------------------------------------------------------------//
    va_list args;
    va_start(args, format);
    color_vprintf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, format, args);
    va_end(args);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SYNTAX_ERROR;
}

//===========================================================================//
