#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "language.h"
#include "lang_dump.h"
#include "colors.h"

static const char *node_color_operation = "#E8D6CB";
static const char *node_color_statement_end = "#D0ADA7";
static const char *node_color_number = "#AD6A6C";
static const char *node_color_ident_function = "#5D2E46";
static const char *node_color_ident_variable = "#B58DB6";

static language_error_t dump_subtree(language_t *language, language_node_t *root, size_t level, FILE *dot_file);
static const char *get_node_color(language_t *language, language_node_t *node);

language_error_t dump_ctor(language_t *language, const char *filename) {
    language->dump_info.filename = filename;
    char dump_name[256] = {};
    snprintf(dump_name, 256, "logs/%s.html", filename);
    language->dump_info.general_dump = fopen(dump_name, "w");
    return LANGUAGE_SUCCESS;
}

language_error_t dump_dtor(language_t *language) {
    fclose(language->dump_info.general_dump);
    return LANGUAGE_SUCCESS;
}

language_error_t dump_tree(language_t *language, const char *format, ...) {
    char dot_filename[256] = {};
    snprintf(dot_filename, 256, "logs/dot/%s%04lx.dot", language->dump_info.filename, language->dump_info.dumps_number);
    FILE *dot_file = fopen(dot_filename, "w");
    if(dot_file == NULL) {
        print_error("Error while opening dot file.\n");
        return LANGUAGE_DUMP_FILE_ERROR;
    }
    fprintf(dot_file,
            "digraph {\n"
            "node[shape = Mrecord, style = filled];\n");
    _RETURN_IF_ERROR(dump_subtree(language, language->root, 0, dot_file));
    fprintf(dot_file, "}\n");
    fclose(dot_file);
    char command[256] = {};
    snprintf(command, 256, "dot %.*s -Tsvg -o logs/img/%s%04lx.svg", 255, dot_filename, language->dump_info.filename, language->dump_info.dumps_number);
    system(command);
    fprintf(language->dump_info.general_dump, "<h1>");
    va_list args;
    va_start(args, format);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    vfprintf(language->dump_info.general_dump, format, args);
#pragma clang diagnostic pop
    va_end(args);
    fprintf(language->dump_info.general_dump, "</h1><img src = \"img/%s%04lx.svg\">", language->dump_info.filename, language->dump_info.dumps_number);
    fflush(language->dump_info.general_dump);
    language->dump_info.dumps_number++;
    return LANGUAGE_SUCCESS;
}

language_error_t dump_subtree(language_t *language, language_node_t *root, size_t level, FILE *dot_file) {
    fprintf(dot_file, "node%p[fillcolor = \"%s\", rank = %lu, label = \"{%p | {%p | %p} | ", root, get_node_color(language, root), level, root, root->left, root->right);

    if(root->type == NODE_TYPE_NUMBER) {
        fprintf(dot_file, "NUMBER | %lg }\"];\n", root->value.number);
    }
    else if(root->type == NODE_TYPE_OPERATION) {
        const char *string_operation = KeyWords[root->value.opcode].name;
        if(root->value.opcode == OPERATION_BODY_START) {
            string_operation = "body_start";
        }
        else if(root->value.opcode == OPERATION_BODY_END) {
            string_operation = "body_end";
        }
        else if(root->value.opcode == OPERATION_BIGGER) {
            string_operation = "bigger";
        }
        else if(root->value.opcode == OPERATION_SMALLER) {
            string_operation = "smaller";
        }
        fprintf(dot_file, "OPERATION | %s}\"];\n", string_operation == NULL ? "NULL" : string_operation);
    }
    else if(root->type == NODE_TYPE_IDENTIFIER) {
        fprintf(dot_file, "IDENTIFIER | %.*s}\"];\n", (int)language->name_table.identifiers[root->value.identifier].length, language->name_table.identifiers[root->value.identifier].name);
    }

    if(root->left != NULL) {
        fprintf(dot_file, "node%p -> node%p;\n", root, root->left);
        _RETURN_IF_ERROR(dump_subtree(language, root->left, level + 1, dot_file));
    }
    if(root->right != NULL) {
        fprintf(dot_file, "node%p -> node%p;\n", root, root->right);
        _RETURN_IF_ERROR(dump_subtree(language, root->right, level + 1, dot_file));
    }
    return LANGUAGE_SUCCESS;
}

const char *get_node_color(language_t *language, language_node_t *node) {
    if(node->type == NODE_TYPE_NUMBER) {
        return node_color_number;
    }
    if(node->type == NODE_TYPE_IDENTIFIER) {
        if(language->name_table.identifiers[node->value.identifier].type == IDENTIFIER_FUNCTION) {
            return node_color_ident_function;
        }
        else {
            return node_color_ident_variable;
        }
    }
    if(node->type == NODE_TYPE_OPERATION) {
        if(node->value.opcode == OPERATION_STATEMENT) {
            return node_color_statement_end;
        }
        else {
            return node_color_operation;
        }
    }
    return NULL;
}
