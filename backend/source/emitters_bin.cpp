//===========================================================================//

#include <stdint.h>
#include <stdlib.h>
#include <elf.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "encoder.h"
#include "custom_assert.h"
#include "buffer.h"
#include "colors.h"
#include "backend.h"

//===========================================================================//

static ModRM_t          create_regs_modrm  (ir_arg_t    *reg,
                                            ir_arg_t    *rm);

static bool             is_extended_reg    (ir_arg_t    *arg);

static REX_prefix_t     create_rex         (ir_arg_t    *reg,
                                            ir_arg_t    *rm);

static language_error_t emit_mov_mem_xmm   (language_t *ctx,
                                            ir_node_t  *node);

static language_error_t emit_mov_xmm_xm   (language_t *ctx,
                                            ir_node_t  *node);

static language_error_t emit_mov_xmm_reg   (language_t *ctx,
                                            ir_node_t  *node);

static language_error_t emit_mov_reg_xmm   (language_t *ctx,
                                            ir_node_t  *node);

static language_error_t emit_mov_reg_imm   (language_t *ctx,
                                            ir_node_t  *node);

static language_error_t emit_mov_rm_reg   (language_t *ctx,
                                            ir_node_t  *node);

//===========================================================================//

static const size_t MaxInstructionSize = 15;

//===========================================================================//

language_error_t emit_add_sub(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // op r64, r64 --> REX opcode ModR/M
    if(node->first.type == ARG_TYPE_REG &&
       node->second.type == ARG_TYPE_REG) {
        uint8_t result[MaxInstructionSize] = {};
        size_t pos = 0;
        REX_prefix_t rex   = create_rex       (&node->second, &node->first);
        ModRM_t      modrm = create_regs_modrm(&node->second, &node->first);
        result[pos++] = rex.byte;
        result[pos++] = IREmitters[node->instruction].op;
        result[pos++] = modrm.byte;
        _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    // op r64, imm32 --> REX opcode=81 ModR/M imm32
    // (ModR/M reg field used as opcode)
    else if(node->first.type == ARG_TYPE_REG &&
            node->second.type == ARG_TYPE_IMM) {
        uint8_t result[MaxInstructionSize] = {};
        size_t pos = 0;
        //-------------------------------------------------------------------//
        // REX
        result[pos++] = create_rex(&node->second, &node->first).byte;
        //-------------------------------------------------------------------//
        // opcode of add and sub for op r64, imm32 is 0x81
        result[pos++] = 0x81;
        //-------------------------------------------------------------------//
        // ModR/M contains operation number in reg field and register in r/m
        ModRM_t modrm = {};
        modrm.mod = 3;
        if(node->instruction == IR_INSTR_ADD) {
            modrm.reg = 0b000;
        }
        else if(node->instruction == IR_INSTR_SUB) {
            modrm.reg = 0b101;
        }
        else {
            print_error("emit_add_sub can only emit addition and subscription.");
            return LANGUAGE_UNEXPECTED_IR_INSTR;
        }
        modrm.rm  = (node->first.reg - 1) & 7;
        result[pos++] = modrm.byte;
        //-------------------------------------------------------------------//
        // Writing imm32
        *(uint32_t *)(result + pos) = (uint32_t)node->second.imm;
        pos += sizeof(uint32_t);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    // op xmm, xmm is encoded as other xmm instructions for doubles
    else if(node->first.type == ARG_TYPE_XMM &&
            node->second.type == ARG_TYPE_XMM) {
        return emit_xmm_math(ctx, node);
    }
    //-----------------------------------------------------------------------//
    else {
        print_error("For sub and add only 'op r64, r64', "
                    "'op r64, imm32' and 'op xmm, xmm' emitters are provided.");
        return LANGUAGE_UNEXPECTED_IR_INSTR;
    }
}

//===========================================================================//

language_error_t emit_xmm_math(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_ADD ||
              node->instruction == IR_INSTR_SUB ||
              node->instruction == IR_INSTR_MUL ||
              node->instruction == IR_INSTR_DIV,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    if(node->first.type != ARG_TYPE_XMM || node->second.type != ARG_TYPE_XMM) {
        print_error("Function emit_xmm_math is expected to call "
                    "only for math operation with two xmm's in arguments.");
        return LANGUAGE_UNEXPECTED_IR_INSTR;
    }
    //-----------------------------------------------------------------------//
    // op xmm, xmm --> 0xF2 REX? 0x0F opcode ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instructions prefix
    result[pos++] = 0xF2;
    //-----------------------------------------------------------------------//
    // Writing REX if needed
    REX_prefix_t rex = create_rex(&node->first, &node->second);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x0F;
    result[pos++] = IREmitters[node->instruction].xmm_op;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_push_pop(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_PUSH ||
              node->instruction == IR_INSTR_POP,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // push/pop r64 --> REX? base_opcode + reg % 8
    if(node->first.type == ARG_TYPE_REG) {
        uint8_t result[MaxInstructionSize] = {};
        size_t pos = 0;
        //-------------------------------------------------------------------//
        // Writing REX if needed
        if(is_extended_reg(&node->first)) {
            REX_prefix_t rex = {};
            rex.unused = 0b0100;
            rex.B = 1;
            result[pos++] = rex.byte;
        }
        //-------------------------------------------------------------------//
        // Determining base opcode
        uint8_t base_opcode = 0x50;
        if(node->instruction == IR_INSTR_POP) {
            base_opcode = 0x58;
        }
        //-------------------------------------------------------------------//
        result[pos++] = base_opcode + ((node->first.reg - 1) & 7);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    // push [reg addr] --> REX? opcode ModR/M offs
    else if (node->first.type == ARG_TYPE_MEM){
        uint8_t result[MaxInstructionSize] = {};
        size_t pos = 0;
        //-------------------------------------------------------------------//
        // Opcode
        result[pos++] = IREmitters[node->instruction].op;
        //-------------------------------------------------------------------//
        // ModR/M
        ModRM_t modrm = {};
        modrm.reg = 0;
        if(node->instruction == IR_INSTR_PUSH) {
            modrm.reg = 0b110;
        }
        if(node->first.mem.base != REGISTER_RIP) {
            modrm.mod = 0b10;
            modrm.rm = (node->first.mem.base - 1) & 7;
        }
        if(node->first.mem.base == REGISTER_RIP) {
            modrm.mod = 0b00;
            modrm.rm = 0b101;
        }
        result[pos++] = modrm.byte;
        //-------------------------------------------------------------------//
        // Adding fixup if variable is global
        if(node->first.mem.base == REGISTER_RIP) {
            size_t id_index = (size_t)node->first.mem.offset;
            _RETURN_IF_ERROR(add_fixup(ctx, pos, id_index));
        }
        //-------------------------------------------------------------------//
        // Offset from base register
        *(uint32_t *)(result + pos) = (uint32_t)node->first.mem.offset;
        pos += sizeof(uint32_t);
        //-------------------------------------------------------------------//
        _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    print_error("Push and pop emitters are provided only for "
                "memory and registers");
    return LANGUAGE_UNEXPECTED_IR_INSTR;
}

//===========================================================================//

language_error_t emit_mov(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    if((node->first.type  == ARG_TYPE_REG ||
        node->first.type  == ARG_TYPE_MEM) &&
       node->second.type == ARG_TYPE_REG) {
        return emit_mov_rm_reg(ctx, node);
    }
    if(node->first.type  == ARG_TYPE_REG &&
       node->second.type == ARG_TYPE_IMM) {
        return emit_mov_reg_imm(ctx, node);
    }
    if(node->first.type  == ARG_TYPE_REG &&
       node->second.type == ARG_TYPE_XMM) {
        return emit_mov_reg_xmm(ctx, node);
    }
    if(node->first.type  == ARG_TYPE_XMM &&
       node->second.type == ARG_TYPE_REG) {
        return emit_mov_xmm_reg(ctx, node);
    }
    if(node->first.type  == ARG_TYPE_XMM &&
       (node->second.type == ARG_TYPE_MEM ||
        node->second.type == ARG_TYPE_XMM)) {
        return emit_mov_xmm_xm(ctx, node);
    }
    if(node->first.type  == ARG_TYPE_MEM &&
       node->second.type == ARG_TYPE_XMM) {
        return emit_mov_mem_xmm(ctx, node);
    }
    //-----------------------------------------------------------------------//
    print_error("Mov %d %d is not supported",
                node->first.type, node->second.type);
    return LANGUAGE_UNEXPECTED_IR_INSTR;
}

//===========================================================================//

language_error_t emit_mov_rm_reg(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              (node->first.type  == ARG_TYPE_REG ||
               node->first.type == ARG_TYPE_MEM) &&
              node->second.type == ARG_TYPE_REG,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov r/m64, r64 --> REX opcode=0x89 ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x89;
    //-----------------------------------------------------------------------//
    // ModR/M
    if(node->first.type == ARG_TYPE_REG) {
        result[pos++] = create_regs_modrm(&node->second, &node->first).byte;
    }
    else {
        ModRM_t modrm = {};
        if(node->first.mem.base == REGISTER_RIP) {
            modrm.mod = 0b00;
            modrm.rm = 0b101;
            size_t id_index = (size_t)node->first.mem.offset;
            _RETURN_IF_ERROR(add_fixup(ctx, pos + 1, id_index));
        }
        else {
            modrm.mod = 0b10;
            modrm.rm = (node->first.mem.base - 1) & 7;
        }
        modrm.reg = (node->second.reg - 1) & 7;
        result[pos++] = modrm.byte;

        if(node->first.mem.base == REGISTER_RSP) {
            SIB_t sib = {};
            sib.scale = 0;
            sib.index = 0b100; // no index
            sib.base = (REGISTER_RSP - 1) & 7;
            result[pos++] = sib.byte;
        }

        *(uint32_t *)(result + pos) = (uint32_t)node->first.mem.offset;
        pos += 4;
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_mov_reg_imm(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              node->first.type  == ARG_TYPE_REG &&
              node->second.type == ARG_TYPE_IMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov r64, imm64 --> REX (base_opcode + reg % 8) imm64
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    // Opcode + reg % 8
    result[pos++] = 0xB8 + (uint8_t)((node->first.reg - 1) & 7);
    //-----------------------------------------------------------------------//
    // imm64
    *(int64_t *)(result + pos) = node->second.imm;
    pos += sizeof(int64_t);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, 10));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_mov_reg_xmm(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              node->first.type  == ARG_TYPE_REG &&
              node->second.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov r64, xmm --> 0x66 REX 0x0F 0x7E ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0x66;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    // opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x7E;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//s

language_error_t emit_mov_xmm_reg(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              node->first.type  == ARG_TYPE_XMM &&
              node->second.type == ARG_TYPE_REG,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov xmm, r64 --> 0x66 REX 0x0F 0x6E ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0x66;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    // opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x6E;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_mov_xmm_xm(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              node->first.type  == ARG_TYPE_XMM &&
              (node->second.type == ARG_TYPE_MEM ||
               node->second.type == ARG_TYPE_XMM),
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov xmm, xmm/[r64 + offs] --> 0xF2 REX? 0x0F 0x10 ModR/M SIB? offs?
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0xF2;
    //-----------------------------------------------------------------------//
    // REX
    REX_prefix_t rex = create_rex(&node->first, &node->second);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x10;
    //-----------------------------------------------------------------------//
    // ModR/M
    if(node->second.type == ARG_TYPE_MEM) {
        ModRM_t modrm = {};
        if(node->second.mem.base == REGISTER_RIP) {
            // RIP relative addressing for global variables
            modrm.mod = 0b00;
            modrm.rm  = 0b101;
            size_t id_index = (size_t)node->second.mem.offset;
            _RETURN_IF_ERROR(add_fixup(ctx, pos + 1, id_index));
        }
        else {
            // General purpose register relative addressing
            modrm.mod = 0b10;
            modrm.rm  = (node->second.mem.base - 1) & 7;
        }
        modrm.reg = (node->first.xmm - 1) & 7;
        result[pos++] = modrm.byte;
        //-------------------------------------------------------------------//
        // SIB if RSP relative addressing
        if(node->second.mem.base == REGISTER_RSP) {
            SIB_t sib = {};
            sib.scale = 0;
            sib.index = 0b100; //no index
            sib.base = (REGISTER_RSP - 1) & 7;
            result[pos++] = sib.byte;
        }
        //-------------------------------------------------------------------//
        // Offset
        *(uint32_t *)(result + pos) = (uint32_t)node->second.mem.offset;
        pos += sizeof(uint32_t);
    }
    else {
        result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_mov_mem_xmm(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_MOV &&
              node->first.type  == ARG_TYPE_MEM &&
              node->second.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // mov [r64 + offs], xmm --> 0xF2 REX? 0x0F 0x11 ModR/M SIB? offs
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0xF2;
    //-----------------------------------------------------------------------//
    // REX
    REX_prefix_t rex = create_rex(&node->second, &node->first);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x11;
    //-----------------------------------------------------------------------//
    // ModR/M
    ModRM_t modrm = {};
    if(node->first.mem.base == REGISTER_RIP) {
        // RIP relative addressing for global variables
        modrm.mod = 0b00;
        modrm.rm = 0b101;
        size_t id_index = (size_t)node->first.mem.offset;
        _RETURN_IF_ERROR(add_fixup(ctx, pos + 1, id_index));
    }
    else {
        // General purpose register relative addressing
        modrm.mod = 0b10;
        modrm.rm = (node->first.mem.base - 1) & 7;
    }
    modrm.reg = (node->second.xmm - 1) & 7;
    result[pos++] = modrm.byte;
    //-----------------------------------------------------------------------//
    // SIB for RSP relative addressing
    if(node->first.mem.base == REGISTER_RSP) {
        SIB_t sib = {};
        sib.scale = 0;
        sib.index = 0b100; // no index
        sib.base = (REGISTER_RSP - 1) & 7;
        result[pos++] = sib.byte;
    }
    //-----------------------------------------------------------------------//
    // Offset
    *(uint32_t *)(result + pos) = (uint32_t)node->first.mem.offset;
    pos += sizeof(uint32_t);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_call   (language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_CALL,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // call func --> 0xE8 offs32
    uint8_t call[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // Opcode
    call[pos++] = IREmitters[IR_INSTR_CALL].op;
    //-----------------------------------------------------------------------//
    // Call offset
    _RETURN_IF_ERROR(add_fixup(ctx, pos, (size_t)node->first.custom));
    pos += sizeof(uint32_t);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, call, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_cmp(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_CMPEQ ||
              node->instruction == IR_INSTR_CMPL,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    _C_ASSERT(node->first.type  == ARG_TYPE_XMM &&
              node->second.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // cmp(lt/eq)sd xmm, xmm --> 0xF2 REX? 0x0F 0xC2 ModR/M imm8
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0xF2;
    //-----------------------------------------------------------------------//
    // REX
    REX_prefix_t rex = create_rex(&node->first, &node->second);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x0F;
    result[pos++] = 0xC2;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    // 1 for cmpltsd and 0 for cmpeqsd
    result[pos++] = IREmitters[node->instruction].cmp_num;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_test   (language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_TEST &&
              node->first.type  == ARG_TYPE_REG &&
              node->second.type == ARG_TYPE_REG,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // test r64, r64 --> REX opcode=0x85 ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x85;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_jumps(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_JMP ||
              node->instruction == IR_INSTR_JZ,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // j(mp/jz) addr = opcode=(0x0F 0x84/0xE9) addr
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // Opcode
    if(node->instruction == IR_INSTR_JMP) {
        result[pos++] = 0xE9;
    }
    else if(node->instruction == IR_INSTR_JZ) {
        result[pos++] = 0x0F;
        result[pos++] = 0x84;
    }
    //-----------------------------------------------------------------------//
    // Offset
    if(node->first.custom != NULL) {
        // Looking for emitted node with address "label" and getting result
        // rip after jump from it
        ir_node_t *control = (ir_node_t *)node->first.custom;
        long jmp_rip = (long)control->first.custom;
        long curr_rip = current_rip(ctx) + (long)pos + 4;
        *(uint32_t *)(result + pos) = (uint32_t)(jmp_rip - curr_rip);
    }
    else {
        // Saving jump end RIP in node to fix it when emitting "label"
        node->first.custom = (void *)((size_t)current_rip(ctx) + pos + 4);
    }
    pos += 4;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_ret    (language_t *ctx, ir_node_t */*node*/) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    // ret --> 0x3C
    _RETURN_IF_ERROR(buffer_write_byte(ctx, 0xC3));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_syscall(language_t *ctx, ir_node_t */*node*/) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    // syscall -> 0x0F 0x05
    _RETURN_IF_ERROR(buffer_write_byte(ctx, 0x0F));
    _RETURN_IF_ERROR(buffer_write_byte(ctx, 0x05));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_xor(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_XOR &&
              node->first.type  == ARG_TYPE_XMM &&
              node->second.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // xor xmm, xmm --> 0x66 REX? 0x0F 0x57 ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0x66;
    //-----------------------------------------------------------------------//
    // REX if needed
    REX_prefix_t rex = create_rex(&node->first, &node->second);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x57;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_fix_jmp(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_CONTROL_JMP &&
              node->first.type  == ARG_TYPE_CST,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    if(node->first.custom != NULL) {
        // Setting jmp address if this node is after jump and jump node stores
        // buffer offset to fix address
        ir_node_t *jmp_node    = (ir_node_t *)node->first.custom;
        size_t     jmp_end_rip = (size_t)jmp_node->first.custom;
        uint8_t   *fix_addr    = ctx->backend_info.buffer + jmp_end_rip - 4 +
                                 sizeof(Elf64_Ehdr) + 2 * sizeof(Elf64_Phdr);
        *(uint32_t *)fix_addr = (uint32_t)((size_t)current_rip(ctx) -
                                           jmp_end_rip);
    }
    else {
        // Saving current RIP in this node if it is before jump node
        node->first.custom = (void *)current_rip(ctx);
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_control_func(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_CONTROL_FUNC &&
              node->first.type  == ARG_TYPE_CST,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // It is expected that name table index of function is saved in custom
    // node value
    size_t id_index = (size_t)node->first.custom;
    identifier_t *ident = ctx->name_table.identifiers + id_index;
    // Setting memory address to current RIP
    ident->memory_addr = current_rip(ctx);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_sqrt(language_t *ctx, ir_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_SQRT &&
              node->first.type  == ARG_TYPE_XMM &&
              node->second.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // sqrt xmm, xmm --> 0xF2 REX? 0x0F 0x51 ModR/M
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // XMM instruction prefix
    result[pos++] = 0xF2;
    //-----------------------------------------------------------------------//
    // REX if needed
    REX_prefix_t rex = create_rex(&node->first, &node->second);
    if(rex.byte != 0) {
        result[pos++] = rex.byte;
    }
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0x0F;
    result[pos++] = 0x51;
    //-----------------------------------------------------------------------//
    // ModR/M
    result[pos++] = create_regs_modrm(&node->first, &node->second).byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_not(language_t *ctx, ir_node_t  *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(node->instruction == IR_INSTR_NOT &&
              node->first.type  == ARG_TYPE_REG,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    // not r64 --> REX 0xF7 ModR/M (reg is used as additional opcode) not == 2
    uint8_t result[MaxInstructionSize] = {};
    size_t pos = 0;
    //-----------------------------------------------------------------------//
    // REX
    result[pos++] = create_rex(&node->second, &node->first).byte;
    //-----------------------------------------------------------------------//
    // Opcode
    result[pos++] = 0xF7;
    //-----------------------------------------------------------------------//
    // ModR/M
    ModRM_t modrm = {};
    modrm.mod = 0b11;
    modrm.rm = (node->first.reg - 1) & 7;
    modrm.reg = 0b010;
    result[pos++] = modrm.byte;
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_write(ctx, result, pos));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t emit_stack_xmm(language_t *ctx, ir_node_t  *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT((node->instruction == IR_INSTR_PUSH_XMM ||
               node->instruction == IR_INSTR_POP_XMM) &&
              node->first.type == ARG_TYPE_XMM,
              return LANGUAGE_UNEXPECTED_IR_INSTR);
    //-----------------------------------------------------------------------//
    if(node->instruction == IR_INSTR_PUSH_XMM) {
        ir_node_t sub_rsp = {
            .instruction = IR_INSTR_SUB,
            .first = _REG(REGISTER_RSP),
            .second = _IMM(sizeof(double)),
            .next = NULL,
            .prev = NULL,
        };
        ir_node_t mov = {
            .instruction = IR_INSTR_MOV,
            .first = _MEM(REGISTER_RSP, 0),
            .second = _XMM(node->first.xmm),
            .next = NULL,
            .prev = NULL,
        };
        _RETURN_IF_ERROR(emit_add_sub(ctx, &sub_rsp));
        _RETURN_IF_ERROR(emit_mov(ctx, &mov));
    }
    else {
        ir_node_t mov = {
            .instruction = IR_INSTR_MOV,
            .first = _XMM(node->first.xmm),
            .second = _MEM(REGISTER_RSP, 0),
            .next = NULL,
            .prev = NULL,
        };
        ir_node_t add_rsp = {
            .instruction = IR_INSTR_ADD,
            .first = _REG(REGISTER_RSP),
            .second = _IMM(8),
            .next = NULL,
            .prev = NULL,
        };
        _RETURN_IF_ERROR(emit_mov(ctx, &mov));
        _RETURN_IF_ERROR(emit_add_sub(ctx, &add_rsp));
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

REX_prefix_t create_rex(ir_arg_t *reg, ir_arg_t *rm) {
    _C_ASSERT(reg != NULL, return (REX_prefix_t){});
    _C_ASSERT(rm  != NULL, return (REX_prefix_t){});
    //-----------------------------------------------------------------------//
    REX_prefix_t rex = {};
    //-----------------------------------------------------------------------//
    // Adding 64 bit registers bit if they are used
    if(reg->type == ARG_TYPE_REG || rm->type == ARG_TYPE_REG) {
        rex.W = 1;
    }
    //-----------------------------------------------------------------------//
    // Adding REG registers extention bit if needed
    if(is_extended_reg(reg)) {
        rex.R = 1;
    }
    //-----------------------------------------------------------------------//
    // Adding RM registers extention bit if needed
    if(is_extended_reg(rm)) {
        rex.B = 1;
    }
    //-----------------------------------------------------------------------//
    // Setting REX default values if only REX is used
    if(rex.byte != 0) {
        rex.unused = 0b0100;
    }
    //-----------------------------------------------------------------------//
    return rex;
}

//===========================================================================//

bool is_extended_reg(ir_arg_t *arg) {
    _C_ASSERT(arg != NULL, return false);
    //-----------------------------------------------------------------------//
    if((arg->type == ARG_TYPE_REG && arg->reg >= REGISTER_R8) ||
       (arg->type == ARG_TYPE_XMM && arg->xmm >= REGISTER_XMM8) ||
       (arg->type == ARG_TYPE_MEM && arg->mem.base >= REGISTER_R8)) {
        return true;
    }
    return false;
    //-----------------------------------------------------------------------//
}

//===========================================================================//

ModRM_t create_regs_modrm(ir_arg_t *reg, ir_arg_t *rm) {
    _C_ASSERT(reg       != NULL,         return (ModRM_t){});
    _C_ASSERT(rm        != NULL,         return (ModRM_t){});
    _C_ASSERT(reg->type == ARG_TYPE_REG ||
              reg->type == ARG_TYPE_XMM,
              return (ModRM_t){});
    _C_ASSERT(rm->type  == ARG_TYPE_REG ||
              rm->type  == ARG_TYPE_XMM,
              return (ModRM_t){});
    //-----------------------------------------------------------------------//
    ModRM_t modrm = {};
    //-----------------------------------------------------------------------//
    // mod = register, register
    modrm.mod = 0b11;
    //-----------------------------------------------------------------------//
    // Registers values
    modrm.reg = (reg->reg - 1) & 7;
    modrm.rm  = (rm->reg  - 1) & 7;
    //-----------------------------------------------------------------------//
    return modrm;
}

//===========================================================================//
