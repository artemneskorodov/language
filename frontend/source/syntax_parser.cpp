#include <string.h>

#include "language.h"
#include "syntax_parser.h"
#include "name_table.h"
#include "frontend_utils.h"
#include "custom_assert.h"
#include "colors.h"

//==========================================================================================================================//

static language_error_t set_reference           (language_t       *language,
                                                 language_node_t  *node);

static language_error_t get_variable            (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_operation           (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_number              (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_element             (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_power               (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_multiple            (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_expression          (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_function_call_params(language_t       *language,
                                                 language_node_t **output,
                                                 identifier_t     *identifier);

static language_error_t get_function_call       (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_assignment          (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_new_variable        (language_t       *language,
                                                 language_node_t **output,
                                                 identifier_type_t type);

static language_error_t get_while               (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_if                  (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_body                (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_new_function_params (language_t       *language,
                                                 language_node_t **output,
                                                 size_t           *params_number);

static language_error_t get_new_function        (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_statement           (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_global_statement    (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_return              (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_comparison          (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_in                  (language_t       *language,
                                                 language_node_t **output);

static language_error_t get_out                 (language_t       *language,
                                                 language_node_t **output);


//====================================================================================//

language_error_t parse_syntax(language_t *language) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_ctor(language, language->input_size));
    language_node_t **current_node = &language->root;
    //--------------------------------------------------------------------------------//
    while(!is_on_operation(language, OPERATION_PROGRAM_END)) {
        //----------------------------------------------------------------------------//
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_global_statement(language, &statement));
        //----------------------------------------------------------------------------//
        if(!is_on_operation(language, OPERATION_STATEMENT)) {
            return syntax_error(language, "Expected ';' after EVERY statement.\n");
        }
        //----------------------------------------------------------------------------//
        if(statement == NULL) {
            return syntax_error(language, "Unexpected ';'.\n");
        }
        *current_node = token_position(language);
        move_next_token(language);
        //----------------------------------------------------------------------------//
        (*current_node)->left = statement;
        current_node = &(*current_node)->right;
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_dtor(language));
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_global_statement(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_NEW_FUNC)) {
        return get_new_function(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_NEW_VAR)) {
        return get_new_variable(language, output, IDENTIFIER_GLOBAL_VAR);
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_statement(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_OUT)) {
        return get_out(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_IF)) {
        return get_if(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_WHILE)) {
        return get_while(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_NEW_VAR)) {
        return get_new_variable(language, output, IDENTIFIER_LOCAL_VAR);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_RETURN)) {
        return get_return(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        _RETURN_IF_ERROR(set_reference(language, token_position(language)));
        //----------------------------------------------------------------------------//
        if(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) ||
           is_on_ident_type(language, IDENTIFIER_LOCAL_VAR)) {
            return get_assignment(language, output);
        }
        //----------------------------------------------------------------------------//
        else if(is_on_ident_type(language, IDENTIFIER_FUNCTION)) {
            return get_function_call(language, output);
        }
        //----------------------------------------------------------------------------//
        return syntax_error(language, "Undefined identifier usage.\n");
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_return(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_RETURN),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_new_function(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_NEW_FUNC),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(language,
                            "It is expected to see identifier "
                            "after new function key word.\n");
    }
    language_node_t *ident = token_position(language);
    _RETURN_IF_ERROR(move_next_token(language));
    (*output)->right = ident;
    //--------------------------------------------------------------------------------//
    const char *name = language->name_table.used_names[ident->value.identifier].name;
    size_t length    = language->name_table.used_names[ident->value.identifier].length;
    _RETURN_IF_ERROR(name_table_add(language,
                                    name,
                                    length,
                                    &ident->value.identifier,
                                    IDENTIFIER_FUNCTION));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see '(' "
                            "after identifier when initializing function.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    size_t old_locals = language->frontend_info.used_locals;
    language->frontend_info.used_locals = 0;
    //--------------------------------------------------------------------------------//
    size_t params_number = 0;
    _RETURN_IF_ERROR(get_new_function_params(language, &ident->left, &params_number));
    language->name_table.identifiers[ident->value.identifier].parameters_number = params_number;//TODO move to function
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see ')' after "
                            "function parameters when initializing.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(language, &(*output)->right->right));
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_remove(language,
                                            language->frontend_info.used_locals));
    language->frontend_info.used_locals = old_locals;
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_new_function_params(language_t       *language,
                                         language_node_t **output,
                                         size_t           *params_number) {
    _C_ASSERT(language      != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output        != NULL, return LANGUAGE_NULL_OUTPUT);
    _C_ASSERT(params_number != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language) - 1;
    (*output)->value.opcode = OPERATION_PARAM_LINKER;
    language_node_t *current_node = *output;
    //--------------------------------------------------------------------------------//
    while(true) {
        _RETURN_IF_ERROR(get_new_variable(language,
                                          &current_node->left,
                                          IDENTIFIER_LOCAL_VAR));
        (*params_number)++;
        //----------------------------------------------------------------------------//
        if(is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
            return LANGUAGE_SUCCESS;
        }
        //----------------------------------------------------------------------------//
        if(!is_on_operation(language, OPERATION_PARAM_LINKER)) {
            return syntax_error(language,
                                "It is expected to see ',' "
                                "between function parameters.\n");
        }
        //----------------------------------------------------------------------------//
        current_node->right = token_position(language);
        current_node = current_node->right;
        move_next_token(language);
        //----------------------------------------------------------------------------//
    }
}

//====================================================================================//

language_error_t get_body(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_BODY_START)) {
        return syntax_error(language, "It is expected to see some dead bodies here.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    size_t old_locals = language->frontend_info.used_locals;
    language->frontend_info.used_locals = 0;
    //--------------------------------------------------------------------------------//
    while(true) {
        //----------------------------------------------------------------------------//
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_statement(language, &statement));
        //----------------------------------------------------------------------------//
        if(!is_on_operation(language, OPERATION_STATEMENT)) {
            return syntax_error(language,
                                "It is expected to see ';' "
                                "after EVERY statement.\n");
        }
        //----------------------------------------------------------------------------//
        if(statement == NULL) {
            return syntax_error(language, "Unexpected ';'.\n");
        }
        //----------------------------------------------------------------------------//
        *output = token_position(language);
        (*output)->left = statement;
        move_next_token(language);
        //----------------------------------------------------------------------------//
        if(is_on_operation(language, OPERATION_BODY_END)) {
            move_next_token(language);
            _RETURN_IF_ERROR(variables_stack_remove(language,
                                                    language->frontend_info.used_locals));
            language->frontend_info.used_locals = old_locals;
            return LANGUAGE_SUCCESS;
        }
        //----------------------------------------------------------------------------//
        output = &(*output)->right;
        //----------------------------------------------------------------------------//
    }
}

//====================================================================================//

language_error_t get_if(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_IF), return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see expression "
                            "in '()' after if key word.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_comparison(language, &(*output)->left));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language, "It is expected to see ')' after expression.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_while(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_WHILE),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see '(' "
                            "after while key word.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_comparison(language, &(*output)->left));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see ')' "
                            "after while statement.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_new_variable(language_t       *language,
                                  language_node_t **output,
                                  identifier_type_t type) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_NEW_VAR)) {
        return syntax_error(language,
                            "It is expected to see new "
                            "variable key word here.\n");
    }
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(language,
                            "I want to see identifier "
                            "after new variable key word.\n");
    }
    language_node_t *identifier = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    size_t *value      = &identifier->value.identifier;
    const char *name   = language->name_table.used_names[*value].name;
    size_t      length = language->name_table.used_names[*value].length;
    _RETURN_IF_ERROR(name_table_add(language, name, length, value, type));
    _RETURN_IF_ERROR(variables_stack_push(language, *value));
    language->frontend_info.used_locals++;
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_ASSIGNMENT)) {
        (*output)->right = token_position(language);
        move_next_token(language);
        (*output)->right->left = identifier;
        if(is_on_operation(language, OPERATION_IN)) {
            _RETURN_IF_ERROR(get_in(language, &(*output)->right->right));
        }
        else {
            _RETURN_IF_ERROR(get_expression(language, &(*output)->right->right));
        }
    }
    //--------------------------------------------------------------------------------//
    else {
        (*output)->right = identifier;
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_assignment(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_type(language, NODE_TYPE_IDENTIFIER),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    language_node_t *lvalue = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_ASSIGNMENT)) {
        return syntax_error(language, "Expected to see asignment here.\n");
    }
    *output = token_position(language);
    move_next_token(language);
    (*output)->left = lvalue;
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_IN)) {
        _RETURN_IF_ERROR(get_in(language, &(*output)->right));
        return LANGUAGE_SUCCESS;
    }
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_function_call(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_ident_type(language, IDENTIFIER_FUNCTION),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    identifier_t *identifier = language->name_table.identifiers +
                               (*output)->value.identifier;
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "'%.*s' can not be used as a variable.\n",
                            identifier->length,
                            identifier->name);
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_function_call_params(language,
                                              &(*output)->right,
                                              identifier));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language,
                            "It is expected to see ')' "
                            "after function parameters.\n");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_function_call_params(language_t       *language,
                                          language_node_t **output,
                                          identifier_t     *identifier) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(identifier->parameters_number == 0) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(language) - 1;
    //--------------------------------------------------------------------------------//
    language_node_t *current_node = *output;
    current_node->value.opcode = OPERATION_PARAM_LINKER;
    for(size_t i = 0; i < identifier->parameters_number; i++) {
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_expression(language, &current_node->left));
        //----------------------------------------------------------------------------//
        if(!is_on_operation(language, OPERATION_PARAM_LINKER) &&
           identifier->parameters_number != i + 1) {
            return syntax_error(language,
                                "Function '%.*s' expected to see %llu parameters.\n",
                                identifier->length,
                                identifier->name,
                                identifier->parameters_number);
        }
        //----------------------------------------------------------------------------//
        current_node->right = token_position(language);
        current_node = current_node->right;
        if(i + 1 != identifier->parameters_number) {
            move_next_token(language);
        }
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_comparison(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    language_node_t *left_side = NULL;
    _RETURN_IF_ERROR(get_expression(language, &left_side));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_BIGGER) &&
       !is_on_operation(language, OPERATION_SMALLER)) {
        *output = left_side;
        return LANGUAGE_SUCCESS;
    }
    //--------------------------------------------------------------------------------//
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    (*output)->left = left_side;
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_expression(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_multiple(language, &root));
    //--------------------------------------------------------------------------------//
    while(is_on_operation(language, OPERATION_ADD) ||
          is_on_operation(language, OPERATION_SUB)) {
        //----------------------------------------------------------------------------//
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_multiple(language, &root->right));
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_multiple(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_power(language, &root));
    //--------------------------------------------------------------------------------//
    while(is_on_operation(language, OPERATION_MUL) ||
          is_on_operation(language, OPERATION_DIV)) {
        //----------------------------------------------------------------------------//
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_power(language, &root->right));
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_power(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_element(language, &root));
    //--------------------------------------------------------------------------------//
    while(is_on_operation(language, OPERATION_POW)) {
        //----------------------------------------------------------------------------//
        language_node_t *new_root = token_position(language);
        new_root->left = root;
        root = new_root;
        move_next_token(language);
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_element(language, &root->right));
        //----------------------------------------------------------------------------//
    }
    //--------------------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_element(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        move_next_token(language);
        //----------------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_expression(language, output));
        //----------------------------------------------------------------------------//
        if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
            return syntax_error(language, "Unclosed bracket.\n");
        }
        //----------------------------------------------------------------------------//
        move_next_token(language);
        return LANGUAGE_SUCCESS;
    }
    //--------------------------------------------------------------------------------//
    if(is_on_type(language, NODE_TYPE_NUMBER)) {
        return get_number(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_type(language, NODE_TYPE_OPERATION) &&
       KeyWords[token_position(language)->value.opcode].is_expression_element) {
        return get_operation(language, output);
    }
    //--------------------------------------------------------------------------------//
    if(is_on_type(language, NODE_TYPE_IDENTIFIER)) {
        _RETURN_IF_ERROR(set_reference(language, token_position(language)));
        //----------------------------------------------------------------------------//
        if(is_on_ident_type(language, IDENTIFIER_FUNCTION)) {
            return get_function_call(language, output);
        }
        //----------------------------------------------------------------------------//
        if(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) ||
           is_on_ident_type(language, IDENTIFIER_LOCAL_VAR)) {
            return get_variable(language, output);
        }
        //----------------------------------------------------------------------------//
        print_error("Unknown identifier type.\n");
        return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
    }
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_number(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_type(language, NODE_TYPE_NUMBER),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_operation(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_type(language, NODE_TYPE_OPERATION),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    if(!KeyWords[token_position(language)->value.opcode].is_expression_element) {
        return syntax_error(language, "Expected only math operations in expression.\n");
    }
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "Operation '%s' expected to have parameter in '()'",
                            KeyWords[(*output)->value.opcode]);
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language, "I supposed to see ')'");
    }
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_variable(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_ident_type(language, IDENTIFIER_GLOBAL_VAR) ||
              is_on_ident_type(language, IDENTIFIER_LOCAL_VAR),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    move_next_token(language);
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_in(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_IN),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    _RETURN_IF_ERROR(move_next_token(language));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_out(language_t *language, language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output   != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(language, OPERATION_OUT),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(language);
    _RETURN_IF_ERROR(move_next_token(language));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_OPEN_BRACKET)) {
        return syntax_error(language,
                            "Expected to see ( after out command.\n");
    }
    _RETURN_IF_ERROR(move_next_token(language));
    //--------------------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(language, &(*output)->right));
    //--------------------------------------------------------------------------------//
    if(!is_on_operation(language, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(language,
                            "Expected to see ( after out command.\n");
    }
    _RETURN_IF_ERROR(move_next_token(language));
    //--------------------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t set_reference(language_t *language, language_node_t *node) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node     != NULL, return LANGUAGE_NULL_OUTPUT);
    //--------------------------------------------------------------------------------//
    if(node->type != NODE_TYPE_IDENTIFIER) {
        print_error("Node type is not identifier. Unable to get reference.\n");
        return LANGUAGE_SYNTAX_UNEXPECTED_CALL;
    }
    //--------------------------------------------------------------------------------//
    name_t *name = language->name_table.used_names + node->value.identifier;
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        identifier_t *ident = language->name_table.identifiers + elem;
        if((ident->type == IDENTIFIER_FUNCTION || ident->type == IDENTIFIER_GLOBAL_VAR) &&
           ident->length == name->length &&
           strncmp(ident->name, name->name, name->length) == 0) {
            node->value.identifier = elem;
            return LANGUAGE_SUCCESS;
        }
    }
    //--------------------------------------------------------------------------------//
    for(size_t elem = language->name_table.stack_size; elem > 0; elem--) {
        size_t nt_index = language->name_table.stack[elem - 1];
        identifier_t *ident = language->name_table.identifiers + nt_index;
        if(ident->length == name->length &&
           strncmp(ident->name, name->name, name->length) == 0) {
            node->value.identifier = nt_index;
            return LANGUAGE_SUCCESS;
        }
    }
    //--------------------------------------------------------------------------------//
    return syntax_error(language,
                        "Undefined reference to '%.*s'.\n",
                        name->length,
                        name->name);
}

//====================================================================================//
