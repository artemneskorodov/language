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

void move_next_symbol(language_t *language);
char current_symbol(language_t *language);
char *input_position(language_t *language);
language_error_t get_word_length(language_t *language, size_t *length);
language_error_t skip_spaces(language_t *language);
language_error_t skip_comment(language_t *language);
void move_next_token(language_t *language);
language_node_t *token_position(language_t *language);
bool is_on_operation(language_t *language, operation_t opcode);
language_error_t syntax_error(language_t *language, const char *format, ...);
bool is_on_ident_type(language_t *language, identifier_type_t type);
bool is_on_type(language_t *language, node_type_t type);
language_error_t read_word(language_t *language);

language_error_t get_variable(language_t *language, language_node_t **output);
language_error_t get_operation(language_t *language, language_node_t **output);
language_error_t get_number(language_t *langauge, language_node_t **output);
language_error_t get_element(language_t *language, language_node_t **output);
language_error_t get_power(language_t *language, language_node_t **output);
language_error_t get_multiple(language_t *language, language_node_t **output);
language_error_t get_expression(language_t *language, language_node_t **output);
language_error_t get_function_call_params(language_t *language, language_node_t **output, identifier_t *identifier);
language_error_t get_function_call(language_t *language, language_node_t **output);
language_error_t get_asignment(language_t *language, language_node_t **output);
language_error_t get_new_variable(language_t *language, language_node_t **output);
language_error_t get_while(language_t *language, language_node_t **output);
language_error_t get_if(language_t *language, language_node_t **output);
language_error_t get_body(language_t *language, language_node_t **output);
language_error_t get_new_function_params (language_t *language, language_node_t **output, size_t *paramers_number);
language_error_t get_new_function(language_t *language, language_node_t **output);
language_error_t get_statement(language_t *language, language_node_t **output);
language_error_t get_global_statement(language_t *language, language_node_t **output);
language_error_t get_return(language_t *language, language_node_t **output);
language_error_t get_comparison(language_t *language, language_node_t **output);

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

    _RETURN_IF_ERROR(nodes_storage_ctor(language, language->input_size));
    _RETURN_IF_ERROR(name_table_ctor(language, language->input_size));
    language->input_position = language->input;
    language->frontend_info.position = language->nodes.nodes;
    language->frontend_info.current_line = 1;
    language->frontend_info.scope = 0;
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

    _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(OPERATION_PROGRAM_END), "", 0, NULL));
    return LANGUAGE_SUCCESS;
}

language_error_t read_word(language_t *language) {
    size_t length = 0;
    _RETURN_IF_ERROR(get_word_length(language, &length));
    bool found = false;
    for(size_t elem = 1; elem < (size_t)OPERATION_PROGRAM_END; elem++) {
        if(strncmp(KeyWords[elem].name, input_position(language), length) == 0) {
            _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(KeyWords[elem].code), input_position(language), length, NULL));
            found = true;
            if(KeyWords[elem].code == OPERATION_BODY_END) {
                _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_OPERATION, OPCODE(OPERATION_STATEMENT), input_position(language), length, NULL));
            }
            break;
        }
    }
    if(!found) {
        size_t name_table_index = PoisonIndex;
        _RETURN_IF_ERROR(name_table_find(language, input_position(language), length, &name_table_index));
        if(name_table_index != PoisonIndex) {
            found = true;
        }
        if(!found) {
            _RETURN_IF_ERROR(name_table_add(language, input_position(language), length, &name_table_index));
        }
        _RETURN_IF_ERROR(nodes_storage_add(language, NODE_TYPE_IDENTIFIER, IDENT(name_table_index), input_position(language), length, NULL));
    }
    language->input_position += length;
    return LANGUAGE_SUCCESS;
}

language_error_t parse_syntax(language_t *language) {
    language_node_t **current_node = &language->root;
    while(!is_on_operation(language, OPERATION_PROGRAM_END)) {
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_global_statement(language, &statement));
        if(!is_on_operation(language, OPERATION_STATEMENT)) {
            return syntax_error(language, "Expected ';' after EVERY statement.\n");
        }
        if(statement == NULL) {
            return syntax_error(language, "Unexpected ';'.\n");
        }
        *current_node = token_position(language);
        (*current_node)->left = statement;
        current_node = &(*current_node)->right;
        move_next_token(language);
    }

    return LANGUAGE_SUCCESS;
}

language_error_t get_global_statement(language_t *language, language_node_t **output) {
    if(is_on_operation(language, OPERATION_NEW_FUNC)) {
        return get_new_function(language, output);
    }
    if(is_on_operation(language, OPERATION_NEW_VAR)) {
        return get_new_variable(language, output);
    }
    return LANGUAGE_SUCCESS;
}

language_error_t get_statement(language_t *language, language_node_t **output) {
    if(is_on_operation(language, OPERATION_IF)) {
        return get_if(language, output);
    }
    if(is_on_operation(language, OPERATION_WHILE)) {
        return get_while(language, output);
    }
    if(is_on_operation(language, OPERATION_NEW_VAR)) {
        return get_new_variable(language, output);
    }
    if(is_on_operation(language, OPERATION_RETURN)) {
        return get_return(language, output);
    }
    if(is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        if(is_on_ident_type(language, IDENTIFIER_VARIABLE)) {
            return get_asignment(language, output);
        }
        else if(is_on_ident_type(language, IDENTIFIER_FUNCTION)) {
            return get_function_call(language, output);
        }
        return syntax_error(language, "Undefined identifier usage.\n");
    }

    return LANGUAGE_SUCCESS;
}

language_error_t get_return(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_operation(language, OPERATION_RETURN), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);

    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    return LANGUAGE_SUCCESS;
}

language_error_t get_new_function(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_operation(language, OPERATION_NEW_FUNC), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);

    if(!is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(language, "It is expected to see identifier after new function key word.\n");
    }
    (*output)->right = token_position(language);
    move_next_token(language);
    _RETURN_IF_ERROR(name_table_set_defined(language, (*output)->right->value.identifier, IDENTIFIER_FUNCTION, 0));

    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "It is expected to see '(' after identifier when initializing function.\n");
    }
    move_next_token(language);

    size_t parameters_number = 0;
    _RETURN_IF_ERROR(get_new_function_params(language, &(*output)->right->left, &parameters_number));
    language->name_table.identifiers[(*output)->right->value.identifier].parameters_number = parameters_number;

    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after function parameters when initializing.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_body(language, &(*output)->right->right));

    return LANGUAGE_SUCCESS;
}

language_error_t get_new_function_params(language_t *language, language_node_t **output, size_t *parameters_number) {
    if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language) - 1;
    (*output)->value.opcode = OPERATION_PARAM_LINKER;
    language_node_t *current_node = *output;
    language->frontend_info.scope++;
    while(true) {
        _RETURN_IF_ERROR(get_new_variable(language, &current_node->left));
        (*parameters_number)++;
        if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
            language->frontend_info.scope--;
            return LANGUAGE_SUCCESS;
        }
        if(!is_on_operation(language, OPERATION_PARAM_LINKER)) {
            return syntax_error(language, "It is expected to see ',' between function parameters.\n");
        }
        current_node->right = token_position(language);
        move_next_token(language);
        current_node = current_node->right;
    }
}

language_error_t get_body(language_t *language, language_node_t **output) {
    if(!is_on_operation(language, OPERATION_BODY_START)) {
        fprintf(stderr, "%d %d\n", token_position(language)->type, token_position(language)->value.opcode);
        return syntax_error(language, "It is expected to see some dead bodies here.\n");
    }
    language->frontend_info.scope++;
    move_next_token(language);
    while(true) {
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_statement(language, &statement));
        if(!is_on_operation(language, OPERATION_STATEMENT)) {
            return syntax_error(language, "It is expected to see ';' after EVERY statement.\n");
        }
        if(statement == NULL) {
            return syntax_error(language, "Unexpected ';'.\n");
        }
        *output = token_position(language);
        (*output)->left = statement;
        move_next_token(language);
        if(is_on_operation(language, OPERATION_BODY_END)) {
            language->frontend_info.scope--;
            move_next_token(language);
            return LANGUAGE_SUCCESS;
        }
        output = &(*output)->right;
    }
}

language_error_t get_if(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_operation(language, OPERATION_IF), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);

    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "It is expected to see expression in '()' after if key word.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_comparison(language, &(*output)->left));

    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after expression.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_body(language, &(*output)->right));

    return LANGUAGE_SUCCESS;
}

language_error_t get_while(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_operation(language, OPERATION_WHILE), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);

    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "It is expected to see '(' after while key word.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_comparison(language, &(*output)->left));

    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after while statement.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_body(language, &(*output)->right));

    return LANGUAGE_SUCCESS;
}

language_error_t get_new_variable(language_t *language, language_node_t **output) {
    if(!is_on_operation(language, OPERATION_NEW_VAR)) {
        return syntax_error(language, "It is expected to see new variable key word here.\n");
    }
    *output = token_position(language);
    move_next_token(language);

    if(!is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(language, "I want to see identifier after new variable key word.\n");
    }
    language_node_t *identifier = token_position(language);
    move_next_token(language);
    _RETURN_IF_ERROR(name_table_set_defined(language, identifier->value.identifier, IDENTIFIER_VARIABLE, language->frontend_info.scope));
    if(is_on_operation(language, OPERATION_ASIGNMENT)) {
        (*output)->right = token_position(language);
        move_next_token(language);

        (*output)->right->left = identifier;

        _RETURN_IF_ERROR(get_expression(language, &(*output)->right->right));
    }
    else {
        (*output)->right = identifier;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t get_asignment(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_type(language, NODE_TYPE_IDENTIFIER), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    language_node_t *lvalue = token_position(language);
    move_next_token(language);

    if(!is_on_operation(language, OPERATION_ASIGNMENT)) {
        return syntax_error(language, "Expected to see asignment here.\n");
    }
    *output = token_position(language);
    move_next_token(language);

    (*output)->left = lvalue;
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));

    return LANGUAGE_SUCCESS;
}

language_error_t get_function_call(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_ident_type(language, IDENTIFIER_FUNCTION), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    identifier_t *identifier = NULL;
    _RETURN_IF_ERROR(get_identifier(language, *output, &identifier));

    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "%.*s can not be used as a variable.\n", identifier->length, identifier->name);
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_function_call_params(language, &(*output)->right, identifier));
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after function parameters.\n");
    }
    move_next_token(language);
    return LANGUAGE_SUCCESS;
}

language_error_t get_function_call_params(language_t *language, language_node_t **output, identifier_t *identifier) {
    if(identifier->parameters_number == 0) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language) - 1;
    language_node_t *current_node = *output;
    current_node->value.opcode = OPERATION_PARAM_LINKER;
    for(size_t i = 0; i < identifier->parameters_number; i++) {
        _RETURN_IF_ERROR(get_expression(language, &current_node->left));
        if(!is_on_operation(language, OPERATION_PARAM_LINKER) && i + 1 != identifier->parameters_number) {
            return syntax_error(language, "Function '%.*s' expected to see %llu parameters.\n", identifier->length, identifier->name, identifier->parameters_number);
        }
        current_node->right = token_position(language);
        current_node = token_position(language);
        if(i + 1 != identifier->parameters_number) {
            move_next_token(language);
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t get_comparison(language_t *language, language_node_t **output) {
    language_node_t *left_side = NULL;
    _RETURN_IF_ERROR(get_expression(language, &left_side));
    if(!is_on_operation(language, OPERATION_BIGGER) && !is_on_operation(language, OPERATION_SMALLER)) {
        *output = left_side;
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language);
    move_next_token(language);
    (*output)->left = left_side;

    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    return LANGUAGE_SUCCESS;
}

language_error_t get_expression(language_t *language, language_node_t **output) {
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_multiple(language, &root));
    while(is_on_operation(language, OPERATION_ADD) || is_on_operation(language, OPERATION_SUB)) {
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        _RETURN_IF_ERROR(get_multiple(language, &root->right));
    }
    *output = root;
    return LANGUAGE_SUCCESS;
}

language_error_t get_multiple(language_t *language, language_node_t **output) {
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_power(language, &root));
    while(is_on_operation(language, OPERATION_MUL) || is_on_operation(language, OPERATION_DIV)) {
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        _RETURN_IF_ERROR(get_power(language, &root->right));
    }
    *output = root;
    return LANGUAGE_SUCCESS;
}

language_error_t get_power(language_t *language, language_node_t **output) {
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_element(language, &root));
    while(is_on_operation(language, OPERATION_POW)) {
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        _RETURN_IF_ERROR(get_element(language, &root->right));
    }
    *output = root;
    return LANGUAGE_SUCCESS;
}

language_error_t get_element(language_t *language, language_node_t **output) {
    if(is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        move_next_token(language);
        _RETURN_IF_ERROR(get_expression(language, output));
        if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
            return syntax_error(language, "Unclosed bracket.\n");
        }
        move_next_token(language);
        return LANGUAGE_SUCCESS;
    }
    if(is_on_type(language, NODE_TYPE_NUMBER)) {
        return get_number(language, output);
    }
    if(is_on_type(language, NODE_TYPE_OPERATION) &&
       KeyWords[token_position(language)->value.opcode].is_expression_element) {
        return get_operation(language, output);
    }
    if(is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        if(is_on_ident_type(language, IDENTIFIER_FUNCTION)) {
            return get_function_call(language, output);
        }
        if(is_on_ident_type(language, IDENTIFIER_VARIABLE)) {
            return get_variable(language, output);
        }
        print_error("Unknown identifier type.\n");
        return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t get_number(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_type(language, NODE_TYPE_NUMBER), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    return LANGUAGE_SUCCESS;
}

language_error_t get_operation(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_type(language, NODE_TYPE_OPERATION), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    if(!KeyWords[token_position(language)->value.opcode].is_expression_element) {
        return syntax_error(language, "Expected only math operations in expression.\n");
    }
    fprintf(stderr, "%d %d\n", language->frontend_info.position->type, language->frontend_info.position->value.opcode);
    *output = token_position(language);
    move_next_token(language);
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        fprintf(stderr, "%d %d\n", language->frontend_info.position->type, language->frontend_info.position->value.opcode);
        return syntax_error(language, "Operation '%s' expected to have parameter in '()'", KeyWords[(*output)->value.opcode]);
    }
    move_next_token(language);
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "I supposed to see ')'");
    }
    move_next_token(language);
    return LANGUAGE_SUCCESS;
}

language_error_t get_variable(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_ident_type(language, IDENTIFIER_VARIABLE), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    return LANGUAGE_SUCCESS;
}


void move_next_token(language_t *language) {
    language->frontend_info.position++;
}

language_node_t *token_position(language_t *language) {
    return language->frontend_info.position;
}

bool is_on_operation(language_t *language, operation_t opcode) {
    if(language->frontend_info.position->type == NODE_TYPE_OPERATION &&
       language->frontend_info.position->value.opcode == opcode) {
        return true;
    }
    return false;
}

language_error_t syntax_error(language_t *language, const char *format, ...) {
    va_list args;
    va_start(args, format);
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                  "%s:%llu", language->input_file, token_position(language)->source_info.line);
    color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, " ~ ");
    color_vprintf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, format, args);
    va_end(args);
    return LANGUAGE_SYNTAX_ERROR;
}

language_error_t frontend_dtor(language_t *language) {
    dump_dtor(language);
    free(language->input);
    _RETURN_IF_ERROR(nodes_storage_dtor(language));
    memset(language, 0, sizeof(*language));
    return LANGUAGE_SUCCESS;
}

bool is_on_ident_type(language_t *language, identifier_type_t type) {
    if(token_position(language)->type == NODE_TYPE_IDENTIFIER &&
       language->name_table.identifiers[token_position(language)->value.identifier].type == type) {
        return true;
    }
    return false;
}

bool is_on_type(language_t *language, node_type_t type) {
    if(token_position(language)->type == type) {
        return true;
    }
    return false;
}

void move_next_symbol(language_t *language) {
    language->input_position++;
}

char current_symbol(language_t *language) {
    return *language->input_position;
}

char *input_position(language_t *language) {
    return language->input_position;
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

    char *position = input_position(language);
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
