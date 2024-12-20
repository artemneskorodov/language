#ifndef ASSEMBLE_H
#define ASSEMBLE_H

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t compile_subtree            (language_t        *ctx,
                                             language_node_t   *root);

language_error_t assemble_two_args          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_one_arg           (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_comparison        (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_statements_line   (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_if                (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_while             (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_return            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_params_line       (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_new_var           (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_new_func          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_assignment        (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_in                (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_out               (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_call              (language_t        *ctx,
                                             language_node_t   *node);

language_error_t assemble_exit              (language_t        *ctx,
                                             language_node_t   *node);

//===========================================================================//

#endif
