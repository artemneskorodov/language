//===========================================================================//
#ifndef EMITTERS_BIN_H
#define EMITTERS_BIN_H
//===========================================================================//

#include "language.h"
#include "encoder.h"

//===========================================================================//

language_error_t emit_add_sub       (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_xmm_math      (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_push_pop      (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov           (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_call          (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_cmp           (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_test          (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_jumps         (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_ret           (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_syscall       (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_fix_jmp       (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_xor           (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_control_func  (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_sqrt          (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_mem_xmm   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_xmm_mem   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_xmm_reg   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_reg_xmm   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_reg_imm   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_mov_reg_reg   (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_not           (language_t *ctx,
                                     ir_node_t  *node);

//===========================================================================//
#endif
//===========================================================================//
