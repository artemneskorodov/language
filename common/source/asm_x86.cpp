#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "asm_x86.h"
#include "utils.h"
#include "colors.h"
#include "nodes_dsl.h"
#include "name_table.h"
#include "custom_assert.h"

//===========================================================================//

static language_error_t x86_compile_function_call (language_t      *ctx,
                                                   language_node_t *root);

static language_error_t x86_compile_identifier    (language_t      *ctx,
                                                   language_node_t *root);

static ir_node_t       *ir_last_node              (language_t      *ctx);

static language_error_t compile_cmp_zero          (language_t      *ctx);

static language_error_t compile_params_addrs      (language_t      *ctx,
                                                   language_node_t *param_linker);

static language_error_t compile_locals_addrs      (language_t      *ctx,
                                                   language_node_t *st_linker);

//===========================================================================//

const char   *StdInName         = "std_in";
const size_t  StdInLen          = 6;
const char   *StdOutName        = "std_out";
const size_t  StdOutLen         = 7;
const char   *MainFunctionName  = "main";
const size_t  MainFunctionLen   = 4;
const char   *StdLibFilename    = "stdkvm/stdkvm.lib";
const size_t  StdLibFilenameLen = 6;

//===========================================================================//

#define _CMD_WRITE(_format, ...) \
    _RETURN_IF_ERROR(write_command((ctx), (_format) ,##__VA_ARGS__))

//===========================================================================//

language_error_t x86_compile_subtree(language_t      *ctx,
                                     language_node_t *node) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(node == NULL) {
        return LANGUAGE_SUCCESS;
    }
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(x86_compile_identifier(ctx, node));
            break;
        }
        case NODE_TYPE_NUMBER: {
            ir_add_node(ctx, IR_INSTR_MOV,
                        _REG(REGISTER_RAX), _IMM(node->value.identifier));
            ir_add_node(ctx, IR_INSTR_PUSH,
                        _REG(REGISTER_RAX), (ir_arg_t){});
            break;
        }
        case NODE_TYPE_OPERATION: {
            _RETURN_IF_ERROR(KeyWords[node->value.opcode].asm_x86(ctx, node));
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_compile_identifier(language_t      *ctx,
                                        language_node_t *node) {
    _C_ASSERT(ctx                         != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node                        != NULL, return LANGUAGE_NODE_NULL  );
    _C_ASSERT(ctx->name_table.identifiers != NULL, return LANGUAGE_NT_NOT_INIT);
    //-----------------------------------------------------------------------//
    identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
    switch(ident->type) {
        case IDENTIFIER_FUNCTION: {
            _RETURN_IF_ERROR(x86_compile_function_call(ctx, node));
            break;
        }
        case IDENTIFIER_VARIABLE: {
            memory_arg_t mem = {};
            if(!ident->is_global) {
                mem.base   = REGISTER_RBP;
                mem.offset = 8 * ident->memory_addr;
            }
            else {
                mem.base   = REGISTER_RIP;
                mem.offset = (long)node->value.identifier;
            }
            ir_add_node(ctx, IR_INSTR_PUSH, _MEM(mem.base, mem.offset), (ir_arg_t){});
            break;
        }
        default: {
            print_error("Unknown identifier type\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_compile_function_call(language_t      *ctx,
                                           language_node_t *node) {
    _C_ASSERT(ctx                         != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(node                        != NULL, return LANGUAGE_NODE_NULL  );
    _C_ASSERT(ctx->name_table.identifiers != NULL, return LANGUAGE_NT_NOT_INIT);
    identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
    //-----------------------------------------------------------------------//
    // Pushing function parameters
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    ir_add_node(ctx, IR_INSTR_CALL,
                _CUSTOM(node->value.identifier), (ir_arg_t){});
    // Removing parameters from stack
    ir_add_node(ctx, IR_INSTR_ADD,
                _REG(REGISTER_RSP), _IMM(8 * ident->parameters_number));
    // Pushing return value to stack
    ir_add_node(ctx, IR_INSTR_PUSH_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_two_args(language_t      *ctx,
                                       language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Calculating left and right
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    // Getting left and right values from stack
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM1), (ir_arg_t){});
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    ir_instr_t instruction = (ir_instr_t)0;
    if(node->value.opcode == OPERATION_ADD) {
        instruction = IR_INSTR_ADD;
    }
    else if(node->value.opcode == OPERATION_SUB) {
        instruction = IR_INSTR_SUB;
    }
    else if(node->value.opcode == OPERATION_MUL) {
        instruction = IR_INSTR_MUL;
    }
    else if(node->value.opcode == OPERATION_DIV) {
        instruction = IR_INSTR_DIV;
    }
    else {
        print_error("Unknown two args instruction.");
        return LANGUAGE_UNKNOWN_NODE_TYPE;
    }
    // Running instruction and pushing result to stack
    ir_add_node(ctx, instruction,
                _XMM(REGISTER_XMM0), _XMM(REGISTER_XMM1));
    ir_add_node(ctx, IR_INSTR_PUSH_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_one_arg(language_t      *ctx,
                                      language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Calculating result
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    // Getting result from stack, calculating instruction value and pushing
    if(is_node_oper_eq(node, OPERATION_SQRT)) {
        ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
        ir_add_node(ctx, IR_INSTR_SQRT,
                    _XMM(REGISTER_XMM0), _XMM(REGISTER_XMM0));
        ir_add_node(ctx, IR_INSTR_PUSH_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    }
    else {
        print_error("Sorry, cos, sin and pow are not implemented for x86");
        return LANGUAGE_NOT_IMPLEMENTED;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_comparison(language_t      *ctx,
                                         language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Calculating left and right values
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    // Getting left and right values from stack to XMM0 and XMM1
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM1), (ir_arg_t){});
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    // Comparing left and right values, depending on operation and moving result
    // to RAX
    if(is_node_oper_eq(node, OPERATION_SMALLER)) {
        ir_add_node(ctx, IR_INSTR_CMPL,
                    _XMM(REGISTER_XMM0), _XMM(REGISTER_XMM1));
        ir_add_node(ctx, IR_INSTR_MOV,
                    _REG(REGISTER_RAX), _XMM(REGISTER_XMM0));
    }
    else if(is_node_oper_eq(node, OPERATION_BIGGER)) {
        ir_add_node(ctx, IR_INSTR_CMPL,
                    _XMM(REGISTER_XMM1), _XMM(REGISTER_XMM0));
        ir_add_node(ctx, IR_INSTR_MOV,
                    _REG(REGISTER_RAX), _XMM(REGISTER_XMM1));
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_assignment(language_t      *ctx,
                                     language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(is_node_type_eq(node->left, NODE_TYPE_IDENTIFIER),
              return LANGUAGE_UNEXPECTED_NODE_TYPE);
    //-----------------------------------------------------------------------//
    size_t        id_index = node->left->value.identifier;
    identifier_t *ident    = ctx->name_table.identifiers + id_index;
    //-----------------------------------------------------------------------//
    // Calculating result of right node
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    // Poping result to variable
    if(!ident->is_global) {
        ir_add_node(ctx, IR_INSTR_POP,
                    _MEM(REGISTER_RBP, 8 * ident->memory_addr), (ir_arg_t){});
    }
    else {
        ir_add_node(ctx, IR_INSTR_POP,
                    _MEM(REGISTER_RIP, id_index), (ir_arg_t){});
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_statements(language_t      *ctx,
                                         language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    while(node != NULL) {
        _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
        node = node->right;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_if(language_t      *ctx,
                                 language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Condition
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    if(!is_node_oper_eq(node->left, OPERATION_BIGGER) &&
       !is_node_oper_eq(node->left, OPERATION_SMALLER)) {
        _RETURN_IF_ERROR(compile_cmp_zero(ctx));
    }
    // Checking if result(RAX) is FALSE
    ir_add_node(ctx, IR_INSTR_TEST, _REG(REGISTER_RAX), _REG(REGISTER_RAX));
    // Jump to skip body
    ir_add_node(ctx, IR_INSTR_JZ, _CUSTOM(NULL), (ir_arg_t){});
    ir_node_t *jmp_node = ir_last_node(ctx);
    //-----------------------------------------------------------------------//
    // Body
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    // Node which is if end
    ir_add_node(ctx, IR_CONTROL_JMP, _CUSTOM(jmp_node), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_while(language_t      *ctx,
                                    language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Condition start label
    ir_add_node(ctx, IR_CONTROL_JMP, _CUSTOM(NULL), (ir_arg_t){});
    // Condition
    ir_node_t *while_start = ir_last_node(ctx);
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    if(!is_node_oper_eq(node->left, OPERATION_BIGGER) &&
       !is_node_oper_eq(node->left, OPERATION_SMALLER)) {
        _RETURN_IF_ERROR(compile_cmp_zero(ctx));
    }
    // Checking that result of condition is false
    ir_add_node(ctx, IR_INSTR_TEST, _REG(REGISTER_RAX), _REG(REGISTER_RAX));
    // Skipping body if false
    ir_add_node(ctx, IR_INSTR_JZ, _CUSTOM(NULL), (ir_arg_t){});
    ir_node_t *skip_jump_node = ir_last_node(ctx);
    //-----------------------------------------------------------------------//
    // Body
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    //-----------------------------------------------------------------------//
    // Jumping to condition
    ir_add_node(ctx, IR_INSTR_JMP, _CUSTOM(while_start), (ir_arg_t){});
    // Skip body label
    ir_add_node(ctx, IR_CONTROL_JMP, _CUSTOM(skip_jump_node), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_return(language_t      *ctx,
                                 language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Calculating value of left node
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    // Return value in XMM0
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    // Deleting locals from stack
    ir_add_node(ctx, IR_INSTR_ADD,
                _REG(REGISTER_RSP), _IMM(8 * ctx->backend_info.used_locals));
    // Resetting RBP
    ir_add_node(ctx, IR_INSTR_POP,
                _REG(REGISTER_RBP), (ir_arg_t){});
    // Return
    ir_add_node(ctx, IR_INSTR_RET,
                (ir_arg_t){}, (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_params_line(language_t      *ctx,
                                      language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(node->right != NULL) {
        // Lower args have to be pushed first as in CDECL
        _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->right));
    }
    // Calculating value of parameter and pushing it to stack
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_new_var(language_t      *ctx,
                                  language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(is_node_oper_eq(node->left, OPERATION_ASSIGNMENT)) {
        _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_new_func(language_t      *ctx,
                                       language_node_t *node) {
    //-----------------------------------------------------------------------//
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(is_node_type_eq(node->left, NODE_TYPE_IDENTIFIER),
              return LANGUAGE_UNEXPECTED_NODE_TYPE);
    //-----------------------------------------------------------------------//
    size_t id_index = node->left->value.identifier;
    // Function label in IR
    ir_add_node(ctx, IR_CONTROL_FUNC,
                _CUSTOM(id_index), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    // Adding memory addresses for parameters
    language_node_t *params = node->left->left;
    _RETURN_IF_ERROR(compile_params_addrs(ctx, params));
    //-----------------------------------------------------------------------//
    // Adding memory addresses for locals
    ctx->backend_info.used_locals = 1;
    _RETURN_IF_ERROR(compile_locals_addrs(ctx, node->left->right));
    ctx->backend_info.used_locals--;
    size_t total_locals = ctx->backend_info.used_locals;

    // Saving RBP value in stack
    ir_add_node(ctx, IR_INSTR_PUSH, _REG(REGISTER_RBP), (ir_arg_t){});
    // Setting RBP to point to old RBP position in stack
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RBP), _REG(REGISTER_RSP));
    // Allocating memory for local variables
    ir_add_node(ctx, IR_INSTR_SUB,
                _REG(REGISTER_RSP), _IMM(8 * total_locals));
    //-----------------------------------------------------------------------//
    // Function body
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_params_addrs(language_t      *ctx,
                                      language_node_t *param_linker) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    long counter = 0;
    while(param_linker != NULL) {
        language_node_t *id_node = param_linker->left->left;
        if(!is_node_type_eq(id_node, NODE_TYPE_IDENTIFIER)) {
            print_error("Expected to see identifier in node.");
            return LANGUAGE_UNEXPECTED_NODE_TYPE;
        }
        identifier_t *ident = ctx->name_table.identifiers +
                               id_node->value.identifier;
        ident->memory_addr = counter + 2;
        counter++;
        param_linker = param_linker->right;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_locals_addrs(language_t *ctx,
                                      language_node_t *node) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(node == NULL) {
        return LANGUAGE_SUCCESS;
    }
    if(is_node_oper_eq(node, OPERATION_NEW_VAR)) {
        size_t id_index = node->left->value.identifier;
        if(is_node_oper_eq(node->left, OPERATION_ASSIGNMENT)) {
            id_index = node->left->left->value.identifier;
        }
        identifier_t *ident = ctx->name_table.identifiers + id_index;
        ident->memory_addr = - (long)ctx->backend_info.used_locals;
        ctx->backend_info.used_locals++;
        return LANGUAGE_SUCCESS;
    }
    _RETURN_IF_ERROR(compile_locals_addrs(ctx, node->left ));
    _RETURN_IF_ERROR(compile_locals_addrs(ctx, node->right));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_in(language_t      *ctx,
                                 language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Searching for std_in in name table
    size_t id_index = 0;
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(strncmp(ident->name, StdInName, StdInLen) == 0) {
            id_index = i;
            break;
        }
    }
    //-----------------------------------------------------------------------//
    ir_add_node(ctx, IR_INSTR_CALL, _CUSTOM(id_index), (ir_arg_t){});
    size_t dst_id_index = node->left->left->value.identifier;
    identifier_t *dst_ident = ctx->name_table.identifiers + dst_id_index;
    long mem_addr = dst_ident->memory_addr;
    if(dst_ident->is_global) {
        ir_add_node(ctx, IR_INSTR_MOV,
                    _MEM(REGISTER_RIP, dst_id_index), _XMM(REGISTER_XMM0));
    }
    else {
        ir_add_node(ctx, IR_INSTR_MOV,
                    _MEM(REGISTER_RBP, 8 * mem_addr), _XMM(REGISTER_XMM0));
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_out(language_t      *ctx,
                              language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Searching for std_out in name table
    size_t id_index = 0;
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(strncmp(ident->name, StdOutName, StdOutLen) == 0) {
            id_index = i;
            break;
        }
    }
    //-----------------------------------------------------------------------//
    // Calculating parameter value
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left->left));
    //-----------------------------------------------------------------------//
    // Moving parameter to XMM0
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    ir_add_node(ctx, IR_INSTR_CALL, _CUSTOM(id_index), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_call(language_t      *ctx,
                               language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    // Skipping this node as it was added to compatibility
    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t x86_assemble_exit(language_t      *ctx,
                               language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RAX), _IMM(0x3C));
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RDI), _IMM(0));
    ir_add_node(ctx, IR_INSTR_SYSCALL, (ir_arg_t){}, (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_cmp_zero(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    // Getting last value from stack
    ir_add_node(ctx, IR_INSTR_POP_XMM, _XMM(REGISTER_XMM0), (ir_arg_t){});
    // Setting XMM1 to 0
    ir_add_node(ctx, IR_INSTR_XOR,
                _XMM(REGISTER_XMM1), _XMM(REGISTER_XMM1));
    // Comparing and moving result to RAX
    ir_add_node(ctx, IR_INSTR_CMPEQ,
                _XMM(REGISTER_XMM0), _XMM(REGISTER_XMM1));
    ir_add_node(ctx, IR_INSTR_MOV,
                _REG(REGISTER_RAX), _XMM(REGISTER_XMM0));
    ir_add_node(ctx, IR_INSTR_NOT,
                _REG(REGISTER_RAX), (ir_arg_t){});
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t backend_ir_ctor(language_t *ctx, size_t capacity) {
    ctx->backend_info.nodes = (ir_node_t *)calloc(capacity, sizeof(ir_node_t));
    if(ctx->backend_info.nodes == NULL) {
        print_error("Error while allocating ir nodes storage.");
        return LANGUAGE_MEMORY_ERROR;
    }

    ctx->backend_info.ir_size = 0;
    ctx->backend_info.ir_capacity = capacity;
    for(size_t i = 1; i + 1 < capacity; i++) {
        ctx->backend_info.nodes[i].next = &ctx->backend_info.nodes[i] + 1;
    }
    ctx->backend_info.free = &ctx->backend_info.nodes[0] + 1;
    ctx->backend_info.nodes[0].next = &ctx->backend_info.nodes[0];
    ctx->backend_info.nodes[0].prev = &ctx->backend_info.nodes[0];
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t backend_ir_dtor(language_t *ctx) {
    free(ctx->backend_info.nodes);
    ctx->backend_info.nodes = NULL;
    ctx->backend_info.ir_size = 0;
    ctx->backend_info.ir_capacity = 0;
    ctx->backend_info.free = NULL;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t ir_add_node(language_t *ctx,
                             ir_instr_t  instr,
                             ir_arg_t    arg1,
                             ir_arg_t    arg2) {
    ir_node_t *new_node    = ctx->backend_info.free;
    ctx->backend_info.free = ctx->backend_info.free->next;

    new_node->instruction  = instr;
    new_node->first        = arg1;
    new_node->second       = arg2;

    new_node->next = &ctx->backend_info.nodes[0];
    ir_node_t *last = ctx->backend_info.nodes[0].prev;
    last->next = new_node;
    new_node->prev = last;
    ctx->backend_info.nodes[0].prev = new_node;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t ir_remove_node(language_t *ctx, ir_node_t *node) {
    ir_node_t *prev = node->prev;
    ir_node_t *next = node->next;
    prev->next = next;
    next->prev = prev;
    memset(node, 0, sizeof(ir_node_t));
    node->next = ctx->backend_info.free;
    ctx->backend_info.free = node;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

ir_node_t *ir_last_node(language_t *ctx) {
    return ctx->backend_info.nodes[0].prev;
}

//===========================================================================//
