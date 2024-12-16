#include "language.h"
#include "syntax_parser.h"
#include "name_table.h"
#include "frontend_utils.h"
#include "custom_assert.h"
#include "colors.h"

language_error_t set_reference(language_t *language, language_node_t *node);

language_error_t get_variable(language_t *language, language_node_t **output);
language_error_t get_operation(language_t *language, language_node_t **output);
language_error_t get_number(language_t *langauge, language_node_t **output);
language_error_t get_element(language_t *language, language_node_t **output);
language_error_t get_power(language_t *language, language_node_t **output);
language_error_t get_multiple(language_t *language, language_node_t **output);
language_error_t get_expression(language_t *language, language_node_t **output);
language_error_t get_function_call_params(language_t *language, language_node_t **output, identifier_t *identifier);
language_error_t get_function_call(language_t *language, language_node_t **output);
language_error_t get_assignment(language_t *language, language_node_t **output);
language_error_t get_new_variable(language_t *language, language_node_t **output, identifier_type_t type);
language_error_t get_while(language_t *language, language_node_t **output);
language_error_t get_if(language_t *language, language_node_t **output);
language_error_t get_body(language_t *language, language_node_t **output);
language_error_t get_new_function_params (language_t *language, language_node_t **output, size_t *paramers_number);
language_error_t get_new_function(language_t *language, language_node_t **output);
language_error_t get_statement(language_t *language, language_node_t **output);
language_error_t get_global_statement(language_t *language, language_node_t **output);
language_error_t get_return(language_t *language, language_node_t **output);
language_error_t get_comparison(language_t *language, language_node_t **output);

language_error_t parse_syntax(language_t *language) {
    _RETURN_IF_ERROR(variables_stack_ctor(language, language->input_size));
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

    _RETURN_IF_ERROR(variables_stack_dtor(language));
    return LANGUAGE_SUCCESS;
}

language_error_t get_global_statement(language_t *language, language_node_t **output) {
    if(is_on_operation(language, OPERATION_NEW_FUNC)) {
        return get_new_function(language, output);
    }
    if(is_on_operation(language, OPERATION_NEW_VAR)) {
        return get_new_variable(language, output, IDENTIFIER_GLOBAL_VAR);
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
        return get_new_variable(language, output, IDENTIFIER_LOCAL_VAR);
    }
    if(is_on_operation(language, OPERATION_RETURN)) {
        return get_return(language, output);
    }
    if(is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        _RETURN_IF_ERROR(set_reference(language, token_position(language)));

        if(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) || is_on_ident_type(language, IDENTIFIER_LOCAL_VAR)) {
            return get_assignment(language, output);
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

    _RETURN_IF_ERROR(name_table_add(language, language->name_table.used_names[token_position(language)->value.identifier].name, language->name_table.used_names[token_position(language)->value.identifier].length, &token_position(language)->value.identifier, IDENTIFIER_FUNCTION));

    (*output)->right = token_position(language);
    move_next_token(language);

    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "It is expected to see '(' after identifier when initializing function.\n");
    }
    move_next_token(language);

    size_t old_locals = language->frontend_info.used_locals;
    language->frontend_info.used_locals = 0;

    size_t parameters_number = 0;
    _RETURN_IF_ERROR(get_new_function_params(language, &(*output)->right->left, &parameters_number));
    language->name_table.identifiers[(*output)->right->value.identifier].parameters_number = parameters_number;

    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after function parameters when initializing.\n");
    }
    move_next_token(language);

    _RETURN_IF_ERROR(get_body(language, &(*output)->right->right));

    _RETURN_IF_ERROR(variables_stack_remove(language, language->frontend_info.used_locals));
    language->frontend_info.used_locals = old_locals;

    return LANGUAGE_SUCCESS;
}

language_error_t get_new_function_params(language_t *language, language_node_t **output, size_t *parameters_number) {
    if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language) - 1;
    (*output)->value.opcode = OPERATION_PARAM_LINKER;
    language_node_t *current_node = *output;
    while(true) {
        _RETURN_IF_ERROR(get_new_variable(language, &current_node->left, IDENTIFIER_LOCAL_VAR));
        (*parameters_number)++;
        if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
            return LANGUAGE_SUCCESS;
        }
        if(!is_on_operation(language, OPERATION_PARAM_LINKER)) {
            fprintf(stderr, "%d %d\n", token_position(language)->type, token_position(language)->value.opcode);
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
    size_t old_locals = language->frontend_info.used_locals;
    language->frontend_info.used_locals = 0;
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
            move_next_token(language);
            _RETURN_IF_ERROR(variables_stack_remove(language, language->frontend_info.used_locals));
            language->frontend_info.used_locals = old_locals;
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

language_error_t get_new_variable(language_t *language, language_node_t **output, identifier_type_t type) {
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

    _RETURN_IF_ERROR(name_table_add(language, language->name_table.used_names[identifier->value.identifier].name, language->name_table.used_names[identifier->value.identifier].length, &identifier->value.identifier, type));
    _RETURN_IF_ERROR(variables_stack_push(language, identifier->value.identifier));
    language->frontend_info.used_locals++;

    if(is_on_operation(language, OPERATION_ASSIGNMENT)) {
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

language_error_t get_assignment(language_t *language, language_node_t **output) {
    _C_ASSERT(is_on_type(language, NODE_TYPE_IDENTIFIER), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    language_node_t *lvalue = token_position(language);
    move_next_token(language);

    if(!is_on_operation(language, OPERATION_ASSIGNMENT)) {
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
        return syntax_error(language, "'%.*s' can not be used as a variable.\n", identifier->length, identifier->name);
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
    if(!is_on_operation(language, OPERATION_BIGGER) &&
       !is_on_operation(language, OPERATION_SMALLER)) {
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
        _RETURN_IF_ERROR(set_reference(language, token_position(language)));

        if(is_on_ident_type(language, IDENTIFIER_FUNCTION)) {
            return get_function_call(language, output);
        }
        if(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) || is_on_ident_type(language, IDENTIFIER_LOCAL_VAR)) {
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
    _C_ASSERT(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) || is_on_ident_type(language, IDENTIFIER_LOCAL_VAR), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    language_node_t *node = token_position(language);
    move_next_token(language);

    *output = node;
    return LANGUAGE_SUCCESS;
}
