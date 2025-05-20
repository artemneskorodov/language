//===========================================================================//
#ifndef ASM_SPU_H
#define ASM_SPU_H
//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t spu_compile_subtree        (language_t        *ctx,
                                             language_node_t   *root);

language_error_t spu_assemble_two_args      (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_one_arg       (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_comparison    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_statements    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_if            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_while         (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_return        (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_params_line   (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_new_var       (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_new_func      (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_assignment    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_in            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_out           (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_call          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t spu_assemble_exit          (language_t        *ctx,
                                             language_node_t   *node);

//===========================================================================//
#endif
//===========================================================================//
