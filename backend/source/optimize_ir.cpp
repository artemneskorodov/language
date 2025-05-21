//===========================================================================//

#include "optimize_ir.h"
#include "lang_dump.h"
#include "custom_assert.h"

//===========================================================================//

static language_error_t optimize_stack    (language_t *ctx,
                                           size_t     *counter);

static language_error_t optimize_neutral  (language_t *ctx,
                                           size_t     *counter);

static language_error_t set_not_optimized (language_t *ctx);

static bool             args_equal        (ir_arg_t   *first,
                                           ir_arg_t   *second);

static bool             is_push           (ir_node_t  *node);

//===========================================================================//

language_error_t optimize_ir(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    while(true) {
        _RETURN_IF_ERROR(dump_ir(ctx));
        _RETURN_IF_ERROR(set_not_optimized(ctx))
        size_t counter = 0;
        _RETURN_IF_ERROR(optimize_stack(ctx, &counter));
        _RETURN_IF_ERROR(optimize_neutral(ctx, &counter));
        if(counter == 0) {
            break;
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t optimize_stack(language_t *ctx, size_t *counter) {
    _C_ASSERT(ctx     != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(counter != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    ir_node_t *node = ctx->backend_info.nodes[0].next;
    while(node->next != &ctx->backend_info.nodes[0]) {
        ir_node_t *next = node->next;
        if(is_push(node) &&
           is_pop(next) &&
           node->second.type != ARG_TYPE_IMM &&
           (node->first.type != ARG_TYPE_MEM ||
            next->first.type != ARG_TYPE_MEM)) {
           //---------------------------------------------------------------//
            (*counter)++;
            node->instruction = IR_INSTR_MOV;
            node->second = node->first; // Copying structure
            node->first  = next->first;
            node->is_optimized = true;
            _RETURN_IF_ERROR(ir_remove_node(ctx, next));
            //---------------------------------------------------------------//
        }
        node = node->next;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t optimize_neutral(language_t *ctx, size_t *counter) {
    _C_ASSERT(ctx     != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(counter != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    ir_node_t *node = ctx->backend_info.nodes[0].next;
    while(node != &ctx->backend_info.nodes[0]) {
        ir_node_t *next = node->next;
        // mov src == dst
        if(node->instruction == IR_INSTR_MOV &&
           args_equal(&node->first, &node->second)) {
            _RETURN_IF_ERROR(ir_remove_node(ctx, node));
            (*counter)++;
        }
        // add/sub ---, 0
        else if((node->instruction == IR_INSTR_SUB ||
                 node->instruction == IR_INSTR_ADD) &&
                node->second.type == ARG_TYPE_IMM && node->second.imm == 0) {
            _RETURN_IF_ERROR(ir_remove_node(ctx, node));
            (*counter)++;
        }
        node = next;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

bool args_equal(ir_arg_t *first, ir_arg_t *second) {
    _C_ASSERT(first  != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(second != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(first->type != second->type) {
        return false;
    }
    if(first->type == ARG_TYPE_MEM &&
       first->mem.base == second->mem.base &&
       first->mem.offset == second->mem.offset) {
        return true;
    }
    if(first->imm == second->imm) {
        return true;
    }
    return false;
    //-----------------------------------------------------------------------//
}

//===========================================================================//

bool is_push(ir_node_t *node) {
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(node->instruction == IR_INSTR_PUSH ||
       node->instruction == IR_INSTR_PUSH_XMM) {
        return true;
    }
    return false;
    //-----------------------------------------------------------------------//
}

//===========================================================================//

bool is_pop(ir_node_t *node) {
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    if(node->instruction == IR_INSTR_POP ||
       node->instruction == IR_INSTR_POP_XMM) {
        return true;
    }
    return false;
    //-----------------------------------------------------------------------//
}

//===========================================================================//

language_error_t set_not_optimized(language_t *ctx) {
    //-----------------------------------------------------------------------//
    ir_node_t *node = ctx->backend_info.nodes[0].next;
    while(node != &ctx->backend_info.nodes[0]) {
        node->is_optimized = false;
        node = node->next;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
