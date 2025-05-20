//===========================================================================//

#include <stdio.h>
#include <stdlib.h>

//===========================================================================//

#include "language.h"
#include "encoder.h"
#include "emitters_bin.h"
#include "emitters_asm.h"
#include "custom_assert.h"
#include "colors.h"
#include "backend.h"

//===========================================================================//

language_error_t encode_ir(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ir_node_t *node = ctx->backend_info.nodes[0].next;
    while(node != &ctx->backend_info.nodes[0]) {
        _RETURN_IF_ERROR(IREmitters[node->instruction].emitter(ctx, node));
        node = node->next;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t encode_ir_nasm(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ir_node_t *node = ctx->backend_info.nodes[0].next;
    while(node != &ctx->backend_info.nodes[0]) {
        _RETURN_IF_ERROR(write_asm_code(ctx, node));
        node = node->next;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t fixups_ctor(language_t *ctx, size_t capacity) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->backend_info.fixups = (fixup_t *)calloc(capacity, sizeof(fixup_t));
    if(ctx->backend_info.fixups == NULL) {
        print_error("Error while allocating memory for fixups table.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //-----------------------------------------------------------------------//
    ctx->backend_info.fixups_capacity = capacity;
    ctx->backend_info.fixups_size     = 0;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t fixups_dtor(language_t *ctx) {
    //-----------------------------------------------------------------------//
    free(ctx->backend_info.fixups);
    //-----------------------------------------------------------------------//
    ctx->backend_info.fixups          = NULL;
    ctx->backend_info.fixups_capacity = 0;
    ctx->backend_info.fixups_size     = 0;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t add_fixup(language_t *ctx, size_t offset, size_t id_index) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    size_t size     = ctx->backend_info.fixups_size;
    size_t capacity = ctx->backend_info.fixups_capacity;
    fixup_t *fixups = ctx->backend_info.fixups;
    //-----------------------------------------------------------------------//
    // Checking size
    if(size >= capacity) {
        fixup_t *new_fixups = (fixup_t *)realloc(fixups, capacity * 2);
        if(new_fixups == NULL) {
            print_error("Error while reallocating memory for fixups table.\n");
            return LANGUAGE_MEMORY_ERROR;
        }
        fixups = new_fixups;
        capacity *= 2;
    }
    //-----------------------------------------------------------------------//
    // Saving address to change value, identifier index and instruction end RIP
    fixups[size].position = ctx->backend_info.buffer_size + offset;
    fixups[size].id_index = id_index;
    fixups[size].rip      = (size_t)current_rip(ctx) + offset + 4;
    size++;
    //-----------------------------------------------------------------------//
    ctx->backend_info.fixups          = fixups;
    ctx->backend_info.fixups_capacity = capacity;
    ctx->backend_info.fixups_size     = size;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t run_fixups(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    fixup_t *fixups = ctx->backend_info.fixups;
    for(size_t i = 0; i < ctx->backend_info.fixups_size; i++) {
        // Position in buffer read saved when making fixup
        size_t        position  = fixups[i].position;
        // Identifier pointer in name table
        identifier_t *ident     = ctx->name_table.identifiers +
                                  fixups[i].id_index;
        // Pointer to fix address
        uint32_t     *value_pos = (uint32_t *)(ctx->backend_info.buffer +
                                               position);
        //-------------------------------------------------------------------//
        uint32_t dest_rip              = (uint32_t)ident->memory_addr;
        uint32_t instruction_end_rip   = (uint32_t)fixups[i].rip;
        *value_pos = dest_rip - instruction_end_rip;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
