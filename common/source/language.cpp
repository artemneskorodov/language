#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "language.h"
#include "colors.h"

struct flag_prototype_t {
    const char *long_name;
    const char *short_name;
    size_t params;
    language_error_t (*handler)(language_t *, int, size_t, const char **);
};

static language_error_t handler_output(language_t *language, int, size_t position, const char *argv[]);
static language_error_t handler_input(language_t *language, int, size_t position, const char *argv[]);
language_error_t write_subtree(language_t *language, language_node_t *node, size_t level, FILE *output);

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

language_error_t nodes_storage_add(language_t *language, node_type_t type, value_t value, const char *name, size_t length) {
    //TODO check size
    language->nodes.nodes[language->nodes.size].type = type;
    language->nodes.nodes[language->nodes.size].value = value;
    language->nodes.nodes[language->nodes.size].source_info.name = name;
    language->nodes.nodes[language->nodes.size].source_info.length = length;
    language->nodes.nodes[language->nodes.size].source_info.line = language->frontend_info.current_line;
    language->nodes.size++;
    return LANGUAGE_SUCCESS;
}

language_error_t nodes_storage_dtor(language_t *language) {
    free(language->nodes.nodes);
    return LANGUAGE_SUCCESS;
}

language_error_t read_tree(language_t *language) {
    FILE *input = fopen(language->input_file, "rb");
    size_t size = file_size(input);
    char *tree_string = (char *)calloc(size + 1, sizeof(char));
    char *position = tree_string;
    if(fread(tree_string, sizeof(char), size, input) != size) {
        print_error("Error while reading tree file.\n");
        return LANGUAGE_READING_TREE_ERROR;
    }
    flose(input);

    int read_symbols = 0;
    size_t name_table_size = 0;
    if(sscanf(position, "%llu%n", &name_table_size, &read_symbols) != 1) {
        print_error("Undefined code tree structure, I supposed it to start with size of name table.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    position += read_symbols;
    while(isspace(*position)) {
        position++;
    }
    _RETURN_IF_ERROR(name_table_ctor(language, name_table_size));
    for(size_t elem = 0; elem < name_table_size; elem++) {
        size_t length = 0;
        if(sscanf("{%llu|%n", &length, &read_symbols));
        position += read_symbols;
        while(isspace(*position)) {
            position++;
        }
        if(*positino != '"') {
            print_error("Unknown code tree structure, I supposed that name table element would look like: \"{LENGTH|\"NAME\"|TYPE}\"");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        position++;
        _RETURN_IF_ERROR(name_table_add(language, position, length, NULL));
        position += length;
        if(*positino != '"') {
            print_error("Unknown code tree structure, I supposed that name table element would look like: \"{LENGTH|\"NAME\"|TYPE}\"");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        positino++;
        if(*position != '|') {
            print_error("Unknown code tree structure, I supposed that name table element would look like: \"{LENGTH|\"NAME\"|TYPE}\"");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        position++;
        identifier_type_t type = (identifier_type_t)0;
        if(scanf("%d}%n", (int *)&type, &read_symbols) != 1) {
            print_error("Unknown code tree structure, I supposed that name table element would look like: \"{LENGTH|\"NAME\"|TYPE}\"");
            return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
        }
        _RETURN_IF_ERROR(name_table_set_defined(language, elem, type));
        while(isspace(*position)) {
            positino++;
        }
    }

    while(isspace(*position)) {
        positino++;
    }
    size_t nodes_number = 0;
    if(sscanf(position, "%llu%n", &nodes_number, &read_symbols) != 1) {
        print_error("I expected to see nodes number after name table.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    positino += read_symbols;
    _RETURN_IF_ERORR(read_subtree(language, &language->root, &position));
    free(tree_string);
    return LANGUAGE_SUCCESS;
}

language_error_t read_subtree(language_t *language, language_node_t **output, char **position) {
    if(**positino != '{') {
        print_error("Nodes are expected to be written in format: { TYPE VALUE LEFT RIGHT} (LEFT and RIGHT are set to - if child is empty)");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    (*position)++;
    while(isspace(**position)) {
        (*position)++;
    }
    int read_symbols = 0;
    node_type_t type = (node_type_t)0;
    if(sscanf(*position, "%d%n", &type, &read_symbols) != 1) {
        print_error("Nodes are expected to be written in format: { TYPE VALUE LEFT RIGHT} (LEFT and RIGHT are set to - if child is empty)");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    *positino += read_symbols;
    while(isspace(**position)) {
        (*position)++;
    }
    value_t value = 0;
    switch(type) {
        case NODE_TYPE_IDENTIFIER: {
            if(sscanf(*position, "%llu%n", &value.identifier, &read_symbols) != 1) {
                print_error("Nodes are expected to be written in format: { TYPE VALUE LEFT RIGHT} (LEFT and RIGHT are set to - if child is empty)");
                return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
            }
            break;
        }
        case NODE_TYPE_NUMBER: {
            if(sscanf(*position, "%lg%n", &value.number, &read_symbols) != 1) {
                print_error("Nodes are expected to be written in format: { TYPE VALUE LEFT RIGHT} (LEFT and RIGHT are set to - if child is empty)");
                return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
            }
            break;
        }
        case NODE_TYPE_OPERATION: {
            if(sscanf(*position, "%d%n", &value.opcode, &read_symbols) != 1) {
                print_error("Nodes are expected to be written in format: { TYPE VALUE LEFT RIGHT} (LEFT and RIGHT are set to - if child is empty)");
                return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
            }
            break;
        }
        default: {
            print_error("Read unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
        (*position) += read_symbols;
    }
    (*position) += read_symbols;

    _RETURN_IF_ERROR(nodes_storage_add(language, type, value, NULL, 0));

}

language_error_t write_tree(language_t *language) {
    FILE *output = fopen(language->output_file, "wb");
    if(output == NULL) {
        print_error("Error while opening file to write tree.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        identifier_t *identifier = language->name_table.identifiers + elem;
        fprintf(output, "{%llu|\"%.*s\"|%d}\r\n", identifier->length, (int)identifier->length, identifier->name, identifier->type);
    }
    fprintf(output, "\r\n");
    _RETURN_IF_ERROR(write_subtree(language, language->root, 0, output));
    fclose(output);
    return LANGUAGE_SUCCESS;
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
    language->name_table.identifiers[language->name_table.size].name = name;
    language->name_table.identifiers[language->name_table.size].length = length;
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

language_error_t name_table_set_defined(language_t *language, size_t index, identifier_type_t type) {
    if(node->type != NODE_TYPE_IDENTIFIER) {
        print_error("Identifier getting function call for non identifier node.\n");
        return LANGUAGE_INVALID_NODE_TYPE;
    }
    if(node->value.identifier >= language->name_table.size) {
        print_error("Identifier index is bigger that name table size.\n");
        return LANGUAGE_INVALID_NODE_VALUE;
    }
    language->name_table.identifiers[node->value.identifier].is_defined = true;
    language->name_table.identifiers[node->value.identifier].type = type;
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_two_args(language_t *language, language_node_t *node) {
    /*TODO writing for backend*/
    fprintf(stderr, "not implemented yet, dot call it plz.\n");
    return LANGUAGE_SUCCESS;
}

language_error_t assemble_one_arg(language_t *language, language_node_t *node) {
    /*TODO writing for backend*/
    fprintf(stderr, "not implemented yet, dot call it plz.\n");
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
            print_error("Flag '%s' is expected to have %llu parameters after it.\n", argv[elem], SupportedFlags[index].params);
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
