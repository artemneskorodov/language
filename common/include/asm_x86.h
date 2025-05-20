//===========================================================================//
#ifndef ASM_X86_H
#define ASM_X86_H
//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t backend_ir_ctor            (language_t        *ctx,
                                             size_t             capacity);

language_error_t backend_ir_dtor            (language_t        *ctx);

language_error_t ir_add_node                (language_t        *ctx,
                                             ir_instr_t         instr,
                                             ir_arg_t           arg1,
                                             ir_arg_t           arg2);

language_error_t ir_remove_node             (language_t        *ctx,
                                             ir_node_t         *node);

language_error_t x86_compile_subtree        (language_t        *ctx,
                                             language_node_t   *root);

language_error_t x86_assemble_two_args      (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_one_arg       (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_comparison    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_statements    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_if            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_while         (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_return        (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_params_line   (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_new_var       (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_new_func      (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_assignment    (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_in            (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_out           (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_call          (language_t        *ctx,
                                             language_node_t   *node);

language_error_t x86_assemble_exit          (language_t        *ctx,
                                             language_node_t   *node);

extern const char   *StdInName;
extern const size_t  StdInLen;
extern const char   *StdOutName;
extern const size_t  StdOutLen;
extern const char   *MainFunctionName;
extern const size_t  MainFunctionLen;
extern const char   *StdLibFilename;
extern const size_t  StdLibFilenameLen;

//===========================================================================//
#endif
//===========================================================================//
