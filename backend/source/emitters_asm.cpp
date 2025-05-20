#include "language.h"
#include "encoder.h"
#include "emitters_asm.h"
#include "custom_assert.h"
#include "colors.h"
#include "buffer.h"

static const size_t MaxInfoSz = 5 * 5;
static const size_t MaxImmSize = 64 / 4 + 2;
static const int LabelNumSize = 8;

struct sup_args_t {
    arg_type_t arg1;
    arg_type_t arg2;
    const char *name;
    size_t len;
};

#define _ARGS(_arg1, _arg2, _name) (sup_args_t){.arg1 = (_arg1), .arg2 = (_arg2), .name = (_name), .len = sizeof(_name) - 1}

struct instr_info_t {
    sup_args_t supported_args[MaxInfoSz];
    size_t supported_size;
};

struct all_instr_info_t {
    ir_instr_t ir_instr;
    const instr_info_t *instr_info;
};

static const name_t GPRNames[] = {
    _NAME("rax"), _NAME("rcx"), _NAME("rdx"), _NAME("rbx"),
    _NAME("rsp"), _NAME("rbp"), _NAME("rsi"), _NAME("rdi"),
    _NAME("r8" ), _NAME("r9" ), _NAME("r10"), _NAME("r11"),
    _NAME("r12"), _NAME("r13"), _NAME("r14"), _NAME("r15")};

static const name_t XMMNames[] = {
    _NAME("xmm0" ), _NAME("xmm1" ), _NAME("xmm2" ), _NAME("xmm3" ),
    _NAME("xmm4" ), _NAME("xmm5" ), _NAME("xmm6" ), _NAME("xmm7" ),
    _NAME("xmm8" ), _NAME("xmm9" ), _NAME("xmm10"), _NAME("xmm11"),
    _NAME("xmm12"), _NAME("xmm13"), _NAME("xmm14"), _NAME("xmm15")};

static const instr_info_t AddAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_IMM, "add"),
                       _ARGS(ARG_TYPE_REG, ARG_TYPE_REG, "add"),
                       _ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "addsd")},
    .supported_size = 3,
};

static const instr_info_t SubAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_IMM, "sub"),
                       _ARGS(ARG_TYPE_REG, ARG_TYPE_REG, "sub"),
                       _ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "subsd")},
    .supported_size = 3,
};

static const instr_info_t MulAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "mulsd")},
    .supported_size = 1,
};

static const instr_info_t DivAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "divsd")},
    .supported_size = 1,
};

static const instr_info_t PushAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_INVALID, "push"),
                       _ARGS(ARG_TYPE_MEM, ARG_TYPE_INVALID, "push")},
    .supported_size = 2,
};

static const instr_info_t PopAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_INVALID, "pop"),
                       _ARGS(ARG_TYPE_MEM, ARG_TYPE_INVALID, "pop")},
    .supported_size = 2,
};

static const instr_info_t MovAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_REG, "mov"),
                       _ARGS(ARG_TYPE_REG, ARG_TYPE_IMM, "mov"),
                       _ARGS(ARG_TYPE_REG, ARG_TYPE_XMM, "movq"),
                       _ARGS(ARG_TYPE_XMM, ARG_TYPE_REG, "movq"),
                       _ARGS(ARG_TYPE_XMM, ARG_TYPE_MEM, "movq"),
                       _ARGS(ARG_TYPE_MEM, ARG_TYPE_XMM, "movq")},
    .supported_size = 6,
};

static const instr_info_t CallAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_CST, ARG_TYPE_INVALID, "call")},
    .supported_size = 1,
};

static const instr_info_t CmplAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "cmpltsd")},
    .supported_size = 1,
};

static const instr_info_t CmpeqAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "cmpeqsd")},
    .supported_size = 1,
};

static const instr_info_t TestAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_REG, "test")},
    .supported_size = 1,
};

static const instr_info_t JzAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_CST, ARG_TYPE_INVALID, "jz")},
    .supported_size = 1,
};

static const instr_info_t JmpAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_CST, ARG_TYPE_INVALID, "jmp")},
    .supported_size = 1,
};

static const instr_info_t RetAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_INVALID, ARG_TYPE_INVALID, "ret")},
    .supported_size = 1,
};

static const instr_info_t SyscallAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_INVALID, ARG_TYPE_INVALID, "syscall")},
    .supported_size = 1,
};

static const instr_info_t XorAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, "xorpd")},
    .supported_size = 1,
};

static const instr_info_t ControlJmpAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_CST, ARG_TYPE_INVALID, NULL)},
    .supported_size = 1,
};

static const instr_info_t ControlFuncAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_CST, ARG_TYPE_INVALID, NULL)},
    .supported_size = 1,
};

static const instr_info_t SqrtAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_XMM, ARG_TYPE_XMM, NULL)},
    .supported_size = 1,
};

static const instr_info_t NotAsmInfo = {
    .supported_args = {_ARGS(ARG_TYPE_REG, ARG_TYPE_INVALID, "not")},
    .supported_size = 1,
};

static const all_instr_info_t AsmInfos[] = {
    {/* Empty field*/},
    {IR_INSTR_ADD,            &AddAsmInfo},
    {IR_INSTR_SUB,            &SubAsmInfo},
    {IR_INSTR_MUL,            &MulAsmInfo},
    {IR_INSTR_DIV,            &DivAsmInfo},
    {IR_INSTR_PUSH,          &PushAsmInfo},
    {IR_INSTR_POP,            &PopAsmInfo},
    {IR_INSTR_MOV,            &MovAsmInfo},
    {IR_INSTR_CALL,          &CallAsmInfo},
    {IR_INSTR_CMPL,          &CmplAsmInfo},
    {IR_INSTR_CMPEQ,        &CmpeqAsmInfo},
    {IR_INSTR_TEST,          &TestAsmInfo},
    {IR_INSTR_JZ,              &JzAsmInfo},
    {IR_INSTR_JMP,            &JmpAsmInfo},
    {IR_INSTR_RET,            &RetAsmInfo},
    {IR_INSTR_SYSCALL,    &SyscallAsmInfo},
    {IR_INSTR_XOR,            &XorAsmInfo},
    {IR_CONTROL_JMP,   &ControlJmpAsmInfo},
    {IR_CONTROL_FUNC, &ControlFuncAsmInfo},
    {IR_INSTR_SQRT,          &SqrtAsmInfo},
    {IR_INSTR_NOT,            &NotAsmInfo},
};

static language_error_t write_asm_operand(language_t *ctx, ir_node_t *node, ir_arg_t *operand);
static language_error_t write_asm_mem(language_t *ctx, ir_arg_t *arg);
static language_error_t write_asm_cst(language_t *ctx, ir_node_t *node);
static language_error_t write_asm_reg_xmm(language_t *ctx, ir_arg_t *arg);
static language_error_t write_asm_imm(language_t *ctx, ir_arg_t *arg);

language_error_t write_asm_code(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    size_t info_index = (size_t)node->instruction;
    _C_ASSERT(AsmInfos[info_index].ir_instr == node->instruction,
              return LANGUAGE_BROKEN_ASM_TABLE);
    const instr_info_t *instr_info = AsmInfos[info_index].instr_info;

    arg_type_t type_first  = node->first.type;
    arg_type_t type_second = node->second.type;
    size_t args_index = instr_info->supported_size;
    for(size_t i = 0; i < instr_info->supported_size; i++) {
        if(type_first  == instr_info->supported_args[i].arg1 &&
           type_second == instr_info->supported_args[i].arg2) {
            args_index = i;
            break;
        }
    }
    if(args_index == instr_info->supported_size) {
        print_error("Unexpected args types for node.");
        return LANGUAGE_UNEXPECTED_IR_INSTR;
    }
    const sup_args_t *arg = instr_info->supported_args + args_index;
    if(arg->name != NULL) {
        _RETURN_IF_ERROR(buffer_write_string(ctx,arg->name, arg->len));
        _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
    }
    if(node->first.type != ARG_TYPE_INVALID) {
        _RETURN_IF_ERROR(write_asm_operand(ctx, node, &node->first));
    }
    if(node->second.type != ARG_TYPE_INVALID) {
        _RETURN_IF_ERROR(buffer_write_byte(ctx, ','));
        _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
        _RETURN_IF_ERROR(write_asm_operand(ctx, node, &node->second));
    }
    _RETURN_IF_ERROR(buffer_write_byte(ctx, '\n'));
    return LANGUAGE_SUCCESS;
}

language_error_t write_asm_operand(language_t *ctx, ir_node_t *node, ir_arg_t *operand) {
    switch(operand->type) {
        case ARG_TYPE_INVALID: {
            print_error("It is not expected that INVALID parameter of IR node will be treated as operand");
            return LANGUAGE_UNEXPECTED_NODE_TYPE;
        }
        case ARG_TYPE_IMM: {
            return write_asm_imm    (ctx, operand);
        }
        case ARG_TYPE_REG: {
            return write_asm_reg_xmm(ctx, operand);
        }
        case ARG_TYPE_XMM: {
            return write_asm_reg_xmm(ctx, operand);
        }
        case ARG_TYPE_CST: {
            return write_asm_cst    (ctx, node);
        }
        case ARG_TYPE_MEM: {
            return write_asm_mem    (ctx, operand);
            break;
        }
        default: {
            print_error("Unknown IR node type.");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
}

language_error_t write_asm_imm(language_t *ctx, ir_arg_t *arg) {
    _RETURN_IF_ERROR(buffer_check_size(ctx, MaxImmSize));
    uint8_t *buffer = ctx->backend_info.buffer + ctx->backend_info.buffer_size;
    int size = sprintf((char *)buffer, "0x%lx", (uint64_t)arg->imm);
    ctx->backend_info.buffer_size += (size_t)size;
    return LANGUAGE_SUCCESS;
}

language_error_t write_asm_reg_xmm(language_t *ctx, ir_arg_t *arg) {
    const name_t *names_table = NULL;
    if(arg->type == ARG_TYPE_REG) {
        names_table = GPRNames;
    }
    else if(arg->type == ARG_TYPE_XMM) {
        names_table = XMMNames;
    }
    else {
        print_error("Unexpected arg type.\n");
        return LANGUAGE_UNEXPECTED_IR_INSTR;
    }

    const char *name = names_table[arg->reg - 1].name;
    size_t      len  = names_table[arg->reg - 1].length;
    _RETURN_IF_ERROR(buffer_write_string(ctx, name, len));
    return LANGUAGE_SUCCESS;
}

language_error_t write_asm_cst(language_t *ctx, ir_node_t *node) {
    ir_arg_t *arg = &node->first;
    if(node->instruction == IR_INSTR_JMP ||
       node->instruction == IR_INSTR_JZ  ||
       node->instruction == IR_CONTROL_JMP) {
        size_t label_num = 0;

        if(node->instruction != IR_CONTROL_JMP) {
            if(arg->custom != NULL) {
                ir_node_t *label_node = (ir_node_t *)arg->custom;
                label_num = (size_t)label_node->first.custom;
            }
            else {
                label_num = ctx->backend_info.used_labels++;
                arg->custom = (void *)label_num;
            }
        }
        else {
            if(arg->custom != NULL) {
                ir_node_t *jmp_node = (ir_node_t *)arg->custom;
                label_num = (size_t)jmp_node->first.custom;
            }
            else {
                label_num = ctx->backend_info.used_labels++;
                arg->custom = (void *)label_num;
            }
        }
        _RETURN_IF_ERROR(buffer_check_size(ctx, LabelNumSize + 5));
        uint8_t *buffer = ctx->backend_info.buffer + ctx->backend_info.buffer_size;
        sprintf((char *)buffer, ".loc_%0*lu", LabelNumSize, label_num);
        ctx->backend_info.buffer_size += LabelNumSize + 5;
        if(node->instruction == IR_CONTROL_JMP) {
            _RETURN_IF_ERROR(buffer_write_byte(ctx, ':'));
        }
    }
    else if(node->instruction == IR_INSTR_CALL ||
            node->instruction == IR_CONTROL_FUNC) {
        size_t id_index = (size_t)arg->custom;
        identifier_t *ident = ctx->name_table.identifiers + id_index;
        _RETURN_IF_ERROR(buffer_write_string(ctx, ident->name, ident->length));
        if(node->instruction == IR_CONTROL_FUNC) {
            _RETURN_IF_ERROR(buffer_write_byte(ctx, ':'));
        }
    }
    else {
        print_error("Unexpected instruction has custom argument");
        return LANGUAGE_UNEXPECTED_IR_INSTR;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t write_asm_mem(language_t *ctx, ir_arg_t *arg) {
    name_t qword = _NAME("qword");
    _RETURN_IF_ERROR(buffer_write_string(ctx, qword.name,  qword.length ));
    _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
    _RETURN_IF_ERROR(buffer_write_byte(ctx, '['));
    if(arg->mem.base != REGISTER_RIP) {
        const char *name = GPRNames[arg->reg - 1].name;
        size_t      len  = GPRNames[arg->reg - 1].length;
        _RETURN_IF_ERROR(buffer_write_string(ctx, name, len));
        _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
        if(arg->mem.offset >= 0) {
            _RETURN_IF_ERROR(buffer_write_byte(ctx, '+'));
        }
        else {
            _RETURN_IF_ERROR(buffer_write_byte(ctx, '-'));
            arg->mem.offset *= -1;
        }
        _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
        _RETURN_IF_ERROR(buffer_check_size(ctx, sizeof(uint32_t) / 4 + 2));
        uint8_t *buffer = ctx->backend_info.buffer + ctx->backend_info.buffer_size;
        int printed_symbols = sprintf((char *)buffer, "0x%x", (uint32_t)arg->mem.offset);
        ctx->backend_info.buffer_size += (size_t)printed_symbols;
    }
    else {
        size_t id_index = (size_t)arg->mem.offset;
        identifier_t *ident = ctx->name_table.identifiers + id_index;
        _RETURN_IF_ERROR(buffer_write_string(ctx, ident->name, ident->length));
    }
    _RETURN_IF_ERROR(buffer_write_byte(ctx, ']'));
    return LANGUAGE_SUCCESS;
}


// language_error_t write_asm_add(language_t *ctx, ir_node_t *node);
// language_error_t write_asm_sub       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_mul       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_div       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_push      (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_pop       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_mov       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_call      (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_cmpl      (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_cmpeq     (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_test      (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_jz        (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_jmp       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_ret       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_syscall   (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_xor       (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_cntrl_jmp (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_cntrl_func(language_t *ctx, ir_node_t *node);
// language_error_t write_asm_sqrt      (language_t *ctx, ir_node_t *node);
// language_error_t write_asm_not       (language_t *ctx, ir_node_t *node);
