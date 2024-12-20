//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    va_list args;
    va_start(args, format);
    vfprintf(ctx->dump_info.general_dump, format, args);
    va_end(args);
#pragma clang diagnostic pop
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
    }
    //-----------------------------------------------------------------------//
    return NULL;
}

//===========================================================================//
