#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

//====================================================================================//

#include "language.h"
#include "frontend.h"
#include "colors.h"
#include "utils.h"
#include "custom_assert.h"
#include "lang_dump.h"
#include "name_table.h"
#include "frontend_utils.h"

//====================================================================================//

static language_error_t get_word_length (language_t *language,
                                         size_t     *length);

static language_error_t skip_spaces     (language_t *language);

static language_error_t skip_comment    (language_t *language);

static language_error_t read_word       (language_t *language);

//====================================================================================//

language_error_t frontend_ctor(language_t *language, int argc, const char *argv[]) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(argv     != NULL, return LANGUAGE_NULL_PROGRAM_INPUT);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(parse_flags(language, argc, argv));
    _RETURN_IF_ERROR(dump_ctor(language, "frontend"));
    //--------------------------------------------------------------------------------//
    FILE *source = fopen(language->input_file, "rb");
    if(source == NULL) {
        print_error("Error while opening source code file '%s'. "
                    "May be the file does not exist.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    language->input_size = file_size(source);
    //--------------------------------------------------------------------------------//
    language->input = (char *)calloc(language->input_size + 1, sizeof(char));
    if(language->input == NULL) {
        fclose(source);
        print_error("Error while allocating memory for input.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //--------------------------------------------------------------------------------//
    if(fread(language->input,
             sizeof(char),
             language->input_size,
             source) != language->input_size) {
        fclose(source);
        print_error("Error while reading source.\n");
        return LANGUAGE_READING_SOURCE_ERROR;
    }
    fclose(source);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(nodes_storage_ctor(language, language->input_size + 1));
    _RETURN_IF_ERROR(name_table_ctor(language, language->input_size));
    _RETURN_IF_ERROR(used_names_ctor(language, language->input_size));
    //--------------------------------------------------------------------------------//
    language->input_position = language->input;
    language->frontend_info.position = language->nodes.nodes;
    language->frontend_info.current_line = 1;
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t parse_tokens(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    while(current_symbol(language) != '\0') {
        //----------------------------------------------------------------------------//
        if(isdigit(current_symbol(language))) {
            char *number_end = NULL;
            double value = strtod(input_position(language), &number_end);
            size_t length = (size_t)number_end - (size_t)input_position(language);
            _RETURN_IF_ERROR(nodes_storage_add(language,
                                               NODE_TYPE_NUMBER,
                                               NUMBER(value),
                                               input_position(language),
                                               length,
                                               NULL));
            language->input_position = number_end;
        }
        //----------------------------------------------------------------------------//
        else {
            _RETURN_IF_ERROR(read_word(language));
        }
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(skip_spaces(language));
    }
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(nodes_storage_add(language,
                                       NODE_TYPE_OPERATION,
                                       OPCODE(OPERATION_PROGRAM_END),
                                       "",
                                       0,
                                       NULL));
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t read_word(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    size_t length = 0;
    _RETURN_IF_ERROR(get_word_length(language, &length));
    bool found = false;
    //--------------------------------------------------------------------------------//
    for(size_t elem = 1; elem < (size_t)OPERATION_PROGRAM_END; elem++) {
        if(strncmp(KeyWords[elem].name, input_position(language), length) == 0) {
            //------------------------------------------------------------------------//
            _RETURN_IF_ERROR(nodes_storage_add(language,
                                               NODE_TYPE_OPERATION,
                                               OPCODE(KeyWords[elem].code),
                                               input_position(language),
                                               length,
                                               NULL));
            found = true;
            //------------------------------------------------------------------------//
            if(KeyWords[elem].code == OPERATION_BODY_END) {
                _RETURN_IF_ERROR(nodes_storage_add(language,
                                                   NODE_TYPE_OPERATION,
                                                   OPCODE(OPERATION_STATEMENT),
                                                   input_position(language),
                                                   length,
                                                   NULL));
            }
            //------------------------------------------------------------------------//
            break;
        }
    }
    //--------------------------------------------------------------------------------//
    if(!found) {
        size_t index = 0;
        _RETURN_IF_ERROR(used_names_add(language, input_position(language), length, &index));
        _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_IDENTIFIER, IDENT(index), input_position(language), length, NULL));
    }
    language->input_position += length;
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t frontend_dtor(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    dump_dtor(language);
    free(language->input);
    _RETURN_IF_ERROR(nodes_storage_dtor(language));
    memset(language, 0, sizeof(*language));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_word_length(language_t *language, size_t *length) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(length   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    for(size_t elem = 0; elem < sizeof(KeyWords) / sizeof(KeyWords[0]); elem++) {
        if(KeyWords[elem].length == 1 && current_symbol(language) == KeyWords[elem].name[0]) {
            *length = 1;
            return LANGUAGE_SUCCESS;
        }
    }
    //--------------------------------------------------------------------------------//
    if(!isalpha(current_symbol(language))) {
        print_error("Unknown word start, words start only with letters.\n");
        return LANGUAGE_UNEXPECTED_WORD_START;
    }
    //--------------------------------------------------------------------------------//
    const char *position = input_position(language);
    while(isalpha(*position) || *position == '_') {
        position++;
    }
    //--------------------------------------------------------------------------------//
    *length = (size_t)position - (size_t)input_position(language);
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t skip_spaces(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    while(true) {
        //----------------------------------------------------------------------------//
        if(isspace(current_symbol(language))) {
            //------------------------------------------------------------------------//
            if(current_symbol(language) == '\n') {
                language->frontend_info.current_line++;
            }
            //------------------------------------------------------------------------//
            move_next_symbol(language);
            continue;
        }
        //----------------------------------------------------------------------------//
        if(input_position(language)[0] == '/' && input_position(language)[1] == '*') {
            _RETURN_IF_ERROR(skip_comment(language));
            continue;
        }
        //----------------------------------------------------------------------------//
        break;
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t skip_comment(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    while(input_position(language)[-2] != '*' || input_position(language)[-1] != '/') {
        move_next_symbol(language);
        //----------------------------------------------------------------------------//
        if(current_symbol(language) == '\0') {
            print_error("Unclosed comment.\n");
            return LANGUAGE_UNCLOSED_COMMENT;
        }
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//
