#include <stdio.h>
#include <stdarg.h>

//====================================================================================//

#include "language.h"
#include "frontend_utils.h"
#include "colors.h"
#include "custom_assert.h"

//====================================================================================//

language_error_t move_next_token(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    language->frontend_info.position++;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_node_t *token_position(language_t *language) {
    _C_ASSERT(language != NULL, return NULL);
    return language->frontend_info.position;
}

//====================================================================================//

language_error_t move_next_symbol(language_t *language) {
    _C_ASSERT(language                 != NULL, return LANGUAGE_CTX_NULL           );
    _C_ASSERT(language->input          != NULL, return LANGUAGE_INPUT_NULL         );
    _C_ASSERT(language->input_position != NULL, return LANGUAGE_INPUT_POSITION_NULL);
    //--------------------------------------------------------------------------------//
    language->input_position++;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

char current_symbol(language_t *language) {
    _C_ASSERT(language                 != NULL, return EOF);
    _C_ASSERT(language->input          != NULL, return EOF);
    _C_ASSERT(language->input_position != NULL, return EOF);
    return *language->input_position;
}

//====================================================================================//

const char *input_position(language_t *language) {
    _C_ASSERT(language                 != NULL, return NULL);
    _C_ASSERT(language->input          != NULL, return NULL);
    _C_ASSERT(language->input_position != NULL, return NULL);
    return language->input_position;
}

//====================================================================================//

bool is_on_ident_type(language_t *language, identifier_type_t type) {
    _C_ASSERT(language != NULL, return false);
    //--------------------------------------------------------------------------------//
    if(token_position(language)->type == NODE_TYPE_IDENTIFIER &&
       language->name_table.identifiers[token_position(language)->value.identifier].type == type) {
        return true;
    }
    //--------------------------------------------------------------------------------//
    return false;
}

//====================================================================================//

bool is_on_type(language_t *language, node_type_t type) {
    _C_ASSERT(language != NULL, return false);
    //--------------------------------------------------------------------------------//
    if(token_position(language)->type == type) {
        return true;
    }
    //--------------------------------------------------------------------------------//
    return false;
}

//====================================================================================//

bool is_on_operation(language_t *language, operation_t opcode) {
    _C_ASSERT(language != NULL, return false);
    //--------------------------------------------------------------------------------//
    if(language->frontend_info.position->type == NODE_TYPE_OPERATION &&
       language->frontend_info.position->value.opcode == opcode) {
        return true;
    }
    //--------------------------------------------------------------------------------//
    return false;
}

//====================================================================================//

language_error_t syntax_error(language_t *language, const char *format, ...) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(format   != NULL, return LANGUAGE_STRING_FORMAT_NULL);
    //--------------------------------------------------------------------------------//
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                  "%s:%llu",
                  language->input_file,
                  token_position(language)->source_info.line);
    //--------------------------------------------------------------------------------//
    color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, " ~ ");
    //--------------------------------------------------------------------------------//
    va_list args;
    va_start(args, format);
    color_vprintf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, format, args);
    va_end(args);
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SYNTAX_ERROR;
}

//====================================================================================//
