#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>

#include "language.h"
#include "buffer.h"
#include "colors.h"
#include "custom_assert.h"
static const size_t BufferInitSize = 1024;

//===========================================================================//

language_error_t buffer_write_byte(language_t *ctx, uint8_t byte) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_check_size(ctx, sizeof(byte)));
    ctx->backend_info.buffer[ctx->backend_info.buffer_size++] = byte;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t buffer_write_qword(language_t *ctx, uint64_t data) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_check_size(ctx, sizeof(data)));
    uint64_t *new_elem = (uint64_t *)(ctx->backend_info.buffer + ctx->backend_info.buffer_size);
    *new_elem = data;
    ctx->backend_info.buffer_size += sizeof(data);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t buffer_write(language_t *ctx,
                              const uint8_t *data,
                              size_t size) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL  );
    _C_ASSERT(data != NULL, return LANGUAGE_INPUT_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_check_size(ctx, size));
    memcpy(ctx->backend_info.buffer + ctx->backend_info.buffer_size, data, size);
    ctx->backend_info.buffer_size += size;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t buffer_reset(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    uint8_t *buffer = ctx->backend_info.buffer;
    size_t size = ctx->backend_info.buffer_size;
    if(fwrite(buffer, sizeof(uint8_t), size, ctx->backend_info.output) != size) {
        print_error("Error while writing output in file.");
        return LANGUAGE_WRITING_ASM_ERROR;
    }
    free(ctx->backend_info.buffer);
    ctx->backend_info.buffer          = NULL;
    ctx->backend_info.buffer_capacity = 0;
    ctx->backend_info.buffer_size     = 0;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t buffer_check_size(language_t *ctx, size_t needed_size) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    size_t size = ctx->backend_info.buffer_size;
    size_t capacity = ctx->backend_info.buffer_capacity;
    size_t old_capacity = capacity;
    //-----------------------------------------------------------------------//
    while(size + needed_size >= capacity) {
        if(capacity == 0) {
            capacity = 256;
        }
        else {
            capacity *= 2;
        }
    }
    if(capacity == old_capacity) {
        return LANGUAGE_SUCCESS;
    }
    uint8_t *new_buffer = (uint8_t *)realloc(ctx->backend_info.buffer,
                                             capacity);
    if(new_buffer == NULL) {
        print_error("Error while reallocating buffer memory.");
        return LANGUAGE_MEMORY_ERROR;
    }

    memset(new_buffer + old_capacity, 0, capacity - old_capacity);
    //-----------------------------------------------------------------------//
    ctx->backend_info.buffer          = new_buffer;
    ctx->backend_info.buffer_capacity = capacity;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t buffer_write_string(language_t *ctx,
                                     const char *data,
                                     size_t size) {
    return buffer_write(ctx, (const uint8_t *)data, size);
}

//===========================================================================//
