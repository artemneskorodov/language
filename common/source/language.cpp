#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "language.h"
#include "colors.h"
#include "utils.h"

struct flag_prototype_t {
    const char *long_name;
    const char *short_name;
    size_t params;
    language_error_t (*handler)(language_t *, int, size_t, const char **);
};

static language_error_t handler_output(language_t *language, int, size_t position, const char *argv[]);
static language_error_t handler_input(language_t *language, int, size_t position, const char *argv[]);
language_error_t write_subtree(language_t *language, language_node_t *node, size_t level, FILE *output);
language_error_t compile_function_call(language_t *language, language_node_t *root, FILE *output);
language_error_t compile_identifier(language_t *language, language_node_t *root, FILE *output);

language_error_t read_name_table(language_t *language);
language_error_t read_subtree(language_t *language, language_node_t **output);

static const flag_prototype_t SupportedFlags[] = {
    {"-o", "--output", 1, handler_output},
    {"-i", "--input", 1, handler_input}
};

language_error_t nodes_storage_ctor(language_t *language, size_t capacity) {
    language->nodes.nodes = (language_node_t *)calloc(capacity, sizeof(language->nodes.nodes[0]));
    if(language->nodes.nodes == NULL) {
        print_error("Error while allocating nodes memory.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    language->nodes.size = 0;
    language->nodes.capacity = capacity;
    return LANGUAGE_SUCCESS;
}

language_error_t nodes_storage_add(language_t *language, node_type_t type, value_t value, const char *name, size_t length, language_node_t **output) {
    //TODO check size
    language->nodes.nodes[language->nodes.size].type = type;
    language->nodes.nodes[language->nodes.size].value = value;
    language->nodes.nodes[language->nodes.size].source_info.name = name;
    language->nodes.nodes[language->nodes.size].source_info.length = length;
    language->nodes.nodes[language->nodes.size].source_info.line = language->frontend_info.current_line;
    if(output != NULL) {
        *output = language->nodes.nodes + language->nodes.size;
    }
    language->nodes.size++;
    return LANGUAGE_SUCCESS;
}

language_error_t nodes_storage_dtor(language_t *language) {
    free(language->nodes.nodes);
    return LANGUAGE_SUCCESS;
}

language_error_t read_tree(language_t *language) {
    FILE *input = fopen(language->input_file, "rb");
    if(input == NULL) {
        print_error("Error while opening file '%s'.\n", language->input_file);
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    language->input_size = file_size(input);
    language->input = (char *)calloc(language->input_size, sizeof(char));
    if(fread(language->input, sizeof(char), language->input_size, input) != language->input_size) {
        print_error("Error while reading code tree file.\n");
        fclose(input);
        return LANGUAGE_READING_TREE_ERROR;
    }
    fclose(input);

    language->input_position = language->input;
    _RETURN_IF_ERROR(read_name_table(language));

    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    char *nodes_number_end = NULL;
    size_t nodes_number = strtoull(language->input_position, &nodes_number_end, 10);
    if(nodes_number_end == NULL) {
        print_error("Unexpected code file structure. It is expected to see nodes number before nodes.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position = nodes_number_end;
    _RETURN_IF_ERROR(nodes_storage_ctor(language, nodes_number));

    _RETURN_IF_ERROR(read_subtree(language, &language->root));
    return LANGUAGE_SUCCESS;
}

language_error_t read_name_table(language_t *language) {
    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    char *length_end = NULL;
    size_t name_table_size = strtoull(language->input_position, &length_end, 10);
    if(length_end == NULL) {
        print_error("Tree file expected to start with name table length.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }

    _RETURN_IF_ERROR(name_table_ctor(language, name_table_size));
    language->input_position = length_end;

    for(size_t elem = 0; elem < name_table_size; elem++) {
        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        if(*language->input_position != '{') {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position++;

        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        char *name_length_end = NULL;
        size_t name_size = strtoull(language->input_position, &name_length_end, 10);
        if(name_length_end == NULL) {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position = name_length_end;

        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        if(*language->input_position != '"') {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position++;

        const char *name_start = language->input_position;
        while(*language->input_position != '"') {
            language->input_position++;
        }
        language->input_position++;

        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        char *type_end = NULL;
        identifier_type_t type = (identifier_type_t)strtol(language->input_position, &type_end, 10);
        if(type_end == NULL) {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position = type_end;

        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        char *scope_end = NULL;
        size_t scope = strtoull(language->input_position, &scope_end, 10);
        if(scope_end == NULL) {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position = scope_end;
        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        char *parameters_end = NULL;
        size_t parameters_number = strtoull(language->input_position, &parameters_end, 10);
        if(parameters_end == NULL) {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position = parameters_end;
        while(isspace(*language->input_position)) {
            language->input_position++;
        }

        if(*language->input_position != '}') {
            print_error("Unexpected name table element structure. It is expected to look like '{LENGTH \"NAME\" TYPE SCOPE}'");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        language->input_position++;

        size_t name_index = 0;
        _RETURN_IF_ERROR(name_table_add(language, name_start, name_size, &name_index));
        _RETURN_IF_ERROR(name_table_set_defined(language, name_index, type, scope));
        language->name_table.identifiers[name_index].parameters_number = parameters_number;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t read_subtree(language_t *language, language_node_t **output) {
    while(isspace(*language->input_position)) {
        language->input_position++;
    }

    if(*language->input_position != '{') {
        print_error("Node must start with '{'.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position++;

    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    char *type_end = NULL;
    node_type_t type = (node_type_t)strtol(language->input_position, &type_end, 10);
    if(type_end == NULL) {
        print_error("Nodes are written as {TYPE VALUE LEFT RIGHT}");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position = type_end;

    while(isspace(*language->input_position)) {
        language->input_position++;
    }

    value_t value;
    char *value_end = NULL;
    switch(type) {
        case NODE_TYPE_IDENTIFIER: {
            value.identifier = strtoull(language->input_position, &value_end, 10);
            break;
        }
        case NODE_TYPE_NUMBER: {
            value.number = strtod(language->input_position, &value_end);
            break;
        }
        case NODE_TYPE_OPERATION: {
            value.opcode = (operation_t)strtol(language->input_position, &value_end, 10);
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    language->input_position = value_end;

    language_node_t *node = NULL;
    _RETURN_IF_ERROR(nodes_storage_add(language, type, value, NULL, 0, &node));

    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    if(*language->input_position != '-') {
        _RETURN_IF_ERROR(read_subtree(language, &node->left));
    }
    else {
        language->input_position++;
    }

    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    if(*language->input_position != '-') {
        _RETURN_IF_ERROR(read_subtree(language, &node->right));
    }
    else {
        language->input_position++;
    }

    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    if(*language->input_position != '}') {
        print_error("Node must end with '}'.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position++;
    *output = node;
    return LANGUAGE_SUCCESS;
}

language_error_t write_tree(language_t *language) {
    FILE *output = fopen(language->output_file, "wb");
    if(output == NULL) {
        print_error("Error while opening file to write tree.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    fprintf(output, "%llu\r\n", language->name_table.size);
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        identifier_t *identifier = language->name_table.identifiers + elem;
        fprintf(output,
                "{%llu \"%.*s\" %d %llu %llu}\r\n",
                identifier->length,
                (int)identifier->length,
                identifier->name,
                identifier->type,
                identifier->scope,
                identifier->parameters_number);
    }
    fprintf(output, "\r\n%llu\r\n", language->nodes.size);
    language_error_t error_code = write_subtree(language, language->root, 0, output);
    fclose(output);
    return error_code;
}

language_error_t write_subtree(language_t *language, language_node_t *node, size_t level, FILE *output) {
    fprintf(output, "{ %d ", node->type);
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            fprintf(output, "%llu ", node->value.identifier);
            break;
        }
        case NODE_TYPE_NUMBER: {
            fprintf(output, "%lg ", node->value.number);
            break;
        }
        case NODE_TYPE_OPERATION: {
            fprintf(output, "%d ", node->value.opcode);
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    if(node->left != NULL) {
        _RETURN_IF_ERROR(write_subtree(language, node->left, level + 1, output)); //FIXME check if level needed
    }
    else {
        fprintf(output, "- ");
    }
    if(node->right != NULL) {
        _RETURN_IF_ERROR(write_subtree(language, node->right, level + 1, output));
    }
    else {
        fprintf(output, "- ");
    }

    fprintf(output, "} ");
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_ctor(language_t *language, size_t capacity) {
    language->name_table.identifiers = (identifier_t *)calloc(capacity, sizeof(language->name_table.identifiers[0]));
    if(language->name_table.identifiers == NULL) {
        print_error("Error while allocating memory to identifiers.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    language->name_table.size = 0;
    language->name_table.capacity = capacity;
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_add(language_t *language, const char *name, size_t length, size_t *index) {
    //TODO add checking size
    identifier_t *identifier = language->name_table.identifiers + language->name_table.size;

    identifier->name = name;
    identifier->length = length;

    if(index != NULL) {
        *index = language->name_table.size;
    }

    language->name_table.size++;
    return LANGUAGE_SUCCESS;
}
//TODO add setting identifier type

language_error_t name_table_find(language_t *language, const char *name, size_t length, size_t *index) {
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        if(language->name_table.identifiers[elem].length == length &&
           strncmp(language->name_table.identifiers[elem].name, name, length) == 0) {
            *index = elem;
            return LANGUAGE_SUCCESS;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_dtor(language_t *language) {
    free(language->name_table.identifiers);
    return LANGUAGE_SUCCESS;
}

language_error_t get_identifier(language_t *language, language_node_t *node, identifier_t **identifier) {
    if(node->type != NODE_TYPE_IDENTIFIER) {
        print_error("Identifier getting function call for non identifier node.\n");
        return LANGUAGE_INVALID_NODE_TYPE;
    }
    if(node->value.identifier >= language->name_table.size) {
        print_error("Identifier index is bigger that name table size.\n");
        return LANGUAGE_INVALID_NODE_VALUE;
    }
    *identifier = language->name_table.identifiers + node->value.identifier;
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_set_defined(language_t *language, size_t index, identifier_type_t type, size_t scope) {
    if(index >= language->name_table.size) {
        print_error("Identifier index is bigger that name table size.\n");
        return LANGUAGE_INVALID_NODE_VALUE;
    }
    identifier_t *identifier = language->name_table.identifiers + index;

    identifier->is_defined = true;
    identifier->type = type;
    identifier->scope = scope;
    return LANGUAGE_SUCCESS;
}

language_error_t parse_flags(language_t *language, int argc, const char *argv[]) {
    if(argc < 1) {
        print_error("Undefined parsing flags error. No falgs found.\n");
        return LANGUAGE_PARSING_FLAGS_ERROR;
    }

    for(size_t elem = 1; elem < (size_t)argc; ) {
        size_t index = PoisonIndex;
        for(size_t i = 0; i < sizeof(SupportedFlags) / sizeof(SupportedFlags[0]); i++) {
            if(strcmp(SupportedFlags[i].short_name, argv[elem]) == 0 ||
               strcmp(SupportedFlags[i].long_name, argv[elem]) == 0) {
                index = i;
                break;
            }
        }
        if(index == PoisonIndex) {
            print_error("Unknown flag '%s'.\n", argv[elem]);
            return LANGUAGE_UNKNOWN_FLAG;
        }
        if(elem + SupportedFlags[index].params >= (size_t)argc) {
            print_error("Flag '%s' is expected to have %llu parameters after it.\n",
                        argv[elem],
                        SupportedFlags[index].params);
            return LANGUAGE_PARSING_FLAGS_ERROR;
        }
        _RETURN_IF_ERROR(SupportedFlags[index].handler(language, argc, elem, argv));
        elem += SupportedFlags[index].params + 1;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t handler_input(language_t *language, int, size_t position, const char *argv[]) {
    language->input_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

language_error_t handler_output(language_t *language, int, size_t position, const char *argv[]) {
    language->output_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

language_error_t compile_subtree(language_t *language, language_node_t *root, FILE *output) {
    if(root == NULL) {
        return LANGUAGE_SUCCESS;
    }
    switch(root->type) {
        case NODE_TYPE_IDENTIFIER: {
            _RETURN_IF_ERROR(compile_identifier(language, root, output));
            break;
        }
        case NODE_TYPE_NUMBER: {
            fprintf(output, "push %lg\r\n", root->value.number);
            break;
        }
        case NODE_TYPE_OPERATION: {
            fprintf(stderr, "%p\n", root);
            _RETURN_IF_ERROR(KeyWords[root->value.opcode].assemble(language, root, output));
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t compile_identifier(language_t *language, language_node_t *root, FILE *output) {
    identifier_t *identifier = language->name_table.identifiers + root->value.identifier;
    switch(identifier->type) {
        case IDENTIFIER_FUNCTION: {
            _RETURN_IF_ERROR(compile_function_call(language, root, output));
            break;
        }
        case IDENTIFIER_VARIABLE: {
            if(identifier->scope == 0) {
                fprintf(output, "push [%llu]\r\n", identifier->memory_addr);
            }
            else {
                fprintf(output, "push [bx + %llu]\r\n", identifier->memory_addr);
            }
            break;
        }
        default: {
            print_error("Unknown identifier type.\n");
            return LANGUAGE_UNKNOWN_IDENTIFIER_TYPE;
        }
    }
    return LANGUAGE_SUCCESS;
}

language_error_t compile_function_call(language_t *language, language_node_t *root, FILE *output) {
    identifier_t *identifier = language->name_table.identifiers + root->value.identifier;
    language_node_t *param = root->right;

    // size_t counter = 0;
    fprintf(output, "push bx ;saving bx\r\n");
    while(param != NULL) {
        _RETURN_IF_ERROR(compile_subtree(language, param->left, output));
        // fprintf(output, "pop [bx + %llu]\r\n", counter++);
        param = param->right;
    }

    fprintf(output,
            "push bx\r\npush bx\r\npush %llu\r\nadd\r\npop bx\r\n",
            identifier->parameters_number + language->backend_info.used_locals);
    for(size_t i = 0; i < identifier->parameters_number; i++) {
        fprintf(output, "pop [bx + %llu]\r\n", i);
    }
    fprintf(output,
            "call %.*s:\r\n"
            "pop bx\r\n"
            "push ax\r\n",
            (int)identifier->length,
            identifier->name);
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_two_args(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    fprintf(output,
            "%s\r\n",
            KeyWords[node->value.opcode].assembler_command);
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_one_arg(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    fprintf(output,
            "%s\r\n",
            KeyWords[node->value.opcode].assembler_command);
    return LANGUAGE_SUCCESS;
}


/*
push LEFT
push RIGHT

jb comp_false_NUM:
push 1
jmp comp_fale_end_NUM:
comp_false_NUM:
    push 0
comp_false_end_NUM:
*/
language_error_t assemble_comparison(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    size_t num = language->backend_info.used_labels++;
    fprintf(output,
            "%s comp_false_%llu:\r\n"
            "push 1\r\n"
            "jmp comp_false_end_%llu:\r\n"
            "comp_false_%llu:\r\n"
            "push 0\r\n"
            "comp_false_end_%llu:\r\n",
            KeyWords[node->value.opcode].assembler_command,
            num, num, num, num);
    return LANGUAGE_SUCCESS;
}

/*
push RIGHT
pop [addr]
*/
language_error_t assemble_asignment(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    // if(!is_ident_type(language, node->left, IDENTIFIER_VARIABLE)) {
    //     return LANGUAGE_TREE_ERROR;
    // }
    identifier_t *identifier = language->name_table.identifiers + node->left->value.identifier;
    fprintf(output,
            "pop [%s%llu] ;pop to %.*s\r\n",
            identifier->scope == 0 ? "" : "bx + ",
            identifier->memory_addr,
            (int)identifier->length,
            identifier->name);
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_statements_line(language_t *language, language_node_t *node, FILE *output) {
    // size_t old_used_locals = language->backend_info.used_locals;
    // language->backend_info.used_locals = 0;
    while(node != NULL) {
        if(node->left->type == NODE_TYPE_OPERATION && node->left->value.opcode == OPERATION_NEW_VAR) {
            language->backend_info.used_locals++;
        }
        _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
        node = node->right;
    }
    // language->backend_info.used_locals = old_used_locals;
    return LANGUAGE_SUCCESS;
}

/*
push LEFT
push 0
je skip_left_NUM:
{body}
skip_left_NUM:
*/
language_error_t assemble_if(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
    fprintf(output,
            "push 0\r\n"
            "je skip_if_%llu:\r\n",
            language->backend_info.used_labels);

    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    fprintf(output,
            "skip_if_%llu:\r\n",
            language->backend_info.used_labels);
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_while(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
    size_t num = language->backend_info.used_labels;
    fprintf(output,
            "while_start%llu:\r\n"
            "push 0\r\n"
            "je skip_while_%llu:\r\n",
            num, num);

    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));

    fprintf(output,
            "jmp while_start%llu:\r\n"
            "skip_if_%llu:\r\n",
            num, num);
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_return(language_t *language, language_node_t *node, FILE *output) {
    _RETURN_IF_ERROR(compile_subtree(language, node->right, output));
    fprintf(output,
            "pop ax\r\n"
            "ret\r\n");
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_params_line(language_t *language, language_node_t *node, FILE *output) {
    while(node != NULL) {
        _RETURN_IF_ERROR(compile_subtree(language, node->left, output));
        node = node->right;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_new_var(language_t *language, language_node_t *node, FILE *output) {
    language_node_t *identifier = NULL;
    //TODO check that identifiers
    if(node->right->type == NODE_TYPE_OPERATION) {
        identifier = node->right->left;
    }
    else {
        identifier = node->right;
    }

    fprintf(stderr, "---------\n%.*s\n%llu\n---------\n", language->name_table.identifiers[identifier->value.identifier].length, language->name_table.identifiers[identifier->value.identifier].name, language->backend_info.used_locals);
    language->name_table.identifiers[identifier->value.identifier].memory_addr = language->backend_info.used_locals; //FIXME

    if(node->right->type == NODE_TYPE_OPERATION) {
        //TODO check that =
        _RETURN_IF_ERROR(assemble_asignment(language, node->right, output));
    }
    if(language->name_table.identifiers[identifier->value.identifier].scope == 0) {
        language->backend_info.used_memory++;
    }
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_new_func(language_t *language, language_node_t *node, FILE *output) {
    identifier_t *identifier = language->name_table.identifiers + node->right->value.identifier;
    fprintf(output,
            "jmp skip_%.*s:\r\n" //FIXME mabe we don't need it
            "%.*s:\r\n",
            (int)identifier->length, identifier->name,
            (int)identifier->length, identifier->name);

    size_t old_locals = language->backend_info.used_locals;
    language->backend_info.used_locals = 0;
    _RETURN_IF_ERROR(compile_subtree(language, node->right->left, output));
    _RETURN_IF_ERROR(compile_subtree(language, node->right->right, output));
    language->backend_info.used_locals = old_locals;

    fprintf(output,
            "skip_%.*s:\r\n",
            (int)identifier->length, identifier->name);
    return LANGUAGE_SUCCESS;
}

