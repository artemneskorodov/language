#ifndef SIMPLIFY_RULES_H
#define SIMPLIFY_RULES_H

language_error_t simplify_add(language_t *ctx, language_node_t **node);
language_error_t simplify_sub(language_t *ctx, language_node_t **node);
language_error_t simplify_mul(language_t *ctx, language_node_t **node);
language_error_t simplify_div(language_t *ctx, language_node_t **node);
language_error_t simplify_pow(language_t *ctx, language_node_t **node);

#endif
