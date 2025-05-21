#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <string.h>

//===========================================================================//

#include "language.h"
#include "backend.h"
#include "colors.h"
#include "lang_dump.h"
#include "utils.h"
#include "name_table.h"
#include "nodes_dsl.h"
#include "custom_assert.h"
#include "buffer.h"
#include "encoder.h"
#include "optimize_ir.h"

//===========================================================================//

static const size_t IRDefaultCapacity     = 1024;
static const size_t FixupsDefaultCapacity = 1024;
static const size_t ElfHeadersSize        = sizeof(Elf64_Ehdr) +
                                            2 * sizeof(Elf64_Phdr);
static const size_t BaseLoadingAddress    = 0x400000;
static const size_t SectionsAlignment     = 0x1000;
static const size_t MaxDoubleLength       = 64;
static const size_t MaxSystemCommandSize  = 256;

//===========================================================================//

static language_error_t compile_only               (language_t    *ctx,
                                                    operation_t    opcode);

static language_error_t write_stdlib               (language_t    *ctx);

static language_error_t create_elf_headers         (language_t    *ctx);

static language_error_t create_elf_file_header     (language_t    *ctx);

static language_error_t create_text_program_header (language_t    *ctx);

static language_error_t create_data_program_header (language_t    *ctx);

static language_error_t find_func_id_index         (language_t    *ctx,
                                                    const char    *name,
                                                    size_t         len,
                                                    size_t        *output);

static language_error_t global_vars_init           (language_t    *ctx);

static language_error_t fixup_headers_sizes        (language_t    *ctx,
                                                    size_t         text_size,
                                                    size_t         data_size);

static language_error_t global_vars_write_text     (language_t    *ctx);

static language_error_t global_vars_write_bin      (language_t    *ctx);

//===========================================================================//

language_error_t backend_ctor(language_t *ctx, int argc, const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(backend_ir_ctor(ctx, IRDefaultCapacity));
    _RETURN_IF_ERROR(fixups_ctor(ctx, FixupsDefaultCapacity));
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    _RETURN_IF_ERROR(read_tree(ctx));
    _RETURN_IF_ERROR(dump_ctor(ctx, "backend"));
    //-----------------------------------------------------------------------//
    ctx->backend_info.output = fopen(ctx->output_file, "wb");
    if(ctx->backend_info.output == NULL) {
        print_error("Error while opening output file.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_elf(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    // Searching for main in name table
    size_t main_index = ctx->name_table.size;
    _RETURN_IF_ERROR(find_func_id_index(ctx,
                                        MainFunctionName,
                                        MainFunctionLen,
                                        &main_index));
    //-----------------------------------------------------------------------//
    // Program start, calling main end exit is here
    ir_add_node(ctx, IR_INSTR_CALL, _CUSTOM(main_index), (ir_arg_t){});
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RDI), _REG(REGISTER_RAX));
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RAX), _IMM(0x3C));
    ir_add_node(ctx, IR_INSTR_SYSCALL, (ir_arg_t){}, (ir_arg_t){});
    // Compiling functions
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_FUNC));
    _RETURN_IF_ERROR(optimize_ir(ctx));
    //-----------------------------------------------------------------------//
    //Writing compiled functions and stdlib in .text section
    _RETURN_IF_ERROR(create_elf_headers(ctx));
    _RETURN_IF_ERROR(encode_ir(ctx));
    _RETURN_IF_ERROR(write_stdlib(ctx));
    //-----------------------------------------------------------------------//
    // Saving .text size and alignment, adding alignment bytes
    size_t text_size = ctx->backend_info.buffer_size - ElfHeadersSize;
    size_t alignment = (SectionsAlignment -
                        text_size % SectionsAlignment) % SectionsAlignment;
    // Adding alignment to text section
    text_size += alignment;
    _RETURN_IF_ERROR(buffer_check_size(ctx, alignment));
    ctx->backend_info.buffer_size += alignment;
    //-----------------------------------------------------------------------//
    // Adding global variables addresses and adding their init in .data
    _RETURN_IF_ERROR(global_vars_init(ctx));
    size_t data_size = ctx->backend_info.used_globals * sizeof(double);
    _RETURN_IF_ERROR(global_vars_write_bin(ctx));
    //-----------------------------------------------------------------------//
    // Fixing functions and global variables addresses, changing sizes in
    // program headers
    _RETURN_IF_ERROR(run_fixups(ctx));
    _RETURN_IF_ERROR(fixup_headers_sizes(ctx, text_size, data_size));
    //-----------------------------------------------------------------------//
    // Writing output to file
    _RETURN_IF_ERROR(buffer_reset(ctx));
    char command[MaxSystemCommandSize] = {};
    snprintf(command, MaxSystemCommandSize, "chmod +x %s", ctx->output_file);
    system(command);
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_spu(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(variables_stack_ctor(ctx, ctx->nodes.capacity));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_VAR));
    //-----------------------------------------------------------------------//
    fprintf(ctx->backend_info.output,
            ";setting bx value to global variables number\r\n"
            "\tpush " SZ_SP "\r\n"
            "\tpop bx\r\n\r\n"
            ";calling main\r\n"
            "\tcall main:\r\n"
            "\tpush ax\r\n"
            "\tout\r\n"
            "\thlt\r\n\r\n",
            ctx->backend_info.used_globals);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_FUNC));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_nasm(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    // Searching for main in name table
    size_t main_index = ctx->name_table.size;
    _RETURN_IF_ERROR(find_func_id_index(ctx,
                                        MainFunctionName,
                                        MainFunctionLen,
                                        &main_index));
    //-----------------------------------------------------------------------//
    // Program start, calling main end exit is here
    ir_add_node(ctx, IR_INSTR_CALL, _CUSTOM(main_index), (ir_arg_t){});
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RDI), _REG(REGISTER_RAX));
    ir_add_node(ctx, IR_INSTR_MOV, _REG(REGISTER_RAX), _IMM(0x3C));
    ir_add_node(ctx, IR_INSTR_SYSCALL, (ir_arg_t){}, (ir_arg_t){});
    //-----------------------------------------------------------------------//
    // Compiling functions
    _RETURN_IF_ERROR(compile_only(ctx, OPERATION_NEW_FUNC));
    dump_ir(ctx);
    //-----------------------------------------------------------------------//
    // Writing .text section
    name_t global_start = _NAME("global _start\n");
    name_t text_section = _NAME("section .text\n");
    name_t start_label  = _NAME("_start:\n");
    _RETURN_IF_ERROR(buffer_write_string(ctx,
                                         global_start.name,
                                         global_start.length));
    _RETURN_IF_ERROR(buffer_write_string(ctx,
                                         text_section.name,
                                         text_section.length));
    _RETURN_IF_ERROR(buffer_write_string(ctx,
                                         start_label.name,
                                         start_label.length));
    _RETURN_IF_ERROR(encode_ir_nasm(ctx));
    //-----------------------------------------------------------------------//
    // Writing data section
    _RETURN_IF_ERROR(global_vars_init(ctx));
    name_t data_section = _NAME("section .data\n");
    _RETURN_IF_ERROR(buffer_write_string(ctx,
                                         data_section.name,
                                         data_section.length));
    _RETURN_IF_ERROR(global_vars_write_text(ctx));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(buffer_reset(ctx));
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t compile_only(language_t *ctx, operation_t opcode) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    language_node_t *node = ctx->root;
    ctx->backend_info.used_locals = 0;
    while(node != NULL) {
        if(is_node_oper_eq(node->left, opcode)) {
            switch(ctx->machine_flag) {
                case MACHINE_SPU: {
                    _RETURN_IF_ERROR(spu_compile_subtree(ctx, node->left));
                    break;
                }
                case MACHINE_ASM_X86:
                case MACHINE_ELF_X86: {
                    _RETURN_IF_ERROR(x86_compile_subtree(ctx, node->left));
                    break;
                }
                default: {
                    print_error("Unexpected machine flag.");
                    return LANGUAGE_UNEXPECTED_MACHINE_FLAG;
                }
            }
        }
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t backend_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->input);
    fclose(ctx->backend_info.output);
    ctx->backend_info.output = NULL;
    ctx->input = NULL;
    _RETURN_IF_ERROR(backend_ir_dtor(ctx));
    _RETURN_IF_ERROR(name_table_dtor(ctx));
    _RETURN_IF_ERROR(nodes_storage_dtor(ctx));
    _RETURN_IF_ERROR(dump_dtor(ctx));
    _RETURN_IF_ERROR(fixups_dtor(ctx));
    free(ctx->backend_info.buffer);
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t write_stdlib(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    FILE *stdlib_file = fopen(StdLibFilename, "rb");
    if(stdlib_file == NULL) {
        print_error("Error while opening stdlib file to read.");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    size_t size = file_size(stdlib_file);
    language_error_t error_code = buffer_check_size(ctx, size);
    if(error_code != LANGUAGE_SUCCESS) {
        fclose(stdlib_file);
        return error_code;
    }
    uint8_t *read_dst = ctx->backend_info.buffer + ctx->backend_info.buffer_size;
    if(fread(read_dst, sizeof(uint8_t), size, stdlib_file) != size) {
        fclose(stdlib_file);
        print_error("Error while reading stdlib file to buffer.");
        return LANGUAGE_READING_STDLIB_ERROR;
    }
    //-----------------------------------------------------------------------//
    size_t funcs_size = ctx->backend_info.buffer_size;
    ctx->backend_info.buffer_size += size;
    //-----------------------------------------------------------------------//
    uint32_t *addresses = (uint32_t *)(ctx->backend_info.buffer +
                                       funcs_size);
    size_t std_in_rip  = addresses[0] + funcs_size - ElfHeadersSize;
    size_t std_out_rip = addresses[1] + funcs_size - ElfHeadersSize;

    size_t std_in_index = 0;
    size_t std_out_index = 0;
    _RETURN_IF_ERROR(find_func_id_index(ctx,
                                        StdInName,
                                        StdInLen,
                                        &std_in_index ));
    _RETURN_IF_ERROR(find_func_id_index(ctx,
                                        StdOutName,
                                        StdOutLen,
                                        &std_out_index));
    identifier_t *std_in_ident  = ctx->name_table.identifiers + std_in_index;
    identifier_t *std_out_ident = ctx->name_table.identifiers + std_out_index;
    std_in_ident->memory_addr  = (long)std_in_rip;
    std_out_ident->memory_addr = (long)std_out_rip;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t add_stdlib_id(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(name_table_add(ctx,
                                    StdInName,
                                    StdInLen,
                                    NULL,
                                    IDENTIFIER_FUNCTION));
    _RETURN_IF_ERROR(name_table_add(ctx,
                                    StdOutName,
                                    StdOutLen,
                                    NULL,
                                    IDENTIFIER_FUNCTION));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t create_elf_headers(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(create_elf_file_header    (ctx));
    _RETURN_IF_ERROR(create_text_program_header(ctx));
    _RETURN_IF_ERROR(create_data_program_header(ctx));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t create_elf_file_header(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    Elf64_Ehdr ehdr = {
        //-------------------------------------------------------------------//
        // Magic number and other info
        .e_ident     = {ELFMAG0,
                        'E', 'L', 'F',
                        ELFCLASS64,
                        ELFDATA2LSB,
                        EV_CURRENT},
        //-------------------------------------------------------------------//
        // Object file type
  	    .e_type      = ET_EXEC,
        //-------------------------------------------------------------------//
        // Architecture
  	    .e_machine   = EM_X86_64,
        //-------------------------------------------------------------------//
        // Object file version
  	    .e_version   = EV_CURRENT,
        //-------------------------------------------------------------------//
        // Entry point virtual address
  	    .e_entry     = BaseLoadingAddress + ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Program header table file offset
  	    .e_phoff     = sizeof(Elf64_Ehdr),
        //-------------------------------------------------------------------//
        // Section header table file offset
  	    .e_shoff     = 0,
        //-------------------------------------------------------------------//
        // Processor-specific flags
  	    .e_flags     = 0,
        //-------------------------------------------------------------------//
        // ELF header size in bytes
  	    .e_ehsize    = sizeof(Elf64_Ehdr),
        //-------------------------------------------------------------------//
        // Program header table entry size
  	    .e_phentsize = sizeof(Elf64_Phdr),
        //-------------------------------------------------------------------//
        // Program header table entry count
  	    .e_phnum     = 2,
        //-------------------------------------------------------------------//
        // Section header table entry size
  	    .e_shentsize = 0,
        //-------------------------------------------------------------------//
        // Section header table entry count
  	    .e_shnum     = 0,
        //-------------------------------------------------------------------//
        // Section header string table index
  	    .e_shstrndx  = SHN_UNDEF,
        //-------------------------------------------------------------------//
    };
    _RETURN_IF_ERROR(buffer_write(ctx,
                                  (uint8_t *)&ehdr,
                                  sizeof(Elf64_Ehdr)));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t create_text_program_header(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    Elf64_Phdr phdr_text = {
        //-------------------------------------------------------------------//
        // Segment type
        .p_type      = PT_LOAD,
        //-------------------------------------------------------------------//
        // Segment flags
        .p_flags     = PF_R | PF_X,
        //-------------------------------------------------------------------//
        // Segment file offset
        .p_offset    = ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment virtual address
        .p_vaddr     = BaseLoadingAddress + ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment physical address
        .p_paddr     = BaseLoadingAddress + ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment size in file
        .p_filesz    = 0,
        //-------------------------------------------------------------------//
        // Segment size in memory
        .p_memsz     = 0,
        //-------------------------------------------------------------------//
        // Segment alignment
        .p_align     = SectionsAlignment,
        //-------------------------------------------------------------------//
    };
    _RETURN_IF_ERROR(buffer_write(ctx,
                                  (uint8_t *)&phdr_text,
                                  sizeof(Elf64_Phdr)));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t create_data_program_header(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    Elf64_Phdr phdr_data = {
        //-------------------------------------------------------------------//
        // Segment type
        .p_type      = PT_LOAD,
        //-------------------------------------------------------------------//
        // Segment flags
        .p_flags     = PF_R | PF_W,
        //-------------------------------------------------------------------//
        // Segment file offset
        .p_offset    = ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment virtual address
        .p_vaddr     = BaseLoadingAddress + ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment physical address
        .p_paddr     = BaseLoadingAddress + ElfHeadersSize,
        //-------------------------------------------------------------------//
        // Segment size in file
        .p_filesz    = 0,
        //-------------------------------------------------------------------//
        // Segment size in memory
        .p_memsz     = 0,
        //-------------------------------------------------------------------//
        // Segment alignment
        .p_align     = SectionsAlignment,
        //-------------------------------------------------------------------//
    };
    _RETURN_IF_ERROR(buffer_write(ctx,
                                  (uint8_t *)&phdr_data,
                                  sizeof(Elf64_Phdr)));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t find_func_id_index(language_t *ctx,
                                    const char *name,
                                    size_t      len,
                                    size_t     *output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(name   != NULL, return LANGUAGE_INPUT_NULL );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    size_t func_id_index = ctx->name_table.size;
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(ident->type == IDENTIFIER_FUNCTION &&
           ident->length == len &&
           ident->name != NULL &&
           strncmp(ident->name, name, len) == 0) {
            func_id_index = i;
            break;
        }
    }
    if(func_id_index == ctx->name_table.size) {
        print_error("There is no standard function '%.*s' in name table.",
                    len,
                    name);
        return LANGUAGE_NO_STD_FUNC;
    }
    *output = func_id_index;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t global_vars_init(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    language_node_t *node = ctx->root;
    while(node != NULL) {
        if(is_node_oper_eq(node->left, OPERATION_NEW_VAR)) {
            ctx->backend_info.used_globals++;
            language_node_t *var = NULL;
            if(is_node_oper_eq(node->left->left, OPERATION_ASSIGNMENT)) {
                var = node->left->left->left;
            }
            else {
                var = node->left->left;
            }
            identifier_t *ident = ctx->name_table.identifiers +
                                  var->value.identifier;

            if(is_node_oper_eq(node->left->left, OPERATION_ASSIGNMENT)) {
                ident->init_value = node->left->left->right->value.number;
            }
            else {
                ident->init_value = 0;
            }
        }
        node = node->right;
    }
    //-----------------------------------------------------------------------//
    // Adding initialize values of global variables in .data segment
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(ident->is_global) {
            ident->memory_addr = current_rip(ctx);
            uint64_t value = (uint64_t)ident->init_value;
            _RETURN_IF_ERROR(buffer_write_qword(ctx, value));
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t fixup_headers_sizes(language_t *ctx,
                                     size_t      text_size,
                                     size_t      data_size) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    Elf64_Phdr *text_phdr_ptr = (Elf64_Phdr *)(ctx->backend_info.buffer +
                                               sizeof(Elf64_Ehdr));
    Elf64_Phdr *data_phdr_ptr = text_phdr_ptr + 1;

    text_phdr_ptr->p_filesz = text_size;
    text_phdr_ptr->p_memsz  = text_size;

    data_phdr_ptr->p_offset += text_size;
    data_phdr_ptr->p_vaddr  += text_size;
    data_phdr_ptr->p_paddr  += text_size;

    data_phdr_ptr->p_filesz = data_size;
    data_phdr_ptr->p_memsz  = data_size;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

long current_rip(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    return (long)(ctx->backend_info.buffer_size - ElfHeadersSize);
}

//===========================================================================//

language_error_t global_vars_write_text(language_t *ctx) {
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(ident->is_global) {
            _RETURN_IF_ERROR(buffer_write_string(ctx,
                                                 ident->name,
                                                 ident->length));
            _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));
            _RETURN_IF_ERROR(buffer_write_byte(ctx, 'd'));
            _RETURN_IF_ERROR(buffer_write_byte(ctx, 'q'));
            _RETURN_IF_ERROR(buffer_write_byte(ctx, ' '));

            _RETURN_IF_ERROR(buffer_check_size(ctx, MaxDoubleLength));
            uint8_t *buffer = ctx->backend_info.buffer +
                              ctx->backend_info.buffer_size;
            int printed_symbols = snprintf((char *)buffer,
                                           MaxDoubleLength,
                                           "%.1f",
                                           ident->init_value);
            ctx->backend_info.buffer_size += (size_t)printed_symbols;

            _RETURN_IF_ERROR(buffer_write_byte(ctx, '\n'));
        }
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t global_vars_write_bin(language_t *ctx) {
    for(size_t i = 0; i < ctx->name_table.size; i++) {
        identifier_t *ident = ctx->name_table.identifiers + i;
        if(ident->is_global) {
            ident->memory_addr = current_rip(ctx);
            uint64_t value = (uint64_t)ident->init_value;
            _RETURN_IF_ERROR(buffer_write_qword(ctx, value));
        }
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
