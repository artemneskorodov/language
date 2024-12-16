#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "language.h"
#include "frontend.h"
#include "colors.h"
#include "utils.h"
#include "custom_assert.h"
#include "lang_dump.h"
#include "name_table.h"
#include "frontend_utils.h"

language_error_t get_word_length(language_t *language, size_t *length);
language_error_t skip_spaces(language_t *language);
language_error_t skip_comment(language_t *language);
language_error_t read_word(language_t *language);

language_error_t frontend_ctor(language_t *language, int argc, const char *argv[]) {
    _RETURN_IF_ERROR(parse_flags(language, argc, argv));
    _RETURN_IF_ERROR(dump_ctor(language, "frontend"));
    FILE *source = fopen(language->input_file, "rb");
    if(source == NULL) {
        print_error("Error while opening source code file '%s'. May be the file does not exist.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    language->input_size = file_size(source);
    language->input = (char *)calloc(language->input_size + 1, sizeof(char));
    if(language->input == NULL) {
        fclose(source);
        print_error("Error while allocating memory for input.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    if(fread(language->input, sizeof(char), language->input_size, source) != language->input_size) {
        fclose(source);
        print_error("Error while reading source.\n");
        return LANGUAGE_READING_SOURCE_ERROR;
    }
    fclose(source);

    _RETURN_IF_ERROR(nodes_storage_ctor(language, language->input_size + 1));
    _RETURN_IF_ERROR(name_table_ctor(language, language->input_size));
    _RETURN_IF_ERROR(used_names_ctor(language, language->input_size));
    language->input_position = language->input;
    language->frontend_info.position = language->nodes.nodes;
    language->frontend_info.current_line = 1;
    return LANGUAGE_SUCCESS;
}

language_error_t parse_tokens(language_t *language) {
    while(current_symbol(language) != '\0') {
        if(isdigit(current_symbol(language))) {
            char *number_end = NULL;
            double value = strtod(input_position(language), &number_end);
            _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_NUMBER, NUMBER(value), input_position(language), (size_t)number_end - (size_t)input_position(language), NULL));

            language->input_position = number_end;
        }
        else {
            _RETURN_IF_ERROR(read_word(language));
        }
        _RETURN_IF_ERROR(skip_spaces(language));
    }

    //TODO verify name table so it does not have same names

    _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(OPERATION_PROGRAM_END), "", 0, NULL));
    return LANGUAGE_SUCCESS;
}

language_error_t read_word(language_t *language) {
    size_t length = 0;
    _RETURN_IF_ERROR(get_word_length(language, &length));
    bool found = false;
    fprintf(stderr, "%.*s --- ", (int)length, input_position(language));
    for(size_t elem = 1; elem < (size_t)OPERATION_PROGRAM_END; elem++) {
        if(strncmp(KeyWords[elem].name, input_position(language), length) == 0) {
            fprintf(stderr, "found\n");
            _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(KeyWords[elem].code), input_position(language), length, NULL));
            found = true;
            if(KeyWords[elem].code == OPERATION_BODY_END) {
                _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(OPERATION_STATEMENT), input_position(language), length, NULL));
            }
            break;
        }
    }
    if(!found) {
        fprintf(stderr, "not found\n");
        size_t index = 0;
        _RETURN_IF_ERROR(used_names_add(language, input_position(language), length, &index));
        _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_IDENTIFIER, IDENT(index), input_position(language), length, NULL));
    }
    language->input_position += length;
    return LANGUAGE_SUCCESS;
}

language_error_t frontend_dtor(language_t *language) {
    dump_dtor(language);
    free(language->input);
    _RETURN_IF_ERROR(nodes_storage_dtor(language));
    memset(language, 0, sizeof(*language));
    return LANGUAGE_SUCCESS;
}

language_error_t get_word_length(language_t *language, size_t *length) {
    for(size_t elem = 0; elem < sizeof(KeyWords) / sizeof(KeyWords[0]); elem++) {
        if(KeyWords[elem].length == 1 && current_symbol(language) == KeyWords[elem].name[0]) {
            *length = 1;
            return LANGUAGE_SUCCESS;
        }
    }

    if(!isalpha(current_symbol(language))) {
        print_error("Unknown word start, words start only with letters.\n");
        return LANGUAGE_UNEXPECTED_WORD_START;
    }

    const char *position = input_position(language);
    while(isalpha(*position) || *position == '_') {
        position++;
    }

    *length = (size_t)position - (size_t)input_position(language);
    return LANGUAGE_SUCCESS;
}

language_error_t skip_spaces(language_t *language) {
    while(true) {
        if(isspace(current_symbol(language))) {
            if(current_symbol(language) == '\n') {
                language->frontend_info.current_line++;
            }
            move_next_symbol(language);
            continue;
        }
        if(input_position(language)[0] == '/' && input_position(language)[1] == '*') {
            _RETURN_IF_ERROR(skip_comment(language));
            continue;
        }
        break;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t skip_comment(language_t *language) {
    //TODO check that on comment start
    while(input_position(language)[-2] != '*' || input_position(language)[-1] != '/') {
        move_next_symbol(language);
        if(current_symbol(language) == '\0') {
            print_error("Unclosed comment.\n");
            return LANGUAGE_UNCLOSED_COMMENT;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t set_reference(language_t *language, language_node_t *node) {
    if(node->type != NODE_TYPE_IDENTIFIER) {
        print_error("Node type is not identifier. Unable to get reference.\n");
        return LANGUAGE_SYNTAX_UNEXPECTED_CALL;
    }
    name_t *name = language->name_table.used_names + node->value.identifier;
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        identifier_t *ident = language->name_table.identifiers + elem;
        if(ident->type == IDENTIFIER_FUNCTION &&
           strncmp(ident->name, name->name, name->length) == 0) {
            node->value.identifier = elem;
            return LANGUAGE_SUCCESS;
        }
    }
    for(size_t elem = 0; elem < language->name_table.stack_size; elem++) {
        identifier_t *ident = language->name_table.identifiers + language->name_table.stack[language->name_table.stack_size - 1 - elem];
        if(strncmp(ident->name, name->name, name->length) == 0) {
            node->value.identifier = language->name_table.stack[language->name_table.stack_size - 1 - elem];
            return LANGUAGE_SUCCESS;
        }
    }
    return syntax_error(language, "Undefined reference to '%.*s'.\n", name->length, name->name);
}
