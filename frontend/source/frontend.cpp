#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "frontend.h"
#include "colors.h"
#include "utils.h"
#include "custom_assert.h"
#include "lang_dump.h"
#include "name_table.h"
#include "frontend_utils.h"

//===========================================================================//

static language_error_t get_word_length (language_t *ctx,
                                         size_t     *length);

static language_error_t skip_spaces     (language_t *ctx);

static language_error_t skip_comment    (language_t *ctx);

static language_error_t read_word       (language_t *ctx);

//===========================================================================//

language_error_t frontend_ctor(language_t *ctx, int argc, const char *argv[]) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(argv != NULL, return LANGUAGE_NULL_PROGRAM_INPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    _RETURN_IF_ERROR(dump_ctor(ctx, "frontend"));
    //-----------------------------------------------------------------------//
    FILE *source = fopen(ctx->input_file, "rb");
    if(source == NULL) {
        print_error("Error while opening source code file '%s'. "
                    "May be the file does not exist.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    ctx->input_size = file_size(source);
    //-----------------------------------------------------------------------//
    ctx->input = (char *)calloc(ctx->input_size + 1, sizeof(char));
    if(ctx->input == NULL) {
        fclose(source);
        print_error("Error while allocating memory for input.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //-----------------------------------------------------------------------//
    if(fread(ctx->input,
             sizeof(char),
             ctx->input_size,
             source) != ctx->input_size) {
        fclose(source);
        print_error("Error while reading source.\n");
        return LANGUAGE_READING_SOURCE_ERROR;
    }
    fclose(source);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(nodes_storage_ctor(ctx, ctx->input_size * 2));
    _RETURN_IF_ERROR(name_table_ctor(ctx, ctx->input_size));
    _RETURN_IF_ERROR(used_names_ctor(ctx, ctx->input_size));
    //-----------------------------------------------------------------------//
    ctx->input_position             = ctx->input;
    ctx->frontend_info.position     = ctx->nodes.nodes;
    ctx->frontend_info.current_line = 1;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t parse_tokens(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    while(current_symbol(ctx) != '\0') {
        _RETURN_IF_ERROR(skip_spaces(ctx));
        //-------------------------------------------------------------------//
        if(isdigit(current_symbol(ctx))) {
            char *number_end = NULL;
            double value = strtod(input_position(ctx), &number_end);
            size_t length = (size_t)number_end - (size_t)input_position(ctx);
            _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                               NODE_TYPE_NUMBER,
                                               NUMBER(value),
                                               input_position(ctx),
                                               length,
                                               NULL));
            ctx->input_position = number_end;
        }
        //-------------------------------------------------------------------//
        else {
            _RETURN_IF_ERROR(read_word(ctx));
        }
        _RETURN_IF_ERROR(skip_spaces(ctx));
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                       NODE_TYPE_OPERATION,
                                       OPCODE(OPERATION_PROGRAM_END),
                                       "",
                                       0,
                                       NULL));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t read_word(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    size_t length = 0;
    _RETURN_IF_ERROR(get_word_length(ctx, &length));
    bool found = false;
    //-----------------------------------------------------------------------//
    for(size_t elem = 1; elem < (size_t)OPERATION_PROGRAM_END; elem++) {
        if(strncmp(KeyWords[elem].name,
                   input_position(ctx),
                   KeyWords[elem].length) == 0) {
            //---------------------------------------------------------------//
            _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                               NODE_TYPE_OPERATION,
                                               OPCODE(KeyWords[elem].code),
                                               input_position(ctx),
                                               length,
                                               NULL));
            found = true;
            //---------------------------------------------------------------//
            if(KeyWords[elem].code == OPERATION_BODY_END) {
                _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                                   NODE_TYPE_OPERATION,
                                                   OPCODE(OPERATION_STATEMENT),
                                                   input_position(ctx),
                                                   length,
                                                   NULL));
            }
            //---------------------------------------------------------------//
            break;
        }
    }
    //-----------------------------------------------------------------------//
    if(!found) {
        size_t index = 0;
        _RETURN_IF_ERROR(used_names_add(ctx,
                                        input_position(ctx),
                                        length,
                                        &index));
        _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                           NODE_TYPE_IDENTIFIER,
                                           IDENT(index),
                                           input_position(ctx),
                                           length,
                                           NULL));
    }
    ctx->input_position += length;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t frontend_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    dump_dtor(ctx);
    free(ctx->input);
    _RETURN_IF_ERROR(nodes_storage_dtor(ctx));
    memset(ctx, 0, sizeof(*ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_word_length(language_t *ctx, size_t *length) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(length != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    size_t keywords_num = sizeof(KeyWords) / sizeof(KeyWords[0]);
    for(size_t elem = 0; elem < keywords_num; elem++) {
        if(KeyWords[elem].length == 1 &&
           current_symbol(ctx) == KeyWords[elem].name[0]) {
            *length = 1;
            return LANGUAGE_SUCCESS;
        }
    }
    //-----------------------------------------------------------------------//
    if(!isalpha(current_symbol(ctx))) {
        print_error("Unknown word start, words start only with letters.\n");
        return LANGUAGE_UNEXPECTED_WORD_START;
    }
    //-----------------------------------------------------------------------//
    const char *position = input_position(ctx);
    while(isalpha(*position) || *position == '_') {
        position++;
    }
    //-----------------------------------------------------------------------//
    *length = (size_t)position - (size_t)input_position(ctx);
    return LANGUAGE_SUCCESS;
}

//---------------------------------------------------------------------------//

language_error_t skip_spaces(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    while(true) {
        //-------------------------------------------------------------------//
        if(isspace(current_symbol(ctx))) {
            //---------------------------------------------------------------//
            if(current_symbol(ctx) == '\n') {
                ctx->frontend_info.current_line++;
            }
            //---------------------------------------------------------------//
            move_next_symbol(ctx);
            continue;
        }
        //-------------------------------------------------------------------//
        if(input_position(ctx)[0] == '/' &&
           input_position(ctx)[1] == '*') {
            _RETURN_IF_ERROR(skip_comment(ctx));
            continue;
        }
        //-------------------------------------------------------------------//
        break;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t skip_comment(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    while(input_position(ctx)[-2] != '*' || input_position(ctx)[-1] != '/') {
        move_next_symbol(ctx);
        //-------------------------------------------------------------------//
        if(current_symbol(ctx) == '\0') {
            print_error("Unclosed comment.\n");
            return LANGUAGE_UNCLOSED_COMMENT;
        }
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
