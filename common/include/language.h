#ifndef LANGUAGE_H
#define LANGUAGE_H

//===========================================================================//

#include <stdio.h>
#include <stdint.h>

//===========================================================================//

enum language_error_t {
    LANGUAGE_SUCCESS                 = 0 ,
    LANGUAGE_OPENING_FILE_ERROR      = 1 ,
    LANGUAGE_MEMORY_ERROR            = 2 ,
    LANGUAGE_READING_SOURCE_ERROR    = 3 ,
    LANGUAGE_UNEXPECTED_WORD_START   = 4 ,
    LANGUAGE_UNCLOSED_COMMENT        = 5 ,
    LANGUAGE_SYNTAX_UNEXPECTED_CALL  = 6 ,
    LANGUAGE_UNKNOWN_NODE_TYPE       = 7 ,
    LANGUAGE_UNKNOWN_IDENTIFIER_TYPE = 8 ,
    LANGUAGE_SYNTAX_ERROR            = 9 ,
    LANGUAGE_INVALID_NODE_TYPE       = 10,
    LANGUAGE_INVALID_NODE_VALUE      = 11,
    LANGUAGE_PARSING_FLAGS_ERROR     = 12,
    LANGUAGE_UNKNOWN_FLAG            = 13,
    LANGUAGE_DUMP_FILE_ERROR         = 14,
    LANGUAGE_READING_TREE_ERROR      = 15,
    LANGUAGE_UNKNOWN_CODE_TREE_TYPE  = 16,
    LANGUAGE_BROKEN_KEYWORDS_TABLE   = 17,
    LANGUAGE_TREE_ERROR              = 18,
    LANGUAGE_CTX_NULL                = 19,
    LANGUAGE_NULL_OUTPUT             = 20,
    LANGUAGE_UNSUPPORTED_TREE        = 21,
    LANGUAGE_NULL_PROGRAM_INPUT      = 22,
    LANGUAGE_WRITING_ASM_ERROR       = 23,
    LANGUAGE_NODE_NULL               = 24,
    LANGUAGE_STRING_FORMAT_NULL      = 25,
    LANGUAGE_INPUT_POSITION_NULL     = 26,
    LANGUAGE_INPUT_NULL              = 27,
    LANGUAGE_NT_NOT_INIT             = 27,
    LANGUAGE_UNEXPECTED_ID_TYPE      = 28,
    LANGUAGE_UNEXPECTED_NODE_TYPE    = 29,
    LANGUAGE_UNEXPECTED_OPER         = 30,
    LANGUAGE_NAME_NULL               = 31,
    LANGUAGE_SOURCE_WRITING_ERROR    = 32,
    LANGUAGE_ROOT_NULL               = 33,
    LANGUAGE_BROKEN_NAME_TABLE_ELEM  = 34,
    LANGUAGE_RULES_NULL              = 35,
    LANGUAGE_UNEXPECTED_IR_INSTR     = 36,
    LANGUAGE_NOT_IMPLEMENTED         = 37,
    LANGUAGE_NO_STD_FUNC             = 38,
    LANGUAGE_UNEXPECTED_MACHINE_FLAG = 39,
    LANGUAGE_UNEXPECTED_MACHINE      = 40,
    LANGUAGE_BROKEN_ASM_TABLE        = 41,
};

//---------------------------------------------------------------------------//

enum node_type_t {
    NODE_TYPE_OPERATION              = 1,
    NODE_TYPE_NUMBER                 = 2,
    NODE_TYPE_IDENTIFIER             = 3,
};

//---------------------------------------------------------------------------//

enum operation_t {
    OPERATION_UNKNOWN                = 0,
    OPERATION_ADD                    = 1,
    OPERATION_SUB                    = 2,
    OPERATION_MUL                    = 3,
    OPERATION_DIV                    = 4,
    OPERATION_COS                    = 5,
    OPERATION_SIN                    = 6,
    OPERATION_SQRT                   = 7,
    OPERATION_POW                    = 8,
    OPERATION_BIGGER                 = 9,
    OPERATION_SMALLER                = 10,
    OPERATION_ASSIGNMENT             = 11,
    OPERATION_OPEN_BRACKET           = 12,
    OPERATION_CLOSE_BRACKET          = 13,
    OPERATION_BODY_START             = 14,
    OPERATION_BODY_END               = 15,
    OPERATION_STATEMENT              = 16,
    OPERATION_IF                     = 17,
    OPERATION_WHILE                  = 18,
    OPERATION_RETURN                 = 19,
    OPERATION_PARAM_LINKER           = 20,
    OPERATION_NEW_VAR                = 21,
    OPERATION_NEW_FUNC               = 22,
    OPERATION_IN                     = 23,
    OPERATION_OUT                    = 24,
    OPERATION_CALL                   = 25,
    //add here
    OPERATION_PROGRAM_END            = 26,
};

//---------------------------------------------------------------------------//

enum default_reg_t {
    REGISTER_RIP                     = 0,
    REGISTER_RAX                     = 1,
    REGISTER_RCX                     = 2,
    REGISTER_RDX                     = 3,
    REGISTER_RBX                     = 4,
    REGISTER_RSP                     = 5,
    REGISTER_RBP                     = 6,
    REGISTER_RSI                     = 7,
    REGISTER_RDI                     = 8,
    REGISTER_R8                      = 9,
    REGISTER_R9                      = 10,
    REGISTER_R10                     = 11,
    REGISTER_R11                     = 12,
    REGISTER_R12                     = 13,
    REGISTER_R13                     = 14,
    REGISTER_R14                     = 15,
    REGISTER_R15                     = 16,
};

//---------------------------------------------------------------------------//

enum xmm_reg_t {
    REGISTER_XMM0                    = 1,
    REGISTER_XMM1                    = 2,
    REGISTER_XMM2                    = 3,
    REGISTER_XMM3                    = 4,
    REGISTER_XMM4                    = 5,
    REGISTER_XMM5                    = 6,
    REGISTER_XMM6                    = 7,
    REGISTER_XMM7                    = 8,
    REGISTER_XMM8                    = 9,
    REGISTER_XMM9                    = 10,
    REGISTER_XMM10                   = 11,
    REGISTER_XMM11                   = 12,
    REGISTER_XMM12                   = 13,
    REGISTER_XMM13                   = 14,
    REGISTER_XMM14                   = 15,
    REGISTER_XMM15                   = 16,
};

//---------------------------------------------------------------------------//

enum ir_instr_t {
    IR_INSTR_ADD                     = 1,
    IR_INSTR_SUB                     = 2,
    IR_INSTR_MUL                     = 3,
    IR_INSTR_DIV                     = 4,
    IR_INSTR_PUSH                    = 5,
    IR_INSTR_POP                     = 6,
    IR_INSTR_MOV                     = 7,
    IR_INSTR_CALL                    = 8,
    IR_INSTR_CMPL                    = 9,
    IR_INSTR_CMPEQ                   = 10,
    IR_INSTR_TEST                    = 11,
    IR_INSTR_JZ                      = 12,
    IR_INSTR_JMP                     = 13,
    IR_INSTR_RET                     = 14,
    IR_INSTR_SYSCALL                 = 15,
    IR_INSTR_XOR                     = 16,
    IR_CONTROL_JMP                   = 17,
    IR_CONTROL_FUNC                  = 18,
    IR_INSTR_SQRT                    = 19,
    IR_INSTR_NOT                     = 20,
};

//---------------------------------------------------------------------------//

enum arg_type_t {
    ARG_TYPE_INVALID                 = 0,
    ARG_TYPE_IMM                     = 1,
    ARG_TYPE_REG                     = 2,
    ARG_TYPE_XMM                     = 3,
    ARG_TYPE_CST                     = 4,
    ARG_TYPE_MEM                     = 5,
};

//---------------------------------------------------------------------------//

enum identifier_type_t {
    IDENTIFIER_VARIABLE              = 1,
    IDENTIFIER_FUNCTION              = 2,
};

//---------------------------------------------------------------------------//

enum machine_t {
    MACHINE_SPU                      = 1,
    MACHINE_ASM_X86                  = 2,
    MACHINE_ELF_X86                  = 3,
};

//===========================================================================//

union value_t {
    double                           number;
    size_t                           identifier;
    operation_t                      opcode;
};

//---------------------------------------------------------------------------//

struct source_info_t {
    const char                      *name;
    size_t                           length;
    size_t                           line;
};

//---------------------------------------------------------------------------//

struct language_node_t {
    source_info_t                    source_info;
    node_type_t                      type;
    value_t                          value;
    language_node_t                 *left;
    language_node_t                 *right;
};

//---------------------------------------------------------------------------//

struct identifier_t {
    const char                      *name;
    size_t                           length;
    identifier_type_t                type;
    size_t                           parameters_number;
    bool                             is_defined;
    long                             memory_addr;
    bool                             is_global;
    double                           init_value;
};

//---------------------------------------------------------------------------//

struct name_t {
    size_t                           length;
    const char                      *name;
};

#define _NAME(_string) (name_t){.length = sizeof(_string) - 1,                \
                                .name = (_string)}                            \

//---------------------------------------------------------------------------//

struct name_table_t {
    identifier_t                    *identifiers;
    size_t                           size;
    size_t                           capacity;
    size_t                          *stack;
    size_t                           stack_size;
    name_t                          *used_names;
    size_t                           used_names_size;
};

//---------------------------------------------------------------------------//

struct nodes_storage_t {
    language_node_t                 *nodes;
    size_t                           size;
    size_t                           capacity;
};

//---------------------------------------------------------------------------//

struct frontend_info_t {
    language_node_t                 *position;
    size_t                           current_line;
    size_t                           used_locals;
};

//---------------------------------------------------------------------------//


struct memory_arg_t {
    default_reg_t                    base;
    long                             offset;
};

//---------------------------------------------------------------------------//

struct ir_arg_t {
    arg_type_t                       type;
    union {
        default_reg_t                reg;
        xmm_reg_t                    xmm;
        long                         imm;
        memory_arg_t                 mem;
        void                        *custom;
    };
};

//---------------------------------------------------------------------------//

struct ir_node_t {
    ir_instr_t                       instruction;
    ir_arg_t                         first;
    ir_arg_t                         second;
    ir_node_t                       *next;
    ir_node_t                       *prev;
};

//---------------------------------------------------------------------------//

struct fixup_t {
    size_t                           position;
    size_t                           id_index;
    size_t                           rip;
};

//---------------------------------------------------------------------------//

struct backend_info_t {
    size_t                           used_globals;
    size_t                           used_labels;
    size_t                           used_locals;
    int                              scope;
    FILE                            *output;
    ir_node_t                       *nodes;
    ir_node_t                       *free;
    size_t                           ir_size;
    size_t                           ir_capacity;
    fixup_t                         *fixups;
    size_t                           fixups_size;
    size_t                           fixups_capacity;
    uint8_t                         *buffer;
    size_t                           buffer_size;
    size_t                           buffer_capacity;
};

//---------------------------------------------------------------------------//

struct frontstart_info_t {
    FILE                            *output;
    int                              depth;
};

//---------------------------------------------------------------------------//

struct middleend_info_t {
    size_t                           changes_counter;
};

//---------------------------------------------------------------------------//

struct dump_info_t {
    FILE                            *general_dump;
    size_t                           dumps_number;
    const char                      *filename;
    size_t                           current_scope;
};

//---------------------------------------------------------------------------//

struct language_t {
    dump_info_t                      dump_info;
    name_table_t                     name_table;
    nodes_storage_t                  nodes;
    language_node_t                 *root;
    char                            *input;
    size_t                           input_size;
    const char                      *input_position;
    frontend_info_t                  frontend_info;
    backend_info_t                   backend_info;
    frontstart_info_t                frontstart_info;
    middleend_info_t                 middleend_info;
    const char                      *input_file;
    const char                      *output_file;
    machine_t                        machine_flag;
};

//===========================================================================//

#define NUMBER(_value) (value_t){.number     = (_value)}
#define OPCODE(_value) (value_t){.opcode     = (_value)}
#define IDENT(_value)  (value_t){.identifier = (_value)}

//---------------------------------------------------------------------------//

#define _REG(_value)         (ir_arg_t){.type = ARG_TYPE_REG,                 \
                                        .reg  = (_value)}                     \

#define _XMM(_value)         (ir_arg_t){.type = ARG_TYPE_XMM,                 \
                                        .xmm  = (_value)}                     \

#define _IMM(_value)         (ir_arg_t){.type = ARG_TYPE_IMM,                 \
                                        .imm  = (long)(_value)}               \

#define _MEM(_base, _offset) (ir_arg_t){.type = ARG_TYPE_MEM,                 \
                                        .mem  = (memory_arg_t)                \
                                            {.base   = (_base),               \
                                             .offset = (long)(_offset)}}      \

#define _CUSTOM(_value)      (ir_arg_t){.type = ARG_TYPE_CST,                 \
                                        .custom = (void *)(_value)}           \

//===========================================================================//

language_error_t nodes_storage_ctor (language_t       *ctx,
                                     size_t            capacity);

language_error_t nodes_storage_add  (language_t       *ctx,
                                     node_type_t       type,
                                     value_t           value,
                                     const char       *name,
                                     size_t            length,
                                     language_node_t **output);

language_error_t nodes_storage_dtor (language_t       *ctx);

language_error_t parse_flags        (language_t       *ctx,
                                     int               argc,
                                     const char       *argv[]);

language_error_t read_tree          (language_t       *ctx);

language_error_t write_tree         (language_t       *ctx);

language_error_t verify_keywords    (void);

//===========================================================================//

#include "asm_x86.h"
#include "asm_spu.h"
#include "to_source.h"
#include "simplify_rules.h"

//===========================================================================//

struct keyword_t {
    const char                      *name;
    size_t                           length;
    operation_t                      code;
    language_error_t               (*asm_spu)(language_t *, language_node_t *);
    language_error_t               (*asm_x86)(language_t *, language_node_t *);
    const char                      *assembler_command;
    bool                             is_expression_element;
    language_error_t               (*to_source)(language_t *, language_node_t *);
    size_t                           priority;
    language_error_t               (*simplifier)(language_t *,language_node_t **);
};

//===========================================================================//

#define STR_LEN(_string) (_string), sizeof(_string) - 1

static const keyword_t KeyWords[] = {
    {/*______________________________THIS_FIELD_MUST_BE_HERE_AS_IT_IS_FOR_UNKNOWN_COMMAND______________________________*/},
    {STR_LEN("+"        ), OPERATION_ADD          , spu_assemble_two_args   , x86_assemble_two_args   , "add" , false, to_source_math_op        , 3, simplify_add},
    {STR_LEN("-"        ), OPERATION_SUB          , spu_assemble_two_args   , x86_assemble_two_args   , "sub" , false, to_source_math_op        , 3, simplify_sub},
    {STR_LEN("*"        ), OPERATION_MUL          , spu_assemble_two_args   , x86_assemble_two_args   , "mul" , false, to_source_math_op        , 2, simplify_mul},
    {STR_LEN("/"        ), OPERATION_DIV          , spu_assemble_two_args   , x86_assemble_two_args   , "div" , false, to_source_math_op        , 2, simplify_div},
    {STR_LEN("cos"      ), OPERATION_COS          , spu_assemble_one_arg    , x86_assemble_one_arg    , "cos" , true , to_source_math_func      , 0, NULL        },
    {STR_LEN("sin"      ), OPERATION_SIN          , spu_assemble_one_arg    , x86_assemble_one_arg    , "sin" , true , to_source_math_func      , 0, NULL        },
    {STR_LEN("sqrt"     ), OPERATION_SQRT         , spu_assemble_one_arg    , x86_assemble_one_arg    , "sqrt", true , to_source_new_func       , 0, NULL        },
    {STR_LEN("^"        ), OPERATION_POW          , spu_assemble_two_args   , x86_assemble_two_args   , "pow" , false, to_source_math_op        , 1, simplify_pow},
    {STR_LEN(">"        ), OPERATION_BIGGER       , spu_assemble_comparison , x86_assemble_comparison , "ja"  , false, to_source_math_op        , 4, NULL        },
    {STR_LEN("<"        ), OPERATION_SMALLER      , spu_assemble_comparison , x86_assemble_comparison , "jb"  , false, to_source_math_op        , 4, NULL        },
    {STR_LEN("="        ), OPERATION_ASSIGNMENT   , spu_assemble_assignment , x86_assemble_assignment , NULL  , false, to_source_math_op        , 4, NULL        },
    {STR_LEN("("        ), OPERATION_OPEN_BRACKET , NULL                    , NULL                    , NULL  , false, NULL                     , 0, NULL        },
    {STR_LEN(")"        ), OPERATION_CLOSE_BRACKET, NULL                    , NULL                    , NULL  , false, NULL                     , 0, NULL        },
    {STR_LEN("{"        ), OPERATION_BODY_START   , NULL                    , NULL                    , NULL  , false, NULL                     , 0, NULL        },
    {STR_LEN("}"        ), OPERATION_BODY_END     , NULL                    , NULL                    , NULL  , false, NULL                     , 0, NULL        },
    {STR_LEN(";"        ), OPERATION_STATEMENT    , spu_assemble_statements , x86_assemble_statements , NULL  , false, to_source_statements_line, 0, NULL        },
    {STR_LEN("if"       ), OPERATION_IF           , spu_assemble_if         , x86_assemble_if         , NULL  , false, to_source_if             , 0, NULL        }, //TODO simplification
    {STR_LEN("while"    ), OPERATION_WHILE        , spu_assemble_while      , x86_assemble_while      , NULL  , false, to_source_while          , 0, NULL        },
    {STR_LEN("return"   ), OPERATION_RETURN       , spu_assemble_return     , x86_assemble_return     , NULL  , false, to_source_return         , 0, NULL        },
    {STR_LEN(","        ), OPERATION_PARAM_LINKER , spu_assemble_params_line, x86_assemble_params_line, NULL  , false, to_source_params_line    , 0, NULL        },
    {STR_LEN("var"      ), OPERATION_NEW_VAR      , spu_assemble_new_var    , x86_assemble_new_var    , NULL  , false, to_source_new_var        , 0, NULL        },
    {STR_LEN("func"     ), OPERATION_NEW_FUNC     , spu_assemble_new_func   , x86_assemble_new_func   , NULL  , false, to_source_new_func       , 0, NULL        },
    {STR_LEN("input"    ), OPERATION_IN           , spu_assemble_in         , x86_assemble_in         , NULL  , false, to_source_in             , 0, NULL        },
    {STR_LEN("output"   ), OPERATION_OUT          , spu_assemble_out        , x86_assemble_out        , NULL  , false, to_source_out            , 0, NULL        },
    {STR_LEN("call"     ), OPERATION_CALL         , spu_assemble_call       , x86_assemble_call       , NULL  , false, to_source_call           , 0, NULL        },
    {STR_LEN("ExitKPM"  ),OPERATION_PROGRAM_END   , spu_assemble_exit       , x86_assemble_exit       , NULL  , false, to_source_exit           , 0, NULL        },
};

#undef STR_LEN

//===========================================================================//

#define _RETURN_IF_ERROR(...) {                     \
    language_error_t _error_code = (__VA_ARGS__);   \
    if(_error_code != LANGUAGE_SUCCESS) {           \
        return _error_code;                         \
    }                                               \
}                                                   \

//===========================================================================//

static const size_t PoisonIndex = (size_t)(-1);

//===========================================================================//

#endif
