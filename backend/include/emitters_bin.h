//===========================================================================//
#ifndef EMITTERS_BIN_H
#define EMITTERS_BIN_H
//===========================================================================//

#include "language.h"
#include "encoder.h"

//===========================================================================//

struct emitter_t {
    ir_instr_t         instruction;
    language_error_t (*emitter  )(language_t *, ir_node_t *);
    language_error_t (*write_asm)(language_t *, ir_node_t *);
    uint8_t            op;
    uint8_t            xmm_op;
    uint8_t            cmp_num;
};

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

language_error_t emit_not           (language_t *ctx,
                                     ir_node_t  *node);

language_error_t emit_stack_xmm     (language_t *ctx,
                                     ir_node_t  *node);

//===========================================================================//

static const emitter_t IREmitters[] = {
    {/* Empty field*/},
    {IR_INSTR_ADD,      emit_add_sub,    .op = 0x01, .xmm_op = 0x58},
    {IR_INSTR_SUB,      emit_add_sub,    .op = 0x29, .xmm_op = 0x5C},
    {IR_INSTR_MUL,      emit_xmm_math,   .xmm_op = 0x59},
    {IR_INSTR_DIV,      emit_xmm_math,   .xmm_op = 0x5E},
    {IR_INSTR_PUSH,     emit_push_pop,   .op = 0xFF},
    {IR_INSTR_POP,      emit_push_pop,   .op = 0x8F},
    {IR_INSTR_MOV,      emit_mov         },
    {IR_INSTR_CALL,     emit_call,       .op = 0xE8},
    {IR_INSTR_CMPL,     emit_cmp,        .cmp_num = 0x01},
    {IR_INSTR_CMPEQ,    emit_cmp,        .cmp_num = 0x00},
    {IR_INSTR_TEST,     emit_test        },
    {IR_INSTR_JZ,       emit_jumps       },
    {IR_INSTR_JMP,      emit_jumps       },
    {IR_INSTR_RET,      emit_ret         },
    {IR_INSTR_SYSCALL,  emit_syscall     },
    {IR_INSTR_XOR,      emit_xor         },
    {IR_CONTROL_JMP,    emit_fix_jmp     },
    {IR_CONTROL_FUNC,   emit_control_func},
    {IR_INSTR_SQRT,     emit_sqrt        },
    {IR_INSTR_NOT,      emit_not         },
    {IR_INSTR_PUSH_XMM, emit_stack_xmm   },
    {IR_INSTR_POP_XMM,  emit_stack_xmm   },
};

//===========================================================================//
#endif
//===========================================================================//
