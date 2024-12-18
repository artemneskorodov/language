#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//====================================================================================//

#include "language.h"
#include "colors.h"
#include "utils.h"
#include "name_table.h"
#include "custom_assert.h"

//====================================================================================//

static language_error_t handler_output   (language_t       *language,
                                          int               argc,
                                          size_t            position,
                                          const char       *argv[]);

static language_error_t handler_input    (language_t       *language,
                                          int               argc,
                                          size_t            position,
                                          const char       *argv[]);

static language_error_t skip_spaces      (language_t       *language);

static language_error_t write_subtree    (language_t       *language,
                                          language_node_t  *node,
                                          FILE             *output);

static language_error_t read_name_table  (language_t       *language);

static language_error_t read_subtree     (language_t       *language,
                                          language_node_t **output);

static language_error_t get_nt_name_size (language_t        *language,
                                          size_t            *length);

static language_error_t get_nt_type      (language_t        *language,
                                          identifier_type_t *type);

static language_error_t get_nt_params    (language_t        *language,
                                          size_t            *params);

static language_error_t nt_error         (language_t        *language);

static language_error_t nt_check_symbol  (language_t        *language,
                                          char               symbol);

static language_error_t tree_read_value  (language_t        *language,
                                          value_t           *value,
                                          node_type_t        type);

//====================================================================================//

struct flag_prototype_t {
    const char *long_name;
    const char *short_name;
    size_t params;
    language_error_t (*handler)(language_t *, int, size_t, const char **);
};

//====================================================================================//

static const flag_prototype_t SupportedFlags[] = {
    {"-o", "--output", 1, handler_output},
    {"-i", "--input" , 1, handler_input }
};

//====================================================================================//

language_error_t nodes_storage_ctor(language_t *language, size_t capacity) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);

    language->nodes.nodes = (language_node_t *)calloc(capacity, sizeof(language->nodes.nodes[0]));
    if(language->nodes.nodes == NULL) {
        print_error("Error while allocating nodes memory.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    language->nodes.size = 0;
    language->nodes.capacity = capacity;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t nodes_storage_add(language_t       *language,
                                   node_type_t       type,
                                   value_t           value,
                                   const char       *name,
                                   size_t            length,
                                   language_node_t **output) {
    _C_ASSERT(language != NULL, return LANGUAGE_CTX_NULL);

    if(language->nodes.size >= language->nodes.capacity) {
        print_error("Nodes storage size does not know how to reallocate memory. "
                    "Plz implement it or, change capacity when initializing.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    language_node_t *node = language->nodes.nodes + language->nodes.size;
    language->nodes.size++;
    node->type = type;
    node->value = value;
    node->source_info.name = name;
    node->source_info.length = length;
    node->source_info.line = language->frontend_info.current_line;

    if(output != NULL) {
        *output = node;
    }
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t nodes_storage_dtor(language_t *language) {
    free(language->nodes.nodes);
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t read_tree(language_t *language) {
    FILE *input = fopen(language->input_file, "rb");
    if(input == NULL) {
        print_error("Error while opening file '%s'.\n", language->input_file);
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    language->input_size = file_size(input);
    language->input = (char *)calloc(language->input_size, sizeof(char));
    if(fread(language->input,
             sizeof(char),
             language->input_size,
             input) != language->input_size) {
        print_error("Error while reading code tree file.\n");
        fclose(input);
        return LANGUAGE_READING_TREE_ERROR;
    }
    fclose(input);

    language->input_position = language->input;
    _RETURN_IF_ERROR(read_name_table(language));

    _RETURN_IF_ERROR(skip_spaces(language));
    char *nodes_number_end = NULL;
    size_t nodes_number = strtoull(language->input_position, &nodes_number_end, 10);
    if(nodes_number_end == NULL) {
        print_error("Unexpected code file structure. "
                    "It is expected to see nodes number before nodes.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position = nodes_number_end;
    _RETURN_IF_ERROR(nodes_storage_ctor(language, nodes_number));

    _RETURN_IF_ERROR(read_subtree(language, &language->root));
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t read_name_table(language_t *language) {
    _RETURN_IF_ERROR(skip_spaces(language));
    char *length_end = NULL;
    size_t name_table_size = strtoull(language->input_position, &length_end, 10);
    if(length_end == NULL) {
        print_error("Tree file expected to start with name table length.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }

    _RETURN_IF_ERROR(name_table_ctor(language, name_table_size));
    language->input_position = length_end;

    for(size_t elem = 0; elem < name_table_size; elem++) {
        _RETURN_IF_ERROR(nt_check_symbol(language, '{'));

        size_t name_size = 0;
        _RETURN_IF_ERROR(get_nt_name_size(language, &name_size));

        _RETURN_IF_ERROR(nt_check_symbol(language, '"'));

        const char *name_start = language->input_position;
        language->input_position += name_size;

        _RETURN_IF_ERROR(nt_check_symbol(language, '"'));

        identifier_type_t type = (identifier_type_t)0;
        _RETURN_IF_ERROR(get_nt_type(language, &type));

        size_t parameters_number = 0;
        _RETURN_IF_ERROR(get_nt_params(language, &parameters_number));

        _RETURN_IF_ERROR(nt_check_symbol(language, '}'));

        size_t name_index = 0;
        _RETURN_IF_ERROR(name_table_add(language, name_start, name_size, &name_index, type));
        language->name_table.identifiers[name_index].parameters_number = parameters_number;
    }
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_nt_name_size (language_t *language, size_t *length) {
    _RETURN_IF_ERROR(skip_spaces(language));
    char *name_length_end = NULL;
    *length = strtoull(language->input_position, &name_length_end, 10);
    if(name_length_end == NULL) {
        return nt_error(language);
    }
    language->input_position = name_length_end;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_nt_type (language_t *language, identifier_type_t *type) {
    _RETURN_IF_ERROR(skip_spaces(language));
    char *type_end = NULL;
    *type = (identifier_type_t)strtol(language->input_position, &type_end, 10);
    if(type_end == NULL) {
        return nt_error(language);
    }
    language->input_position = type_end;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t get_nt_params (language_t *language, size_t *params) {
    _RETURN_IF_ERROR(skip_spaces(language));
    char *parameters_end = NULL;
    *params = strtoull(language->input_position, &parameters_end, 10);
    if(parameters_end == NULL) {
        return nt_error(language);
    }
    language->input_position = parameters_end;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t nt_check_symbol(language_t *language, char symbol) {
    _RETURN_IF_ERROR(skip_spaces(language));
    if(*language->input_position != symbol) {
        return nt_error(language);
    }
    language->input_position++;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t nt_error(language_t *language) {
    print_error("Error on " SZ_SP "Name table element is "
                "expected to have structure: "
                "{LENGTH \"NAME\" TYPE PARAMS}\n",
                (size_t)language->input_position -
                (size_t)language->input);
    return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
}

//====================================================================================//

language_error_t read_subtree(language_t *language, language_node_t **output) {
    _RETURN_IF_ERROR(skip_spaces(language));
    if(*language->input_position != '{') {
        print_error("Node must start with '{'.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position++;

    _RETURN_IF_ERROR(skip_spaces(language));
    char *type_end = NULL;
    node_type_t type = (node_type_t)strtol(language->input_position, &type_end, 10);
    if(type_end == NULL) {
        print_error("Nodes are written as {TYPE VALUE LEFT RIGHT}");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position = type_end;

    value_t value = {};
    _RETURN_IF_ERROR(tree_read_value(language, &value, type));

    language_node_t *node = NULL;
    _RETURN_IF_ERROR(nodes_storage_add(language, type, value, NULL, 0, &node));

    _RETURN_IF_ERROR(skip_spaces(language));
    if(*language->input_position != '-') {
        _RETURN_IF_ERROR(read_subtree(language, &node->left));
    }
    else {
        language->input_position++;
    }

    _RETURN_IF_ERROR(skip_spaces(language));
    if(*language->input_position != '-') {
        _RETURN_IF_ERROR(read_subtree(language, &node->right));
    }
    else {
        language->input_position++;
    }

    _RETURN_IF_ERROR(skip_spaces(language));
    if(*language->input_position != '}') {
        print_error("Node must end with '}'.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    language->input_position++;
    *output = node;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t tree_read_value(language_t *language, value_t *value, node_type_t type) {
    _RETURN_IF_ERROR(skip_spaces(language));
    char *value_end = NULL;
    switch(type) {
        case NODE_TYPE_IDENTIFIER: {
            value->identifier = strtoull(language->input_position, &value_end, 10);
            break;
        }
        case NODE_TYPE_NUMBER: {
            value->number = strtod(language->input_position, &value_end);
            break;
        }
        case NODE_TYPE_OPERATION: {
            value->opcode = (operation_t)strtol(language->input_position, &value_end, 10);
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    language->input_position = value_end;
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t write_tree(language_t *language) {
    FILE *output = fopen(language->output_file, "wb");
    if(output == NULL) {
        print_error("Error while opening file to write tree.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    fprintf(output, SZ_SP "\r\n", language->name_table.size);
    for(size_t elem = 0; elem < language->name_table.size; elem++) {
        identifier_t *identifier = language->name_table.identifiers + elem;
        fprintf(output,
                "{" SZ_SP " \"%.*s\" %d %lu}\n",
                identifier->length,
                (int)identifier->length,
                identifier->name,
                identifier->type,
                identifier->parameters_number);
    }
    fprintf(output, "\r\n" SZ_SP "\r\n", language->nodes.size);
    language_error_t error_code = write_subtree(language, language->root, output);
    fclose(output);
    return error_code;
}

//====================================================================================//

language_error_t write_subtree(language_t *language, language_node_t *node, FILE *output) {
    fprintf(output, "{ %d ", node->type);
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            fprintf(output, SZ_SP " ", node->value.identifier);
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
        _RETURN_IF_ERROR(write_subtree(language, node->left, output));
    }
    else {
        fprintf(output, "- ");
    }
    if(node->right != NULL) {
        _RETURN_IF_ERROR(write_subtree(language, node->right, output));
    }
    else {
        fprintf(output, "- ");
    }

    fprintf(output, "} ");
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t verify_keywords(void) {
    for(size_t i = 1; i < sizeof(KeyWords) / sizeof(KeyWords[0]); i++) {
        if((size_t)KeyWords[i].code != i) {
            print_error("Broken keywords table.\n");
            return LANGUAGE_BROKEN_KEYWORDS_TABLE;
        }
    }
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

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
            print_error("Flag '%s' is expected to have " SZ_SP " parameters after it.\n",
                        argv[elem],
                        SupportedFlags[index].params);
            return LANGUAGE_PARSING_FLAGS_ERROR;
        }
        _RETURN_IF_ERROR(SupportedFlags[index].handler(language, argc, elem, argv));
        elem += SupportedFlags[index].params + 1;
    }
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t handler_input(language_t *language,
                               int       /*argc*/,
                               size_t      position,
                               const char *argv[]) {
    language->input_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t handler_output(language_t *language,
                                int       /*argc*/,
                                size_t      position,
                                const char *argv[]) {
    language->output_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

//====================================================================================//

language_error_t skip_spaces(language_t *language) {
    while(isspace(*language->input_position)) {
        language->input_position++;
    }
    return LANGUAGE_SUCCESS;
}

//====================================================================================//
