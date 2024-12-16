#ifndef FRONTEND_UTILS_H
#define FRONTEND_UTILS_H

#include "language.h"

language_error_t move_next_symbol(language_t *language);
char current_symbol(language_t *language);
const char *input_position(language_t *language);

language_error_t move_next_token(language_t *language);
language_node_t *token_position(language_t *language);

bool is_on_operation(language_t *language, operation_t opcode);
language_error_t syntax_error(language_t *language, const char *format, ...);
bool is_on_ident_type(language_t *language, identifier_type_t type);
bool is_on_type(language_t *language, node_type_t type);

#endif
