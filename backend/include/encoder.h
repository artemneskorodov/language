//===========================================================================//
#ifndef ENCODER_H
#define ENCODER_H
//===========================================================================//

#include "language.h"
#include "emitters_bin.h"
#include "emitters_asm.h"

//===========================================================================//

enum fixup_type_t {
    FIXUP_FUNC_ADDR = 1,
    FIXUP_VAR_ADDR  = 2,
};

//===========================================================================//

union REX_prefix_t{
    struct {
        unsigned int B : 1;
        unsigned int X : 1;
        unsigned int R : 1;
        unsigned int W : 1;
        unsigned int unused : 4;
    };
    uint8_t byte;
};

//===========================================================================//

union ModRM_t {
    struct {
        unsigned int rm : 3;
        unsigned int reg : 3;
        unsigned int mod : 2;
    };
    uint8_t byte;
};

//===========================================================================//

union SIB_t {
    struct {
        unsigned int base : 3;
        unsigned int index : 3;
        unsigned int scale : 2;
    };
    uint8_t byte;
};

//===========================================================================//

struct emitter_t {
    ir_instr_t instruction;
    language_error_t (*emitter)(language_t *, ir_node_t *);
    language_error_t (*write_asm)(language_t *, ir_node_t *);
    uint8_t op;
    uint8_t xmm_op;
    uint8_t cmp_num;
};

static const emitter_t IREmitters[] = {
    {/* Empty field*/},
    {IR_INSTR_ADD,      emit_add_sub,    .op = 0x01, .xmm_op = 0x58},
    {IR_INSTR_SUB,      emit_add_sub,    .op = 0x29, .xmm_op = 0x5C},
    {IR_INSTR_MUL,      emit_xmm_math,   .xmm_op = 0x59},
    {IR_INSTR_DIV,      emit_xmm_math,   .xmm_op = 0x5E},
    {IR_INSTR_PUSH,     emit_push_pop,   .op = 0xFF},
    {IR_INSTR_POP,      emit_push_pop,   .op = 0x8F},
    {IR_INSTR_MOV,      emit_mov         },
    {IR_INSTR_CALL,     emit_call        },
    {IR_INSTR_CMPL,     emit_cmp,         .cmp_num = 0x01},
    {IR_INSTR_CMPEQ,    emit_cmp,         .cmp_num = 0x00},
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

language_error_t encode_ir      (language_t *ctx);

language_error_t encode_ir_nasm (language_t *ctx);

language_error_t fixups_ctor    (language_t *ctx,
                                 size_t      capacity);

language_error_t fixups_dtor    (language_t *ctx);

language_error_t run_fixups     (language_t *ctx);

language_error_t add_fixup      (language_t *ctx,
                                 size_t      offset,
                                 size_t      id_index);

//===========================================================================//
#endif
//===========================================================================//
