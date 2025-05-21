//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

//===========================================================================//

#include "language.h"
#include "lang_dump.h"
#include "colors.h"
#include "custom_assert.h"

//===========================================================================//

static const char *node_color_operation        = "#E8D6CB";
static const char *node_color_statement_end    = "#D0ADA7";
static const char *node_color_number           = "#AD6A6C";
static const char *node_color_ident_function   = "#5D2E46";
static const char *node_color_ident_global_var = "#B58DB6";
static const char *node_color_ident_local_var  = "#CCE2A3";

//===========================================================================//

static const size_t BufferSize = 256;

//===========================================================================//

static language_error_t dump_subtree   (language_t         *ctx,
                                        language_node_t    *root,
                                        size_t              level,
                                        FILE               *dot_file);

static language_error_t write_value     (language_t        *ctx,
                                         language_node_t   *node,
                                         FILE              *dot_file);

static const char      *get_node_color (language_t         *ctx,
                                        language_node_t    *node);

static language_error_t dump_ir_arg(FILE *dot_file, ir_arg_t *arg);
static const char *intr_string(ir_instr_t instr);
static const char *reg_string(default_reg_t reg);


//===========================================================================//

language_error_t dump_ctor(language_t *ctx, const char *filename) {
    _C_ASSERT(ctx      != NULL, return LANGUAGE_CTX_NULL       );
    _C_ASSERT(filename != NULL, return LANGUAGE_DUMP_FILE_ERROR);
    //-----------------------------------------------------------------------//
    ctx->dump_info.filename = filename;
    char dump_name[BufferSize] = {};
    snprintf(dump_name, BufferSize, "logs/%s.html", filename);
    ctx->dump_info.general_dump = fopen(dump_name, "w");
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t dump_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    fclose(ctx->dump_info.general_dump);
    if(memset(&ctx->dump_info, 0, sizeof(ctx->dump_info)) != &ctx->dump_info) {
        print_error("Error while setting dump info to zeros\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t dump_tree(language_t *ctx, const char *format, ...) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(format != NULL, return LANGUAGE_STRING_FORMAT_NULL);
    //-----------------------------------------------------------------------//
    char dot_filename[BufferSize] = {};
    snprintf(dot_filename,
             BufferSize,
             "logs/dot/%s%04lx.dot",
             ctx->dump_info.filename,
             ctx->dump_info.dumps_number);
    FILE *dot_file = fopen(dot_filename, "w");
    if(dot_file == NULL) {
        print_error("Error while opening dot file.\n");
        return LANGUAGE_DUMP_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    fprintf(dot_file,
            "digraph {\n"
            "node[shape = Mrecord, style = filled];\n");
    _RETURN_IF_ERROR(dump_subtree(ctx, ctx->root, 0, dot_file));
    fprintf(dot_file, "}\n");
    fclose(dot_file);
    //-----------------------------------------------------------------------//
    char command[BufferSize] = {};
    snprintf(command,
             BufferSize,
             "dot %s -Tsvg -o logs/img/%s%04lx.svg",
             dot_filename,
             ctx->dump_info.filename,
             ctx->dump_info.dumps_number);
    system(command);
    //-----------------------------------------------------------------------//
    fprintf(ctx->dump_info.general_dump, "<h1>");
    //-----------------------------------------------------------------------//
    va_list args;
    va_start(args, format);
    vfprintf(ctx->dump_info.general_dump, format, args);
    va_end(args);
    fprintf(ctx->dump_info.general_dump,
            "</h1><img src = \"img/%s%04lx.svg\">\n",
            ctx->dump_info.filename,
            ctx->dump_info.dumps_number);
    //-----------------------------------------------------------------------//
    fflush(ctx->dump_info.general_dump);
    ctx->dump_info.dumps_number++;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t dump_subtree(language_t       *ctx,
                              language_node_t  *root,
                              size_t            level,
                              FILE             *dot_file) {
    _C_ASSERT(ctx      != NULL, return LANGUAGE_CTX_NULL);
    _C_ASSERT(root     != NULL, return LANGUAGE_NODE_NULL);
    _C_ASSERT(dot_file != NULL, return LANGUAGE_DUMP_FILE_ERROR);
    //-----------------------------------------------------------------------//
    if(fprintf(dot_file,
               "node%p[fillcolor = \"%s\", rank = %lu, label = \"{%p | {%p | %p} | ",
               root,
               get_node_color(ctx, root),
               level,
               root,
               root->left,
               root->right) < 0) {
        print_error("Error while writing to dot file.\n");
        return LANGUAGE_DUMP_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(write_value(ctx, root, dot_file));
    //-----------------------------------------------------------------------//
    if(root->left != NULL) {
        fprintf(dot_file, "node%p -> node%p;\n", root, root->left);
        _RETURN_IF_ERROR(dump_subtree(ctx, root->left, level + 1, dot_file));
    }
    if(root->right != NULL) {
        fprintf(dot_file, "node%p -> node%p;\n", root, root->right);
        _RETURN_IF_ERROR(dump_subtree(ctx, root->right, level + 1, dot_file));
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t write_value(language_t      *ctx,
                             language_node_t *node,
                             FILE            *dot_file) {
    _C_ASSERT(ctx      != NULL, return LANGUAGE_CTX_NULL       );
    _C_ASSERT(node     != NULL, return LANGUAGE_NODE_NULL      );
    _C_ASSERT(dot_file != NULL, return LANGUAGE_DUMP_FILE_ERROR);
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_NUMBER: {
            if(fprintf(dot_file, "NUMBER | %lg }\"];\n", node->value.number) < 0) {
                print_error("Error while writing to dot_file.\n");
                return LANGUAGE_DUMP_FILE_ERROR;
            }
            break;
        }
        case NODE_TYPE_OPERATION: {
            const char *string_operation = KeyWords[node->value.opcode].name;
            if     (node->value.opcode == OPERATION_BODY_START) {
                string_operation = "body_start";
            }
            else if(node->value.opcode == OPERATION_BODY_END) {
                string_operation = "body_end";
            }
            else if(node->value.opcode == OPERATION_BIGGER) {
                string_operation = "bigger";
            }
            else if(node->value.opcode == OPERATION_SMALLER) {
                string_operation = "smaller";
            }

            if(fprintf(dot_file, "OPERATION | %s}\"];\n", string_operation) < 0) {
                print_error("Error while writing to dot file.\n");
            }
            break;
        }
        case NODE_TYPE_IDENTIFIER: {
            identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
            if(fprintf(dot_file,
                       "IDENTIFIER | %lu - %.*s}\"];\n",
                       node->value.identifier,
                       (int)ident->length,
                       ident->name) < 0) {
                print_error("Error while writing to dot file.\n");
                return LANGUAGE_DUMP_FILE_ERROR;
            }
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

const char *get_node_color(language_t *ctx, language_node_t *node) {
    _C_ASSERT(ctx  != NULL, return NULL );
    _C_ASSERT(node != NULL, return NULL);
    //-----------------------------------------------------------------------//
    switch(node->type) {
        case NODE_TYPE_NUMBER: {
            return node_color_number;
        }
        case NODE_TYPE_IDENTIFIER: {
            identifier_t *ident = ctx->name_table.identifiers + node->value.identifier;
            switch(ident->type) {
                case IDENTIFIER_FUNCTION: {
                    return node_color_ident_function;
                }
                case IDENTIFIER_VARIABLE: {
                    if(ident->is_global) {
                        return node_color_ident_global_var;
                    }
                    return node_color_ident_local_var;
                }
                default: {
                    print_error("Unknown identifier type.\n");
                    return NULL;
                }
            }
        }
        case NODE_TYPE_OPERATION: {
            if(node->value.opcode == OPERATION_STATEMENT) {
                return node_color_statement_end;
            }
            else {
                return node_color_operation;
            }
        }
        default: {
            assert(false);
        }
    }
    //-----------------------------------------------------------------------//
    return NULL;
}

//===========================================================================//

language_error_t dump_ir(language_t *ctx) {
    char dot_filename[BufferSize] = {};
    snprintf(dot_filename,
             BufferSize,
             "logs/dot/%s%04lx.dot",
             ctx->dump_info.filename,
             ctx->dump_info.dumps_number);
    FILE *dot_file = fopen(dot_filename, "w");
    if(dot_file == NULL) {
        print_error("Error while opening dot file.\n");
        return LANGUAGE_DUMP_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    fprintf(dot_file,
            "digraph {\n"
            "rankdir=TB;\n"
            "node[shape = Mrecord, style = filled];\n");
    ir_node_t *node = ctx->backend_info.nodes[0].next;

    while(node->next != &ctx->backend_info.nodes[0]) {
        fprintf(dot_file, "node%p->node%p;\n", node, node->next);
        node = node->next;
    }
    node = ctx->backend_info.nodes[0].next;

    while(node != &ctx->backend_info.nodes[0]) {
        fprintf(dot_file, "node%p[label = \"%s | ", node, intr_string(node->instruction));
        _RETURN_IF_ERROR(dump_ir_arg(dot_file, &node->first));
        fprintf(dot_file, " | ");
        _RETURN_IF_ERROR(dump_ir_arg(dot_file, &node->second));
        fprintf(dot_file, "\" ");
        if(node->is_optimized) {
            fprintf(dot_file, "fillcolor = \"#CCE2A3\"");
        }
        fprintf(dot_file, "]\n");
        if((node->instruction == IR_INSTR_JMP ||
            node->instruction == IR_INSTR_JZ) &&
           node->first.custom != NULL) {
            fprintf(dot_file, "node%p->node%p;\n", node->first.custom, node);
        }
        if(node->instruction == IR_CONTROL_JMP &&
           node->first.custom != NULL) {
            fprintf(dot_file, "node%p->node%p;\n", node->first.custom, node);
        }
        node = node->next;
    }
    fprintf(dot_file, "}\n");
    fclose(dot_file);
    char command[BufferSize] = {};
    snprintf(command,
             BufferSize,
             "dot %s -Tsvg -o logs/img/%s%04lx.svg",
             dot_filename,
             ctx->dump_info.filename,
             ctx->dump_info.dumps_number);
    system(command);
    //-----------------------------------------------------------------------//
    fprintf(ctx->dump_info.general_dump, "<h1>");
    //-----------------------------------------------------------------------//
    fprintf(ctx->dump_info.general_dump,
            "</h1><img src = \"img/%s%04lx.svg\">\n",
            ctx->dump_info.filename,
            ctx->dump_info.dumps_number);
    //-----------------------------------------------------------------------//
    fflush(ctx->dump_info.general_dump);
    ctx->dump_info.dumps_number++;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

language_error_t dump_ir_arg(FILE *dot_file, ir_arg_t *arg) {
    switch(arg->type) {
        case ARG_TYPE_REG: {
            fprintf(dot_file, "REG(%s)", reg_string(arg->reg));
            break;
        }
        case ARG_TYPE_XMM: {
            fprintf(dot_file, "XMM(xmm%d)", arg->xmm - 1);
            break;
        }
        case ARG_TYPE_IMM: {
            if(arg->imm <= 64) {
                fprintf(dot_file, "IMM(%ld)", arg->imm);
            }
            else {
                fprintf(dot_file, "IMM(%f)", *(double *)&arg->imm);
            }
            break;
        }
        case ARG_TYPE_MEM: {
            fprintf(dot_file, "mem_[REG(%s) + %ld]", reg_string(arg->mem.base), arg->mem.offset);
            break;
        }
        case ARG_TYPE_CST: {
            fprintf(dot_file, "%p", arg->custom);
            break;
        }
        case ARG_TYPE_INVALID: {
            break;
        }
        default: {
            break;
        }
    }
    return LANGUAGE_SUCCESS;
}

const char *reg_string(default_reg_t reg) {
    switch(reg) {
        case REGISTER_RIP: {return "rip";}
        case REGISTER_RAX: {return "rax";}
        case REGISTER_RCX: {return "rcx";}
        case REGISTER_RDX: {return "rdx";}
        case REGISTER_RBX: {return "rbx";}
        case REGISTER_RSP: {return "rsp";}
        case REGISTER_RBP: {return "rbp";}
        case REGISTER_RSI: {return "rsi";}
        case REGISTER_RDI: {return "rdi";}
        case REGISTER_R8 : {return "r8" ;}
        case REGISTER_R9 : {return "r9" ;}
        case REGISTER_R10: {return "r10";}
        case REGISTER_R11: {return "r11";}
        case REGISTER_R12: {return "r12";}
        case REGISTER_R13: {return "r13";}
        case REGISTER_R14: {return "r14";}
        case REGISTER_R15: {return "r15";}
        default:           {return NULL ;}
    }
}

const char *intr_string(ir_instr_t instr) {
    switch(instr) {
        case IR_INSTR_ADD    : {return "add";}
        case IR_INSTR_SUB    : {return "sub";}
        case IR_INSTR_MUL    : {return "mul";}
        case IR_INSTR_DIV    : {return "div";}
        case IR_INSTR_PUSH   : {return "push";}
        case IR_INSTR_POP    : {return "pop";}
        case IR_INSTR_MOV    : {return "mov";}
        case IR_INSTR_CALL   : {return "call";}
        case IR_INSTR_CMPL   : {return "cmpl";}
        case IR_INSTR_CMPEQ  : {return "cmpeq";}
        case IR_INSTR_TEST   : {return "test";}
        case IR_INSTR_JZ     : {return "jz";}
        case IR_INSTR_JMP    : {return "jmp";}
        case IR_INSTR_RET    : {return "ret";}
        case IR_INSTR_SYSCALL: {return "syscall";}
        case IR_INSTR_XOR    : {return "xor";}
        case IR_CONTROL_JMP  : {return "C_jmp";}
        case IR_CONTROL_FUNC : {return "C_func";}
        case IR_INSTR_SQRT   : {return "sqrt";}
        case IR_INSTR_NOT    : {return "not";}
        case IR_INSTR_PUSH_XMM: {return "pushXMM";}
        case IR_INSTR_POP_XMM: {return "popXMM";}
        default              : {return NULL;}
    }
}
