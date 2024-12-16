#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <stdio.h>

enum language_error_t {
    LANGUAGE_SUCCESS = 0,
    LANGUAGE_OPENING_FILE_ERROR = 1,
    LANGUAGE_MEMORY_ERROR = 2,
    LANGUAGE_READING_SOURCE_ERROR = 3,
    LANGUAGE_UNEXPECTED_WORD_START = 4,
    LANGUAGE_UNCLOSED_COMMENT = 5,
    LANGUAGE_SYNTAX_UNEXPECTED_CALL = 6,
    LANGUAGE_UNKNOWN_NODE_TYPE = 7,
    LANGUAGE_UNKNOWN_IDENTIFIER_TYPE = 8,
    LANGUAGE_SYNTAX_ERROR = 9,
    LANGUAGE_INVALID_NODE_TYPE = 10,
    LANGUAGE_INVALID_NODE_VALUE = 11,
    LANGUAGE_PARSING_FLAGS_ERROR = 12,
    LANGUAGE_UNKNOWN_FLAG = 13,
    LANGUAGE_DUMP_FILE_ERROR = 14,
    LANGUAGE_READING_TREE_ERROR = 15,
    LANGUAGE_UNKNOWN_CODE_TREE_TYPE = 16,
    LANGUAGE_BROKEN_KEYWORDS_TABLE = 17,
    LANGUAGE_TREE_ERROR = 18,
    //TODO
};

enum node_type_t {
    NODE_TYPE_OPERATION = 1,
    NODE_TYPE_NUMBER = 2,
    NODE_TYPE_IDENTIFIER = 3,
};

enum operation_t {
    OPERATION_UNKNOWN = 0,
    OPERATION_ADD = 1,
    OPERATION_SUB = 2,
    OPERATION_MUL = 3,
    OPERATION_DIV = 4,
    OPERATION_COS = 5,
    OPERATION_SIN = 6,
    OPERATION_POW = 7,
    OPERATION_BIGGER = 8,
    OPERATION_SMALLER = 9,
    OPERATION_ASSIGNMENT = 10,
    OPERATION_OPEN_BRACKET = 11,
    OPERATION_CLOSE_BRACKET = 12,
    OPERATION_BODY_START = 13,
    OPERATION_BODY_END = 14,
    OPERATION_STATEMENT = 15,
    OPERATION_IF = 16,
    OPERATION_WHILE = 17,
    OPERATION_RETURN = 18,
    OPERATION_PARAM_LINKER = 19,
    OPERATION_NEW_VAR = 20,
    OPERATION_NEW_FUNC = 21,
    //TODO
    OPERATION_PROGRAM_END = 22,
};

union value_t {
    double number;
    size_t identifier;
    operation_t opcode;
};

struct source_info_t {
    const char *name;
    size_t length;
    size_t line;
};

struct language_node_t {
    source_info_t source_info;
    node_type_t type;
    value_t value;
    language_node_t *left;
    language_node_t *right;
};

enum identifier_type_t {
    IDENTIFIER_FUNCTION = 1,
    IDENTIFIER_GLOBAL_VAR = 2,
    IDENTIFIER_LOCAL_VAR = 3,
};

struct identifier_t {
    const char *name;
    size_t length;
    identifier_type_t type;
    size_t parameters_number;
    bool is_defined;
    size_t memory_addr;
};

struct name_t {
    size_t length;
    const char *name;
};

struct name_table_t {
    identifier_t *identifiers;
    size_t size;
    size_t capacity;
    size_t *stack;
    size_t stack_size;
    name_t *used_names;
    size_t used_names_size;
};

struct nodes_storage_t {
    language_node_t *nodes;
    size_t size;
    size_t capacity;
};

struct frontend_info_t {
    language_node_t *position;
    size_t current_line;
    size_t used_locals;
};

struct backend_info_t {
    size_t used_globals;
    size_t used_labels;
    size_t used_locals;
    int scope;
    FILE *output;
};

struct dump_info_t {
    FILE *general_dump;
    size_t dumps_number;
    const char *filename;
    size_t current_scope;
};

struct language_t {
    dump_info_t dump_info;
    name_table_t name_table;
    nodes_storage_t nodes;
    language_node_t *root;

    char *input;
    size_t input_size;
    const char *input_position;

    frontend_info_t frontend_info;
    backend_info_t backend_info;
    const char *input_file;
    const char *output_file;
};

//FIXME
#define NUMBER(_value) (value_t){.number = (_value)}
#define OPCODE(_value) (value_t){.opcode = (_value)}
#define IDENT(_value)  (value_t){.identifier = (_value)}

language_error_t nodes_storage_ctor(language_t *language, size_t capacity);
language_error_t nodes_storage_add(language_t *language, node_type_t type, value_t value, const char *name, size_t length, language_node_t **output);
language_error_t nodes_storage_dtor(language_t *language);
language_error_t parse_flags(language_t *language, int argc, const char *argv[]);
language_error_t read_tree(language_t *language);
language_error_t write_tree(language_t *language);
language_error_t get_identifier(language_t *language, language_node_t *node, identifier_t **identifier);
language_error_t verify_keywords(void);

//TODO
#include "assemble.h"

struct keyword_t {
    const char *name;
    size_t length;
    operation_t code;
    language_error_t (*assemble)(language_t *, language_node_t *);
    const char *assembler_command;
    bool is_expression_element;
};

#define STR_LEN(_string) (_string), sizeof(_string) - 1

static const keyword_t KeyWords[] = {
    {/*THIS FIELD MUST BE HERE AS IT IS FOR UNKNOWN COMMAND*/},
    {STR_LEN("+"), OPERATION_ADD, assemble_two_args, "add", false},
    {STR_LEN("-"), OPERATION_SUB, assemble_two_args, "sub", false},
    {STR_LEN("*"), OPERATION_MUL, assemble_two_args, "mul", false},
    {STR_LEN("/"), OPERATION_DIV, assemble_two_args, "div", false},
    {STR_LEN("cos"), OPERATION_COS, assemble_one_arg, "cos", true},
    {STR_LEN("sin"), OPERATION_SIN, assemble_one_arg, "sin", true},
    {STR_LEN("^"), OPERATION_POW, assemble_two_args, "pow", false},
    {STR_LEN(">"), OPERATION_BIGGER, assemble_comparison, "ja", false},
    {STR_LEN("<"), OPERATION_SMALLER, assemble_comparison, "jb", false},
    {STR_LEN("="), OPERATION_ASSIGNMENT, assemble_assignment, NULL, false},
    {STR_LEN("("), OPERATION_OPEN_BRACKET, NULL, NULL, false},
    {STR_LEN(")"), OPERATION_CLOSE_BRACKET, NULL, NULL, false},
    {STR_LEN("{"), OPERATION_BODY_START, NULL, NULL, false},
    {STR_LEN("}"), OPERATION_BODY_END, NULL, NULL, false},
    {STR_LEN(";"), OPERATION_STATEMENT, assemble_statements_line, NULL, false},
    {STR_LEN("reskni"), OPERATION_IF, assemble_if, NULL, false},
    {STR_LEN("dohuya"), OPERATION_WHILE, assemble_while, NULL, false},
    {STR_LEN("otday"), OPERATION_RETURN, assemble_return, NULL, false},
    {STR_LEN(","), OPERATION_PARAM_LINKER, assemble_params_line, NULL, false},
    {STR_LEN("blyadskiy"), OPERATION_NEW_VAR, assemble_new_var, NULL, false},
    {STR_LEN("ebal"), OPERATION_NEW_FUNC, assemble_new_func, NULL, false},
    //TODO
    {NULL, 0, OPERATION_PROGRAM_END, NULL, NULL, false},
};

#define _RETURN_IF_ERROR(...) {                     \
    language_error_t _error_code = (__VA_ARGS__);   \
    if(_error_code != LANGUAGE_SUCCESS) {           \
        return _error_code;                         \
    }                                               \
}                                                   \

static const size_t PoisonIndex = (size_t)(-1);

#endif
