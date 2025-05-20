#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "name_table.h"
#include "colors.h"
#include "custom_assert.h"

//===========================================================================//

language_error_t name_table_ctor(language_t *ctx, size_t capacity) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->name_table.identifiers =
        (identifier_t *)calloc(capacity, sizeof(ctx->name_table.identifiers[0]));
    if(ctx->name_table.identifiers == NULL) {
        print_error("Error while allocating memory to identifiers.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    ctx->name_table.size = 0;
    ctx->name_table.capacity = capacity;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t name_table_add(language_t         *ctx,
                                const char         *name,
                                size_t              length,
                                size_t             *output,
                                identifier_type_t   type) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    identifier_t *ident = ctx->name_table.identifiers + ctx->name_table.size;
    ident->name     = name;
    ident->length   = length;
    ident->type     = type;
    //-----------------------------------------------------------------------//
    if(output != NULL) {
        *output = ctx->name_table.size;
    }
    ctx->name_table.size++;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t name_table_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->name_table.identifiers);
    ctx->name_table.identifiers = NULL;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t set_memory_addr(language_t      *ctx,
                                 language_node_t *node,
                                 size_t           addr) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(node != NULL, return LANGUAGE_NODE_NULL);
    //-----------------------------------------------------------------------//
    _C_ASSERT(node->type == NODE_TYPE_IDENTIFIER, return LANGUAGE_INVALID_NODE_TYPE);
    size_t index = node->value.identifier;
    if(index >= ctx->name_table.size) {
        print_error("Node index is bigger that name table size.\n");
    }
    ctx->name_table.identifiers[index].memory_addr = (long)addr;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t variables_stack_ctor(language_t *ctx, size_t capacity) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->name_table.stack = (size_t *)calloc(capacity, sizeof(ctx->name_table.stack[0]));
    if(ctx->name_table.stack == NULL) {
        print_error("Error while allocating memory for variables stack.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    ctx->name_table.stack_size = 0;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t variables_stack_push(language_t *ctx, size_t index) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->name_table.stack[ctx->name_table.stack_size++] = index;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t variables_stack_remove(language_t *ctx, size_t number) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    size_t *new_end = ctx->name_table.stack +
                      ctx->name_table.stack_size -
                      number;
    if(memset(new_end, 0, number * sizeof(ctx->name_table.stack[0])) != new_end) {
        print_error("Error while setting memory to zeros.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    ctx->name_table.stack_size -= number;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t variables_stack_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->name_table.stack);
    ctx->name_table.stack = NULL;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t used_names_ctor(language_t *ctx, size_t capacity) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->name_table.used_names =
        (name_t *)calloc(capacity, sizeof(ctx->name_table.used_names[0]));
    if(ctx->name_table.used_names == NULL) {
        print_error("Error while allocating used names memory.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    ctx->name_table.used_names_size = 0;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t used_names_add(language_t *ctx,
                                const char *name,
                                size_t      length,
                                size_t     *index) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL );
    _C_ASSERT(name != NULL, return LANGUAGE_NAME_NULL);
    //-----------------------------------------------------------------------//
    ctx->name_table.used_names[ctx->name_table.used_names_size].length = length;
    ctx->name_table.used_names[ctx->name_table.used_names_size].name   = name;
    *index = ctx->name_table.used_names_size;

    ctx->name_table.used_names_size++;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t used_names_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->name_table.used_names);
    ctx->name_table.used_names = NULL;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
