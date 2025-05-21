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
