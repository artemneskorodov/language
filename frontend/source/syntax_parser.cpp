#include <string.h>

#include "language.h"
#include "syntax_parser.h"
#include "name_table.h"
#include "frontend_utils.h"
#include "custom_assert.h"
#include "colors.h"
#include "nodes_dsl.h"

//==========================================================================================================================//

static language_error_t set_reference           (language_t       *ctx,
                                                 language_node_t  *node);

static language_error_t get_variable            (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_operation           (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_number              (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_element             (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_power               (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_multiple            (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_expression          (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_function_call_params(language_t       *ctx,
                                                 language_node_t **output,
                                                 identifier_t     *identifier);

static language_error_t get_function_call       (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_assignment          (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_new_variable        (language_t       *ctx,
                                                 language_node_t **output,
                                                 bool              is_global);

static language_error_t get_while               (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_if                  (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_body                (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_new_function_params (language_t       *ctx,
                                                 language_node_t **output,
                                                 size_t           *params_number);

static language_error_t get_new_function        (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_statement           (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_global_statement    (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_return              (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_comparison          (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_in                  (language_t       *ctx,
                                                 language_node_t **output);

static language_error_t get_out                 (language_t       *ctx,
                                                 language_node_t **output);


//===========================================================================//

language_error_t parse_syntax(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_ctor(ctx, ctx->input_size));
    language_node_t **current_node = &ctx->root;
    //-----------------------------------------------------------------------//
    while(!is_on_operation(ctx, OPERATION_PROGRAM_END)) {
        //-------------------------------------------------------------------//
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_global_statement(ctx, &statement));
        //-------------------------------------------------------------------//
        if(!is_on_operation(ctx, OPERATION_STATEMENT)) {
            return syntax_error(ctx, "Expected ';' after statements.\n");
        }
        //-------------------------------------------------------------------//
        if(statement == NULL) {
            return syntax_error(ctx, "Unexpected ';'.\n");
        }
        *current_node = token_position(ctx);
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        (*current_node)->left = statement;
        current_node = &(*current_node)->right;
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_dtor(ctx));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_global_statement(language_t       *ctx,
                                      language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_NEW_FUNC)) {
        return get_new_function(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_NEW_VAR)) {
        _RETURN_IF_ERROR(get_new_variable(ctx, output, true));
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_statement(language_t       *ctx,
                               language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_OUT)) {
        return get_out(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_IF)) {
        return get_if(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_WHILE)) {
        return get_while(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_NEW_VAR)) {
        return get_new_variable(ctx, output, false);
    }
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_RETURN)) {
        return get_return(ctx, output);
    }
    if(is_on_operation(ctx, OPERATION_IN)) {
        return get_in(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_type(ctx, NODE_TYPE_IDENTIFIER)) {
        _RETURN_IF_ERROR(set_reference(ctx, token_position(ctx)));
        //-------------------------------------------------------------------//
        if(is_on_ident_type(ctx, IDENTIFIER_VARIABLE)) {
            return get_assignment(ctx, output);
        }
        //-------------------------------------------------------------------//
        else if(is_on_ident_type(ctx, IDENTIFIER_FUNCTION)) {
            return syntax_error(ctx, "CHECK FUCKING RETURN VALUES. "
                                     "(sry i didn't want to implement this)");
        }
        //-------------------------------------------------------------------//
        return syntax_error(ctx, "Undefined identifier usage.\n");
    }
    //----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_return(language_t       *ctx,
                            language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_RETURN),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(ctx, &(*output)->left));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_new_function(language_t       *ctx,
                                  language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_NEW_FUNC),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_type(ctx, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(ctx,
                            "It is expected to see identifier "
                            "after new function key word.\n");
    }
    language_node_t *ident = token_position(ctx);
    _RETURN_IF_ERROR(move_next_token(ctx));
    (*output)->left = ident;
    //-----------------------------------------------------------------------//
    const char *name   = ctx->name_table.used_names[ident->value.identifier].name;
    size_t      length = ctx->name_table.used_names[ident->value.identifier].length;
    _RETURN_IF_ERROR(name_table_add(ctx,
                                    name,
                                    length,
                                    &ident->value.identifier,
                                    IDENTIFIER_FUNCTION));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see '(' "
                            "after identifier when initializing function.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    size_t old_locals = ctx->frontend_info.used_locals;
    ctx->frontend_info.used_locals = 0;
    //-----------------------------------------------------------------------//
    size_t params_number = 0;
    _RETURN_IF_ERROR(get_new_function_params(ctx, &ident->left, &params_number));
    ctx->name_table.identifiers[ident->value.identifier].parameters_number = params_number;
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see ')' after "
                            "function parameters when initializing.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(ctx, &ident->right));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_remove(ctx,
                                            ctx->frontend_info.used_locals));
    ctx->frontend_info.used_locals = old_locals;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_new_function_params(language_t       *ctx,
                                         language_node_t **output,
                                         size_t           *params_number) {
    _C_ASSERT(ctx           != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output        != NULL, return LANGUAGE_NULL_OUTPUT);
    _C_ASSERT(params_number != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(ctx) - 1;
    (*output)->value.opcode = OPERATION_PARAM_LINKER;
    language_node_t *current_node = *output;
    //-----------------------------------------------------------------------//
    while(true) {
        _RETURN_IF_ERROR(get_new_variable(ctx,
                                          &current_node->left,
                                          false));
        (*params_number)++;
        //-------------------------------------------------------------------//
        if(is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
            return LANGUAGE_SUCCESS;
        }
        //-------------------------------------------------------------------//
        if(!is_on_operation(ctx, OPERATION_PARAM_LINKER)) {
            return syntax_error(ctx,
                                "It is expected to see ',' "
                                "between function parameters.\n");
        }
        //-------------------------------------------------------------------//
        current_node->right = token_position(ctx);
        current_node = current_node->right;
        move_next_token(ctx);
        //-------------------------------------------------------------------//
    }
}

//===========================================================================//

language_error_t get_body(language_t       *ctx,
                          language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_BODY_START)) {
        return syntax_error(ctx,
                            "It is expected to see some dead bodies here.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    size_t old_locals = ctx->frontend_info.used_locals;
    ctx->frontend_info.used_locals = 0;
    //-----------------------------------------------------------------------//
    while(true) {
        //-------------------------------------------------------------------//
        language_node_t *statement = NULL;
        _RETURN_IF_ERROR(get_statement(ctx, &statement));
        //-------------------------------------------------------------------//
        if(!is_on_operation(ctx, OPERATION_STATEMENT)) {
            return syntax_error(ctx,
                                "It is expected to see ';' "
                                "after EVERY statement.\n");
        }
        //-------------------------------------------------------------------//
        if(statement == NULL) {
            return syntax_error(ctx, "Unexpected ';'.\n");
        }
        //-------------------------------------------------------------------//
        *output = token_position(ctx);
        (*output)->left = statement;
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        if(is_on_operation(ctx, OPERATION_BODY_END)) {
            move_next_token(ctx);
            _RETURN_IF_ERROR(variables_stack_remove(ctx,
                                                    ctx->frontend_info.used_locals));
            ctx->frontend_info.used_locals = old_locals;
            return LANGUAGE_SUCCESS;
        }
        //-------------------------------------------------------------------//
        output = &(*output)->right;
        //-------------------------------------------------------------------//
    }
}

//===========================================================================//

language_error_t get_if(language_t       *ctx,
                        language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_IF),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see expression "
                            "in '()' after if key word.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_comparison(ctx, &(*output)->left));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see ')' after expression.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(ctx, &(*output)->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_while(language_t       *ctx,
                           language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_WHILE),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see '(' "
                            "after while key word.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_comparison(ctx, &(*output)->left));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see ')' "
                            "after while statement.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_body(ctx, &(*output)->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_new_variable(language_t       *ctx,
                                  language_node_t **output,
                                  bool              is_global) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_NEW_VAR)) {
        return syntax_error(ctx,
                            "It is expected to see new "
                            "variable key word here.\n");
    }
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_type(ctx, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(ctx,
                            "I want to see identifier "
                            "after new variable key word.\n");
    }
    language_node_t *ident = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    size_t     *value  = &ident->value.identifier;
    const char *name   = ctx->name_table.used_names[*value].name;
    size_t      length = ctx->name_table.used_names[*value].length;
    _RETURN_IF_ERROR(name_table_add(ctx,
                                    name,
                                    length,
                                    value,
                                    IDENTIFIER_VARIABLE));
    _RETURN_IF_ERROR(variables_stack_push(ctx, *value));
    identifier_t *nt_info = ctx->name_table.identifiers + (*value);
    nt_info->is_global = is_global;
    ctx->frontend_info.used_locals++;
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_ASSIGNMENT)) {
        (*output)->left = token_position(ctx);
        move_next_token(ctx);
        (*output)->left->left = ident;
        _RETURN_IF_ERROR(get_expression(ctx, &(*output)->left->right));
    }
    //-----------------------------------------------------------------------//
    else {
        (*output)->left = ident;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_assignment(language_t       *ctx,
                                language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_type(ctx, NODE_TYPE_IDENTIFIER),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    language_node_t *lvalue = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_ASSIGNMENT)) {
        return syntax_error(ctx, "Expected to see assignment here.\n");
    }
    *output = token_position(ctx);
    move_next_token(ctx);
    (*output)->left = lvalue;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(ctx, &(*output)->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_function_call(language_t       *ctx,
                                   language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_ident_type(ctx, IDENTIFIER_FUNCTION),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                       NODE_TYPE_OPERATION,
                                       OPCODE(OPERATION_CALL),
                                       "", 0,
                                       output));
    (*output)->left = token_position(ctx);
    move_next_token(ctx);
    identifier_t *ident = ctx->name_table.identifiers +
                          (*output)->left->value.identifier;
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "'%.*s' can not be used as a variable.\n",
                            ident->length,
                            ident->name);
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_function_call_params(ctx,
                                              &(*output)->left->left,
                                              ident));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx,
                            "It is expected to see ')' "
                            "after function parameters.\n");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_function_call_params(language_t       *ctx,
                                          language_node_t **output,
                                          identifier_t     *ident) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(ident->parameters_number == 0) {
        return LANGUAGE_SUCCESS;
    }
    *output = token_position(ctx) - 1;
    //-----------------------------------------------------------------------//
    language_node_t *current_node = *output;
    current_node->value.opcode = OPERATION_PARAM_LINKER;
    for(size_t i = 0; i < ident->parameters_number; i++) {
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_expression(ctx, &current_node->left));
        //-------------------------------------------------------------------//
        if(!is_on_operation(ctx, OPERATION_PARAM_LINKER) &&
           ident->parameters_number != i + 1) {
            return syntax_error(ctx,
                                "Function '%.*s' expected to "
                                "have %llu parameters.\n",
                                ident->length,
                                ident->name,
                                ident->parameters_number);
        }
        //-------------------------------------------------------------------//
        if(i + 1 != ident->parameters_number) {
            current_node->right = token_position(ctx);
            move_next_token(ctx);
        }
        current_node = current_node->right;
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_comparison(language_t       *ctx,
                                language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    language_node_t *left_side = NULL;
    _RETURN_IF_ERROR(get_expression(ctx, &left_side));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_BIGGER ) &&
       !is_on_operation(ctx, OPERATION_SMALLER)) {
        *output = left_side;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    (*output)->left = left_side;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(ctx, &(*output)->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_expression(language_t       *ctx,
                                language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_multiple(ctx, &root));
    //-----------------------------------------------------------------------//
    while(is_on_operation(ctx, OPERATION_ADD) ||
          is_on_operation(ctx, OPERATION_SUB)) {
        //-------------------------------------------------------------------//
        language_node_t *new_root = token_position(ctx);
        new_root->left = root;
        root = new_root;
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_multiple(ctx, &root->right));
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_multiple(language_t       *ctx,
                              language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_power(ctx, &root));
    //-----------------------------------------------------------------------//
    while(is_on_operation(ctx, OPERATION_MUL) ||
          is_on_operation(ctx, OPERATION_DIV)) {
        //-------------------------------------------------------------------//
        language_node_t *new_root = token_position(ctx);
        new_root->left = root;
        root = new_root;
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_power(ctx, &root->right));
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_power(language_t       *ctx,
                           language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    language_node_t *root = NULL;
    _RETURN_IF_ERROR(get_element(ctx, &root));
    //-----------------------------------------------------------------------//
    while(is_on_operation(ctx, OPERATION_POW)) {
        //-------------------------------------------------------------------//
        language_node_t *new_root = token_position(ctx);
        new_root->left = root;
        root = new_root;
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_element(ctx, &root->right));
        //-------------------------------------------------------------------//
    }
    //-----------------------------------------------------------------------//
    *output = root;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_element(language_t       *ctx,
                             language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(get_expression(ctx, output));
        //-------------------------------------------------------------------//
        if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
            return syntax_error(ctx, "Unclosed bracket.\n");
        }
        move_next_token(ctx);
        //-------------------------------------------------------------------//
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    if(is_on_type(ctx, NODE_TYPE_NUMBER)) {
        return get_number(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_type(ctx, NODE_TYPE_OPERATION) &&
       KeyWords[token_position(ctx)->value.opcode].is_expression_element) {
        return get_operation(ctx, output);
    }
    //-----------------------------------------------------------------------//
    if(is_on_type(ctx, NODE_TYPE_IDENTIFIER)) {
        _RETURN_IF_ERROR(set_reference(ctx, token_position(ctx)));
        //-------------------------------------------------------------------//
        if(is_on_ident_type(ctx, IDENTIFIER_FUNCTION)) {
            return get_function_call(ctx, output);
        }
        //-------------------------------------------------------------------//
        if(is_on_ident_type(ctx, IDENTIFIER_VARIABLE)) {
            return get_variable(ctx, output);
        }
        //-------------------------------------------------------------------//
        print_error("Unknown identifier type.\n");
        return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_number(language_t       *ctx,
                            language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_type(ctx, NODE_TYPE_NUMBER),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_operation(language_t       *ctx,
                               language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_type(ctx, NODE_TYPE_OPERATION),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    if(!KeyWords[token_position(ctx)->value.opcode].is_expression_element) {
        return syntax_error(ctx,
                            "Expected only math operations in expression.\n");
    }
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "Operation '%s' expected to have parameter in '()'",
                            KeyWords[(*output)->value.opcode]);
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(ctx, &(*output)->left));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx, "I supposed to see ')'");
    }
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_variable(language_t       *ctx,
                              language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_ident_type(ctx, IDENTIFIER_VARIABLE),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    move_next_token(ctx);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_in(language_t       *ctx,
                        language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_IN),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    _RETURN_IF_ERROR(move_next_token(ctx));
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx, "Expected to see '(' after input keyword.");
    }
    (*output)->left = token_position(ctx);
    _RETURN_IF_ERROR(move_next_token(ctx));
    _RETURN_IF_ERROR(set_val((*output)->left,
                             NODE_TYPE_OPERATION,
                             OPCODE(OPERATION_PARAM_LINKER),
                             NULL, NULL));
    if(!is_on_type(ctx, NODE_TYPE_IDENTIFIER)) {
        return syntax_error(ctx, "Expected to see in parameter as variable");
    }
    _RETURN_IF_ERROR(set_reference(ctx, token_position(ctx)));
    if(!is_on_ident_type(ctx, IDENTIFIER_VARIABLE)) {
        return syntax_error(ctx, "Expected to see in parameter as variable");
    }
    (*output)->left->left = token_position(ctx);
    _RETURN_IF_ERROR(move_next_token(ctx));
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx, "Expected to see ')' after variable in input.");
    }
    _RETURN_IF_ERROR(move_next_token(ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_out(language_t       *ctx,
                         language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _C_ASSERT(is_on_operation(ctx, OPERATION_OUT),
              return LANGUAGE_SYNTAX_UNEXPECTED_CALL);
    *output = token_position(ctx);
    _RETURN_IF_ERROR(move_next_token(ctx));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_OPEN_BRACKET)) {
        return syntax_error(ctx,
                            "Expected to see ( after out command.\n");
    }
    _RETURN_IF_ERROR(move_next_token(ctx));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(get_expression(ctx, &(*output)->left));
    //-----------------------------------------------------------------------//
    if(!is_on_operation(ctx, OPERATION_CLOSE_BRACKET)) {
        return syntax_error(ctx,
                            "Expected to see ) after out command.\n");
    }
    _RETURN_IF_ERROR(move_next_token(ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t set_reference(language_t      *ctx,
                               language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    if(node->type != NODE_TYPE_IDENTIFIER) {
        print_error("Node type is not identifier. Unable to get reference.\n");
        return LANGUAGE_SYNTAX_UNEXPECTED_CALL;
    }
    //-----------------------------------------------------------------------//
    name_t *name = ctx->name_table.used_names + node->value.identifier;
    for(size_t elem = 0; elem < ctx->name_table.size; elem++) {
        identifier_t *ident = ctx->name_table.identifiers + elem;

        if(ident->type != IDENTIFIER_FUNCTION) {
            continue;
        }
        if(strncmp(ident->name, name->name, ident->length) != 0) {
            continue;
        }
        node->value.identifier = elem;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    for(size_t elem = ctx->name_table.stack_size; elem > 0; elem--) {
        size_t nt_index     = ctx->name_table.stack[elem - 1];
        identifier_t *ident = ctx->name_table.identifiers + nt_index;

        if(ident->length != name->length) {
            continue;
        }
        if(strncmp(ident->name, name->name, ident->length) != 0) {
            continue;
        }
        node->value.identifier = nt_index;
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    return syntax_error(ctx,
                        "Undefined reference to '%.*s'.\n",
                        name->length,
                        name->name);
}

//===========================================================================//
