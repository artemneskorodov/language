#ifndef TO_SOURCE_H
#define TO_SOURCE_H

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t to_source_subtree          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_math_op          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_math_func        (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_statements_line  (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_if               (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_while            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_return           (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_params_line      (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_new_var          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_new_func         (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_in               (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_out              (language_t        *ctx,
                                             language_node_t   *node);

language_error_t to_source_call             (language_t        *ctx,
                                             language_node_t   *node);

//===========================================================================//

#endif
